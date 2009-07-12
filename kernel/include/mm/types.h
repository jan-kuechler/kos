#ifndef MM_TYPES_H
#define MM_TYPES_H

#include <types.h>

#define _aligned_
#define _unaligned_

#define _phys_
#define _virt_

typedef dword pany_entry_t;
typedef dword pdir_entry_t;
typedef dword ptab_entry_t;

typedef pany_entry_t *pany_t;
typedef pdir_entry_t *pdir_t;
typedef ptab_entry_t *ptab_t;

struct addrspace
{
	paddr_t phys;
	pdir_t  pdir;
};

struct addrpair
{
	paddr_t phys;
	vaddr_t virt;
};

#endif /*MM_TYPES_H*/
