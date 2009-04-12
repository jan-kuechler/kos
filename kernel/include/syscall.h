#ifndef SYSCALL_H
#define SYSCALL_H

#include <types.h>

void syscall(dword *esp);

typedef void (*syscall_t)(dword calln, dword arg0, dword arg1, dword arg2);

void syscall_register(dword calln, syscall_t call);

#define sc_arg0(r) (r->ebx)
#define sc_arg1(r) (r->ecx)
#define sc_arg2(r) (r->edx)
#define sc_result(r, v)  do { r->eax = v; } while (0);

#endif /*SYSCALL_H*/
