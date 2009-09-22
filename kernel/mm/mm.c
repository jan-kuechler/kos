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
#include "mm/dma.h"
#include "mm/mm.h"

#include "intern.h"

enum block_type
{
	USABLE_BLOCK,
	UNUSABLE_BLOCK,
	END_OF_BLOCKS,
};

#define NUM_DMA 8

#define DMA_BEGIN 0x10000

static const paddr_t dma_addrs[NUM_DMA] =
{
	(paddr_t)0x10000,
	(paddr_t)0x20000,
	(paddr_t)0x30000,
	(paddr_t)0x40000,
	(paddr_t)0x50000,
	(paddr_t)0x60000,
	(paddr_t)0x70000,
	(paddr_t)0x80000,
};
static const paddr_t dma_start = DMA_BEGIN;
static const paddr_t dma_end = DMA_BEGIN + (NUM_DMA * DMA_RANGE_LENGTH);

/* FIXME: Move this to an arch header */
#define MMAP_SIZE 0x8000
#define MMAP_TYPE uint32_t
#define MMAP_BITS (sizeof(MMAP_TYPE) * 8)

#define addr_to_idx(a) (((uintptr_t)a / PAGE_SIZE) / 32)
#define idx_to_addr(i) (i * PAGE_SIZE * 32)

#define page_to_pos(p) (((uintptr_t)p / PAGE_SIZE) & 31)
#define pos_to_offs(p) ((uintptr_t)p * PAGE_SIZE)

static MMAP_TYPE mmap[MMAP_SIZE];
static size_t total_mem;
static paddr_t mem_end;

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

static inline bool is_free(paddr_t page)
{
	return bissetn(mmap[addr_to_idx(page)], page_to_pos(page));
}

static inline bool is_range_free(paddr_t start, size_t num)
{
	int i=0;
	for (; i < num; ++i) {
		if (!is_free(start + (i * PAGE_SIZE)))
			return false;
	}
	return true;
}

/* helper function for find */
static inline bool test(int idx, int pos, size_t num, int *count, paddr_t *result)
{
	if (bissetn(mmap[idx], pos)) {
		if (!*count)
			*result = (paddr_t)(idx_to_addr(idx) + pos_to_offs(pos));

		if (++*count >= num)
			return true;
	}
	else {
		*count = 0;
	}
	return false;
}

/*
 * Finds num contigious pages between start and end.
 */
static paddr_t find(size_t num, paddr_t start, paddr_t end)
{
	int start_index = addr_to_idx(start);
	int end_index   = addr_to_idx(end);
	int start_pos   = page_to_pos(start);
	int end_pos     = page_to_pos(end);

	int count = 0;
	paddr_t result = NO_PAGE;

	int i=0;

	if (end_pos)
		end_index--;

	if (start_pos) {
		int i=0;
		for (i = start_pos; i < MMAP_BITS; ++i) {
			if (test(start_index, i, num, &count, &result))
				return result;
		}
		start_index++;
	}

	for (i=start_index; i < end_index; ++i) {
		if (mmap[i] == 0) { // no free pages here
			count = 0;
			continue;
		}

		int p=0;
		for (; p < MMAP_BITS; ++p) {
			if (test(i, p, num, &count, &result))
				return result;
		}
	}

	if (end_pos) {
		end_index++;
		for (i=0; i < end_pos; ++i) {
			if (test(end_index, i, num, &count, &result))
				return result;
		}
	}
	return NO_PAGE;
}

paddr_t mm_alloc_page(void)
{
	paddr_t page = find(1, dma_end, mem_end);
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

	paddr_t start = find(num, dma_end, mem_end);
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

bool mm_alloc_phys_range(paddr_t start, size_t num)
{
	if (!IS_PAGE_ALIGNED(start)) {
		seterr(E_ALIGN);
		return false;
	}
	if (!num) {
		seterr(E_INVALID);
		return false;
	}

	int i=0;
	for (; i < num; ++i) {
		if (!is_free(start + i * PAGE_SIZE)) {
			seterr(E_NO_MEM);
			return false;
		}
	}
	mark_range_used(start, num);
	return true;
}

void mm_free_phys_range(paddr_t start, size_t num)
{
	if (!IS_PAGE_ALIGNED(start)) {
		seterr(E_ALIGN);
		return;
	}
	if (!num) {
		seterr(E_INVALID);
		return;
	}

	mark_range_free(start, num);
}

paddr_t mm_alloc_dma(void)
{
	int i=0;
	for (; i < NUM_DMA; ++i) {
		if (is_range_free(dma_addrs[i], DMA_RANGE_LENGTH)) {
			mark_range_used(dma_addrs[i], DMA_RANGE_LENGTH);
			return dma_addrs[i];
		}
	}
	seterr(E_NO_MEM);
	return NO_PAGE;
}

void mm_free_dma(paddr_t start)
{
	if (start == NO_PAGE)
		return;

	/* TODO: Add checks for non-dma addrs? */
	mark_range_free(start, DMA_RANGE_LENGTH);
}

size_t mm_total_mem(void)
{
	return total_mem;
}

size_t mm_num_pages(void)
{
	return total_mem / PAGE_SIZE;
}

size_t mm_num_free_pages(void)
{
	size_t num = 0;
	int i=0;

	for(; i < MMAP_SIZE; ++i) {
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

	total_mem = 0;
	mem_end   = NULL;

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
		paddr_t this_end = (paddr_t)((uint8_t)start + len);
		if (this_end > mem_end)
			mem_end = this_end;
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
