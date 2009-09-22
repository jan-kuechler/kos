#ifndef MM_DMA_H
#define MM_DMA_H

#include "mm/types.h"

#define DMA_RANGE_LENGTH 0x10000 // thats 64kb

struct addrpair *dma_alloc_64k();
void dma_free_64k(struct addrpair *addr);

#endif /*MM_DMA_H*/
