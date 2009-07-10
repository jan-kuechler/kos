#include <page.h>
#include <string.h>
#include <kos/config.h>
#include "debug.h"
#include "gdt.h"
#include "idt.h"
#include "kernel.h"
#include "pm.h"
#include "syscall.h"
#include "tss.h"
#include "fs/fs.h"
#include "mm/kmalloc.h"
#include "mm/util.h"
#include "mm/mm.h"

static int koop_mode = 0;

struct proc procs[MAX_PROCS];
static struct proc *plist_head, *plist_tail;

struct proc *cur_proc = NULL;
struct proc *last_proc = NULL;

static dword *prepare_stack(dword *kstack, void (*entry)(), enum proc_mode mode)
{
	dword code_seg = mode == PM_USER ? GDT_SEL_UCODE + 0x03 : GDT_SEL_CODE; // +3 for ring 3
	dword data_seg = mode == PM_USER ? GDT_SEL_UDATA + 0x03 : GDT_SEL_DATA;

 	if (mode == PM_USER) {
		*(--kstack) = data_seg;      // ss
		*(--kstack) = USER_STACK_ADDR; // esp
	}
	*(--kstack) = 0x0202;        // eflags: 0000000100000010b -> IF
	*(--kstack) = code_seg;      // cs
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

	return kstack;
}

/**
 *  pm_create(entry, cmdline, parent)
 *
 * Creates a new process.
 */
struct proc *pm_create(void (*entry)(), const char *cmdline, proc_mode_t mode, pid_t parent, proc_status_t status)
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

	struct proc *proc = &procs[id];
	struct proc *pproc = parent ? pm_get_proc(parent) : NULL;

	proc->status = status;
	if (status != PS_BLOCKED)
		proc->block = BR_NOT_BLOCKED;
	else
		proc->block = BR_INIT;
	proc->parent = parent;

	proc->wait_proc = 0;
	proc->wait_for  = 0;
	proc->exit_status = -1;
	proc->wakeup = 0;

	proc->cmdline_mapped = 0;
	proc->cmdline = kmalloc(strlen(cmdline) + 1);
	strcpy(proc->cmdline, cmdline);

	proc->ticks_left = PROC_START_TICKS;

	proc->msg_wait_buffer = NULL;
	proc->msg_head = proc->msg_buffer;
	proc->msg_tail = proc->msg_buffer;
	proc->msg_count = 0;

	proc->tty = pproc ? pproc->tty : "/dev/tty7";
	proc->cwd = fs_root;
	memset(proc->fds, 0, PROC_NUM_FDS * sizeof(struct file *));
	proc->numfds = 0;

	proc->ldata   = NULL;
	proc->cleanup = 0;

	proc->mem_brk  = NULL;
	proc->brk_page = NULL;
	proc->num_dyn  = 0;

	proc->as = vm_create_addrspace();

	paddr_t ustackp = mm_alloc_page();
	proc->ustack_addr = ustackp;
	vm_map_page(proc->as->pdir, ustackp, (vaddr_t)(USER_STACK_ADDR - PAGE_SIZE),
	            PE_PRESENT | PE_READWRITE | PE_USERMODE);

	dword *kstack = km_alloc_page();
	proc->kstack_addr = kstack;
	kstack += (PAGE_SIZE / sizeof(dword)); // it's a dword* so += PAGE_SIZE would actualy mean += 0x4000

	kstack = prepare_stack(kstack, entry, mode);

	proc->kstack = (dword)kstack;
	proc->esp    = (dword)kstack;

	if (status == PS_READY)
		pm_activate(proc);

	return proc;
}

/**
 *  pm_destroy(proc)
 *
 * Destroys a process
 */
void pm_destroy(struct proc *proc)
{
	pm_deactivate(proc);
	proc->status = PS_SLOT_FREE;

	if (proc->wait_proc) {
		struct proc *other = pm_get_proc(proc->wait_proc);

		sc_late_result(other, proc->exit_status);
		pm_unblock(other);
	}

	if (proc->wait_for) {
		struct proc *other = pm_get_proc(proc->wait_for);
		if (other->wait_proc == proc->pid)
			other->wait_proc = 0;
	}

	if (proc->cleanup)
		proc->cleanup(proc);

	kfree(proc->cmdline);

	/* TODO: We cannot free the kstack, as that is
	 *       the stack we're working on at the moment.
	 */
	mm_free_page(proc->ustack_addr); // this one not

	vm_destroy_addrspace(proc->as);
}

