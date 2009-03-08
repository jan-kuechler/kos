#include "console.h"
#include "idt.h"
#include "pm.h"
#include "regs.h"
#include <kos/syscalln.h>

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
	byte   block = arg(2, byte);

	byte result = ipc_send(cur_proc->pid, tar, msg, block);
	sc_return(result);
}

void do_receive(regs_t *regs)
{
	msg_t *msg = arg(0, msg_t*);
	byte block = arg(1, byte);

	byte status = ipc_receive(cur_proc->pid, msg, block);

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

	switch (regs->eax) {

	MAP(SC_PRINT,      do_print)

	MAP(SC_EXIT,       do_exit)
	MAP(SC_YIELD,      do_yield)

	MAP(SC_GET_PID,    do_get_pid)
	MAP(SC_GET_UID,    do_get_uid)

	MAP(SC_SEND,       do_send)
	MAP(SC_RECEIVE,    do_receive)

	MAP(SC_GET_ANSWER, do_get_answer)

	default: panic("Invalid syscall: %d", regs->eax);
	}
}
