#include "gdt.h"
#include "idt.h"
#include "multiboot.h"
#include "pm.h"
#include "timer.h"

void kmain(int mb_magic, multiboot_info_t *mb_info)
{
	init_console();

	con_puts("kOS booting...\n");

	con_puts("Setting up gdt:");
	init_gdt();
	con_puts("\t\t\t\tdone!\n");

	con_puts("Setting up idt:");
	init_idt();
	con_puts("\t\t\t\tdone!\n");

	con_puts("Setting up pm:");
	init_pm();
	con_puts("\t\t\t\tdone!\n");

	con_puts("Setting up timer:");
	init_timer();
	con_puts("\t\t\tdone!\n");

	con_puts("kOS booted.\n\n");

	enable_intr();
	/* send fake timer interrupt to start scheduling */
	asm volatile ("int $0x20"); /* 0x20 is IRQ_BASE */

	for (;;)
		;
}

__attribute__((noreturn)) void panic(const char *fmt, ...)
{
	int *args = ((int*)&fmt) + 1;

	con_set_color(0x4F); /* white on red */
	con_puts("Panic: ");
	con_aprintf(fmt, &args);
	while (1) {
		asm volatile("hlt");
	}
}
