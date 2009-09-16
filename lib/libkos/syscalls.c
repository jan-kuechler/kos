#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/times.h>
#include <sys/time.h>

#include <kos/strparam.h>
#include <kos/syscalln.h>
#include "helper.h"

#undef errno
int errno;

void _exit(int status)
{
	SYSCALL1(SC_EXIT, status);
	for (;;); /* noreturn... */
}

int close(int file)
{
	return SYSCALL1(SC_CLOSE, file);
}

int execve(const char *filename, char *const argv [], char *const envp[])
{
	STR_PARAM(filename_p, filename);

	SYSCALL3(SC_EXECVE, filename_p, (int)argv, (int)envp);

	errno=ENOMEM;
	return -1;
}

int fork()
{
	return SYSCALL0(SC_FORK);
}

int fstat(int file, struct stat *st)
{
	SYSCALL2(SC_STAT, file, (int)st);
	st->st_mode = S_IFCHR;
	return 0;
}

int getpid()
{
	return SYSCALL0(SC_GETPID);
}

int isatty(int file)
{
	return SYSCALL1(SC_ISATTY, file);
}

int kill(int pid, int sig)
{
	SYSCALL2(SC_KILL, pid, sig);
	errno=EINVAL;
	return(-1);
}

int link(const char *oldpath, const char *newpath)
{
	STR_PARAM(old_p, oldpath);
	STR_PARAM(new_p, newpath);
	SYSCALL2(SC_LINK, old_p, new_p);
	errno=EMLINK;
	return -1;
}

off_t lseek(int file, off_t offs, int dir)
{
	SYSCALL3(SC_LSEEK, file, offs, dir);
	return 0;
}

int open(const char *name, int flags, ...)
{
	int mode = 0;

	if (flags & O_CREAT) {
		va_list args;
		va_start(args, flags);

		mode = va_arg(args, int);

		va_end(args);
	}

	/*static struct kos_strparam name_param;
	name_param.len = strlen(name);
	name_param.ptr = name;*/

	STR_PARAM(namep, name);

	if (namep == 0)
		return -1;

	//do_syscall(SC_TEST, DATA_namep.len, (int)DATA_namep.ptr, namep);

	return SYSCALL3(SC_OPEN, namep, flags, mode);
}

ssize_t read(int file, void *ptr, size_t len)
{
	return SYSCALL3(SC_READ, file, (int)ptr, len);
}

void *sbrk(ptrdiff_t incr)
{
	return (void *)SYSCALL1(SC_SBRK, incr);
}

int stat(const char *path, struct stat *sbuf)
{
	STR_PARAM(path_p, path);
	SYSCALL2(SC_STAT, path_p, (int)sbuf);
	sbuf->st_mode = S_IFCHR;
	return 0;
}

clock_t times(struct tms *buf)
{
	return 0;
}

int unlink(const char *name)
{
	errno=ENOENT;
	return -1;
}

int wait(int *status)
{
	errno=ECHILD;
	return -1;
}

ssize_t write(int file, const void *ptr, size_t len)
{
		return SYSCALL3(SC_WRITE, file, (int)ptr, len);
}

int gettimeofday(struct timeval *p, void *z)
{
	return -1;
}
