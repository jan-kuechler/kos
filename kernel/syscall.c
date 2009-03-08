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

void syscall(dword *esp)
{
	regs_t *regs = (regs_t*)*esp;

	switch (regs->eax) {
	case SC_PRINT:   do_print(regs);   break;

	case SC_EXIT:    do_exit(regs);    break;
	case SC_YIELD:   do_yield(regs);   break;

	case SC_GET_PID: do_get_pid(regs); break;
	case SC_GET_UID: do_get_uid(regs); break;

	default: panic("Invalid syscall: %d", regs->eax);
	}
}
