#include "console.h"
#include "gdt.h"
#include "kernel.h"
#include "pm.h"

proc_t procs[MAX_PROCS];
proc_t *cur_proc;
proc_t *plist_head;

dword user_stacks[MAX_PROCS][USTACK_SIZE];
dword kernel_stacks[MAX_PROCS][KSTACK_SIZE];

static void add_proc(proc_t *proc)
{
	if (!plist_head)
		plist_head = proc;

	proc->next = plist_head;
	plist_head = proc;
}

void idle()
{
	for (;;) {
		asm volatile("int $0x20");
	}
}

proc_t *pm_create(void (*entry)(), pid_t parent)
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
	procs[id].parent = parent;

	dword *ustack = user_stacks[id];
	ustack += USTACK_SIZE;

	procs[id].ustack = (dword)ustack;

	dword *kstack = kernel_stacks[id];
	kstack += KSTACK_SIZE;

	*(--kstack) = 0x10;   // ss
	*(--kstack) = (dword)ustack; // esp
	*(--kstack) = 0x0202; // eflags: 0000000100000010b -> IF
	*(--kstack) = 0x08;   // cs
	*(--kstack) = (dword)entry;  // eip

	*(--kstack) = 0; // errc
	*(--kstack) = 0; // intr

	*(--kstack) = 0x10;
	*(--kstack) = 0x10;
	*(--kstack) = 0x10;
	*(--kstack) = 0x10;

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

	add_proc(&procs[id]);

	return &procs[id];
}

void pm_schedule(dword *esp)
{
	if (cur_proc)
		cur_proc->esp = (dword)*esp;

	cur_proc = plist_head;
	while (cur_proc && cur_proc->status != PS_READY) {
		cur_proc = cur_proc->next;
	}

	if (!cur_proc)
		cur_proc = &procs[IDLE_PROC];

	*esp = cur_proc->esp;
}

void init_pm(void)
{
	int i=0;
	for (; i < MAX_PROCS; ++i) {
		procs[i].pid    = i;
		procs[i].status = PS_SLOT_FREE;
	}

	plist_head = 0;

	cur_proc = 0;

	/* create special process 0: idle */
	proc_t *idle_proc = pm_create(idle, 0);
	idle_proc->status = PS_BLOCKED;

	/* DEBUG */
	extern void task1(void);
	extern void task2(void);
	extern void task3(void);
	extern void task4(void);
	extern void task5(void);

	pm_create(task1, 0);
	pm_create(task2, 0);
	pm_create(task3, 0);
	pm_create(task4, 0);
	pm_create(task5, 0);
}

