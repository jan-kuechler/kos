#include <stdlib.h>
#include <string.h>
#include <kos/syscalln.h>
#include "debug.h"
#include "idt.h"
#include "ipc.h"
#include "kernel.h"
#include "regs.h"
#include "syscall.h"
#include "util/list.h"

#define sc_arg1(r) ((r)->ebx)
#define sc_arg2(r) ((r)->ecx)
#define sc_arg3(r) ((r)->edx)

struct syscall_data
{
	syscall_func func;
	uint32_t     args;
};

struct proc *syscall_proc;

static int32_t sys_test();
struct syscall_data syscalls[NUM_SYSCALLS] = {
	{sys_test, 0},
};

int32_t sys_test()
{
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

	struct syscall_info *info = kmalloc(sizeof(*info));
	info->regs = regs;
	info->proc = cur_proc;

	//list_add_back(syscall_list, info);

	/* COMPAT */
	syscall_execute(info);
	kfree(info);
	/* END */
}

void syscall_execute(struct syscall_info *info)
{
	dbg_set_last_syscall(info->regs->eax, sc_arg1(info->regs),
	                     sc_arg2(info->regs), sc_arg3(info->regs));

	if (info->regs->eax >= NUM_SYSCALLS) {
		dbg_error("Invalid syscall %d by %s (%d)\n", info->regs->eax,
		          info->proc->cmdline, info->proc->pid);
		pm_destroy(info->proc);
		return;
	}

	syscall_proc = info->proc;
	syscall_proc->sc_regs = info->regs;

	syscall_func func = syscalls[info->regs->eax].func;
	if (!func) {
		dbg_error("Syscall %d is not implemented yet.\n", info->regs->eax);
		pm_destroy(syscall_proc);

		return;
	}

	int32_t result = 0;
	switch (syscalls[info->regs->eax].args) {
	case 0:
		result = func();
		break;

	case 1:
		result = func(sc_arg1(info->regs));
		break;

	case 2:
		result = func(sc_arg1(info->regs), sc_arg2(info->regs));
		break;

	case 3:
		result = func(sc_arg1(info->regs), sc_arg2(info->regs), sc_arg3(info->regs));
		break;

	default:
		panic("Too many arguments for syscall %d!\n", info->regs->eax);
	};

	dbg_printf(DBG_SC, "Syscall done, result is 0x%x\n", result);
	info->regs->eax = result;
}

void syscall_register(uint32_t id, syscall_func func, uint32_t nargs)
{
	kassert(id < NUM_SYSCALLS);
	kassert(!syscalls[id].func);
	kassert(nargs <= SC_MAX_ARGS);

	syscalls[id].func = func;
	syscalls[id].args = nargs;
}
