#include <stdlib.h>
#include <kos/error.h>
#include <kos/syscalln.h>
#include "console.h"
#include "idt.h"
#include "pm.h"
#include "regs.h"
#include "fs/fs.h"


#define arg(n,type) (*((type*)((char*)&regs->u_esp + ((n)*4))))

#define sc_return(v) {regs->eax = v; return;}

void do_print(regs_t *regs)
{
	const char *str = arg(0, const char *);

	con_puts(str);

	sc_return(0);
}

void do_exit(regs_t *regs)
{
	pm_destroy(cur_proc);
	pm_schedule();

	sc_return(0);
}

void do_yield(regs_t *regs)
{
	pm_schedule();
	sc_return(0)
}

void do_sleep(regs_t *regs)
{
	dword msec = arg(0, dword);

	timer_sleep(cur_proc, msec);
	sc_return(0);
}

void do_get_pid(regs_t *regs)
{
	sc_return(cur_proc->pid);
}

void do_get_uid(regs_t *regs)
{
	/* Not yet implemented */
	sc_return(0);
}

void do_send(regs_t *regs)
{
	pid_t  tar   = arg(0, pid_t);
	msg_t *msg   = arg(1, msg_t*);

	byte result = ipc_send(cur_proc, pm_get_proc(tar), msg);
	sc_return(result);
}

void do_receive(regs_t *regs)
{
	msg_t *msg = arg(0, msg_t*);
	byte block = arg(1, byte);

	byte status = ipc_receive(cur_proc, msg, block);

	// This may not return immediately to the calling process
  // sc_return just sets the return value.
	sc_return(status);
}

void do_open(regs_t *regs)
{
	const char *fname = arg(0, const char *);
	dword flags       = arg(1, dword);
	dword mode        = arg(2, dword);

	const char *file = fname;
	if (fname[0] != '/') { // relative path, prepend the cur working dir
		char *tmp = malloc(strlen(fname) + strlen(cur_proc->cwd) + 1);
		strcpy(tmp, cur_proc->cwd);
		strcpy(tmp + strlen(cur_proc->cwd), fname); //strcat(tmp, fname);
		file = tmp;
	}

	fs_handle_t *handle = fs_open(file, flags);

	if (!handle) {
		sc_return(-1);
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
		sc_return(-1);
	}

	cur_proc->fds[fd] = handle;

	sc_return(fd);
}

void do_close(regs_t *regs)
{
	int fd = arg(0, int);

	if (fd < 0 || fd > PROC_NUM_FDS)
		sc_return(E_INVALID_ARG);

	fs_handle_t *file = cur_proc->fds[fd];
	int res = fs_close(file);

	if (res == OK)
		cur_proc->fds[fd] = 0;

	sc_return(res);
}

void do_readwrite(regs_t *regs)
{
	int fd    = arg(0, int);
	char *buf = arg(1, char*);
	dword len = arg(2, dword);

	if (fd < 0 || fd > PROC_NUM_FDS)
		sc_return(E_INVALID_ARG);

	fs_handle_t *handle = cur_proc->fds[fd];

	int status = fs_readwrite(handle, buf, len, regs->eax == SC_READ ? FS_READ : FS_WRITE);

	sc_return(status);
}

void do_get_answer(regs_t *regs)
{
	sc_return(42);
}

#define MAP(calln,func) case calln: func(regs); break;

void syscall(dword *esp)
{
	regs_t *regs = (regs_t*)*esp;

	cur_proc->sc_regs = regs;

	switch (regs->eax) {

	MAP(SC_PRINT,      do_print)

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

	MAP(SC_GET_ANSWER, do_get_answer)

	default: panic("Invalid syscall: %d", regs->eax);
	}
}
