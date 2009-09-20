#include <bitop.h>
#include <multiboot.h>
#include <page.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "debug.h"
#include "error.h"
#include "elf.h"
#include "kernel.h"
#include "mm/mm.h"

#include "intern.h"

enum block_type
{
	USABLE_BLOCK,
	UNUSABLE_BLOCK,
	END_OF_BLOCKS,
};

#define addr_to_idx(a) (((dword)a / PAGE_SIZE) / 32)
#define idx_to_addr(i) (i * PAGE_SIZE * 32)

#define page_to_pos(p) (((dword)p / PAGE_SIZE) & 31)
#define pos_to_offs(p) ((dword)p * PAGE_SIZE)

// this is enough for 4GB memory
#define MMAP_SIZE 0x8000

static uint32_t mmap[MMAP_SIZE];
static size_t mmap_length;
static size_t total_mem;

static inline void mark_free(paddr_t page)
{
	bsetn(mmap[addr_to_idx(page)], page_to_pos(page));
}

static inline void mark_used(paddr_t page)
{
	bclrn(mmap[addr_to_idx(page)], page_to_pos(page));
}

static inline void mark_range_free(paddr_t start, size_t num)
{
	int i=0;
	for (; i < num; ++i)
		mark_free((paddr_t)((char*)start + (i*PAGE_SIZE)));
}

static inline void mark_range_used(paddr_t start, size_t num)
{
	int i=0;
	for (; i < num; ++i)
		mark_used(start + (i*PAGE_SIZE));
}

static paddr_t find_free_page()
{
	int i=0;

	for (; i < mmap_length; ++i) {
		if ((mmap[i] & BMASK_DWORD) == 0)
			continue;
		int p = bscanfwd(mmap[i]);

		paddr_t page = (paddr_t)(idx_to_addr(i) + pos_to_offs(p));
		return page;
	}
	return NO_PAGE;
}

static paddr_t find_free_range(size_t msize)
{
	int i=0;
	int found = 0;
	paddr_t start = NO_PAGE;

	for (; i < mmap_length; ++i) {
		if ((mmap[i] & BMASK_DWORD) == 0) {
			found = 0;
			continue;
		}
		int j=0;
		for (; j < (sizeof(dword) * 8); ++j) {
			if (bissetn(mmap[i], j)) {
				if (!found)
					start = (paddr_t)(idx_to_addr(i) + pos_to_offs(j));
				if (++found >= msize)
					return start;
			}
			else {
				found = 0;
			}
		}
	}

	return NO_PAGE;
}

/**
 *  mm_alloc_page()
 *
 * Returns the address of a 4k page of usable memory.
 */
paddr_t mm_alloc_page()
{
	paddr_t page = find_free_page();
	if (page == NO_PAGE)
		panic("Not enough memory to alloc 1 page.");
	mark_used(page);
	return page;
}

/**
 *  mm_free_page(page)
 *
 * Frees a previously allocated page.
 */
void mm_free_page(paddr_t page)
{
	mark_free(page);
}

/**
 *  mm_alloc_range(num)
 *
 * Returns the address of num sequenced pages.
 */
paddr_t mm_alloc_range(size_t num)
{
	paddr_t start = find_free_range(num);
	if (start == NO_PAGE)
		panic("Not enough memory to alloc %d pages.", num);
	mark_range_used(start, num);
	return start;
}

/**
 *  mm_free_range(start, num)
 *
 * Frees num sequenced pages starting from start.
 */
void mm_free_range(paddr_t start, size_t num)
{
	mark_range_free(start, num);
}

/**
 *  mm_total_mem()
 *
 * Returns the ammount of total memory in the system
 */
size_t mm_total_mem()
{
	return total_mem;
}


/**
 *  mm_num_pages()
 *
 * Returns the number of usable (=> not kernel owned) pages in the system.
 */
size_t mm_num_pages()
{
	return mmap_length * 32; /* 32 pages per mmap entry */
}

/**
 *  mm_num_free_pages()
 *
 * Returns the number of free pages in the system.
 */
