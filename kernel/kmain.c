#include "gdt.h"
#include "idt.h"
#include "kernel.h"
#include "mm.h"
#include <multiboot.h>
#include <page.h>
#include "pm.h"
#include "timer.h"

multiboot_info_t multiboot_info;

static void print_info();

void kmain(int mb_magic, multiboot_info_t *mb_info)
{
	init_console();

	memcpy(&multiboot_info, mb_info, sizeof(multiboot_info_t));

	byte oc = con_set_color(0x04);
	con_puts("\t\t\t\t       kOS\n");
	con_set_color(oc);
	con_puts("kOS booting...\n");

	con_puts("Setting up gdt:");
	init_gdt();
	con_puts("\t\t\t\tdone!\n");

	con_puts("Setting up idt:");
	init_idt();
	con_puts("\t\t\t\tdone!\n");

	con_puts("Setting up mm:");
	init_mm();
	con_puts("\t\t\t\tdone!\n");

	con_puts("Setting up pm:");
	init_pm();
	con_puts("\t\t\t\tdone!\n");

	con_puts("Setting up timer:");
	init_timer();
	con_puts("\t\t\tdone!\n");

	con_puts("kOS booted.\n\n");

	print_info();

	enable_intr();
	/* send fake timer interrupt to start scheduling */
	asm volatile ("int $0x20"); /* 0x20 is IRQ_BASE */

	for (;;)
		;
}

#define cpuid(op, a, b, c, d) asm("mov $" #op ", %%eax\ncpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):)

static void print_info()
{
	con_puts("System information:\n");

	{ // CPU
		dword unused;
		dword vendor[4] = {0};
		cpuid(0, unused, vendor[0], vendor[2], vendor[1]);

		dword signature = 0;
		cpuid(1, signature, unused, unused, unused);
		byte model, family, type, brand, stepping;
		stepping = signature        & 0xF;
		model    = (signature >> 4) & 0xF;
		family   = (signature >> 8) & 0xF;
		type     = (signature >> 12) & 0x3;

		con_puts("== CPU ==\n");
		con_printf("%s Family %d Model %d Stepping %d Type %d\n", vendor, family, model, stepping, type);
	}

	{ // Memory
		con_puts("== Memory ==\n");
		con_printf("Total:       %4d MB\n", mm_total_mem());
		con_printf("Free:        %4d MB\n", (mm_num_free_pages() * 4) / 1024);
		con_printf("Page size:   %4d  B\n", PAGE_SIZE);
		con_printf("Kernel size: %4d KB\n", (kernel_end - kernel_start) / 1024);
	}

	if (multiboot_info.flags & 9) { // Bootloader
		con_printf("== Bootloader ==\n");
		con_printf("%s\n", multiboot_info.boot_loader_name);
	}

	con_printf("\n");
}

__attribute__((noreturn)) void panic(const char *fmt, ...)
{
	int *args = ((int*)&fmt) + 1;

	con_set_color(0x4F); /* white on red */
	con_puts("Panic: ");
	con_aprintf(fmt, &args);
	con_putc('\n');
	while (1) {
		asm volatile("hlt");
	}
}
