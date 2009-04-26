#include "syscall.h"
#include "fs/fs.h"
#include "intern.h"


dword sys_open(dword calln, dword fname, dword flags, dword arg2)
{
	return 0;
}

dword sys_close(dword calln, dword fd, dword arg1, dword arg2)
{
	return 0;
}

dword sys_read(dword calln, dword fd, dword buffer, dword size)
{
	return 0;
}

dword sys_write(dword calln, dword fd, dword buffer, dword size)
{
	return 0;
}

void init_fs(void)
{
	syscall_register(SC_OPEN,  sys_open);
	syscall_register(SC_CLOSE, sys_close);
	syscall_register(SC_READ,  sys_read);
	syscall_register(SC_WRITE, sys_write);

	init_devfs();
}
