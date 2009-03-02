#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"

#define CON_PTRINF_HEX_0X

#define CON_TAB_WIDTH 8

void init_console(void);

void con_clear_screen(void);
void con_set_cursor_pos(unsigned int x, unsigned int y);
void con_set_hw_cursor(void);

byte con_set_color(byte clr);

void con_putc(const char c);
void con_putn(unsigned long long n, int r, int pad, char padc);
void con_puts(const char *s);

void con_printf(const char *fmt, ...);
void con_aprintf(const char *fmt, int **args);

#define con_putl(str) con_puts(str "\n");

#endif /* CONSOLE_H */
