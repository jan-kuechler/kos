#ifndef KBD_H
#define KBD_H

#include <types.h>

void init_kbd(void);

byte kbd_echo(byte flag);

void kbd_read(char *buf, dword num);

#endif /*KBD_H*/
