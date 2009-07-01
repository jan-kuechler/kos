#ifndef COM_H
#define COM_H

#define COM_BAUD   38400
#define COM_PARITY 0
#define COM_BITS   8

void init_com(void);

void com_putc(int port, char c);


#endif /*COM_H*/
