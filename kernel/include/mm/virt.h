#ifndef MM_VIRT_H
#define MM_VIRT_H

#include <types.h>
#include "mm/types.h"

#define PE_PRESENT      0x0001
#define PE_READWRITE    0x0002
#define PE_USERMODE     0x0004
#define PE_WRITETRHOUGH 0x0008
#define PE_CACHEDISABLE 0x0010
#define PE_ACCESSED     0x0020

#define PD_4MBSIZE      0x0080

#define PT_DIRTY        0x0040
#define PT_GLOBAL       0x0100

#define VM_COMMON_FLAGS (PE_PRESENT | PE_READWRITE)
#define VM_USER_FLAGS (VM_COMMON_FLAGS | PE_USERMODE)


#define USER_STACK_ADDR 0xFFFF0000

/* the kernel page directory */
extern pdir_t kernel_pdir;
/* the current revision of the kernel page directory,
   as changes may have to be copied to process pdirs */
extern dword  kpdir_rev;

void init_paging(void);

void vm_map_page(pdir_t pdir, _aligned_ paddr_t paddr, _aligned_ vaddr_t vaddr, dword flags);
void vm_unmap_page(pdir_t pdir, _aligned_ vaddr_t vaddr);

void vm_map_range(pdir_t pdir, _aligned_ paddr_t pstart, _aligned_ vaddr_t vstart, dword flags, int num);
void vm_unmap_range(pdir_t pdir, _aligned_ vaddr_t vstart, int num);

vaddr_t vm_find_addr(pdir_t pdir);
vaddr_t vm_find_range(pdir_t pdir, int num);

vaddr_t vm_alloc_addr(pdir_t pdir, _unaligned_ paddr_t pstart, dword flags, size_t size);
void    vm_free_addr(pdir_t pdir, _unaligned_ vaddr_t vstart, size_t size);
void    vm_identity_map(pdir_t pdir, _unaligned_ paddr_t pstart, dword flags, size_t size);

vaddr_t vm_alloc_page(pdir_t pdir, int user);
vaddr_t vm_alloc_range(pdir_t pdir, int user, int num);

void    vm_free_page(pdir_t pdir, _aligned_ vaddr_t page);
void    vm_free_range(pdir_t pdir, _aligned_ vaddr_t start, int num);

paddr_t vm_resolve_virt(pdir_t pdir, _unaligned_ vaddr_t vaddr);

int     vm_is_mapped(pdir_t pdir, _unaligned_ vaddr_t vaddr, dword size, dword flags);

/* define km_* as an abbreviation for vm_*(kernel_pdir, ...) */
#define km_map_page(paddr,vaddr,flags)        vm_map_page(kernel_pdir, paddr, vaddr, flags)
#define km_unmap_page(vaddr)                  vm_unmap_page(kernel_pdir, vaddr)
#define km_map_range(pstart,vstart,flags,num) vm_map_range(kernel_pdir, pstart, vstart, flags, num)
#define km_unmap_range(vstart,num)            vm_unmap_range(kernel_pdir, vstart, num)

#define km_find_addr()                        vm_find_addr(kernel_pdir)
#define km_find_range(num)                    vm_find_range(kernel_pdir, num)

#define km_alloc_addr(paddr,flags,size)       vm_alloc_addr(kernel_pdir, paddr, flags, size)
#define km_free_addr(vaddr,size)              vm_free_addr(kernel_pdir, vaddr, size)
#define km_identity_map(paddr,flags,size)     vm_identity_map(kernel_pdir, paddr, flags, size)

#define km_alloc_page()                       vm_alloc_page(kernel_pdir, 0)
#define km_alloc_range(num)                   vm_alloc_range(kernel_pdir, 0, num)
#define km_free_page(page)                    vm_free_page(kernel_pdir, page)
#define km_free_range(start, num)             vm_free_range(kernel_pdir, start, num)

#define km_resolve_virt(vaddr)                vm_resolve_virt(kernel_pdir, vaddr);

#endif /*MM_VIRT_H*/
