#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

int kos_open(const char *file, dword flags, ...)
{
	dword mode = 0;
	if (flags & O_CREAT) {
		int *args = ((int*)&flags) + 1;
		mode = (dword)*args;
	}

	return SYSCALL3(SC_OPEN, (dword)file, flags, mode);
}
