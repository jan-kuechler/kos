#include <page.h>
#include <string.h> // memcpy/set
#include "debug.h"
#include "mm/mm.h"
#include "mm/util.h"
#include "mm/virt.h"

pdir_t mm_create_pagedir()
{
	pdir_t pdir = mm_alloc_page();
	km_identity_map(pdir, VM_COMMON_FLAGS, PAGE_SIZE);

	memcpy(pdir, kernel_pdir, PAGE_SIZE);

	return pdir;
}

#define map(addr) km_alloc_addr(addr, VM_COMMON_FLAGS, size)
#define unmap(vaddr) km_free_addr(vaddr, size)

void vm_cpy_pp(paddr_t dst, paddr_t src, size_t size)
{
	vaddr_t vdst = map(dst);
	vaddr_t vsrc = map(src);

	memcpy(vdst, vsrc, size);

	unmap(vdst);
	unmap(vsrc);
}

void vm_cpy_pv(paddr_t dst, vaddr_t src, size_t size)
{
	vaddr_t vdst = map(dst);
	memcpy(vdst, src, size);
	unmap(vdst);
}

void vm_cpy_vp(vaddr_t dst, paddr_t src, size_t size)
{
	vaddr_t vsrc = map(src);
	memcpy(dst, vsrc, size);
	unmap(vsrc);
}

void vm_set_p(paddr_t dst, byte val, size_t size)
{
	vaddr_t vdst = map(dst);
	memset(vdst, val, size);
	unmap(vdst);
}

dword vm_switch_pdir(pdir_t pdir, dword rev)
{
	//if (rev < kpdir_rev) {
	//	/* copy the kernel addr space (lower half) */
	//	memcpy(pdir, kernel_pdir, PAGE_SIZE / 2);
	//}
	asm volatile("mov %0, %%cr3" : : "r"(pdir));
	return kpdir_rev;
}
