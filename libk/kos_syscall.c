#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

dword kos_syscall(int calln, dword arg1, dword arg2, dword arg3)
{
	return SYSCALL3(calln, arg1, arg2, arg3);
}
