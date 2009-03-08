#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

void kos_print(const char *str)
{
	do_syscall(SC_PRINT, (dword)str, 0);
}
