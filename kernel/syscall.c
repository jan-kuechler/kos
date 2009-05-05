#include <stdlib.h>
#include <string.h>
#include <kos/error.h>
#include <kos/syscalln.h>
#include "debug.h"
#include "idt.h"
#include "ipc.h"
#include "kernel.h"
#include "pm.h"
#include "regs.h"
#include "syscall.h"
#include "timer.h"
#include "tty.h"
#include "fs/fs.h"
#include "mm/kmalloc.h"

// FIXME: does not handle user space!

// "returns" the nth argument of the call
#define sc_arg0(r) (r->ebx)
#define sc_arg1(r) (r->ecx)
#define sc_arg2(r) (r->edx)

syscall_t syscalls[SYSCALL_MAX] = {0};

void do_puts(regs_t *regs)
{
	const char *str = sc_arg0(regs);

	str = vm_user_to_kernel(cur_proc->pagedir, str, 1024);

	tty_puts(str);

	km_free_addr(str, 1024);

	sc_result(regs, 0);
}

void do_putn(regs_t *regs)
{
	int num  = sc_arg0(regs);
	int base = sc_arg1(regs);

	tty_putn(num, base);

	sc_result(regs, 0);
}

void do_exit(regs_t *regs)
{
	pm_destroy(cur_proc);
	pm_schedule();

	sc_result(regs, 0);
}

void do_yield(regs_t *regs)
{
	pm_schedule();
	sc_result(regs, 0)
}

void do_sleep(regs_t *regs)
{
	dword msec = sc_arg0(regs);

	timer_sleep(cur_proc, msec);
	sc_result(regs, 0);
}

void do_get_pid(regs_t *regs)
{
	sc_result(regs, cur_proc->pid);
}

void do_get_uid(regs_t *regs)
{
	/* Not yet implemented */
	sc_result(regs, 0);
}

void do_send(regs_t *regs)
{
	pid_t  tar   = sc_arg0(regs);
	msg_t *msg   = sc_arg1(regs);

	byte result = ipc_send(cur_proc, pm_get_proc(tar), msg);
	sc_result(regs, result);
}

void do_receive(regs_t *regs)
{
	msg_t *msg = sc_arg0(regs);
	byte block = sc_arg1(regs);

	byte status = ipc_receive(cur_proc, msg, block);

	// This may not return immediately to the calling process
  // sc_return just sets the return value.
	sc_result(regs, status);
}

void do_open(regs_t *regs)
{
	const char *fname = sc_arg0(regs);
	dword flags       = sc_arg1(regs);
	dword mode        = sc_arg2(regs);

	const char *file = fname;
	if (fname[0] != '/') { // relative path, prepend the cur working dir
		char *tmp = kmalloc(strlen(fname) + strlen(cur_proc->cwd) + 1);
		strcpy(tmp, cur_proc->cwd);
		strcpy(tmp + strlen(cur_proc->cwd), fname); //strcat(tmp, fname);
		file = tmp;
	}

	fs_handle_t *handle = fs_open(file, flags);

	if (!handle) {
		sc_result(regs, -1);
	}

	int fd=-1;
	int i=0;
	for (; i< PROC_NUM_FDS; ++i) {
		if (cur_proc->fds[i] == 0) {
			fd = i;
			break;
		}
	}

	if (fd == -1) {
		fs_close(handle);
		sc_result(regs, -1);
	}

	cur_proc->fds[fd] = handle;

	sc_result(regs, fd);
}

void do_close(regs_t *regs)
{
	int fd = sc_arg0(regs);

	if (fd < 0 || fd > PROC_NUM_FDS)
		sc_result(regs, E_INVALID_ARG);

	fs_handle_t *file = cur_proc->fds[fd];
	int res = fs_close(file);

	if (res == OK)
		cur_proc->fds[fd] = 0;

	sc_result(regs, res);
}

void do_readwrite(regs_t *regs)
{
	int fd    = sc_arg0(regs);
	char *buf = sc_arg1(regs);
	dword len = sc_arg2(regs);

	if (fd < 0 || fd > PROC_NUM_FDS)
		sc_result(regs, E_INVALID_ARG);

	fs_handle_t *handle = cur_proc->fds[fd];

	int status = fs_readwrite(handle, buf, len, regs->eax == SC_READ ? FS_READ : FS_WRITE);

	sc_result(regs, status);
}

void do_get_answer(regs_t *regs)
{
	sc_result(regs, 42);
}

void do_test(regs_t *regs)
{
	kout_printf("Test syscall from process %d (%s)\n", cur_proc->pid, cur_proc->cmdline);
}

#define MAP(calln,func) case calln: func(regs); break;

/**
 *  syscall(esp)
 *
 * Handles a syscall interrupt.
 */
void syscall(dword *esp)
{
	regs_t *regs = (regs_t*)*esp;

	cur_proc->sc_regs = regs;

	dbg_set_last_syscall(regs->eax, sc_arg0(regs),
	                     sc_arg1(regs), sc_arg2(regs));

	switch (regs->eax) {

	MAP(SC_PUTS,       do_puts)
	MAP(SC_PUTN,       do_putn)

	MAP(SC_EXIT,       do_exit)
	MAP(SC_YIELD,      do_yield)
	MAP(SC_SLEEP,      do_sleep)

	MAP(SC_GET_PID,    do_get_pid)
	MAP(SC_GET_UID,    do_get_uid)

	MAP(SC_SEND,       do_send)
	MAP(SC_RECEIVE,    do_receive)

	MAP(SC_OPEN,       do_open)
	MAP(SC_CLOSE,      do_close)
	MAP(SC_READ,       do_readwrite)
	MAP(SC_WRITE,      do_readwrite)

	MAP(SC_TEST,       do_test)

	MAP(SC_GET_ANSWER, do_get_answer)

	default: panic("Invalid syscall: %d", regs->eax);
	}

#if 0
	if (regs->eax >= SYSCALL_MAX)
		panic("Invalid syscall: %d", regs->eax);

	syscall_t call = syscalls[regs->eax];
	if (!call)
		panic("Syscall %d is not defined", regs->eax);

	call(regs);
#endif
}

void syscall_register(dword calln, syscall_t call)
{
	kassert(calln < SYSCALL_MAX);
	kassert(!syscalls[calln]);

	syscalls[calln] = call;
}
