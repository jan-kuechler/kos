#include <elf.h>
#include <multiboot.h>
#include <page.h>
#include <string.h>
#include <stdarg.h>
#include <kos/syscall.h>
#include <kos/version.h>
#include "acpi.h"
#include "com.h"
#include "debug.h"
#include "gdt.h"
#include "idt.h"
#include "ipc.h"
#include "kernel.h"
#include "keymap.h"
#include "loader.h"
#include "module.h"
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
static dword sys_answer(dword, dword, dword, dword);

multiboot_info_t multiboot_info;
byte kernel_init_done;

static int in_panic = 0;

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

	extern void ksh(void);
	pm_create(ksh, "ksh", PM_KERNEL, 1, PS_READY);

	// HACK!!
	cur_proc->tty = "/dev/tty0";

	if (!strstr((char*)multiboot_info.cmdline, "nosh")) {

		do {
			pid_t pid = exec_file("/bin/sh", "/bin/sh", getpid());
			int status = waitpid(pid, NULL, 0);

			kout_printf("/bin/sh ended with status %d.\n", status);

		} while (1);
	}

	_exit(0);
}

void kmain(int mb_magic, multiboot_info_t *mb_info)
{
	kernel_init_done = 0;
	init_kout();
	init_com();

	memcpy(&multiboot_info, mb_info, sizeof(multiboot_info_t));
	init_debug();

	//kout_puts("\t\t\t\t       kOS\n");
	banner();

	kout_puts("kOS booting...\n");

	dbg_printf(DBG_LOAD, "* Setting up gdt...\n");
	init_gdt();

	dbg_printf(DBG_LOAD, "* Setting up idt...\n");
	init_idt();

	dbg_printf(DBG_LOAD, "* Setting up mm...\n");
	init_mm();

	dbg_printf(DBG_LOAD, "* Setting up paging...\n");
	init_paging();

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

	init_loader();

	syscall_register(SC_ANSWER, sys_answer);

	kout_puts("kOS booted.\n\n");

	print_info();

	pm_create(kinit, "kinit", PM_KERNEL, 0, PS_READY);

	kernel_init_done = 1;

	enable_intr();
	/* send fake timer interrupt to start scheduling */
	asm volatile ("int $0x20"); /* 0x20 is IRQ_BASE */

	for (;;)
		;
}


static void banner()
{
	int len = strlen(kos_version);
	len += 4; // strlen("kOS ");

	int linerest = 80 - len;
	int begin = linerest / 2;

	int i=0;
	for (; i < begin; ++i) {
		kout_puts(" ");
	}
	byte oc = kout_set_status(0x04);
	kout_puts("kOS ");
	kout_puts(kos_version);
	kout_puts("\n");
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

	if (multiboot_info.flags & 9) { // Bootloader
		km_identity_map((char*)multiboot_info.boot_loader_name, VM_COMMON_FLAGS, 1024);

		// hopefully the boot_load_name isn't larger than 1024 bytes...
		kout_printf("== Bootloader ==\n");
		kout_printf("%s\n", (char*)multiboot_info.boot_loader_name);
	}

	kout_printf("\n");
}

dword sys_answer(dword calln, dword arg0, dword arg1, dword arg2)
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
	while (1) {
		asm volatile("hlt");
	}
}

__attribute__((noreturn)) void panic(const char *fmt, ...)
{
	if (in_panic) {
		for (;;)
			asm("hlt");
	}

	in_panic = 1;

	kernel_init_done = 0; // interrupts won't get enabled again

	va_list args;
	va_start(args, fmt);

	kout_select();

	kout_set_status(0x04); /* white on red */
	dbg_error("Kernel Panic!\n");
	dbg_aerror(fmt, args);
	dbg_error("\n");

	if (dbg_check(DBG_PANICBT))
		dbg_stack_backtrace();

	dbg_print_last_syscall();

	while (1) {
		asm volatile("hlt");
	}
}
