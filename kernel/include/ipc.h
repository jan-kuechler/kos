#ifndef IPC_H
#define IPC_H

#include <types.h>
#include <kos/msg.h>

#define SUCCESS      0
#define INVALID_PROC 1
#define INVALID_MSG  2

byte ipc_send(pid_t from, pid_t to, msg_t *msg, byte block);
byte ipc_receive(pid_t p, msg_t *msg, byte block);

#endif /*IPC_H*/
