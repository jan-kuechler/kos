#include <types.h>
#include "console.h"

dword do_syscall(dword calln, dword arg1, dword arg2)
{
	dword result = 0;

	asm("push %3           \n" /* arg2 */
			"push %2           \n" /* arg1 */
	    "mov  %1, %%eax    \n" /* calln */
	    "int  $0x30        \n"
	    "add  $0x08, %%esp \n" /* remove the args from the stack */
	   : "=a"(result)
	   : "r"(calln), "r"(arg1), "r"(arg2)
	   );

	 return result;
}
