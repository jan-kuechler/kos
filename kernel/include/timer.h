#ifndef TIMER_H
#define TIMER_H

#include "types.h"

#define TIMER_HZ 100
#define TIMER_DEFAULT 1193180

#define TIMER_DATA 0x43
#define TIMER_CMD  0x40

extern dword timer_ticks;

void init_timer(void);

#endif /*TIMER_H*/
