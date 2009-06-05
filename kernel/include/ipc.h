#ifndef IPC_H
#define IPC_H

#include <types.h>
#include <kos/msg.h>
#include "pm.h"

int ipc_send(proc_t *from, proc_t *to, msg_t *msg);
int ipc_receive(proc_t *p, msg_t *msg, byte block);

void init_ipc(void);

#endif /*IPC_H*/
