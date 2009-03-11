#ifndef IPC_H
#define IPC_H

#include <types.h>
#include <kos/msg.h>

#include "pm.h"

#define SUCCESS      0
#define INVALID_PROC 1
#define INVALID_MSG  2

int ipc_send(proc_t *from, proc_t *to, msg_t *msg);
int ipc_receive(proc_t *p, msg_t *msg, byte block);

#endif /*IPC_H*/