/**
 *  pm_get_proc(pid)
 *
 * Returns a pointer to the process with the given id.
 */
struct proc *pm_get_proc(pid_t pid)
{
	return &procs[pid];
}

/**
 *  pm_activate(proc)
 *
 * Activates the process proc.
 */
void pm_activate(struct proc *proc)
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
void pm_deactivate(struct proc *proc)
{
	disable_intr();
	struct proc *prev = 0, *cur = plist_head;

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
	if (!cur_proc || cur_proc->status != PS_READY || (!koop_mode && (--cur_proc->ticks_left) <= 0))
		pm_schedule();
}

/**
 *  pm_schedule()
 *
 * Schedules a new process or idle
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

	cur_proc = plist_head;
	if (cur_proc) {
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
	if (cur_proc != last_proc) {
		vm_select_addrspace(cur_proc->as);

		*esp = cur_proc->esp;

		tss.esp0 = cur_proc->kstack;

		last_proc = cur_proc;
	}
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
	}

	//vm_select_addrspace(kernel_addrspace);
}

/**
 *  pm_block(proc, reason)
 *
 * Blocks the process for the given reason.
 * Returns 1 on success, 0 otherwise.
 */
byte pm_block(struct proc *proc, block_reason_t reason)
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
void pm_unblock(struct proc *proc)
{
	proc->status = PS_READY;
	proc->block  = BR_NOT_BLOCKED;

	pm_activate(proc);
}

void pm_set_koop(int mode)
{
	koop_mode = mode;
}

int pm_get_koop()
{
	return koop_mode;
}

dword sys_exit(dword calln, dword status, dword arg1, dword arg2)
{
	cur_proc->exit_status = status;
	pm_destroy(cur_proc);
	pm_schedule();
	return 0;
}

dword sys_yield(dword calln, dword arg0, dword arg1, dword arg2)
{
	pm_schedule();
	return 0;
}

dword sys_waitpid(dword calln, dword pid, dword statusptr, dword options)
{
	struct proc *other = pm_get_proc(pid);

	if (other->status == PS_SLOT_FREE || other->wait_proc != 0)
		return (dword)-1;

	other->wait_proc = cur_proc->pid;
	cur_proc->wait_for = pid;
	pm_block(cur_proc, BR_WAIT_PROC);

	return 0; // Dummy!
}

dword sys_getpid(dword calln, dword arg0, dword arg1, dword arg2)
{
	return cur_proc->pid;
}

dword sys_getcmdline(dword calln, dword arg0, dword arg1, dword arg2)
{
	if (!cur_proc->cmdline) return 0;

	paddr_t page = NULL;

	if (!cur_proc->cmdline_mapped) {
		page = mm_alloc_page();
		vm_map_page(cur_proc->as->pdir,	page, (vaddr_t)CONF_PROG_BASE_MIN - PAGE_SIZE,
		            PE_PRESENT | PE_READWRITE | PE_USERMODE);
	}

	vm_cpy_pv(page, cur_proc->cmdline, strlen(cur_proc->cmdline) + 1);

	return CONF_PROG_BASE_MIN - PAGE_SIZE;
}

static void idle()
{
	for (;;) {
		asm("hlt");
	}
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

	syscall_register(SC_EXIT,       sys_exit);
	syscall_register(SC_YIELD,      sys_yield);
	syscall_register(SC_GETPID,     sys_getpid);
	syscall_register(SC_WAITPID,    sys_waitpid);
	syscall_register(SC_GETCMDLINE, sys_getcmdline);

	/* create special process 0: idle */
	struct proc *idle_proc  = pm_create(idle, "idle", 0, 0, PS_BLOCKED);
	pm_activate(idle_proc); // Note: Does and should not unblock idle

	cur_proc = idle_proc;
}

