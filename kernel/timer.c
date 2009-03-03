#include "bitop.h"
#include "idt.h"
#include "ports.h"
#include "timer.h"
#include "pm.h"

dword timer_ticks;

void timer_irq(int irq, dword *esp)
{
 	timer_ticks++;

	pm_update(esp);
}

void init_timer(void)
{
	timer_ticks = 0;

	int div = TIMER_DEFAULT / TIMER_HZ;
	outb(TIMER_DATA, 0x36);
	outb(TIMER_CMD,  bmask(div, BMASK_BYTE));
	outb(TIMER_CMD,  div >> 8);

	idt_set_irq_handler(0, timer_irq); /* 0 is the timer irq */
}
