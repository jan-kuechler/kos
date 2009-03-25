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

typedef dword pany_entry_t;
typedef dword pdir_entry_t;
typedef dword ptab_entry_t;

typedef pany_entry_t *pany_t;
typedef pdir_entry_t *pdir_t;
typedef ptab_entry_t *ptab_t;

void init_paging(void);

void mm_map_page(pdir_t pdir, paddr_t paddr, vaddr_t vaddr, dword flags);
void mm_unmap_page(pdir_t pdir, vaddr_t vaddr);

void mm_map_range(pdir_t pdir, paddr_t pstart, vaddr_t vstart, dword flags, int num);

#endif /*MM_VIRT_H*/
