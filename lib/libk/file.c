#include <kos/syscalln.h>
#include <kos/syscall.h>

#include "syscall_helper.h"

int open(const char *file, dword flags, ...)
{
	dword mode = 0;
	if (flags & O_CREAT) {
		int *args = ((int*)&flags) + 1;
		mode = (dword)*args;
	}

	STR_PARAM(f, file);

	return SYSCALL3(SC_OPEN, f, flags, mode);
}

int close(int fd)
{
	return SYSCALL1(SC_CLOSE, fd);
}

int read(int fd, char *buf, dword size)
{
	return SYSCALL3(SC_READ, fd, (dword)buf, size);
}

int write(int fd, const char *buf, dword size)
{
	return SYSCALL3(SC_WRITE, fd, (dword)buf, size);
}

int mount(const char *mountp, const char *type, const char *device)
{
//	STR_PARAM(mp, mountp);
//	STR_PARAM(t, type);
//	STR_PARAM(dev, device);
//
//	return SYSCALL3(SC_MOUNT, mp, t, dev);
	return -1;
}
