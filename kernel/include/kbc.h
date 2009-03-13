#ifndef KBD_H
#define KBD_H

#include <types.h>

#define KBC_NORMAL 0
#define KBC_E0     1
#define KBC_E1     2

byte kbc_getkey(void);
byte kbc_scan_to_keycode(byte elvl, word scancode);

void kbc_led(byte caps, byte num, byte scroll);

void kbc_reset_cpu(void);

#endif /*KBD_H*/
