#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

void kos_exit(void)
{
	do_syscall(SC_EXIT, 0, 0);
}
