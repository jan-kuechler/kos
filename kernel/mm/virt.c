#include <bitop.h>
#include <string.h>
#include "bios.h"
#include "debug.h"
#include "kernel.h"
#include "syscall.h"
#include "tty.h"
#include "mm/mm.h"
#include "mm/virt.h"

#include "intern.h"

// this is the last addr in kernel space
#define MAP_TABLE_VADDR 0x3FFFF000

extern dword *mm_get_mmap(void);
extern dword  mm_get_mmap_size();

int32_t sys_sbrk(int32_t);

pdir_t kernel_pdir;
ptab_t working_table_map;
dword  kpdir_rev;
struct addrspace kernel_addrspace;

static byte paging_enabled;

// invalidate page
#define invlpg(addr) do { asm volatile("invlpg %0" : : "m"(*(char*)addr)); } while (0);

// maps a page table to a virtual addr
ptab_t map_working_table(ptab_t tab)
{
	if (paging_enabled) {
		working_table_map[ptab_index(MAP_TABLE_VADDR)] = (dword)tab | PE_PRESENT | PE_READWRITE | PE_CACHEDISABLE;
		invlpg(MAP_TABLE_VADDR);
		return (ptab_t)MAP_TABLE_VADDR;
	}
	return tab;
}

/**
 *  init_paging()
 */
void init_paging(void)
{
	paging_enabled = 0;

	/* alloc the kernel's page directory */
	kernel_pdir = mm_alloc_page();
	kassert(kernel_pdir != NO_PAGE);
	memset(kernel_pdir, 0, PAGE_SIZE);

	kernel_addrspace.phys = kernel_pdir;
	kernel_addrspace.pdir = kernel_pdir;

	/* this is the page table where the working table is mapped into */
	working_table_map = mm_alloc_page();
	kassert(working_table_map != NO_PAGE);
	memset(working_table_map, 0, PAGE_SIZE);

	dbg_vprintf(DBG_VM, "wtm is at %p\n", working_table_map);

	kernel_pdir[pdir_index(MAP_TABLE_VADDR)] = (pdir_entry_t)working_table_map | PE_PRESENT | PE_READWRITE;
	map_working_table(0);

	kpdir_rev = 0;

	/* map the kernel, the video memory and some other stuff */
	km_map_range(kernel_phys_start, kernel_phys_start, PE_PRESENT | PE_READWRITE,
	             NUM_PAGES((dword)kernel_phys_end - (dword)kernel_phys_start));

	//km_map_range((paddr_t)0xB8000, (vaddr_t)0xB8000, PE_PRESENT | PE_READWRITE, NUM_PAGES(2*25*80));
	km_identity_map((paddr_t)CON_MEM_START,  PE_PRESENT | PE_READWRITE, CON_MEM_SIZE);

	//km_map_range(mm_get_mmap(), mm_get_mmap(), PE_PRESENT | PE_READWRITE,
	//             NUM_PAGES(mm_get_mmap_size()));

	/* bios data area is readonly mapped */
	km_identity_map((paddr_t)BIOS_DATA_ADDR, PE_PRESENT, sizeof(struct bios_data_area));

	km_map_page(kernel_pdir, kernel_pdir, PE_PRESENT | PE_READWRITE);
	km_map_page(working_table_map, working_table_map, PE_PRESENT | PE_READWRITE);

	/* reset the revision to 0, as there are no
	   proc pdirs yet, that could have older data */
	kpdir_rev = 0;

	/* and enable paging */
	asm volatile("movl %%eax, %%cr3       \n"
	             "movl %%cr0, %%eax       \n"
	             "or   $0x80000000, %%eax \n"
	             "mov  %%eax, %%cr0       \n"
	            :
	            : "a"(kernel_pdir)
	            );

	paging_enabled = 1;

	syscall_register(SC_SBRK, sys_sbrk, 1);
}

/**
 *  vm_map_page(pdir, paddr, vaddr, flags)
 *
 * Maps a virtual addr to a physical addr in the given page
 * directory with the given flags.
 */
