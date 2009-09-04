#include <elf.h>
#include <multiboot.h>
#include <page.h>
#include <string.h>
#include <stdarg.h>
#include <kos/syscall.h>
#include <kos/version.h>
#include "acpi.h"
#include "cdi_driver.h"
#include "com.h"
#include "context.h"
#include "debug.h"
#include "gdt.h"
#include "idt.h"
#include "ipc.h"
#include "kernel.h"
#include "keymap.h"
#include "loader.h"
#include "module.h"
#include "params.h"
#include "pci.h"
#include "pm.h"
#include "syscall.h"
#include "timer.h"
#include "tss.h"
#include "tty.h"
#include "mm/mm.h"
#include "mm/virt.h"
#include "fs/fs.h"
#include "fs/devfs.h"
#include "fs/initrd.h"

static void banner();
static void print_info();
static int32_t sys_answer();

static bool print_sysinfo = false;
static bool print_pciinfo = false;
static bool dump_stack    = false;
static bool run_cdi       = false;
static char init_file[256] = "/bin/sh";

multiboot_info_t multiboot_info;

static int initfile(char *val)
{
	strncpy(init_file, val, 255);
	init_file[255] = '\0';

	dbg_printf(DBG_LOAD, "initfile is now %s.\n", val ? val : "NULL");

	return 0;
}
BOOT_PARAM("init", initfile);

static int noinit(char *val)
{
	init_file[0] = '\0';
	return 0;
}
BOOT_PARAM("noinit", noinit);

BOOT_FLAG(sysinfo, print_sysinfo, true);
BOOT_FLAG(pciinfo, print_pciinfo, true);
BOOT_FLAG(stackdump, dump_stack, true);
BOOT_FLAG(cdi, run_cdi, true);

static void kinit_fs(void)
{
	kout_puts("Initializing FS:");
	init_fs();
	init_initrd();
	init_devfs();

	int err = vfs_mount(vfs_gettype("initrd"), fs_root, NULL, FSM_READ);
	if (err != 0) {
		kout_printf("Error mounting initrd: %d\n", err);
	}

	struct inode *dev = vfs_lookup("/dev", fs_root);
	if (!dev) {
		kout_printf("/dev was not created... (%d)\n", vfs_geterror());
	}
	else {
		err = vfs_mount(vfs_gettype("devfs"), vfs_lookup("/dev", fs_root), NULL, FSM_READ | FSM_WRITE);
		if (err != 0) {
			kout_printf("Error mounting devfs: %d\n", err);
		}
	}

	init_tty();
	init_com_devices();
	tty_register_keymap("de", keymap_de);
	kout_puts("\tdone!\n");
}

void kinit()
{
	kinit_fs();

	kout_puts("\n");
	if (print_sysinfo)
		print_info();
	if (print_pciinfo)
		pci_print_table();

	if (run_cdi)
		cdi_run_drivers();

	// HACK!!
	cur_proc->tty = "/dev/tty0";

	if (init_file[0]) {
		do {
			pid_t pid = exec_file(init_file, init_file, getpid());
			if (!pid) {
				kout_printf("Error! Cannot execute %s\n", init_file);
				strcpy(init_file, "/bin/sh"); /* default to shell */
			}
			else {
				int status = waitpid(pid, NULL, 0);

				kout_printf("/bin/sh ended with status %d.\n", status);
			}
		} while (1);
	}
	else {
		kout_printf("No init!\n");
	}

	_exit(0);
}

static void init_cdi_driver()
{
	dbg_printf(DBG_LOAD, "* Loading CDI drivers...\n");

	const char *ata_args[] = {
		"ata", "nodam"
	};
	init_ata(2, ata_args);
}

