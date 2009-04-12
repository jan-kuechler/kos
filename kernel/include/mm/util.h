#ifndef MM_UTIL_H
#define MM_UTIL_H

#include "mm/types.h"

pdir_t mm_create_pagedir();

vaddr_t vm_user_to_kernel(pdir_t pdir, vaddr_t vaddr, size_t size);
vaddr_t vm_kernel_to_user(pdir_t pdir, vaddr_t vaddr, size_t size);

/* memcpy wrapper */
void vm_cpy_pp(paddr_t dst, paddr_t src, size_t num);
void vm_cpy_pv(paddr_t dst, vaddr_t src, size_t num);
void vm_cpy_vp(vaddr_t dst, paddr_t src, size_t num);

/* memset wrapper */
void vm_set_p(paddr_t dst, byte val, size_t num);

dword vm_switch_pdir(pdir_t pdir, dword rev);

#endif /*MM_UTIL_H*/
