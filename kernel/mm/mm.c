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

// this is enough for 4GB memory
#define MMAP_SIZE 0x8000

#define addr_to_idx(a) (((uintptr_t)a / PAGE_SIZE) / 32)
#define idx_to_addr(i) (i * PAGE_SIZE * 32)

#define page_to_pos(p) (((uintptr_t)p / PAGE_SIZE) & 31)
#define pos_to_offs(p) ((uintptr_t)p * PAGE_SIZE)

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
		mark_free((paddr_t)((uint8_t*)start + (i*PAGE_SIZE)));
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

paddr_t mm_alloc_page()
{
	paddr_t page = find_free_page();
	if (page == NO_PAGE) {
		seterr(E_NO_MEM);
		return NO_PAGE;
	}
	mark_used(page);
	return page;
}

void mm_free_page(paddr_t page)
{
	if (page == NO_PAGE)
		return;
	if (!IS_PAGE_ALIGNED(page))
		seterr(E_ALIGN);
	else
		mark_free(page);
}


paddr_t mm_alloc_range(size_t num)
{
	if (!num) {
		seterr(E_INVALID);
		return NO_PAGE;
	}

	paddr_t start = find_free_range(num);
	if (start == NO_PAGE) {
		seterr(E_NO_MEM);
		return NO_PAGE;
	}

	mark_range_used(start, num);
	return start;
}


void mm_free_range(paddr_t start, size_t num)
{
	if (start == NO_PAGE)
		return;

	if (!IS_PAGE_ALIGNED(start))
		seterr(E_ALIGN);
	else if (!num)
		seterr(E_INVALID);
	else
		mark_range_free(start, num);
}

size_t mm_total_mem()
{
	return total_mem;
}

size_t mm_num_pages()
{
	return total_mem / PAGE_SIZE;
}

size_t mm_num_free_pages()
{
	size_t num = 0;
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

	*start = (paddr_t)((uintptr_t)mb_mmap->base_addr);
	*len   = mb_mmap->length;

	return USABLE_BLOCK;
}

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
			paddr_t aligned = PAGE_ALIGN_ROUND_UP(start);
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
