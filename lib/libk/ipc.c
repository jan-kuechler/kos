#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

byte send(pid_t target, msg_t *msg)
{
	//return SYSCALL2(SC_SEND,target,(dword)msg);
	return 0;
}

byte receive(msg_t *buffer, byte block)
{
	//return SYSCALL2(SC_RECEIVE,(dword)buffer, block);
	return 0;
}
