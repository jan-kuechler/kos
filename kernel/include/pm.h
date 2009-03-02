#ifndef PM_H
#define PM_H

#include "regs.h"
#include "types.h"

#define MAX_PROCS 256

#define KSTACK_SIZE 1024
#define USTACK_SIZE 1024

#define IDLE_PROC 0

typedef enum proc_status {
	PS_SLOT_FREE = 0,
	PS_READY,
	PS_BLOCKED,
	PS_ZOMBIE,
} proc_status_t;

typedef struct proc {
	pid_t  pid;
	pid_t  parent;

	proc_status_t status;

	dword  kstack;
	dword  ustack;
	dword  esp;

	struct proc *next;
} proc_t;

void init_pm(void);

proc_t *pm_create(void (*entry)(), pid_t parent);
void    pm_schedule(dword *esp);

#endif /*PM_H*/
