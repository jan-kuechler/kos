#ifndef IDT_H
#define IDT_H

#include "types.h"

#define IDT_GATE_PRESENT   0x80 /* 1000000b */

#define IDT_INTERRUPT_GATE 0xE /* for 32 bit */
#define IDT_TRAP_GATE      0xF /* for 32 bit */
#define IDT_TASK_GATE      0x5

#define IRQ_BASE 0x20 /* Change int.s if you change this value */
#define SYSCALL  0x30

#define PIC1      0x20
#define PIC2      0xA0

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

#define PIC_EOI   0x20

#define ICW1_ICW4	0x01		   /* ICW4 (not) needed */
#define ICW1_SINGLE	0x02		 /* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		 /* Level triggered (edge) mode */
#define ICW1_INIT	0x10       /* Initialization - required! */

#define ICW4_8086	0x01		   /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		   /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08 /* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C /* Buffered mode/master */
#define ICW4_SFNM	0x10		   /* Special fully nested (not) */


#define IDT_SIZE 256 /* Number of idt entries */

typedef struct idt_entry {
	word base_low;
	word selector;
	byte zero;
	byte type;
	word base_high;
} __attribute__((packed)) idt_entry_t;

void init_idt(void);
void idt_set_gate(int intr, word selector, void *handler,
                  byte dpl, byte type);

typedef void (*irq_handler_t)(int, dword*);

byte idt_set_irq_handler(byte irq, irq_handler_t handler);
byte idt_clr_irq_handler(byte irq);

static inline void enable_intr()
{
	/* only enables interrupts when the kernel initialization is done. */
	extern byte kernel_init_done;
	if (kernel_init_done)
		asm volatile("sti");
}

static inline void disable_intr()
{
	asm volatile("cli");
}

#endif /*IDT_H*/
