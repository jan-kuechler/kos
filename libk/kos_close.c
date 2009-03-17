#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

int kos_close(int fd)
{
	return SYSCALL1(SC_CLOSE, fd);
}
