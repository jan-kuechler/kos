#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <types.h>

void kos_print(const char *str);

void kos_exit(void);
void kos_yield(void);

pid_t kos_get_pid(void);
pid_t kos_get_uid(void);

#endif /*SYSCALLS_H*/
