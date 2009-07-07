#ifndef SYSCALL_H
#define SYSCALL_H

#include <types.h>
#include <kos/syscalln.h>

typedef dword (*syscall_t)(dword calln, dword arg0, dword arg1, dword arg2);

void handle_syscall(dword *esp);
void syscall_register(dword calln, syscall_t call);

#define sc_late_result(proc,result) do { proc->sc_regs->eax = (result); } while (0);

#endif /*SYSCALL_H*/
