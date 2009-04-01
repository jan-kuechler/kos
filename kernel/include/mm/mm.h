#ifndef MM_H
#define MM_H

#include <types.h>

void init_mm(void);

paddr_t mm_alloc_page();
void    mm_free_page(paddr_t page);

paddr_t mm_alloc_range(dword num);
void    mm_free_range(paddr_t start, dword num);

dword   mm_total_mem();
dword   mm_num_pages();
dword   mm_num_free_pages();

#endif /*MM_H*/
