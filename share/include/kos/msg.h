#ifndef MSG_H
#define MSG_H

#include <stdint.h>

typedef struct msg
{
 	uint32_t sender;
 	uint32_t cmd;
 	uint32_t subcmd;
 	uint32_t param1;
 	uint32_t param2;
} msg_t;

#endif /*MSG_H*/
