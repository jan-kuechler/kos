#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

byte kos_get_answer(void)
{
	SYSCALL0(SC_GET_ANSWER);
}
