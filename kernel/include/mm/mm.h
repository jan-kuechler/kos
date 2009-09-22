#ifndef MM_H
#define MM_H

#include <page.h>
#include <stdlib.h>
#include "mm/types.h"

/**
 * Initializes the physical memory manager.
 */
void init_mm(void);

/**
 * Allocates a physical memory page. If any error occures
 * NO_PAGE is returned.
 * @return The address of the page or NO_PAGE
 */
_aligned_ paddr_t mm_alloc_page(void);

/**
 * Frees a physical memory page.
 * @param page The address of the page or NO_PAGE
 */
void mm_free_page(_aligned_ paddr_t page);

/**
 * Allocates physical memory of num contigious pages. If any error
 * occures NO_PAGE is returned.
 * @param num Number of pages
 * @return The start address of the memory range
 */
_aligned_ paddr_t mm_alloc_range(size_t num);

/**
 * Frees num physical memory pages.
 * @param start Start address of the pages
 * @param num Number of pages
 */
void mm_free_range(_aligned_ paddr_t start, size_t num);

/**
 * Allocates a DMA feasible memory range of 64kb.
 * @return Start address of the memory range.
 */
_aligned_ paddr_t mm_alloc_dma(void);

/**
 * Reserves num pages from start for any use.
 * @param start The start address
 * @param num Number of pages
 * @return True for success, false otherwise.
 */
bool mm_alloc_phys_range(_aligned_ paddr_t start, size_t num);

/**
 * Frees a previously reserved memory area.
 * @param start The start address
 * @param num Number of pages
 */
void mm_free_phys_range(_aligned_ paddr_t start, size_t num);

/**
 * Returns the total number of memory.
 * @return Total memory in bytes
 */
size_t mm_total_mem(void);

/**
 * Returns the total number of memory pages.
 * @return Total memory in pages
 */
size_t mm_num_pages(void);

/**
 * Returns the number of free pages.
 * @return Free memory in pages
 */
size_t mm_num_free_pages(void);

#endif /*MM_H*/
