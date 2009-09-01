#ifndef IDT_H
#define IDT_H

#include <stdbool.h>
#include <stdint.h>
#include "types.h"
#include "context.h"

#define IDT_GATE_PRESENT   0x80 /* 1000000b */

#define IDT_INTR_GATE 0xE /* for 32 bit */
#define IDT_TRAP_GATE      0xF /* for 32 bit */
#define IDT_TASK_GATE      0x5

#define IRQ_BASE 0x20 /* Change int.s if you change this value */
#define SYSCALL  0x30

#define NUM_IRQ  0x10 /* = 16dec */

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
	uint16_t base_low;
	uint16_t selector;
	uint8_t  zero;
	uint8_t  type;
	uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

void init_idt(void);
void idt_set_gate(int intr, uint16_t selector, void *handler,
                  uint8_t dpl, uint8_t type);

enum excpt_policy
{
	EP_DEFAULT,
	EP_PANIC,
	EP_ABORT,
	EP_RETRY,
	EP_GO_ON,
	EP_UNKNOWN,
};

typedef void (*irq_handler_t)(int, dword*);
typedef enum excpt_policy (*exception_handler_t)(uint32_t*);

bool idt_set_irq_handler(uint8_t irq, irq_handler_t handler);
bool idt_clr_irq_handler(uint8_t irq);

void idt_set_exception_handler(uint8_t intr, exception_handler_t handler);

void idt_reset_irq_counter(uint8_t irq);
bool idt_wait_irq(uint8_t irq, bool since_reset, uint32_t timeout);

static inline void enable_intr()
{
	if (cx_allowed(A_CHANGE_IF))
		asm volatile("sti");
}

static inline void disable_intr()
{
	if (cx_allowed(A_CHANGE_IF))
		asm volatile("cli");
}

#endif /*IDT_H*/
