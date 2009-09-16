#include <page.h>
#include <string.h> // memcpy/set
#include <kos/strparam.h>
#include "debug.h"
#include "pm.h"
#include "mm/kmalloc.h"
#include "mm/mm.h"
#include "mm/util.h"
#include "mm/virt.h"

#include "intern.h"

static bool vm_cpy_pp_hack = false;

struct addrspace *vm_create_addrspace()
{
	struct addrspace *as = kmalloc(sizeof(*as));
	as->phys = mm_alloc_page();
	as->pdir = km_alloc_addr(as->phys, VM_COMMON_FLAGS, PAGE_SIZE);

	memset(as->pdir, 0, PAGE_SIZE);
	memcpy(as->pdir, kernel_pdir, PAGE_SIZE / 4);

	return as;
}

static bool clone_entry(pdir_t newpd, vaddr_t vaddr, ptab_entry_t pte)
{
	dbg_printf(DBG_MM, "Clone entry for %p\n", vaddr);

	paddr_t newphys = km_alloc_page();
	if (newphys == NO_PAGE)
		return false;

	dbg_vprintf(DBG_MM, " New page at phys:%p\n", newphys);
	dbg_vprintf(DBG_MM, " Old page at phys:%p\n", getaddr(pte));

	vm_map_page(newpd, newphys, vaddr, getflags(pte));
	// FIXME: error checking

	dbg_printf(DBG_MM, "vm_cpy_pp(%p, %p, %x)\n", newphys, getaddr(pte), PAGE_SIZE);
	vm_cpy_pp_hack = true;
	vm_cpy_pp(newphys, getaddr(pte), PAGE_SIZE);
	vm_cpy_pp_hack = false;
	return true;
}

struct addrspace *vm_clone_addrspace(struct proc *proc, struct addrspace *as)
{
	struct addrspace *newas = vm_create_addrspace();

	memset(newas->pdir, 0, PAGE_SIZE);

	vaddr_t addr = (vaddr_t)(KERN_SPACE_END + 1);

	for (; addr < proc->brk_page; addr += PAGE_SIZE) {
		ptab_entry_t pte = get_ptab_entry(as->pdir, addr);
		if (!pte)
			continue;
		if (!clone_entry(newas->pdir, addr, pte)) {
			return NULL;
		}
	}


	/*
	ptab_entry_t pte = get_ptab_entry(as->pdir, (vaddr_t)(USER_STACK_ADDR - PAGE_SIZE));
	if (!pte)
		dbg_error("No userstack??");
	else {
		clone_entry(newas->pdir, (vaddr_t)(USER_STACK_ADDR - PAGE_SIZE), pte);
	}
	*/

	return newas;
}

void vm_select_addrspace(struct addrspace *as)
{
	/* cr3 contains the phys addr of the page directory! */
	asm volatile("mov %0, %%cr3" : : "r"(as->phys));
}

void vm_destroy_addrspace(struct addrspace *as)
{
	km_free_addr(as->pdir, PAGE_SIZE);
	mm_free_page(as->phys);
	kfree(as);
}

/**
 *  vm_map_string(pdir, vaddr, length)
 *
 * Maps a string from user to kernel space and returns it's new virtual
 * addr. Length can be a pointer to a size_t and will be filled with the
 * string's length including the '\0' postfix if it is not NULL.
 */
vaddr_t vm_map_string(pdir_t pdir, vaddr_t vaddr, size_t *length)
{
	dbg_vprintf(DBG_VM, "vm_map_string: \n");
	dbg_vprintf(DBG_VM, "  vaddr = %p\n", vaddr);

	paddr_t info_addr = vm_resolve_virt(pdir, vaddr);
	dbg_vprintf(DBG_VM, "  info_addr = %p\n", info_addr);

	struct strparam *info =
		(struct strparam *)km_alloc_addr(info_addr, VM_COMMON_FLAGS,
		                                 sizeof(struct strparam));

	dbg_vprintf(DBG_VM, "  info = %p\n", info);
	dbg_vprintf(DBG_VM, "  info->ptr = %p\n", info->ptr);
	dbg_vprintf(DBG_VM, "  info->len = %d\n", info->len);

	vaddr_t str = vm_user_to_kernel(pdir, info->ptr, info->len + 1);

	dbg_vprintf(DBG_VM, "  str = %p - '%s'\n", str, str);

	if (length) {
		dbg_vprintf(DBG_VM, "  assigning length + 1\n");
		*length = info->len + 1;
	}
	km_free_addr(info, sizeof(struct strparam));

	return str;
}

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

	if (!vm_cpy_pp_hack)
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

/**
 *  vm_alloc_page(pdir, user)
 *
 * Allocates a page mapped to the page directory
 * optionally accessible by usermode.
 */
vaddr_t vm_alloc_page(pdir_t pdir, int user)
{
	paddr_t page = mm_alloc_page();

	if (page == NO_PAGE)
		panic("vm_alloc_page: mm_alloc_page failed");

	dword flags = PE_PRESENT | PE_READWRITE;
	if (user)
		flags |= PE_USERMODE;

	return vm_alloc_addr(pdir, page, flags, PAGE_SIZE);
}

/**
 *  vm_alloc_page(pdir, user, num)
 *
 * Allocates a page range mapped to the page directory
 * optionally accessible by usermode.
 *
 * TODO: Change this to _not_ use mm_alloc_range() (this is why we have paging...)
 */
vaddr_t vm_alloc_range(pdir_t pdir, int user, int num)
{
	paddr_t pstart = mm_alloc_range(num);

	if (pstart == NO_PAGE)
		panic("vm_alloc_range: mm_alloc_range failed");

	dword flags = PE_PRESENT | PE_READWRITE;
	if (user)
		flags |= PE_USERMODE;

	return vm_alloc_addr(pdir, pstart, flags, num * PAGE_SIZE);

}

void vm_free_page(pdir_t pdir, vaddr_t page)
{
	paddr_t paddr = vm_resolve_virt(pdir, page);

	vm_free_addr(pdir, page, PAGE_SIZE);
	mm_free_page(paddr);
}

void vm_free_range(pdir_t pdir, vaddr_t start, int num)
{
	paddr_t pstart = vm_resolve_virt(pdir, start);

	vm_free_addr(pdir, start, num * PAGE_SIZE);
	mm_free_range(pstart, num);
}
