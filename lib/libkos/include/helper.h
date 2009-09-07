#ifndef SYSCALL_HELPER_H
#define SYSCALL_HELPER_H

#include <stdint.h>
#include <kos/strparam.h>
#include <kos/syscalln.h>
#include <string.h>

extern int32_t do_syscall(int32_t, int32_t, int32_t, int32_t);

#define SYSCALL0(n)       do_syscall(n, 0, 0, 0)
#define SYSCALL1(n,a)     do_syscall(n, a, 0, 0)
#define SYSCALL2(n,a,b)   do_syscall(n, a, b, 0)
#define SYSCALL3(n,a,b,c) do_syscall(n, a, b, c)

#define STR_PARAM(var, str)\
	struct strparam DATA_##var; \
	uint32_t var; \
	var = (uint32_t)&DATA_##var; \
	DATA_##var.len = strlen(str); \
	DATA_##var.ptr = (char*)str;

#endif /*SYSCALL_HELPER_H*/
