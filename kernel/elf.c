#include <elf.h>
#include <types.h>
#include "pm.h"

byte elf_check(vaddr_t start)
{
	Elf32_Ehdr *header = (Elf32_Ehdr*)start;
	if (header->e_ident[EI_MAG0] != ELFMAG0 ||
	    header->e_ident[EI_MAG1] != ELFMAG1 ||
	    header->e_ident[EI_MAG2] != ELFMAG2 ||
	    header->e_ident[EI_MAG3] != ELFMAG3)
	{
		return 0;
	}
	return 1;
}

byte elf_check_type(vaddr_t start, Elf32_Half type)
{
	Elf32_Ehdr *header = (Elf32_Ehdr*)start;
	return (header->e_type == type);
}

proc_t *elf_execute(vaddr_t start, const char *cmdline, byte user, pid_t parent, byte running)
{
	Elf32_Ehdr *header = (Elf32_Ehdr*)start;

	return pm_create((void*)header->e_entry, cmdline, user, parent, running);
}
