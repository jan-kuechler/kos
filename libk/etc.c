#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

dword kos_syscall(int calln, dword arg1, dword arg2, dword arg3)
{
	return SYSCALL3(calln, arg1, arg2, arg3);
}

void kos_exit(int status)
{
	SYSCALL1(SC_EXIT, status);
}


void kos_sleep(dword msec)
{
	SYSCALL1(SC_SLEEP,msec);
}

void kos_yield(void)
{
	SYSCALL0(SC_YIELD);
}

byte kos_get_answer(void)
{
	return SYSCALL0(SC_GET_ANSWER);
}

