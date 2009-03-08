#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

byte kos_receive(msg_t *buffer, byte block)
{
	return SYSCALL2(SC_RECEIVE,(dword)buffer, block);
}
