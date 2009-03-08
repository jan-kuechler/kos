#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <types.h>
#include <kos/msg.h>

void   kos_print(const char *str);

void   kos_exit(void);
void   kos_yield(void);

pid_t  kos_get_pid(void);
pid_t  kos_get_uid(void);

byte   kos_send(pid_t target, msg_t *msg, byte block);
byte   kos_receive(msg_t *buffer, byte block);

byte   kos_get_answer();

#endif /*SYSCALLS_H*/
