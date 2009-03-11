#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

void kos_sleep(dword msec)
{
	SYSCALL1(SC_SLEEP,msec);
}