void kmain(int mb_magic, multiboot_info_t *mb_info)
{
	init_kout();
	init_com();

	init_debug();

	memcpy(&multiboot_info, mb_info, sizeof(multiboot_info_t));

	//kout_puts("\t\t\t\t       kOS\n");
	banner();

	parse_params((char*)multiboot_info.cmdline, (struct parameter*)init_param_start,
	              num_init_params, false);

	kout_puts("kOS booting...\n");

	dbg_printf(DBG_LOAD, "* Setting up gdt...\n");
	init_gdt();

	dbg_printf(DBG_LOAD, "* Setting up idt...\n");
	init_idt();

	dbg_printf(DBG_LOAD, "* Setting up mm...\n");
	init_mm();

	dbg_printf(DBG_LOAD, "* Setting up paging...\n");
	init_paging();

	cx_set(CX_INIT);

	dbg_printf(DBG_LOAD, "* Setting up stack backtrace...\n");
	init_stack_backtrace();

	dbg_printf(DBG_LOAD, "* Setting up pm...\n");
	init_pm();

	dbg_printf(DBG_LOAD, "* Setting up timer...\n");
	init_timer();

	dbg_printf(DBG_LOAD, "* Setting up IPC...\n");
	init_ipc();

	dbg_printf(DBG_LOAD, "* Setting up module support...\n");
	init_mod();

	dbg_printf(DBG_LOAD, "* Setting up pci devices...\n");
	init_pci();

	dbg_printf(DBG_LOAD, "* Setting up loader...\n");
	init_loader();

	syscall_register(SC_ANSWER, sys_answer, 0);

	if (run_cdi)
		init_cdi_driver();

	cx_set(CX_INIT_DONE);
	kout_puts("kOS booted.\n\n");

	pm_create(kinit, "kinit", PM_KERNEL, 0, PS_READY);

	enable_intr();
	/* send fake timer interrupt to start scheduling */
	asm volatile ("int $0x20");

	for (;;) asm("hlt");
}

static void banner()
{
	/* Banner: kos *version* *name* (*builddate*) */
	int len = 4 + strlen(kos_version) + 3 + strlen(kos_builddate);

	int linerest = 80 - len;
	int begin = linerest / 2;

	int i=0;
	for (; i < begin; ++i) {
		kout_puts(" ");
	}
	byte oc = kout_set_status(0x04);
	kout_puts("kOS ");
	kout_puts(kos_version);
	kout_puts(" (");
	kout_puts(kos_builddate);
	kout_puts(")\n");
	kout_set_status(oc);
}

#define cpuid(op, a, b, c, d) asm("mov $" #op ", %%eax\ncpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):)

static void print_info()
{
	kout_puts("System information:\n");

	{ // CPU
		dword unused;
		dword vendor[4] = {0};
		cpuid(0, unused, vendor[0], vendor[2], vendor[1]);

		dword signature = 0;
		cpuid(1, signature, unused, unused, unused);
		byte model, family, type, stepping;
		stepping = signature         & 0xF;
		model    = (signature >> 4)  & 0xF;
		family   = (signature >> 8)  & 0xF;
		type     = (signature >> 12) & 0x3;

		kout_puts("== CPU ==\n");
		kout_printf("%s Family %d Model %d Stepping %d Type %d\n", (char*)vendor, family, model, stepping, type);
	}

	{ // Memory
		kout_puts("== Memory ==\n");
		kout_printf("Total:       %4d MB\n", mm_total_mem() / 1024 / 1024);
		kout_printf("Free:        %4d MB\n", (mm_num_free_pages() * 4) / 1024);
		kout_printf("Kernel size: %4d KB\n", (int)(kernel_end - kernel_start) / 1024);
	}

	kout_puts("\n");
}

int32_t sys_answer()
{
	return 42;
}

static inline dword get_cr2()
{
	dword val = 0;
	asm volatile("mov %%cr2, %0" : "=r"(val) :);
	return val;
}

void print_state(regs_t *regs)
{
	dbg_error("cs:eip: %06x:%p ss:esp: %06x:%p\n",
	          regs->cs, regs->eip,regs->u_ss, regs->u_esp);
	dbg_error("error code: %016b cr2: %010x\n",
	          regs->errc, get_cr2());
	dbg_error("eax: %p ebx: %p ecx: %p edx: %p\n",
	          regs->eax, regs->ebx, regs->ecx, regs->edx);
	dbg_error("ebp: %p esp: %p esi: %p edi: %p\n",
	          regs->ebp, regs->esp, regs->esi, regs->edi);
	dbg_error("eflags: %010x ds: %06x es: %06x fs: %06x gs: %06x\n",
	          regs->eflags, regs->ds, regs->es, regs->fs, regs->gs);

	dbg_error("Current process: '%s' (%d)\n", cur_proc->cmdline, cur_proc->pid);
}

__attribute__((noreturn)) void shutdown()
{
	disable_intr();
	kout_select();
	kout_clear();
	for (;;) asm("hlt");
}

__attribute__((noreturn)) void panic(const char *fmt, ...)
{
	if (cx_get() == CX_PANIC) {
		/* recusive panic, just kill the machine */
		for (;;) asm("hlt");
	}

	disable_intr();
	cx_set(CX_PANIC);

	va_list args;
	va_start(args, fmt);

	kout_select();

	kout_set_status(0x04); /* white on red */
	dbg_error("Kernel Panic!\n");
	dbg_aerror(fmt, args);
	dbg_error("\n");

	if (dump_stack)
		dbg_stack_backtrace();

	dbg_print_last_syscall();

	for (;;) asm ("hlt");
}
