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
#include "mm/kmalloc.h"
#include "mm/mm.h"
#include "mm/util.h"
#include "mm/virt.h"

static int elf_check(void *mem);
static int elf_check_type(void *mem, Elf32_Half type);

pid_t exec_file(const char *filename, const char *args, pid_t parent)
{
	void *mem = NULL;
	size_t size = load_file_to_mem(filename, &mem);
	if (!size)
		return 0;
	return exec_mem(mem, args, parent);
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
	case ET_ELF_EXEC:
		return exec_elf_exec(mem, args, parent);
		break;

	default:
		return 0;
	};
}

enum exec_type get_exec_type(void *mem)
{
	if (elf_check(mem)) {
		if (elf_check_type(mem, ET_EXEC))
			return ET_ELF_EXEC;
	}
	return ET_UNKNOWN;
}

/* Elf32 stuff */

static int elf_check(void *mem)
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

static int elf_check_type(void *mem, Elf32_Half type)
{
	Elf32_Ehdr *hdr = mem;
	return (hdr->e_type == type);
}

static int elf_has_entry(Elf32_Phdr *phdr, Elf32_Ehdr *hdr)
{
	return hdr->e_entry >= phdr->p_vaddr &&
	       hdr->e_entry <  phdr->p_vaddr + phdr->p_memsz;
}

static void elf_alloc_mem(pdir_t pdir, vaddr_t base, int pages)
{
	int i=0;
	for (; i < pages; ++i) {
		paddr_t page = mm_alloc_page();
		vm_map_page(pdir, page, (vaddr_t)(base + i * PAGE_SIZE), VM_USER_FLAGS);
	}
}

static int elf_first_page_bytes(Elf32_Phdr *phdr)
{
	if (phdr->p_filesz > PAGE_SIZE - (phdr->p_vaddr % PAGE_SIZE))
		return PAGE_SIZE - (phdr->p_vaddr % PAGE_SIZE);
	else
		return phdr->p_filesz;
}

static void elf_map_segment(pdir_t pdir, Elf32_Phdr *phdr, void *mem)
{
	if (!phdr->p_memsz)
		return;

	vaddr_t base = (vaddr_t)PAGE_ALIGN_ROUND_DOWN(phdr->p_vaddr);
	if (base < (vaddr_t)CONF_PROG_BASE_MIN)
		base = (vaddr_t)CONF_PROG_BASE_MIN;

	int memsz_pages = NUM_PAGES(phdr->p_memsz);
	elf_alloc_mem(pdir, base, memsz_pages);

	/* the data may begin anywhere on the first page... */
	int first_page_bytes = elf_first_page_bytes(phdr);

	/* the last page may have to be filled with 0s */
	int last_page_padding = PAGE_SIZE - ((phdr->p_offset + phdr->p_filesz) % PAGE_SIZE);

	vaddr_t src = (vaddr_t)((dword)mem + phdr->p_offset);
	paddr_t pdst = vm_resolve_virt(pdir, (vaddr_t)phdr->p_vaddr);

	vm_cpy_pv(pdst, src, first_page_bytes);

	src += first_page_bytes;

	int filesz_pages = NUM_PAGES(phdr->p_filesz);

	dbg_vprintf(DBG_ELF, "  Rest of %d pages are copied...\n", filesz_pages);

	/* copy all other pages */
	int i=1;
	for (; i < filesz_pages; ++i) {
		pdst = vm_resolve_virt(pdir, (vaddr_t)(mem + i * PAGE_SIZE));
		vm_cpy_pv(pdst, src, PAGE_SIZE);
		src += PAGE_SIZE;
	}

	/* fill the rest of the last page with 0 */
	if (last_page_padding) {
		dbg_vprintf(DBG_ELF, "  Last page is padded with %d 0s\n", last_page_padding);
		vaddr_t data_end = mem + ((filesz_pages - 1) * PAGE_SIZE) +
		                   (PAGE_SIZE - last_page_padding);
		pdst = vm_resolve_virt(pdir, data_end);
		vm_set_p(pdst, 0, last_page_padding);
	}

	dbg_vprintf(DBG_ELF, "  Filling %d pages with 0s\n", memsz_pages - filesz_pages);
	/* if the programs size in memory is greater than its filesize,
	   the memory has to be filled with 0 */
	for (i = filesz_pages; i < memsz_pages; ++i) {
		pdst = vm_resolve_virt(pdir, (vaddr_t)(mem + (i * PAGE_SIZE)));

		vm_set_p(pdst, 0, PAGE_SIZE);
	}
}

pid_t exec_elf_exec(void *mem, const char *args, pid_t parent)
{
	Elf32_Ehdr *hdr = mem;

	Elf32_Phdr *phdr = (Elf32_Phdr*)((dword)hdr + hdr->e_phoff);
	proc_t *proc = NULL;

	int i=0;
	for (; i < hdr->e_phnum; ++i, ++phdr) {
		if (phdr->p_type == PT_LOAD) {
			if (elf_has_entry(phdr, hdr)) {
				proc = pm_create((void*)hdr->e_entry, args, PM_USER, parent, PS_BLOCKED);
			}
			if (!proc) return 0;

			elf_map_segment(proc->pagedir, phdr, mem);
		}
	}

	pm_unblock(proc);

	return proc->pid;
}

void init_loader(void)
{
}
