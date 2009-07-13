#ifndef COM_H
#define COM_H

#define COM_MAX_PORTS 4

#define COM_BAUD   38400
#define COM_PARITY 0
#define COM_BITS   8

void init_com(void);

void com_putc(int port, char c);
void com_puts(int port, const char *str);

#endif /*COM_H*/
