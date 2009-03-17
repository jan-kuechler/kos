#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "kernel.h"
#include "pm.h"
#include "tss.h"

proc_t procs[MAX_PROCS];
static proc_t *plist_head, *plist_tail;

proc_t *cur_proc;

dword user_stacks[MAX_PROCS][USTACK_SIZE];
dword kernel_stacks[MAX_PROCS][KSTACK_SIZE];

#if 0
#define debug con_printf
#else
static inline void debug(const char *fmt, ...) {}
#endif

void idle()
{
	for (;;) {
	}
}

/**
 *  pm_create(entry, cmdline, parent)
 *
 * Creates a new process.
 */
proc_t *pm_create(void (*entry)(), const char *cmdline, byte usermode, pid_t parent, byte running)
{
	int i=0;
	int id = MAX_PROCS;

	for (; i < MAX_PROCS; ++i) {
		if (procs[i].status == PS_SLOT_FREE) {
			id = i;
			break;
		}
	}

	if (id == MAX_PROCS) {
		panic("No free slot to create another process.\n");
	}

	procs[id].status = PS_READY;
	procs[id].block  = BR_NOT_BLOCKED;
	procs[id].parent = parent;

	procs[id].cmdline = cmdline; // TODO: use malloc and strcpy here!

	procs[id].ticks_left = PROC_START_TICKS;

	procs[id].msg_head = procs[id].msg_buffer;
	procs[id].msg_tail = procs[id].msg_buffer;
	procs[id].msg_count = 0;

	procs[id].cwd = "/";

	procs[id].numfds   = 0;

	procs[id].wakeup = 0;
	procs[id].msg_wait_buffer = (void*)0;

	dword *ustack = user_stacks[id];
	ustack += USTACK_SIZE;

	procs[id].ustack = (dword)ustack;

	dword *kstack = kernel_stacks[id];
	kstack += KSTACK_SIZE;

	dword code_seg = usermode ? GDT_SEL_UCODE : GDT_SEL_CODE;
	dword data_seg = usermode ? GDT_SEL_UDATA : GDT_SEL_DATA;

	*(--kstack) = data_seg;   // ss
	*(--kstack) = (dword)ustack; // esp
	*(--kstack) = 0x0202; // eflags: 0000000100000010b -> IF
	*(--kstack) = code_seg;   // cs
	*(--kstack) = (dword)entry;  // eip

	*(--kstack) = 0; // errc
	*(--kstack) = 0; // intr

	// ds-gs
	*(--kstack) = data_seg;
	*(--kstack) = data_seg;
	*(--kstack) = data_seg;
	*(--kstack) = data_seg;

	// gp registers
	*(--kstack) = 0;
	*(--kstack) = 0;
	*(--kstack) = 0;
	*(--kstack) = 0;
	*(--kstack) = 0;
	*(--kstack) = 0;
	*(--kstack) = 0;
	*(--kstack) = 0;

	procs[id].kstack = (dword)kstack;
	procs[id].esp    = (dword)kstack;

	if (running)
		pm_activate(&procs[id]);

	return &procs[id];
}

/**
 *  pm_destroy(proc)
 *
 * Destroys a process
 */
void pm_destroy(proc_t *proc)
{
//	debug("destroying %d\n", proc->pid);

	pm_deactivate(proc);
	procs[proc->pid].status = PS_SLOT_FREE;

//	debug("done!\n");
}

/**
 *  pm_get_proc(pid)
 *
 * Returns a pointer to the process with the given id.
 */
proc_t *pm_get_proc(pid_t pid)
{
	return &procs[pid];
}

/**
 *  pm_activate(proc)
 *
 * Activates the process proc.
 */
void pm_activate(proc_t *proc)
{
	disable_intr();

	if (!plist_head) {
		plist_head = proc;
	}

	if (!plist_tail) {
		plist_tail = proc;
	}
	else {
		plist_tail->next = proc;
	}

	plist_tail = proc;
	proc->next = 0;

	enable_intr();
}

/**
 *  pm_deactivate(proc)
 *
 * Deactivates the process proc.
 */
void pm_deactivate(proc_t *proc)
{
	disable_intr();
	proc_t *prev = 0, *cur = plist_head;

	while (cur && cur != proc) {
		prev = cur;
		cur  = cur->next;
	}

	if (prev) {
		prev->next = cur->next;
	}
	else {
		plist_head = cur->next;
	}

	if (proc == cur_proc)
		pm_schedule();

	enable_intr();
}

/**
 *  pm_update()
 *
 * Schedules a new process if the current one may not run any longer.
 */
void pm_update()
{
	if (!cur_proc || cur_proc->status != PS_READY || (--cur_proc->ticks_left) <= 0)
		pm_schedule();
}

/**
 *  pm_schedule()
 *
 * Schedules a new process or IDLE
 */
void pm_schedule()
{
	/*
	  Give the now-to-be-disabled process new time.
	  It may have some time from the last schedule,
	  so just add the new one.
	*/
	cur_proc->ticks_left += PROC_START_TICKS;
	if (cur_proc->ticks_left > PROC_MAX_TICKS)
		cur_proc->ticks_left = PROC_MAX_TICKS;

	if (cur_proc->status == PS_RUNNING)
		cur_proc->status = PS_READY;

	/* Very simple round robin */
	plist_tail->next = plist_head;
	plist_tail = plist_head;
	plist_head = plist_head->next;
	plist_tail->next = 0;

	if (cur_proc) {
		cur_proc = plist_head;
		cur_proc->status = PS_RUNNING;
	}
	else {
		cur_proc = &procs[IDLE_PROC];
	}
}

/**
 *  pm_pick(esp)
 *
 * Switches the stack to the current process
 */
void pm_pick(dword *esp)
{
	*esp = cur_proc->esp;
}

/**
 *  pm_restore(esp)
 *
 * Saves the stack for the current process
 */
void pm_restore(dword *esp)
{
	if (cur_proc) {
		cur_proc->esp = (dword)*esp;
		tss.esp0 = cur_proc->kstack;
	}
}

/**
 *  pm_block(proc, reason)
 *
 * Blocks the process for the given reason.
 * Returns 1 on success, 0 otherwise.
 */
byte pm_block(proc_t *proc, block_reason_t reason)
{
	if (proc->status == PS_BLOCKED)
		return 0;

	proc->status = PS_BLOCKED;
	proc->block = reason;

	pm_deactivate(proc);

	return 1;
}

/**
 *  pm_unblock(esp)
 *
 * Unblocks the process.
 */
void pm_unblock(proc_t *proc)
{
	proc->status = PS_READY;
	proc->block  = BR_NOT_BLOCKED;

	pm_activate(proc);
}

/**
 *  init_pm
 *
 * Initializes the process manager.
 */
void init_pm(void)
{
	int i=0;
	for (; i < MAX_PROCS; ++i) {
		procs[i].pid    = i;
		procs[i].status = PS_SLOT_FREE;
	}

	plist_head = 0;
	plist_tail = 0;


	/* create special process 0: idle */
	proc_t *idle_proc  = pm_create(idle, "idle", 0, 0, 1);
	idle_proc->status = PS_BLOCKED;

	cur_proc = idle_proc;

	/* DEBUG */
	extern void task1(void);
	extern void task2(void);
	extern void task3(void);
	extern void task4(void);
	extern void task5(void);

	//pm_create(task1, "task1", 0, 0, 1);
	//pm_create(task2, "task2", 0, 0, 1);
	//pm_create(task3, "task3", 0, 0, 1);
	//pm_create(task4, "task4", 0, 0, 1);
	//pm_create(task5, "task5", 0, 0, 1);
}

