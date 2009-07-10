#ifndef MM_UTIL_H
#define MM_UTIL_H

#include "mm/types.h"

struct addrspace *vm_create_addrspace();
void vm_select_addrspace(struct addrspace *as);
void vm_destroy_addrspace(struct addrspace *as);

vaddr_t vm_map_string(pdir_t pdir, vaddr_t vaddr, size_t *length);

vaddr_t vm_user_to_kernel(pdir_t pdir, vaddr_t vaddr, size_t size);
vaddr_t vm_kernel_to_user(pdir_t pdir, vaddr_t vaddr, size_t size);

vaddr_t vm_alloc_page(pdir_t pdir, int user);
vaddr_t vm_alloc_range(pdir_t pdir, int user, int num);
void    vm_free_page(pdir_t pdir, _aligned_ vaddr_t page);
void    vm_free_range(pdir_t pdir, _aligned_ vaddr_t start, int num);

#define km_alloc_page()            vm_alloc_page(kernel_pdir, 0)
#define km_alloc_range(num)        vm_alloc_range(kernel_pdir, 0, num)
#define km_free_page(page)         vm_free_page(kernel_pdir, page)
#define km_free_range(start, num)  vm_free_range(kernel_pdir, start, num)


/* memcpy wrapper */
void vm_cpy_pp(paddr_t dst, paddr_t src, size_t num);
void vm_cpy_pv(paddr_t dst, vaddr_t src, size_t num);
void vm_cpy_vp(vaddr_t dst, paddr_t src, size_t num);

/* memset wrapper */
void vm_set_p(paddr_t dst, byte val, size_t num);

#endif /*MM_UTIL_H*/
