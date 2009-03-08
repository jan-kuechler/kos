#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

void kos_yield(void)
{
	SYSCALL0(SC_YIELD);
}
