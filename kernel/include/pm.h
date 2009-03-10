#ifndef PM_H
#define PM_H

#include <types.h>

#include "ipc.h"
#include "regs.h"

#define MAX_PROCS 256

#define KSTACK_SIZE 1024
#define USTACK_SIZE 1024

#define IDLE_PROC 0

#define PROC_START_TICKS 50
#define PROC_MAX_TICKS 100

#define PROC_MSG_BUFFER_SIZE 24 /* messages */

#define PF_RECEIVING 0x01
#define PF_SENDING   0x02

typedef enum proc_status {
	PS_SLOT_FREE = 0,
	PS_READY,
	PS_RUNNING,
	PS_BLOCKED,
} proc_status_t;

typedef struct proc {
	pid_t  pid;
	pid_t  parent;

	proc_status_t status;
	byte   flags;

	dword  ustack;
	//dword  kstack;
	//dword  esp;
	regs_t *kstack;
	regs_t *esp;

	const char *cmdline;

	dword  ticks_left;

	msg_t  msg_buffer[PROC_MSG_BUFFER_SIZE];
	msg_t  *msg_head, *msg_tail;
	byte   msg_count;

	struct proc *next;
} proc_t;

extern proc_t *cur_proc;

void init_pm(void);

proc_t *pm_create(void (*entry)(), const char *cmdline, byte usermode, pid_t parent);
void    pm_destroy(proc_t *proc);

proc_t *pm_get_proc(pid_t pid);

void    pm_update();
void    pm_schedule();

void    pm_activate(proc_t *proc);
void    pm_deactivate(proc_t *proc);

void    pm_restore(dword *esp);
void    pm_pick(dword *esp);

#endif /*PM_H*/
