#include <elf.h>
#include <format.h>
#include <stdarg.h>
#include <string.h>
#include <types.h>
#include <kos/config.h>
#include "com.h"
#include "kernel.h"
#include "pm.h"
#include "tty.h"
#include "mm/virt.h"

#define COM_ALL  0
#define COM_ERR  1
#define COM_DBG  2
#define COM_VDBG 3

#define CLR_ERR  0x04
#define CLR_DBG  0x02
#define CLR_VDBG 0x0A

#ifdef CONF_DEBUG
dword dbg_lsc_calln;
dword dbg_lsc_arg0;
dword dbg_lsc_arg1;
dword dbg_lsc_arg2;
pid_t dbg_lsc_proc;
#endif

static int com_loglvl = COM_ERR;

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

	for (; vm_is_mapped(cur_proc->as->pdir, (vaddr_t)stack_frame, sizeof(struct stack_frame), PE_PRESENT);
	       stack_frame = stack_frame->ebp)
	{
		print_sym((dword)stack_frame->ebp, stack_frame->eip);
	}

	if (stack_frame && !vm_is_mapped(cur_proc->as->pdir, (vaddr_t)stack_frame, sizeof(struct stack_frame), PE_USERMODE)) {
		kout_puts("Stack corrupted!\n");
	}
}

void dbg_stack_backtrace(void)
{
	dbg_stack_backtrace_ex(0, 0);
}

void dbg_proc_backtrace(proc_t *proc)
{
	proc = proc ? proc : cur_proc;

	regs_t *regs = (regs_t*)(proc->esp);
	dbg_stack_backtrace_ex(regs->ebp, regs->eip);
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
	if (opts) {
		opts += 6;

		while (*opts && *opts != ' ') {
			char o = *opts++;

			if (o >= 'a' && o <= 'z')
				dbg_flags[o - 'a'] = 1;
			else if (o >= 'A' && o <= 'Z')
				dbg_flags[o - 'A'] = 2;
		}
	}

	const char *com = strstr((char*)multiboot_info.cmdline, "com=");
	if (com) {
		com += 4;

		if (*com >= '0' && *com <= '9') {
			com_loglvl = *com - '0';
		}
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

void dbg_error(const char *fmt, ...)
{
	static char buffer[256];

	va_list args;
	va_start(args, fmt);

	strafmt(buffer, fmt, args);

	if (com_loglvl >= COM_ERR) {
		com_puts(COM_ALL, buffer);
		com_puts(COM_ERR, buffer);
	}

	byte old = kout_set_status(CLR_ERR);
	kout_puts(buffer);
	kout_set_status(old);
}

void dbg_printf(char flag, const char *fmt, ...)
{
	static char buffer[256];

	va_list args;
	va_start(args, fmt);

	strafmt(buffer, fmt, args);

	if (com_loglvl >= COM_DBG) {
		com_puts(COM_ALL, buffer);
		com_puts(COM_DBG, buffer);
	}
	if (dbg_check(flag)) {
		byte old = kout_set_status(CLR_DBG);
		kout_puts(buffer);
		kout_set_status(old);
	}
}

void dbg_vprintf(char flag, const char *fmt, ...)
{
	static char buffer[256];

	va_list args;
	va_start(args, fmt);

	strafmt(buffer, fmt, args);

	if (com_loglvl >= COM_VDBG) {
		com_puts(COM_ALL, buffer);
		com_puts(COM_VDBG, buffer);
	}

	if (dbg_verbose(flag)) {
		byte old = kout_set_status(CLR_VDBG);
		kout_puts(buffer);
		kout_set_status(old);
	}
}
