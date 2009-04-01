#ifndef MM_KMALLOC_H
#define MM_KMALLOC_H

#include <types.h>

void *kmalloc(size_t size)             __attribute__((malloc));
void *kmallocu(size_t size)            __attribute__((malloc));
void *kcalloc(size_t num, size_t size) __attribute__((malloc));
void *krealloc(void *ptr, size_t size) __attribute__((malloc));

void  kfree(void *ptr);

#endif /*MM_KMALLOC_H*/
