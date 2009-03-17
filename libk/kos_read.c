#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

int kos_read(int fd, char *buf, dword size)
{
	return SYSCALL3(SC_READ, fd, (dword)buf, size);
}
