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
paddr_t mm_alloc_page();

/**
 * Frees a physical memory page.
 * @param page The address of the page or NO_PAGE
 */
void mm_free_page(paddr_t page);

paddr_t mm_alloc_range(size_t num);
void    mm_free_range(paddr_t start, size_t num);

size_t  mm_total_mem();
size_t  mm_num_pages();
size_t  mm_num_free_pages();

#endif /*MM_H*/
