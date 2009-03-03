#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"

#define CON_PTRINF_HEX_0X

#define CON_SCREEN_WIDTH 80
#define CON_SCREEN_HEIGHT 25

#define CON_TAB_WIDTH 8

#define CON_INBUFFER_SIZE 512 /* chars */
#define CON_SCREENBUFFER_SIZE (CON_SCREEN_WIDTH * CON_SCREEN_HEIGHT) /* words */

#define CON_NUM_VC 6

typedef struct console {
	byte  id;

	char *inbuffer[CON_INBUFFER_SIZE];
	dword in_avail;

	word  screenbuffer[CON_SCREENBUFFER_SIZE];
	dword x, y;
	byte  color;
} console_t;

void init_console(void);

void con_select(dword id);

void con_flush();
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