void vm_map_page(pdir_t pdir, _aligned_ paddr_t paddr, _aligned_ vaddr_t vaddr, dword flags)
{
	CHECK_ALIGN(vaddr);
	CHECK_ALIGN(paddr);

	/* get page directory entry for the virt. address */
	pdir_entry_t pde = pdir[pdir_index(vaddr)];
	ptab_t ptab = 0;

	if (bnotset(pde, PE_PRESENT)) {
		/* there is no page table for this addr, create one */
		ptab = (ptab_t)mm_alloc_page();

		pdir[pdir_index(vaddr)] = (dword)ptab | flags;

		ptab = map_working_table(ptab);
		memset(ptab, 0, PAGE_SIZE);

		dbg_vprintf(DBG_VM, "New page table created for 0x%08x\n", vaddr);
	}
	else {
		ptab = map_working_table(getaddr(pde));
	}

	if (bisset(ptab[ptab_index(vaddr)], PE_PRESENT) && bisset(flags, PE_PRESENT)) {
		/* attempt to create a new mapping to an addr that allready exists */

		ptab_entry_t pte = ptab[ptab_index(vaddr)];
		pte &= ~(PE_ACCESSED | PT_DIRTY); // clear any proccessor flags for comparison
		if (pte != (ptab_entry_t)((dword)paddr | flags)) {
			panic("vm_map_page: Double mapping!\n"
			      "\tVirt:     0x%08x\n"
			      "\tNew phys: 0x%08x\n"
			      "\tOld phys: 0x%08x\n"
			      "\t           xxxG0DACWURP\n"
			      "\tNew flags: %012b\n"
			      "\tOld flags: %012b\n",
			      vaddr, paddr, getaddr(pte), flags, getflags(pte));
		}
		/* ignore the rest */
	}
	else {
		ptab[ptab_index(vaddr)] = (dword)paddr | flags;
		invlpg(vaddr);

		if (pdir == kernel_pdir) {
			dword old = kpdir_rev;
			kpdir_rev++;
			/* just to be on the safe side (-; */
			if (kpdir_rev < old)
				panic("vm_map_page: Overflow in kernel page directory revision (%d => %d)", old, kpdir_rev);
		}
	}
}

/**
 *  vm_unmap_page(pdir, vaddr)
 *
 * Unmaps a virtual addr from a page directory
 */
void vm_unmap_page(pdir_t pdir, _aligned_ vaddr_t vaddr)
{
	CHECK_ALIGN(vaddr);
	vm_map_page(pdir, NULL, vaddr, 0);
}

/**
 *  vm_map_range(pdir, pstart, vstart, flags, num)
 *
 * Maps num contiguous pages beginning at pstart to vstart in a page directory
 */
void vm_map_range(pdir_t pdir, _aligned_ paddr_t pstart, _aligned_ vaddr_t vstart, dword flags, int num)
{
	CHECK_ALIGN(pstart);
	CHECK_ALIGN(vstart);

	int i=0;
	for (; i < num; ++i) {
		paddr_t paddr = (paddr_t)((dword)pstart + (i * PAGE_SIZE));
		vaddr_t vaddr = (vaddr_t)((dword)vstart + (i * PAGE_SIZE));

		vm_map_page(pdir, paddr, vaddr, flags);
	}
}

/**
 *  vm_unmap_range(pdir, vstart, num)
 *
 * Unmaps a range of num pages from a page directory.
 */
void vm_unmap_range(pdir_t pdir, _aligned_ vaddr_t vstart, int num)
{
	CHECK_ALIGN(vstart);
	vm_map_range(pdir, NULL, vstart, 0, num);
}

/**
 *  vm_find_addr(pdir_t pdir)
 *
 * Returns a free virtual addr in the page directory.
 */
vaddr_t vm_find_addr(pdir_t pdir)
{
	int pdi = 0;

	for (; pdi < PDIR_LEN; ++pdi) {
		pdir_entry_t pde = pdir[pdi];

		if (bnotset(pde, PE_PRESENT)) {
			/* the page table does not exist, so it's vaddrs are free */
			return create_addr(pdi, safe_pti(pdi), 0);
		}
		else {
			/* loop through the page table to find a free entry */
			ptab_t tab = map_working_table(getaddr(pde));

			int pti = safe_pti(pdi);
			for (; pti < PTAB_LEN; ++pti) {
				ptab_entry_t pte = tab[pti];

				if (bnotset(pte, PE_PRESENT)) {
					return create_addr(pdi, pti, 0);
				}
			}
		}
	}

	return 0;
}

/**
 *  vm_find_range(pdir, num)
 *
 * Returns a virtual addr pointing to a range of num free page addrs
 */
vaddr_t vm_find_range(pdir_t pdir, int num)
{
	int pdi = 0;
	int count = 0;

	int pdstart = 0;
	int ptstart = 0;

	for (; pdi < PDIR_LEN; ++pdi) {
		if (!count)
			pdstart = pdi;

		pdir_entry_t pde = pdir[pdi];

		if (bisset(pde, PE_PRESENT)) {
			int    pti = safe_pti(pdi);
			ptab_t tab = map_working_table(getaddr(pde));

			for (; pti < PTAB_LEN; ++pti) {
				ptab_entry_t pte = tab[pti];

				if (bnotset(pte, PE_PRESENT)) {
					/* yeah, another free addr */
					if (!count)
						ptstart = pti;

					count++;
					if (count >= num)
						break;
				}
				else {
					/* this addr is not free, return to start )-: */
					count = 0;
				}
			}
		}
		else {
			/* the page table is not present, so add PTAB_LEN free addrs */

			if (!count)
				ptstart = safe_pti(pdi);

			count += (PTAB_LEN - safe_pti(pdi));
		}

		if (count >= num) {
			/* epic win! */
			return create_addr(pdstart, ptstart, 0);
		}
	}

	dbg_vprintf(DBG_VM, "nothing found )-:\n");
	return 0;
}

