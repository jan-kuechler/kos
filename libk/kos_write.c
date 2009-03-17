#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

int kos_write(int fd, const char *buf, dword size)
{
	return SYSCALL3(SC_WRITE, fd, (dword)buf, size);
}
