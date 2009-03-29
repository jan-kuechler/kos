#include <elf.h>
#include <multiboot.h>
#include <page.h>
#include <string.h>
#include <kos/syscall.h>
#include "acpi.h"
#include "gdt.h"
#include "idt.h"
#include "kernel.h"
#include "keymap.h"
#include "pm.h"
#include "timer.h"
#include "tty.h"
#include "mm/mm.h"
#include "mm/virt.h"
#include "fs/fs.h"
#include "fs/devfs.h"


multiboot_info_t multiboot_info;

static void print_info();

byte kernel_init_done;

extern byte elf_check(vaddr_t);
extern byte elf_check_type(vaddr_t,Elf32_Half);
extern proc_t *elf_execute(vaddr_t,const char*,byte,pid_t,byte);

void kinit()
{
	//kout_puts("Initializing ACPI:");
	//if (init_acpi() == 0)
	//	kout_puts("\tdone!\n");
	//else
	//	kout_puts("\tfailed!\n");

	kout_puts("Initializing FS:");
	init_fs();
	init_devfs();
	fs_mount(fs_get_driver("devfs"), "/dev/", 0, 0);
	init_tty();
	tty_register_keymap("de", keymap_de);
	kout_puts("\tdone!\n");

	int stdout = kos_open("/dev/tty0", 0, 0);
	if (stdout == -1) {
		kout_puts("Error opening tty0");
	}

	write(stdout, "This is /dev/tty0\n", 18);

	extern void ksh(void);
	pm_create(ksh, "ksh", 0, 1, 1);

	kos_exit(0);
}

void kmain(int mb_magic, multiboot_info_t *mb_info)
{
	kernel_init_done = 0;
	//init_console();
	init_kout();

	memcpy(&multiboot_info, mb_info, sizeof(multiboot_info_t));

	byte oc = kout_set_status(0x04);
	kout_puts("\t\t\t\t       kOS\n");
	kout_set_status(oc);
	kout_puts("kOS booting...\n");

	kout_puts("Setting up gdt:");
	init_gdt();
	kout_puts("\t\t\t\tdone!\n");

	kout_puts("Setting up idt:");
	init_idt();
	kout_puts("\t\t\t\tdone!\n");

	kout_puts("Setting up mm:");
	init_mm();
	kout_puts("\t\t\t\tdone!\n");

	init_paging();

	kout_puts("Setting up pm:");
	init_pm();
	kout_puts("\t\t\t\tdone!\n");

	kout_puts("Setting up timer:");
	init_timer();
	kout_puts("\t\t\t\tdone!\n");

	kout_puts("kOS booted.\n");

	kout_puts("\n");

	print_info();

	pm_create(kinit, "kinit", 0, 0, 1);

	kernel_init_done = 1;

	enable_intr();
	/* send fake timer interrupt to start scheduling */
	asm volatile ("int $0x20"); /* 0x20 is IRQ_BASE */

	for (;;)
		;
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
		kout_printf("Page size:   %4d  B\n", PAGE_SIZE);
		kout_printf("Kernel size: %4d KB\n", (int)(kernel_end - kernel_start) / 1024);
	}

	if (multiboot_info.flags & 9) { // Bootloader
		km_identity_map((char*)multiboot_info.boot_loader_name, VM_COMMON_FLAGS, 1024);
		// hopefully the cmdline isn't larger than 1024 bytes...
		kout_printf("== Bootloader ==\n");
		kout_printf("%s\n", (char*)multiboot_info.boot_loader_name);
	}

	kout_printf("\n");
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
	kernel_init_done = 0; // interrupts won't get enabled again

	int *args = ((int*)&fmt) + 1;

	kout_select();

	kout_set_status(0x04); /* white on red */
	kout_puts("Panic: ");
	kout_aprintf(fmt, &args);
	kout_puts("\n");
	shutdown();
}
