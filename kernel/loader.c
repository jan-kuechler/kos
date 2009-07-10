#include <bitop.h>
#include <elf.h>
#include <page.h>
#include <stdlib.h>
#include <string.h>
#include <kos/config.h>
#include "loader.h"
#include "fs/fs.h"
#include "debug.h"
#include "kernel.h"
#include "pm.h"
#include "syscall.h"
#include "mm/kmalloc.h"
#include "mm/mm.h"
#include "mm/util.h"
#include "mm/virt.h"
#include "util/list.h"

static int elf32_check(void *mem);
static int elf32_check_type(void *mem, Elf32_Half type);

pid_t exec_file(const char *filename, const char *args, pid_t parent)
{
	void *mem = NULL;
	size_t size = load_file_to_mem(filename, &mem);
	if (!size)
		return 0;
	pid_t pid = exec_mem(mem, args, parent);
	kfree(mem);
	return pid;
}

size_t load_file_to_mem(const char *filename, void **mem)
{
	struct inode *ino = vfs_lookup(filename, cur_proc->cwd);
	if (!ino) return 0;

	size_t len = ino->length;
	struct file *file = vfs_open(ino, FSO_READ);
	if (!file) return 0;

	void *buffer = kmalloc(len);
	if (vfs_read(file, buffer, len, 0) <= 0) {
		kfree(buffer);
		return 0;
	}

	*mem = buffer;
	return len;
}

pid_t exec_mem(void *mem, const char *args, pid_t parent)
{
	switch (get_exec_type(mem)) {
	case ET_ELF32_EXEC:
		return elf32_exec(mem, args, parent);
		break;

	default:
		return 0;
	};
}

enum exec_type get_exec_type(void *mem)
{
	if (elf32_check(mem)) {
		if (elf32_check_type(mem, ET_EXEC))
			return ET_ELF32_EXEC;
	}
	return ET_UNKNOWN;
}

/* Elf32 stuff */

struct elf32_ldata
{
	vaddr_t base;
	size_t  pages;
};

static int elf32_check(void *mem)
{
	Elf32_Ehdr *hdr = mem;
	if (hdr->e_ident[EI_MAG0] != ELFMAG0 ||
	    hdr->e_ident[EI_MAG1] != ELFMAG1 ||
	    hdr->e_ident[EI_MAG2] != ELFMAG2 ||
	   	hdr->e_ident[EI_MAG3] != ELFMAG3)
	{
		return 0;
	}
	return 1;
}

static int elf32_check_type(void *mem, Elf32_Half type)
{
	Elf32_Ehdr *hdr = mem;
	return (hdr->e_type == type);
}

static int elf32_has_entry(Elf32_Phdr *phdr, Elf32_Ehdr *hdr)
{
	return hdr->e_entry >= phdr->p_vaddr &&
	       hdr->e_entry <  phdr->p_vaddr + phdr->p_memsz;
}

static void elf32_alloc_mem(pdir_t pdir, vaddr_t base, int pages)
{
	int i=0;
	for (; i < pages; ++i) {
		paddr_t page = mm_alloc_page();
		vm_map_page(pdir, page, (vaddr_t)(base + i * PAGE_SIZE), VM_USER_FLAGS);
	}
}

static void elf32_free_mem(pdir_t pdir, vaddr_t base, int pages)
{
	int i=0;
	for (; i < pages; ++i) {
		paddr_t page = vm_resolve_virt(pdir, (vaddr_t)(base + i * PAGE_SIZE));
		/* no need to unmap, as the pdir will be purged anyway */
		mm_free_page(page);
	}
}

static int elf32_first_page_bytes(Elf32_Phdr *phdr)
{
	if (phdr->p_filesz > PAGE_SIZE - (phdr->p_vaddr % PAGE_SIZE))
		return PAGE_SIZE - (phdr->p_vaddr % PAGE_SIZE);
	else
		return phdr->p_filesz;
}

static void elf32_getbrk(Elf32_Phdr *phdr, vaddr_t *brk, vaddr_t *page)
{
	vaddr_t base = (vaddr_t)PAGE_ALIGN_ROUND_DOWN(phdr->p_vaddr);
	int memsz_pages = NUM_PAGES(phdr->p_memsz);

	*page = (vaddr_t)(base + (memsz_pages * PAGE_SIZE));
	*brk  = (vaddr_t)(phdr->p_vaddr + phdr->p_memsz);
}

static void elf32_cleanup(struct proc *proc)
{
	if (!proc->ldata) return;

	list_entry_t *e = NULL;
	list_iterate(e, (list_t*)proc->ldata) {
		struct elf32_ldata *d = e->data;
		elf32_free_mem(proc->as->pdir, d->base, d->pages);
	}
	list_destroy(proc->ldata);
}

