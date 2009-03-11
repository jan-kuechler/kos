#include <bitop.h>
#include <ports.h>

#include "idt.h"
#include "timer.h"

dword timer_ticks;
dword next_wakeup;

extern proc_t procs[MAX_PROCS];

void timer_irq(int irq, dword *esp)
{
 	timer_ticks++;

	if (timer_ticks >= next_wakeup) {
		next_wakeup = 0xFFFFFFFF;

		proc_t *p = procs;
		for (; p < &procs[MAX_PROCS]; p++) {
			if (p->wakeup) {
				if(p->wakeup <= timer_ticks) {
					p->wakeup = 0;
					pm_unblock(p);
				}
				else {
					if (p->wakeup < next_wakeup)
						next_wakeup = p->wakeup;
				}
			}
		}
	}

	pm_update();
}

void timer_sleep(proc_t *proc, dword msec)
{
	if (pm_block(proc, BR_SLEEPING)) {
		dword wakeup = timer_ticks + (msec * TIMER_HZ/1000);
		proc->wakeup = wakeup;

		if (next_wakeup > wakeup)
			next_wakeup = wakeup;
	}
}

void init_timer(void)
{
	timer_ticks = 0;
	next_wakeup = 0xFFFFFFFF;

	int div = TIMER_DEFAULT / TIMER_HZ;
	outb(TIMER_DATA, 0x36);
	outb(TIMER_CMD,  bmask(div, BMASK_BYTE));
	outb(TIMER_CMD,  div >> 8);

	idt_set_irq_handler(0, timer_irq); /* 0 is the timer irq */
}
