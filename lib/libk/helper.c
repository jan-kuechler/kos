#include <types.h>

dword do_syscall(dword calln, dword arg1, dword arg2, dword arg3)
{
	dword result = 0;

	asm("int  $0x30"
	   : "=a"(result)
	   : "a"(calln), "b"(arg1), "c"(arg2), "d"(arg3)
	   );

	return result;
}
