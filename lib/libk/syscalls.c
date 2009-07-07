#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/times.h>
//#include <sys/errno.h>
#include <sys/time.h>

#ifdef LIBK

#include <kos/strparam.h>
#include <kos/syscalln.h>
#include "helper.h"

#undef errno
int errno;

#else

#include "syscalln.h"

struct kos_strparam
{
	unsigned int len;
	const char *ptr;
};

static volatile int do_syscall(int calln, int arg1, int arg2, int arg3)
{
	volatile int result = 0;

	asm volatile (
		 "int  $0x30"
	   : "=a"(result)
	   : "a"(calln), "b"(arg1), "c"(arg2), "d"(arg3)
	   );

	return result;
}

#define SYSCALL0(n)       do_syscall(n, 0, 0, 0)
#define SYSCALL1(n,a)     do_syscall(n, a, 0, 0)
#define SYSCALL2(n,a,b)   do_syscall(n, a, b, 0)
#define SYSCALL3(n,a,b,c) do_syscall(n, a, b, c)

#define STR_PARAM(var, str)              \
	static struct kos_strparam DATA_##var; \
	int var = (int)&DATA_##var;            \
	DATA_##var.len = strlen(str);          \
	DATA_##var.ptr = (char*)str;

char **environ; /* pointer to array of char * strings that define the current environment variables */

#undef errno
extern int errno;

#endif /* LIBK */

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
	errno=ENOMEM;
	return -1;
}

int fork()
{
	errno=EAGAIN;
	return -1;
}

int fstat(int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int getpid()
{
	return SYSCALL0(SC_GETPID);
}

int isatty(int file)
{
	return 1;
}

int kill(int pid, int sig)
{
	errno=EINVAL;
	return(-1);
}

int link(const char *oldpath, const char *newpath)
{
	errno=EMLINK;
	return -1;
}

off_t lseek(int file, off_t offs, int dir)
{
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

int gettimeofday(struct timeval *p, struct timezone *z)
{
	return -1;
}

/* generic syscall */

