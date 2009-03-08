#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

void kos_print(const char *str)
{
	SYSCALL1(SC_PRINT,(dword)str);
}
