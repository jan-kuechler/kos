#include <elf.h>
#include <stdarg.h>
#include <string.h>
#include <types.h>
#include <kos/config.h>
#include "kernel.h"
#include "pm.h"
#include "tty.h"
#include "mm/virt.h"

#ifdef CONF_DEBUG
dword dbg_lsc_calln;
dword dbg_lsc_arg0;
dword dbg_lsc_arg1;
dword dbg_lsc_arg2;
pid_t dbg_lsc_proc;
#endif

static Elf32_Shdr *symtab;
static Elf32_Shdr *strtab;

static byte dbg_flags[26];

static inline Elf32_Sym *find_sym(dword addr)
{
	if (!symtab)
		return 0;

	Elf32_Sym *sym = (Elf32_Sym*)symtab->sh_addr;
	int i=0;
	for (; i < symtab->sh_size / sizeof(Elf32_Sym); ++i) {
		if (addr >= sym[i].st_value && addr < sym[i].st_value + sym[i].st_size)
			return sym + i;
	}

	return 0;
}

static inline const char *get_str(Elf32_Word index)
{
	if (!strtab)
		return 0;
	return (const char*)strtab->sh_addr + index;
}

static inline void print_sym(dword ebp, dword eip)
{
	Elf32_Sym *sym = find_sym(eip);
	kout_printf("ebp 0x%08x eip 0x%08x", ebp, eip);
	if (sym) {
		kout_printf(" <%s + 0x%x>", get_str(sym->st_name), eip - sym->st_value);
	}
	kout_puts("\n");
}

void dbg_stack_backtrace_ex(dword ebp, dword eip)
{
	struct stack_frame
	{
		struct stack_frame *ebp;
		dword  eip;
	} *stack_frame;

	kout_puts("Stack backtrace:\n");

	if (ebp != 0) {
		print_sym(ebp, eip);
		stack_frame = (struct stack_frame*)ebp;
	}
	else {
		asm volatile("mov %%ebp, %0" : "=r"(stack_frame));
	}

	for (; vm_is_mapped(cur_proc->pagedir, (vaddr_t)stack_frame, sizeof(struct stack_frame), PE_PRESENT);
	       stack_frame = stack_frame->ebp)
	{
		print_sym((dword)stack_frame->ebp, stack_frame->eip);
	}

	if (stack_frame && !vm_is_mapped(cur_proc->pagedir, (vaddr_t)stack_frame, sizeof(struct stack_frame), PE_USERMODE)) {
		kout_puts("Stack broken!\n");
	}
}

void dbg_stack_backtrace(void)
{
	dbg_stack_backtrace_ex(0, 0);
}

void dbg_proc_backtrace(proc_t *proc)
{
	proc = proc ? proc : cur_proc;


}

void init_stack_backtrace(void)
{
	int hdrcount = multiboot_info.u.elf_sec.num;
	Elf32_Shdr *elfshdr = (Elf32_Shdr*)multiboot_info.u.elf_sec.addr;
	km_identity_map(elfshdr, VM_COMMON_FLAGS, hdrcount * sizeof(Elf32_Shdr));

	symtab = 0;
	strtab = 0;

	int i=0;
	for (; i < hdrcount; ++i) {
		if (elfshdr[i].sh_type == SHT_SYMTAB) {
			symtab = &elfshdr[i];
			strtab = &elfshdr[symtab->sh_link];
			break;
		}
	}

	if (symtab && strtab) {
		km_identity_map((paddr_t)symtab->sh_addr, VM_COMMON_FLAGS, symtab->sh_size);
		km_identity_map((paddr_t)strtab->sh_addr, VM_COMMON_FLAGS, strtab->sh_size);
	}
}

void init_debug(void)
{
	memset(dbg_flags, 0, 26);

#ifdef CONF_DEBUG
	const char *opts = strstr((char*)multiboot_info.cmdline, "debug=");
	if (!opts)
		return;

	opts += 6;

	while (*opts && *opts != ' ') {
		char o = *opts++;

		if (o >= 'a' && o <= 'z')
			dbg_flags[o - 'a'] = 1;
		else if (o >= 'A' && o <= 'Z')
			dbg_flags[o - 'A'] = 2;
	}
#endif
}

int dbg_check(char flag)
{
	return dbg_flags[flag - 'a'] > 0;
}

int dbg_verbose(char flag)
{
	return dbg_flags[flag- 'a'] == 2;
}

void dbg_printf(char flag, const char *fmt, ...)
{
	if (dbg_check(flag)) {
		va_list args;
		va_start(args, fmt);

		byte old = kout_set_status(0x02);
		kout_aprintf(fmt, args);
		kout_set_status(old);
	}
}

void dbg_vprintf(char flag, const char *fmt, ...)
{
	if (dbg_verbose(flag)) {
		va_list args;
		va_start(args, fmt);

		byte old = kout_set_status(0x0A);
		kout_aprintf(fmt, args);
		kout_set_status(old);
	}
}
