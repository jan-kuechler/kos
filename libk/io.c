#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

void kos_puts(const char *str)
{
	SYSCALL1(SC_PUTS,(dword)str);
}

void kos_putn(int num, int base)
{
	SYSCALL2(SC_PUTN, num, base);
}

