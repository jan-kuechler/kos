#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

void kos_yield(void)
{
	do_syscall(SC_YIELD, 0, 0);
}