size_t mm_num_free_pages()
{
	dword num = 0;
	int i=0;

	for(; i < mmap_length; ++i) {
		int j=0;
		for (; j < 32; ++j) {
			if (bissetn(mmap[i], j))
				++num;
		}
	}

	return num;
}

/**
 *  mm_get_mmap();
 *
 * Returns a pointer to the memory map.
 * This should only be used by mm/virt.c and is
 * not exported in mm/mm.h.
 */
dword *mm_get_mmap(void)
{
	return mmap;
}

dword mm_get_mmap_size(void)
{
	return (MMAP_SIZE * sizeof(dword));
}

static enum block_type get_avail_block(paddr_t *start, size_t *len, size_t i)
{
	struct multiboot_mmap *mb_mmap =
		(struct multiboot_mmap*)multiboot_info.mmap_addr;
	struct multiboot_mmap *mb_end = mb_mmap + multiboot_info.mmap_length;

	mb_mmap += i;

	if (mb_mmap > mb_end)
		return END_OF_BLOCKS;
	if (mb_mmap->type != 1)
		return UNUSABLE_BLOCK;

	*start = (paddr_t)((uint32_t)mb_mmap->base_addr); // truncate 64 bit value
	*len   = mb_mmap->length;

	return USABLE_BLOCK;
}

/**
 *  init_mm()
 *
 * Initializes the physical memory manager.
 */
void init_mm(void)
{
	// mark everything as not available
	memset(mmap, 0, 4 * MMAP_SIZE); // mmap_size is in dwords
	mmap_length = MMAP_SIZE;

	total_mem = 0;

	if (!bisset(multiboot_info.flags, MB_MMAP)) {
		panic("No mulitboot memory map available.");
	}

	int i=0;
	while (true) {
		paddr_t start = NULL;
		size_t  len   = 0;
		enum block_type type = get_avail_block(&start, &len, i++);

		if (type == END_OF_BLOCKS)
			break;
		if (type == UNUSABLE_BLOCK)
			continue;

		dbg_printf(DBG_MM, "Memory: %dkb at %p\n", len/1024, start);

		if (!IS_PAGE_ALIGNED(start)) {
			paddr_t aligned = PAGE_ALIGN_ROUND_UP((uint32_t)start);
			dbg_vprintf(DBG_MM, " align %p to %p\n", start, aligned);
			ptrdiff_t offs = aligned - start;
			len -= offs;
			start = aligned;
		}
		total_mem += len;
		mark_range_free(start, NUM_PAGES(len));
	}

	mark_range_used(kernel_phys_start, NUM_PAGES(kernel_phys_end - kernel_phys_start));
	multiboot_mod_t *mod = (multiboot_mod_t*)multiboot_info.mods_addr;
	for (i=0; i < multiboot_info.mods_count; ++i) {
		mark_range_used((paddr_t)mod, NUM_PAGES(sizeof(multiboot_mod_t)));
		mark_range_used((paddr_t)mod->mod_start, NUM_PAGES(mod->mod_end - mod->mod_start));
		if (mod->cmdline) {
			dbg_vprintf(DBG_MM, "marking cmdline %p %d\n", mod->cmdline, NUM_PAGES(strlen((char*)mod->cmdline)));
			mark_range_used((paddr_t)mod->cmdline, NUM_PAGES(strlen((char*)mod->cmdline)));
		}
	}

	paddr_t sec_addr = (paddr_t)multiboot_info.u.elf_sec.addr;
	size_t  sec_size = multiboot_info.u.elf_sec.size;
	size_t  sec_num  = multiboot_info.u.elf_sec.num;
	mark_range_used(sec_addr, NUM_PAGES(sec_size));

	Elf32_Shdr *shdr = (Elf32_Shdr*)sec_addr;
	for (i=0; i < sec_num; ++i) {
		mark_range_used((paddr_t)shdr[i].sh_addr, NUM_PAGES(shdr[i].sh_size));
	}

	mark_used((paddr_t)0x00);
}
