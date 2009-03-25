#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <types.h>
#include <kos/msg.h>

#define O_CREAT 0x02

// Generic syscall, send anything you want
dword  generic_syscall(int calln, dword arg1, dword arg2, dword arg3);

void   puts(const char *str);
void   putn(int num, int base);

void   exit(int status);
void   yield(void);
void   sleep(dword msec);

pid_t  get_pid(void);
pid_t  get_uid(void);

byte   send(pid_t target, msg_t *msg);
byte   receive(msg_t *buffer, byte block);

int    open(const char *file, dword mode, ...);
int    close(int fd);
int    read(int fd, char *buf, dword size);
int    write(int fd, const char *buf, dword size);

byte   get_answer();

#define kos_syscall generic_syscall
#define kos_puts    puts
#define kos_putn    putn
#define kos_exit    exit
#define kos_yield   yield
#define kos_sleep   sleep
#define kos_get_pid get_pid
#define kos_get_uid get_uid
#define kos_send    send
#define kos_receive receive
#define kos_open    open
#define kos_close   close
#define kos_read    read
#define kos_write   write
#define kos_get_answer get_answer

#endif /*SYSCALLS_H*/
