#ifndef MSG_H
#define MSG_H

#include <types.h>

typedef struct msg
{
 	pid_t sender;
 	dword cmd;
 	dword subcmd;
 	dword param1;
 	dword param2;
} msg_t;

#endif /*MSG_H*/