static void elf32_map_segment(pdir_t pdir, Elf32_Phdr *phdr, void *mem, list_t *linfo)
{
	if (!phdr->p_memsz)
		return;

	dbg_vprintf(DBG_LOADER, "Loading ELF segment...\n");
	dbg_vprintf(DBG_LOADER, "Program Header:\n"
	                        " Type:   %08d \n"
	                        " Flags:  %08b \n"
	                        " Offs:   %p   \n"
	                        " Vaddr:  %p   \n"
	                        " Paddr:  %p   \n"
	                        " Filesz: %08d \n"
	                        " Memsz:  %08d \n"
	                        " Align:  %08x \n",
	                        phdr->p_type, phdr->p_flags, phdr->p_offset,
	                        phdr->p_vaddr, phdr->p_paddr, phdr->p_filesz,
	                        phdr->p_memsz, phdr->p_align);

	vaddr_t base = (vaddr_t)PAGE_ALIGN_ROUND_DOWN(phdr->p_vaddr);
	if (base < (vaddr_t)CONF_PROG_BASE_MIN)
		base = (vaddr_t)CONF_PROG_BASE_MIN;

	int memsz_pages = NUM_PAGES(phdr->p_memsz);
	elf32_alloc_mem(pdir, base, memsz_pages);

	/* save the number of allocated pages for elf32_cleanup */
	struct elf32_ldata *ldata = kmalloc(sizeof(*ldata));
	ldata->base = base;
	ldata->pages = memsz_pages;
	list_add_front(linfo, ldata);

	/* the data may begin anywhere on the first page... */
	int first_page_bytes = elf32_first_page_bytes(phdr);

	vaddr_t src = (vaddr_t)((dword)mem + phdr->p_offset);
	paddr_t pdst = vm_resolve_virt(pdir, (vaddr_t)phdr->p_vaddr);

	dbg_vprintf(DBG_LOADER,"  Copy first %d bytes to first page %p (%p)\n"
	                       "  Source is %p\n",
	                       first_page_bytes, phdr->p_vaddr, pdst, src);
	vm_cpy_pv(pdst, src, first_page_bytes);

	src += first_page_bytes;

	int filesz_pages = NUM_PAGES(phdr->p_filesz);

	dbg_vprintf(DBG_LOADER, "  Rest of %d pages are copied...\n", filesz_pages - 1);

	/* copy all other pages */
	int i=1;
	for (; i < filesz_pages; ++i) {
		pdst = vm_resolve_virt(pdir, (vaddr_t)(base + i * PAGE_SIZE));
		vm_cpy_pv(pdst, src, PAGE_SIZE);
		dbg_vprintf(DBG_LOADER, "  Copy from %p to %p (%p)\n", src, (base + i * PAGE_SIZE), pdst);
		src += PAGE_SIZE;
	}


	/* the last page may have to be filled with 0s */
	int last_page_padding = PAGE_SIZE - ((phdr->p_offset + phdr->p_filesz) % PAGE_SIZE);
	if (last_page_padding) {
		dbg_vprintf(DBG_LOADER, "  Last page is padded with %d 0s\n", last_page_padding);
		vaddr_t data_end = base + ((filesz_pages - 1) * PAGE_SIZE) +
		                   (PAGE_SIZE - last_page_padding);
		pdst = vm_resolve_virt(pdir, data_end);
		vm_set_p(pdst, 0, last_page_padding);
	}

	dbg_vprintf(DBG_LOADER, "  Filling %d pages with 0s\n", memsz_pages - filesz_pages);
	/* if the programs size in memory is greater than its filesize,
	   the memory has to be filled with 0 */
	for (i = filesz_pages; i < memsz_pages; ++i) {
		pdst = vm_resolve_virt(pdir, (vaddr_t)(base + (i * PAGE_SIZE)));

		vm_set_p(pdst, 0, PAGE_SIZE);
	}
}

pid_t elf32_exec(void *mem, const char *args, pid_t parent)
{
	Elf32_Ehdr *hdr = mem;

	Elf32_Phdr *phdr = (Elf32_Phdr*)((dword)hdr + hdr->e_phoff);

	struct proc *proc = NULL;

	dbg_printf(DBG_LOADER, "Loading '%s'...\n", args);
	dbg_vprintf(DBG_LOADER, " Header:\n"
	                        "  Type:  %d\n"
	                        "  Entry: %p\n"
	                        "  #Phdr: %d\n",
	                        hdr->e_type, hdr->e_entry, hdr->e_phnum);


	int i=0;
	for (; i < hdr->e_phnum; ++i, ++phdr) {
		if (phdr->p_type == PT_LOAD) {
			if (elf32_has_entry(phdr, hdr)) {
				proc = pm_create((void*)hdr->e_entry, args, PM_USER, parent, PS_BLOCKED);

				proc->cleanup = elf32_cleanup;
				proc->ldata   = list_create();
			}
			if (!proc) return 0;

			elf32_map_segment(proc->as->pdir, phdr, mem, proc->ldata);

			elf32_getbrk(phdr, &proc->mem_brk, &proc->brk_page);
		}
	}

	dbg_vprintf(DBG_LOADER, " Brk is %p at page %p\n", proc->mem_brk, proc->brk_page);

	pm_unblock(proc);

	return proc->pid;
}

dword sys_execute(dword calln, dword path, dword cmd, dword arg2)
{
	size_t flen = 0;
	char *file = vm_map_string(cur_proc->as->pdir, (vaddr_t)path, &flen);

	size_t clen = 0;
	char *cmdline = vm_map_string(cur_proc->as->pdir, (vaddr_t)cmd, &clen);

	pid_t pid = exec_file(file, cmdline, cur_proc->pid);

	km_free_addr(file, flen);
	km_free_addr(cmdline, clen);
	return pid;
}

void init_loader(void)
{
	syscall_register(SC_EXECUTE, sys_execute);
}
