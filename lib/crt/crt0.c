#include <kos/syscall.h>

extern int main(int, char**);

unsigned int errno;

void _start(void)
{
	open("/dev/tty0", 0, 0);
	open("/dev/tty0", 0, 0);
	open("/dev/tty0", 0, 0);

	char *args = "";
	int result = main(1, &args);

	exit(result);

	for (;;);
}
