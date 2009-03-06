#include "bitop.h"
#include "kernel.h"
#include <multiboot.h>
#include "mm.h"
#include <page.h>
#include <string.h>
#include <types.h>

#define NO_PAGE ((paddr_t)1) /* 1 is not a valid addr for a page, as they must be 4k-aligned */

#define addr_to_idx(a) (((dword)a / PAGE_SIZE) / 32)
#define idx_to_addr(i) (i * PAGE_SIZE * 32)

#define page_to_pos(p) (((dword)p / PAGE_SIZE) % 32)
#define pos_to_offs(p) ((dword)p * PAGE_SIZE)

static dword *mmap;
static dword mmap_length;

struct memblock
{
	dword start, end;
};

static inline void mark_free(paddr_t page)
{
	bsetn(mmap[addr_to_idx(page)], page_to_pos(page));
}

static inline void mark_used(paddr_t page)
{
	bclrn(mmap[addr_to_idx(page)], page_to_pos(page));
}

static inline void mark_range_free(paddr_t start, dword num)
{
	int i=0;
	for (; i < num; ++i)
		mark_free(start + (i*PAGE_SIZE));
}

static inline void mark_range_used(paddr_t start, dword num)
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
		return (paddr_t)(idx_to_addr(i) + pos_to_offs(p));
	}
	return NO_PAGE;
}

static paddr_t find_free_range(dword msize)
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
paddr_t mm_alloc_range(dword num)
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
void mm_free_range(paddr_t start, dword num)
{
	mark_range_free(start, num);
}

static void mark(struct memblock *blocks, int *nblocks, dword start, dword end)
{
	int i = 0;

	start = PAGE_ALIGN_ROUND_UP(start);
	end   = PAGE_ALIGN_ROUND_DOWN(end);

	for (; i < *nblocks; ++i) {
		if (blocks[i].start == start) {
			if (blocks[i].end == end) {  // it's a whole block
				int j=i;

				*nblocks--;
				for (; j < *nblocks; ++j) {
					blocks[j].start = blocks[j+1].start;
					blocks[j].end   = blocks[j+1].end;

				}
				break;
			}
			else if (blocks[i].end > end) {
				blocks[i].start = end;
				break;
			}
		}
		else if (blocks[i].start < start) {
			if (blocks[i].end == end) {
				blocks[i].end = start;
				break;
			}
			else if (blocks[i].end > end) {
				blocks[*nblocks].start = end;
				blocks[*nblocks].end   = blocks[i].end;

				blocks[i].end = start;

				*nblocks++;

				break;
			}
		}
	}
}

/**
 *  init_mm()
 *
 * Initializes the physical memory manager.
 */
void init_mm(void)
{
	multiboot_mmap_t *mb_mmap = 0;
	unsigned int mb_mmap_length = multiboot_info.mmap_length;
	struct memblock blocks[32] = {0};
	int nblocks = 0;

	int i = 0;
	for (mb_mmap = (multiboot_mmap_t*) multiboot_info.mmap_addr;
			 mb_mmap < (multiboot_mmap_t*) multiboot_info.mmap_addr + mb_mmap_length;
			 mb_mmap = (multiboot_mmap_t*) ((char*)mb_mmap + mb_mmap->size - 4)) {


		blocks[nblocks].start = PAGE_ALIGN_ROUND_UP(mb_mmap->base_addr);
		blocks[nblocks].end   = PAGE_ALIGN_ROUND_DOWN(mb_mmap->base_addr + mb_mmap->length);
		++nblocks;
	}

	mark(blocks, &nblocks, (dword)kernel_phys_start, (dword)kernel_phys_end);

	for (i=0; i < multiboot_info.mods_count; ++i) {
		multiboot_mod_t *mod = (multiboot_mod_t*)((char*)multiboot_info.mods_addr + (i * sizeof(multiboot_mod_t)));
		mark(blocks, &nblocks, mod->mod_start, mod->mod_end);
		mark(blocks, &nblocks, mod->string, mod->string + strlen((const char*)mod->string) + 1);
	}

	dword upper_end = 0;
	for (i=0; i < nblocks; ++i) {
		if (blocks[i].end > upper_end)
			upper_end = blocks[i].end;
	}

	mmap_length = upper_end / PAGE_SIZE / (sizeof(dword) * 8);
	for (i=0; i < nblocks; ++i) {
		if (blocks[i].start && ((blocks[i].end - blocks[i].start) >= (mmap_length * 4))) {
			mmap = (dword*)blocks[i].start;
			blocks[i].start = PAGE_ALIGN_ROUND_UP(blocks[i].start + mmap_length * 4);
			break;
		}
	}

	for (i=0; i < nblocks; ++i) {
		mark_range_free((paddr_t)blocks[i].start, blocks[i].end - blocks[i].start);
	}
}
