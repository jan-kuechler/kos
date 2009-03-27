#include <bitop.h>
#include <page.h>
#include <string.h>
#include "kernel.h"
#include "tty.h"
#include "mm/mm.h"
#include "mm/virt.h"

#define MAP_TABLE_VADDR 0xFFFFF000

extern dword *mm_get_mmap(void);
extern dword  mm_get_mmap_size();

pdir_t kernel_pdir;
ptab_t working_table_map;

static byte paging_enabled;

/* some helper functions to clarify the code */
static inline paddr_t getaddr(pany_entry_t entry)
{
	return (paddr_t)bmask((dword)entry, BMASK_PE_ADDR);
}

#define addr2page(addr) ((dword)addr / PAGE_SIZE)

#define pdir_index(addr) (addr2page(addr) / PDIR_LEN)
#define ptab_index(addr) (addr2page(addr) % PTABLE_LEN)

static inline ptab_t map_working_table(ptab_t tab)
{
	if (paging_enabled) {
		working_table_map[ptab_index(MAP_TABLE_VADDR)] = (dword)tab | PE_PRESENT | PE_READWRITE | PE_CACHEDISABLE;
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

	kernel_pdir = mm_alloc_page();
	memset(kernel_pdir, 0, PAGE_SIZE);
	kout_printf("kernel_pdir: 0x%x\n", kernel_pdir);

	working_table_map = mm_alloc_page();
	memset(working_table_map, 0, PAGE_SIZE);
	kout_printf("working_table_map: 0x%x\n", working_table_map);

	kernel_pdir[pdir_index(MAP_TABLE_VADDR)] = (pdir_entry_t)working_table_map | PE_PRESENT | PE_READWRITE;
	map_working_table(0);

	kout_puts("init_paging: kernel_phys_start\n");
	mm_map_range(kernel_pdir, kernel_phys_start, kernel_phys_start, PE_PRESENT | PE_READWRITE,
	             ((dword)kernel_phys_end - (dword)kernel_phys_start) / PAGE_SIZE);

	kout_puts("init_paging: vmem\n");
	mm_map_range(kernel_pdir, (paddr_t)0xB8000, (vaddr_t)0xB8000, PE_PRESENT | PE_READWRITE, 1);

	kout_puts("init_paging: mmap\n");
	mm_map_range(kernel_pdir, mm_get_mmap(), mm_get_mmap(), PE_PRESENT | PE_READWRITE,
	             mm_get_mmap_size() / PAGE_SIZE);

	kout_puts("init_paging: kernel_pdir\n");
	mm_map_page(kernel_pdir, kernel_pdir, kernel_pdir, PE_PRESENT | PE_READWRITE);
	kout_puts("init_paging: working_table_map\n");
	mm_map_page(kernel_pdir, working_table_map, working_table_map, PE_PRESENT | PE_READWRITE);

	asm volatile("movl %%eax, %%cr3       \n"
	             "movl %%cr0, %%eax       \n"
	             "or   $0x80000000, %%eax \n"
	             "mov  %%eax, %%cr0       \n"
	            :
	            : "a"(kernel_pdir)
	            );

	paging_enabled = 1;
}

void mm_map_page(pdir_t pdir, paddr_t paddr, vaddr_t vaddr, dword flags)
{
	/* parameter validation */
	if (vaddr == NULL) {
		panic("map_page: vaddr is NULL.");
	}

	if (bmask((dword)vaddr,BMASK_4K_ALIGN) || bmask((dword)paddr,BMASK_4K_ALIGN)) {
		panic("map_page: paddr or vaddr is not 4k aligned.");
	}

	/* get page directory entry for the virt. address */
	pdir_entry_t pde = pdir[pdir_index(vaddr)];
	ptab_t ptab = 0;

	if (bnotset(pde, PE_PRESENT)) {
		/* there is no page table for this addr, create one */
		ptab = (ptab_t)mm_alloc_page();

		pdir[pdir_index(vaddr)] = (dword)ptab | flags;
	}
	else {
		ptab = getaddr(pde);
	}

	/* map the page table, so it can be accessed */
	ptab = map_working_table(ptab);

	if (bisset(ptab[ptab_index(vaddr)], PE_PRESENT) && bnotset(flags, PE_PRESENT)) {
		/* attempt to create a new mapping to an addr that allready exists */

		ptab_entry_t pte = ptab[ptab_index(vaddr)];
		pte &= (PE_ACCESSED | PT_DIRTY); // clear any proccessor flags for comparison
		if (pte != (ptab_entry_t)((dword)paddr | flags)) {
			panic("mm_map_page: There is allready a mapping to virt 0x%x (phys: 0x%x flags: %b)", vaddr, paddr, flags);
		}
		/* ignore the rest */
	}
	else {
		ptab[ptab_index(vaddr)] = (dword)paddr | flags;
	}

}

void mm_unmap_page(pdir_t pdir, vaddr_t vaddr)
{
	mm_map_page(pdir, NULL, vaddr, 0);
}

void mm_map_range(pdir_t pdir, paddr_t pstart, vaddr_t vstart, dword flags, int num)
{
	int i=0;

	for (; i < num; ++i) {
		paddr_t paddr = (paddr_t)((dword)pstart + (i * PAGE_SIZE));
		vaddr_t vaddr = (vaddr_t)((dword)vstart + (i * PAGE_SIZE));

		mm_map_page(pdir, paddr, vaddr, flags);
	}
}
