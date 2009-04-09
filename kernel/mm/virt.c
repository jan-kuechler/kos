#include <bitop.h>
#include <page.h>
#include <string.h>
#include "debug.h"
#include "kernel.h"
#include "tty.h"
#include "mm/mm.h"
#include "mm/virt.h"

// this is the last addr in kernel space
#define MAP_TABLE_VADDR 0x3FFFF000

extern dword *mm_get_mmap(void);
extern dword  mm_get_mmap_size();

pdir_t kernel_pdir;
ptab_t working_table_map;
dword  kpdir_rev;

static byte paging_enabled;

/* some helper functions to clarify the code */
static inline paddr_t getaddr(pany_entry_t entry)
{
	return (paddr_t)bmask((dword)entry, BMASK_PE_ADDR);
}

static inline dword getflags(pany_entry_t entry)
{
	return (dword)bmask((dword)entry, BMASK_PE_FLAGS);
}

#define CHECK_ALIGN(var) kassert(IS_PAGE_ALIGNED(var))

#define page2addr(page) ((paddr_t)(page * PAGE_SIZE))
#define addr2page(addr) ((dword)addr / PAGE_SIZE)

#define align_addr(addr)      ((paddr_t)((dword)addr - PAGE_OFFSET(addr)))
#define align_size(addr,size) (size + PAGE_OFFSET(addr))

// returns the page directory/table index from an virtual addr
#define pdir_index(addr) (addr2page(addr) / PDIR_LEN)
#define ptab_index(addr) (addr2page(addr) % PTAB_LEN)

// assembles a virtual addr of the page directory/table indices and an offset
#define create_addr(pdi,pti,off) ((vaddr_t)((pdi << PE_ADDR_SHIFT) + (pti << PAGE_SHIFT) + off))

// returns a NULL safe first page table index
#define safe_pti(pdi) ((pdi == 0) ? 1 : 0)

// invalidate page
#define invlpg(addr) do { asm volatile("invlpg %0" : : "m"(*(char*)addr)); } while (0);

// maps a page table to a virtual addr
static inline ptab_t map_working_table(ptab_t tab)
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

	km_map_range((paddr_t)0xB8000, (vaddr_t)0xB8000, PE_PRESENT | PE_READWRITE, NUM_PAGES(2*25*80));

	//km_map_range(mm_get_mmap(), mm_get_mmap(), PE_PRESENT | PE_READWRITE,
	//             NUM_PAGES(mm_get_mmap_size()));

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

	/* parameter validation */
	if (vaddr == NULL) {
		panic("map_page: vaddr is NULL.");
	}

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
			      vaddr, getaddr(pte), paddr, flags, getflags(pte));
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

	if (vaddr == (vaddr_t)0x2000) {
		dbg_vprintf(DBG_VM, "!! mapped %p => 0x2000\n", paddr);
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
 *  vm_alloc_page(pdir, user)
 *
 * Allocates a page mapped to the page directory
 * optionally accessible by usermode.
 */
vaddr_t vm_alloc_page(pdir_t pdir, int user)
{
	paddr_t page = mm_alloc_page();

	if (page == NO_PAGE)
		panic("vm_alloc_page: mm_alloc_page failed");

	dword flags = PE_PRESENT | PE_READWRITE;
	if (user)
		flags |= PE_USERMODE;

	return vm_alloc_addr(pdir, page, flags, PAGE_SIZE);
}

/**
 *  vm_alloc_page(pdir, user, num)
 *
 * Allocates a page range mapped to the page directory
 * optionally accessible by usermode.
 */
vaddr_t vm_alloc_range(pdir_t pdir, int user, int num)
{
	paddr_t pstart = mm_alloc_range(num);

	if (pstart == NO_PAGE)
		panic("vm_alloc_range: mm_alloc_range failed");

	dword flags = PE_PRESENT | PE_READWRITE;
	if (user)
		flags |= PE_USERMODE;

	return vm_alloc_addr(pdir, pstart, flags, num * PAGE_SIZE);

}

void vm_free_page(pdir_t pdir, vaddr_t page)
{

}

void vm_free_range(pdir_t pdir, vaddr_t start, int num)
{
}

static ptab_entry_t get_ptab_entry(pdir_t pdir, _unaligned_ vaddr_t vaddr)
{
	ptab_t       tab;
	ptab_entry_t pte = 0;

	int pdidx = pdir_index(vaddr);

	if (bnotset(pdir[pdidx], PE_PRESENT)) {
		return 0;
	}
	else {
		tab = getaddr(pdir[pdidx]);
	}

	tab = map_working_table(tab);

	if (tab[ptab_index(vaddr)] & PE_PRESENT) {
		pte = tab[ptab_index(vaddr)];
	}

	return pte;
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

int vm_is_userspace(vaddr_t vaddr, dword size)
{
	return 0;
}
