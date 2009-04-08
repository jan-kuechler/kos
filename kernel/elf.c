#include <elf.h>
#include <page.h>
#include <types.h>
#include "config.h"
#include "debug.h"
#include "kernel.h"
#include "pm.h"
#include "mm/mm.h"
#include "mm/util.h"
#include "mm/virt.h"

/**
 *  elf_check(obj)
 *
 * Checks the ELF magic of the object
 */
byte elf_check(vaddr_t obj)
{
	Elf32_Ehdr *header = (Elf32_Ehdr*)obj;
	if (header->e_ident[EI_MAG0] != ELFMAG0 ||
	    header->e_ident[EI_MAG1] != ELFMAG1 ||
	    header->e_ident[EI_MAG2] != ELFMAG2 ||
	    header->e_ident[EI_MAG3] != ELFMAG3)
	{
		return 0;
	}
	return 1;
}

/**
 *  elf_check_type(obj, type)
 *
 * Checks the given ELF object for a specific type
 */
byte elf_check_type(vaddr_t obj, Elf32_Half type)
{
	Elf32_Ehdr *header = (Elf32_Ehdr*)obj;
	return (header->e_type == type);
}

static inline int has_entry(Elf32_Phdr *phdr, Elf32_Ehdr *header)
{
	return header->e_entry >= phdr->p_vaddr &&
	       header->e_entry < phdr->p_vaddr + phdr->p_memsz;
}

static inline void map_segment_mem(pdir_t pdir, Elf32_Phdr *phdr, vaddr_t start)
{
	if (!phdr->p_memsz) {
		/* nothing todo */
		return;
	}

	int memsz_pages = NUM_PAGES(phdr->p_memsz);
	vaddr_t base = (vaddr_t)PAGE_ALIGN_ROUND_DOWN(phdr->p_vaddr);

	if (base < (vaddr_t)CONF_PROG_BASE_MIN)
		base = (vaddr_t)CONF_PROG_BASE_MIN;

	/* allocate the memory for the segment */
	int i=0;
	for (; i < memsz_pages; ++i) {
		paddr_t page = mm_alloc_page();
		vm_map_page(pdir, page, (vaddr_t)(base + i * PAGE_SIZE), VM_USER_FLAGS);
	}

	/* the data may begin anywhere on the first page... */
	dword first_page_bytes;
	if (phdr->p_filesz > PAGE_SIZE - (phdr->p_vaddr % PAGE_SIZE)) {
		first_page_bytes = PAGE_SIZE - (phdr->p_vaddr % PAGE_SIZE);
	}
	else {
		first_page_bytes = phdr->p_filesz;
	}

	/* the last page may have to be filled with 0s */
	dword last_page_padding = PAGE_SIZE - ((phdr->p_offset + phdr->p_filesz) % PAGE_SIZE);

	/* copy the first page */
	vaddr_t src = (vaddr_t)((dword)start + phdr->p_offset);
	paddr_t pdst = vm_resolve_virt(pdir, (vaddr_t)phdr->p_vaddr);

	vm_cpy_pv(pdst, src, first_page_bytes);

	src += first_page_bytes;

	int filesz_pages = NUM_PAGES(phdr->p_filesz);

	/* copy all other pages */
	for (i = 1; i < filesz_pages; ++i) {
		pdst = vm_resolve_virt(pdir, (vaddr_t)(start + i * PAGE_SIZE));
		vm_cpy_pv(pdst, src, PAGE_SIZE);
		src += PAGE_SIZE;
	}

	/* fill the rest of the last page with 0 */
	if (last_page_padding) {
		vaddr_t data_end = start + ((filesz_pages - 1) * PAGE_SIZE) +
		                   (PAGE_SIZE - last_page_padding);
		pdst = vm_resolve_virt(pdir, data_end);
		vm_set_p(pdst, 0, last_page_padding);
	}

	/* if the programs size in memory is greater than its filesize,
	   the memory has to be filled with 0 */
	for (i = filesz_pages; i < memsz_pages; ++i) {
		pdst = vm_resolve_virt(pdir, (vaddr_t)(start + (i * PAGE_SIZE)));

		vm_set_p(pdst, 0, PAGE_SIZE);
	}
}

/**
 *  elf_load(obj, cmdline)
 *
 * Loads an ELF executable
 */
void elf_load(vaddr_t obj, const char *cmdline)
{
	Elf32_Ehdr *header = (Elf32_Ehdr*)obj;

	/* input validation */
	if (!elf_check(obj)) {
		panic("elf_load: no valid ELF program (%s)", cmdline ? cmdline : "<NULL>");
	}
	if (!elf_check_type(obj, ET_EXEC)) {
		panic("elf_load: ELF program is no executable (%s)", cmdline ? cmdline : "<NULL>");
	}

	dbg_printf(DBG_ELF, "Loading %s\n", cmdline);

	Elf32_Phdr *phdr = (Elf32_Phdr*)((dword)header + header->e_phoff);
	proc_t *proc = NULL;

	int i=0;
	for (; i < header->e_phnum; ++i, ++phdr) {
		if (phdr->p_type == PT_LOAD) {

			/* the first proghdr containa the entry point */
			if (has_entry(phdr, header)) {
				proc = pm_create((void*)header->e_entry, cmdline, 1, 0, PS_BLOCKED);
			}
			kassert(proc);

			dbg_vprintf(DBG_ELF, "Mapping segment %d\n", i);

			map_segment_mem(proc->pagedir, phdr, obj);
		}
	}

	pm_unblock(proc);
}
