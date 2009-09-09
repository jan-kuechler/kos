#include <stdlib.h>
#include <sys/utsname.h>
#include "helper.h"

int uname(struct utsname *name)
{
	if (!name) return -1;

	int err = SYSCALL2(SC_UNAME, (uint32_t)name, L_uname);

	if (!err) {
		return 0;
	}

	return -1;
}
