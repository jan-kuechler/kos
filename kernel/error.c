#include <error.h>

int last_error = 0;

int geterr(void)
{
	return last_error;
}

void seterr(int err)
{
	err = err < 0 ? -err : err;
	if (err >= E_NUM_ERRORS)
		err = E_UNKNOWN;
	last_error = err;
}
