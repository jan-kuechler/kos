#ifndef PM_H
#define PM_H

#include <types.h>
#include <kos/config.h>
#include <kos/msg.h>

#include "regs.h"
#include "fs/types.h"
#include "mm/virt.h"

#define MAX_PROCS 256

#define KSTACK_SIZE 4096
#define USTACK_SIZE 4096

#define IDLE_PROC 0

#define PROC_START_TICKS 50
#define PROC_MAX_TICKS 100

#define PROC_MSG_BUFFER_SIZE 24 /* messages */

#define PROC_NUM_FDS 32

struct fs_handle;

typedef enum proc_status
{
	PS_SLOT_FREE = 0,
	PS_READY,
	PS_RUNNING,
	PS_BLOCKED,
} proc_status_t;

typedef enum block_reason
{
	BR_NOT_BLOCKED = 0,
	BR_RECEIVING,
	BR_SLEEPING,
	BR_WAIT_FS,
	BR_INIT,
} block_reason_t;

typedef enum proc_mode
{
	PM_KERNEL = 0,
	PM_USER,
} proc_mode_t;

typedef struct proc {
	pid_t  pid;
	pid_t  parent;

	proc_status_t  status;
	block_reason_t block;
	msg_t         *msg_wait_buffer;
	dword          wakeup;

	dword  kstack;
	dword  ustack;
	dword  esp;

	pdir_t pagedir;
	dword  pdrev;

	regs_t *sc_regs;

	inode_t *cwd;
	inode_t *fds[PROC_NUM_FDS];
	dword  numfds;

	char *cmdline;

	dword  ticks_left;

	msg_t  msg_buffer[PROC_MSG_BUFFER_SIZE];
	msg_t  *msg_head, *msg_tail;
	byte   msg_count;

#ifdef CONF_DEBUG
	int    debug;
#endif

	struct proc *next;
} proc_t;

extern proc_t *cur_proc;

#ifdef CONF_DEBUG
extern int proc_debug;
#define pm_set_debug() do { proc_debug = 1; } while (0);
#else
#define pm_set_debug() do { } while (0)
#endif

void init_pm(void);

proc_t *pm_create(void (*entry)(), const char *cmdline, proc_mode_t mode, pid_t parent, proc_status_t status);
void    pm_destroy(proc_t *proc);

proc_t *pm_get_proc(pid_t pid);

void    pm_update();
void    pm_schedule();

void    pm_activate(proc_t *proc);
void    pm_deactivate(proc_t *proc);

byte    pm_block(proc_t *proc, block_reason_t reason);
void    pm_unblock(proc_t *proc);

static inline byte pm_is_blocked_for(proc_t *proc, block_reason_t reason)
{
	return (proc->status == PS_BLOCKED && proc->block == reason);
}

void    pm_restore(dword *esp);
void    pm_pick(dword *esp);

#endif /*PM_H*/
