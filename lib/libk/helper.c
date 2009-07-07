#include <stdint.h>

int32_t do_syscall(int32_t calln, int32_t arg1, int32_t arg2, int32_t arg3)
{
	volatile int32_t result = 0;

	asm volatile (
		 "int  $0x30"
	   : "=a"(result)
	   : "a"(calln), "b"(arg1), "c"(arg2), "d"(arg3)
	   );

	return result;
}
