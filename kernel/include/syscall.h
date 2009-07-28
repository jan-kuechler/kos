#ifndef SYSCALL_H
#define SYSCALL_H

#include <types.h>
#include <stdint.h>
#include <kos/syscalln.h>

#define SC_MAX_ARGS 3

struct regs;
struct proc;

struct syscall_info
{
	struct regs *regs;
	struct proc *proc;
};

typedef int32_t (*syscall_func)();

struct list;
extern struct list *syscall_list;
extern struct proc *syscall_proc;

void handle_syscall(dword *esp);
void syscall_execute(struct syscall_info *info);
void syscall_register(uint32_t id, syscall_func func, uint32_t nargs);

#define sc_late_result(proc,result) do { proc->sc_regs->eax = (result); } while (0);

#endif /*SYSCALL_H*/
