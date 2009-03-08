#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

byte kos_send(pid_t target, msg_t *msg, byte block)
{
	return SYSCALL3(SC_SEND,target,(dword)msg,block);
}
