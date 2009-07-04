#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

dword generic_syscall(int calln, dword arg1, dword arg2, dword arg3)
{
	return SYSCALL3(calln, arg1, arg2, arg3);
}

void exit(int status)
{
	SYSCALL1(SC_EXIT, status);
}

int wait(pid_t pid)
{
	return SYSCALL1(SC_WAIT, pid);
}

void sleep(dword msec)
{

}
