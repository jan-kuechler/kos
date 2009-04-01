#ifndef SYSCALL_H
#define SYSCALL_H

#include <types.h>

void syscall(dword *esp);

typedef void (*syscall_t)(regs_t *);

void syscall_register(dword calln, syscall_t call);

#define sc_arg(n,t,r) (*((t*)((char*)&r->u_esp + ((n)*4))))
#define sc_result(v,r)  do { r->eax = v; } while (0);

#endif /*SYSCALL_H*/
