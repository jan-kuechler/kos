#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <types.h>

#include "pm.h"

#define TIMER_HZ 100
#define TIMER_DEFAULT 1193180

#define TIMER_DATA 0x43
#define TIMER_CMD  0x40

#define msec_to_tick(ms) ((ms) * TIMER_HZ/1000)
#define sec_to_tick(s)   ((s)  * TIMER_HZ)

extern volatile uint64_t timer_ticks;

void init_timer(void);
void timer_sleep(proc_t *proc, dword msec);

void ksleep(uint32_t msec);

#endif /*TIMER_H*/