/**
 *  vm_alloc_addr(pdir, pstart, size)
 *
 * Maps size bytes anywhere in the page directory
 * and returns their virtual address.
 * pstart does not need to be 4k aligned
 */
vaddr_t vm_alloc_addr(pdir_t pdir, _unaligned_ paddr_t pstart, dword flags, size_t size)
{
	paddr_t aligned_pstart = align_addr(pstart);
	size_t  aligned_size   = align_size(pstart, size);

	vaddr_t vstart = vm_find_range(pdir, NUM_PAGES(aligned_size));
	if (!vstart) {
		panic("vm_alloc_addr: page directory is full.");
	}

	vm_map_range(pdir, aligned_pstart, vstart, flags, NUM_PAGES(aligned_size));

	return vstart + PAGE_OFFSET(pstart);
}

/**
 *  vm_free_addr(pdir, vstart, size)
 *
 * Unmaps an addr returned by vm_alloc_addr
 */
void vm_free_addr(pdir_t pdir, _unaligned_ vaddr_t vstart, size_t size)
{
	vaddr_t aligned_vstart = align_addr(vstart);
	size_t  aligned_size   = align_size(vstart, size);

	vm_unmap_range(pdir, aligned_vstart, NUM_PAGES(aligned_size));
}

/**
 *  vm_identity_map(pdir, pstart, flags, size)
 *
 * Identity maps size bytes from pstart in a page directory
 */
void vm_identity_map(pdir_t pdir, _unaligned_ paddr_t pstart, dword flags, size_t size)
{
	paddr_t aligned_pstart = align_addr(pstart);
	size_t  aligned_size   = align_size(pstart, size);

	dbg_vprintf(DBG_VM, "Aligning addr: 0x%08x => 0x%08x\n", pstart, aligned_pstart);

	vm_map_range(pdir, aligned_pstart, aligned_pstart, flags, NUM_PAGES(aligned_size));
}

/**
 *  vm_resolve_virt(pdir, vaddr)
 *
 * Resolves a virtual addr using any page directory.
 */
paddr_t vm_resolve_virt(pdir_t pdir, _unaligned_ vaddr_t vaddr)
{
	ptab_entry_t pte = get_ptab_entry(pdir, vaddr);

	if (bnotset(pte, PE_PRESENT)) {
		return NULL;
	}

	return getaddr(pte) + PAGE_OFFSET(vaddr);
}

/**
 *  vm_is_mapped(pdir, vaddr, size, flags)
 *
 * Returns true if the addr range is mapped with the given flags
 */
int vm_is_mapped(pdir_t pdir, _unaligned_ vaddr_t vaddr, dword size, dword flags)
{
	if (vaddr + size < vaddr) // overflow
		return 0;

	vaddr_t cur = vaddr;
	for (; cur < (vaddr + size); cur += PAGE_SIZE) {
		ptab_entry_t pte = get_ptab_entry(pdir, cur);
		if (!pte || bnotset(pte, flags)) {
			return 0;
		}
	}

	return 1;
}

static void increase_heap(struct proc *proc, int pages)
{
	CHECK_ALIGN(proc->brk_page);

	int i=0;
	for (; i < pages; ++i) {
		paddr_t page = mm_alloc_page();
		vm_map_page(proc->as->pdir, page, proc->brk_page, PE_PRESENT | PE_READWRITE | PE_USERMODE);
		proc->brk_page += PAGE_SIZE;
	}
}

int32_t sys_sbrk(int32_t incr)
{
	dbg_vprintf(DBG_SC, "sys_sbrk(%d) from %s (%d)\n", incr, syscall_proc->cmdline, syscall_proc->pid);

	vaddr_t old_brk = syscall_proc->mem_brk;

	if (incr == 0)
		return (int32_t)old_brk;

	if (incr < 0) {
		dbg_vprintf(DBG_SC, " sbrk: incr < 0 is not implemented.\n");
		return (int32_t)old_brk;
	}

	size_t rest = syscall_proc->brk_page - syscall_proc->mem_brk;

	if (rest < incr) {
		dbg_vprintf(DBG_SC, " Increase heap by %d pages\n", NUM_PAGES(incr));
		increase_heap(syscall_proc, NUM_PAGES(incr));
	}
	else {
		dbg_vprintf(DBG_SC, " Rest (%d) is big enough...\n", rest);
	}

	syscall_proc->mem_brk += incr;
	return (int32_t)old_brk;
}
