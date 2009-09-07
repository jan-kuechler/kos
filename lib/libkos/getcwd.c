#include <stdlib.h>
#include "helper.h"

char *getcwd(char *buffer, size_t count)
{
	return (char*)SYSCALL2(SC_GETCWD, (uint32_t)buffer, count);
}
