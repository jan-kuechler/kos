#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <types.h>
#include <kos/msg.h>

#define O_CREAT 0x02

// Generic syscall, send anything you want
dword  kos_syscall(int calln, dword arg1, dword arg2, dword arg3);

void   kos_puts(const char *str);
void   kos_putn(int num, int base);

void   kos_exit(int status);
void   kos_yield(void);
void   kos_sleep(dword msec);

pid_t  kos_get_pid(void);
pid_t  kos_get_uid(void);

byte   kos_send(pid_t target, msg_t *msg);
byte   kos_receive(msg_t *buffer, byte block);

int    kos_open(const char *file, dword mode, ...);
int    kos_close(int fd);
int    kos_read(int fd, char *buf, dword size);
int    kos_write(int fd, const char *buf, dword size);

byte   kos_get_answer();

#endif /*SYSCALLS_H*/
