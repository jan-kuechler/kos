#ifndef MM_VIRT_H
#define MM_VIRT_H

#include <types.h>

#define PE_PRESENT      0x0001
#define PE_READWRITE    0x0002
#define PE_USERMODE     0x0004
#define PE_WRITETRHOUGH 0x0008
#define PE_CACHEDISABLE 0x0010
#define PE_ACCESSED     0x0020
#define PD_4MBSIZE      0x0080
#define PT_GLOBAL       0x0040
#define PT_DIRTY        0x0100

#define VM_COMMON_FLAGS (PE_PRESENT | PE_READWRITE)

typedef dword pany_entry_t;
typedef dword pdir_entry_t;
typedef dword ptab_entry_t;

typedef pany_entry_t *pany_t;
typedef pdir_entry_t *pdir_t;
typedef ptab_entry_t *ptab_t;

extern pdir_t kernel_pdir;

void init_paging(void);

void vm_map_page(pdir_t pdir, paddr_t paddr, vaddr_t vaddr, dword flags);
void vm_unmap_page(pdir_t pdir, vaddr_t vaddr);

void vm_map_range(pdir_t pdir, paddr_t pstart, vaddr_t vstart, dword flags, int num);

vaddr_t vm_find_addr(pdir_t pdir);
vaddr_t vm_find_range(pdir_t pdir, int num);

vaddr_t vm_map_anywhere(pdir_t pdir, paddr_t pstart, dword flags, size_t size);
void    vm_identity_map(pdir_t pdir, paddr_t pstart, dword flags, size_t size);

/* define km_* as an abbreviation for vm_*(kernel_pdir, ...) */
#define km_map_page(paddr,vaddr,flags)        vm_map_page(kernel_pdir, paddr, vaddr, flags)
#define km_unmap_page(vaddr)                  vm_unmap_page(kernel_pdir, vaddr)
#define km_map_range(pstart,vstart,flags,num) vm_map_range(kernel_pdir, pstart, vstart, flags, num)

#define km_find_addr()                        vm_find_addr(kernel_pdir)
#define km_find_range(num)                    vm_find_range(kernel_pdir, num)

#define km_map_anywhere(paddr,flags,size)     vm_map_anywhere(kernel_pdir, paddr, flags, size)
#define km_identity_map(paddr,flags,size)     vm_identity_map(kernel_pdir, paddr, flags, size)

#endif /*MM_VIRT_H*/
