#ifndef MM_KMALLOC_H
#define MM_KMALLOC_H

#include <stdlib.h>

void *kmalloc(size_t size);
void *kmallocu(size_t size);
void *kcalloc(size_t num, size_t size);
void *krealloc(void *ptr, size_t size);

void  kfree(void *ptr);

#endif /*MM_KMALLOC_H*/
