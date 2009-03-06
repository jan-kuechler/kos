#ifndef MM_H
#define MM_H

#include <types.h>

void init_mm(void);

paddr_t mm_alloc_page();
void    mm_free_page(paddr_t page);

paddr_t mm_alloc_range(dword num);
void    mm_free_range(paddr_t start, dword num);

#endif /*MM_H*/
