#include <elf.h>
#include <errno.h>
#include <libutil.h>
#include <stdarg.h>
#include <string.h>
#include <types.h>
#include <kos/config.h>
#include "debug.h"
#include "com.h"
#include "gdt.h"
#include "idt.h"
#include "kernel.h"
#include "params.h"
#include "pm.h"
#include "tty.h"
#include "mm/virt.h"

#define COM_DBG 0

#define CLR_ERR  0x04
#define CLR_DBG  0x02
#define CLR_VDBG 0x0A

#define PAGE_FAULT 14
#define GENERAL_PROTECTION_FAULT 13

#ifdef CONF_DEBUG
dword dbg_lsc_calln;
dword dbg_lsc_arg0;
dword dbg_lsc_arg1;
dword dbg_lsc_arg2;
pid_t dbg_lsc_proc;

const char *dbg_lsc_name[] = {
	"test",
	"exit",
	"wait",
	"kill",
	"fork",
	"execve",
	"getpid",
	"open",
	"close",
	"read",
	"write",
	"link",
	"unlink",
	"readlink",
	"symlink",
	"chown",
	"fstat",
	"stat",
	"isatty",
	"lseek",
	"gettimeofday",
	"times",
	"sbrk",
	0
};
#endif

static int com_loglvl = 1;

static Elf32_Shdr *symtab;
static Elf32_Shdr *strtab;

static byte dbg_flags[26] = {0};

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
	dbg_error("ebp 0x%08x eip 0x%08x", ebp, eip);
	if (sym) {
		dbg_error(" <%s + 0x%x>", get_str(sym->st_name), eip - sym->st_value);
	}
	dbg_error("\n");
}

void dbg_stack_backtrace_ex(dword ebp, dword eip)
{
	struct stack_frame
	{
		struct stack_frame *ebp;
		dword  eip;
	} *stack_frame;

	dbg_error("Stack backtrace:\n");

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
		dbg_error("Stack corrupted!\n");
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

static inline uint32_t get_cr2()
{
	dword val = 0;
	asm volatile("mov %%cr2, %0" : "=r"(val) :);
	return val;
}

static enum excpt_policy pf_handler(uint32_t *esp)
{
	enum {
		PROT  = 0x01,
		WRITE = 0x02,
		USER  = 0x04,
		RSVD  = 0x08,
		INSTR = 0x10,
	};

	regs_t *regs = (regs_t*)*esp;

	kout_select();
	dbg_error("Page Fault in %s mode.\n", IS_USER(regs->ds) ? "user" : "kernel");

	print_state(regs);

	dbg_error("%s while %s in %s mode%s%s\n",
	  (regs->errc & PROT) ? "Protection fault" : "Page n/a",
	  (regs->errc & WRITE) ? "writing" : "reading",
	  (regs->errc & USER)  ? "user" : "kernel",
		(regs->errc & RSVD) ? " (reserved bit in PDE)" : "",
		(regs->errc & INSTR) ? " (during instruction fetch)" : "."
	);

	uint32_t cr2 = get_cr2();
	dbg_error("Virtual address: %p\n", cr2);
	if (cr2 < 1024) { /* NULL pointer access or structure at NULL */
		dbg_error("Hint: Thou shalt not follow the NULL pointer, for chaos and madness await thee at its end.\n");
	}
	else if (cr2 < USER_SPACE_START && (regs->errc & USER)) {
		dbg_error("Hint: Tztztz... Bad user! The kernel lives here... and it bites.\n");
	}

	return EP_DEFAULT;
}

static enum excpt_policy gpf_handler(uint32_t *esp)
{
	enum {
		TBL_MASK = 0x0006, /* bits 2 & 3 */
		IDX_MASK = 0xFFF8, /* high 13 bits of a word */
	};
	enum {
		EXTERN = 0x01,
	};
	enum {
		GDT  = 0x0,
		IDT1 = 0x1,
		LDT  = 0x2,
		IDT2 = 0x3,
	};

	regs_t *regs = (regs_t*)*esp;

	kout_select();
	dbg_error("General Protection Fault in %s mode.\n", IS_USER(regs->ds) ? "user" : "kernel");

	print_state(regs);

	bool external = regs->errc & EXTERN;
	uint8_t tab = (regs->errc & TBL_MASK) >> 1;
	uint16_t index = (regs->errc & IDX_MASK) >> 3;

	dbg_error("Selector %x in %s%s\n", index,
		tab == GDT ? "GDT" : (tab == LDT ? "LDT" : "IDT"),
		external ? " (external origin)." : "."
	);

	return EP_DEFAULT;
}

void init_debug(void)
{
	idt_set_exception_handler(PAGE_FAULT, pf_handler);
	idt_set_exception_handler(GENERAL_PROTECTION_FAULT, gpf_handler);
}

static int parse_debug(char *opts)
{
	while (*opts) {
		char o = *opts++;

		if (o >= 'a' && o <= 'z')
			dbg_flags[o - 'a'] = 1;
		else if (o >= 'A' && o <= 'Z')
			dbg_flags[o - 'A'] = 2;
	}
	return 0;
}
BOOT_PARAM("debug", parse_debug);

static int parse_com(char *com)
{
	if (!com) return -EINVAL;
	if (*com >= '0' && *com <= '9') {
		com_loglvl = *com - '0';
	}
	return 0;
}
BOOT_PARAM("com", parse_com);

int dbg_check(char flag)
{
	return dbg_flags[flag - 'a'] > 0;
}

int dbg_verbose(char flag)
{
	return dbg_flags[flag- 'a'] == 2;
}

void dbg_aerror(const char *fmt, va_list args)
{
	static char buffer[256];

	strafmt(buffer, fmt, args);

	if (com_loglvl >= 1) {
		com_puts(COM_DBG, buffer);
	}

	byte old = kout_set_status(CLR_ERR);
	kout_puts(buffer);
	kout_set_status(old);

}

void dbg_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	dbg_aerror(fmt, args);

	va_end(args);
}

void dbg_printf(char flag, const char *fmt, ...)
{
	static char buffer[256];

	va_list args;
	va_start(args, fmt);

	strafmt(buffer, fmt, args);
	va_end(args);

	if (dbg_check(flag) || com_loglvl >= 2) {
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
	va_end(args);

	if (dbg_verbose(flag) || com_loglvl >= 3) {
		com_puts(COM_DBG, buffer);
	}

	if (dbg_verbose(flag)) {
		byte old = kout_set_status(CLR_VDBG);
		kout_puts(buffer);
		kout_set_status(old);
	}
}
