#ifndef MM_DMA_H
#define MM_DMA_H

#include "mm/types.h"

struct addrpair *dma_alloc_64k();
void dma_free_64k(struct addrpair *addr);

#endif /*MM_DMA_H*/
