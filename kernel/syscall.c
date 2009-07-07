#include <stdlib.h>
#include <string.h>
#include <kos/error.h>
#include <kos/syscalln.h>
#include "debug.h"
#include "idt.h"
#include "ipc.h"
#include "kernel.h"
#include "regs.h"
#include "syscall.h"
#include "tty.h"
#include "fs/fs.h"
#include "mm/kmalloc.h"
#include "mm/util.h"

#define sc_arg0(r) ((r)->ebx)
#define sc_arg1(r) ((r)->ecx)
#define sc_arg2(r) ((r)->edx)
#define sc_result(r, v)  do { (r)->eax = (v); } while (0);

static dword sys_testcall(dword, dword, dword, dword);

syscall_t syscalls[NUM_SYSCALLS] = {
	sys_testcall, 0
};

dword sys_testcall(dword calln, dword arg0, dword arg1, dword arg2)
{
//	kout_printf("Test syscall from %s with args:\n1: %d\n2: 0x%x\n3: %p\n",
//	            cur_proc->cmdline, arg0, arg1, arg2);

	return 0;
}

/**
 *  syscall(esp)
 *
 * Handles a syscall interrupt.
 */
void handle_syscall(dword *esp)
{
	regs_t *regs = (regs_t*)*esp;

	cur_proc->sc_regs = regs;

	dbg_set_last_syscall(regs->eax, sc_arg0(regs),
	                     sc_arg1(regs), sc_arg2(regs));

	if (regs->eax >= NUM_SYSCALLS)
		panic("Invalid syscall: %d", regs->eax);

	syscall_t call = syscalls[regs->eax];
	if (call) {
		dword res = call(regs->eax, sc_arg0(regs),
		                 sc_arg1(regs), sc_arg2(regs));
		sc_result(regs, res);
		return;
	}

	dbg_error("Syscall %d is not implemented yet.\n", regs->eax);
	pm_destroy(cur_proc);
}

void syscall_register(dword calln, syscall_t call)
{
	kassert(calln < NUM_SYSCALLS);
	kassert(!syscalls[calln]);

	syscalls[calln] = call;
}
