#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/times.h>
#include <sys/time.h>

#include <kos/msg.h>

/* Generic syscall, send anything you want */
int32_t  syscall(int32_t calln, int32_t arg1, int32_t arg2, int32_t arg3);

/* Newlib interface */
void _exit(int status);
int wait(int *status);

int kill(int pid, int sig);

int fork();
int execve(const char *filename, char *const argv [], char *const envp[]);
int getpid();

int open(const char *name, int flags, ...);
int close(int fd);
ssize_t read(int file, void *ptr, size_t len);
ssize_t write(int file, const void *ptr, size_t len);

int link(const char *oldpath, const char *newpath);
int unlink(const char *name);
//int readlink
//int symlink

//int chown
int fstat(int fd, struct stat *st);
int stat(const char *path, struct stat *st);
int isatty(int fd);
off_t lseek(int file, off_t offs, int dir);

int gettimeofday(struct timeval *p, struct timezone *z);
clock_t times(struct tms *buf);

void *sbrk(ptrdiff_t incr);

/* kOS interface */
int open_std_files(void);

void yield(void);
void _sleep(uint32_t msec);

int send(int target, msg_t *msg);
int recv(msg_t *buf, int block);


#endif /*SYSCALLS_H*/
