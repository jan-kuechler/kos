#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

void puts(const char *str)
{
	//SYSCALL1(SC_PUTS,(dword)str);
}

void putn(int num, int base)
{
	//SYSCALL2(SC_PUTN, num, base);
}

