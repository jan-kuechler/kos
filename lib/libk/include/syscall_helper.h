#ifndef SYSCALL_HELPER_H
#define SYSCALL_HELPER_H

#include <types.h>
#include <kos/strparam.h>
#include <string.h>

extern dword do_syscall(dword, dword, dword, dword);

#define SYSCALL0(n)       do_syscall(n, 0, 0, 0)
#define SYSCALL1(n,a)     do_syscall(n, a, 0, 0)
#define SYSCALL2(n,a,b)   do_syscall(n, a, b, 0)
#define SYSCALL3(n,a,b,c) do_syscall(n, a, b, c)

#define STR_PARAM(var, str)       \
	struct strparam DATA_##var;   \
	dword var;                    \
	var = (dword)&DATA_##var;     \
	DATA_##var.len = strlen(str); \
	DATA_##var.ptr = (char*)str;  \

#endif /*SYSCALL_HELPER_H*/
