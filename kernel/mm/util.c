#include <page.h>
#include <string.h> // memcpy/set
#include <kos/strparam.h>
#include "debug.h"
#include "pm.h"
#include "mm/mm.h"
#include "mm/util.h"
#include "mm/virt.h"

/**
 *  mm_create_pagedir()
 *
 * Creates a page directory mapping the full kernel.
 */
pdir_t mm_create_pagedir()
{
	dbg_vprintf(DBG_VM, "Creating new page directory...");

	pdir_t pdir = mm_alloc_page();
	km_identity_map(pdir, VM_COMMON_FLAGS, PAGE_SIZE);

	memcpy(pdir, kernel_pdir, PAGE_SIZE);

	dbg_vprintf(DBG_VM, "done (%p)\n", pdir);

	return pdir;
}

/**
 *  vm_map_string(pdir, vaddr, length)
 *
 * Maps a string from user to kernel space and returns it's new virtual
 * addr. Length can be a pointer to a size_t and will be filled with the
 * string's length if it is not NULL.
 */
vaddr_t vm_map_string(pdir_t pdir, vaddr_t vaddr, size_t *length)
{
	paddr_t info_addr = vm_resolve_virt(pdir, vaddr);
	struct strparam *info =
		(struct strparam *)km_alloc_addr(info_addr, VM_COMMON_FLAGS,
		                                 sizeof(struct strparam));

	vaddr_t str = km_alloc_addr(info->ptr, VM_COMMON_FLAGS, info->len + 1);
	if (length)
		*length = info->len;
	km_free_addr(info_addr, sizeof(struct strparam));

	return str;
};

/**
 *  vm_user_to_kernel(pdir, vaddr, size)
 *
 * Maps a memory region from user- to kernelspace.
 * NOTE: Remember to free the address after using!
 */
vaddr_t vm_user_to_kernel(pdir_t pdir, vaddr_t vaddr, size_t size)
{
	paddr_t paddr = vm_resolve_virt(pdir, vaddr);
	vaddr_t kaddr = km_alloc_addr(paddr, VM_COMMON_FLAGS, size);
	return kaddr;
}

/**
 *  vm_kernel_to_user(pdir, vaddr, size)
 *
 * Maps a memory region from kernel- to userspace.
 */
vaddr_t vm_kernel_to_user(pdir_t pdir, vaddr_t vaddr, size_t size)
{
	paddr_t paddr = vm_resolve_virt(kernel_pdir, vaddr);
	vaddr_t uaddr = vm_alloc_addr(pdir, paddr, VM_USER_FLAGS, size);
	return uaddr;
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
	//	//dbg_vprintf(DBG_VM, "Updating pdir (%d => %d)\n", rev, kpdir_rev);
	//	memcpy(pdir, kernel_pdir, PAGE_SIZE / 2);
	//	//dbg_vprintf(DBG_VM, "Update done.\n");
	//}
	asm volatile("mov %0, %%cr3" : : "r"(pdir));
	return kpdir_rev;
}
