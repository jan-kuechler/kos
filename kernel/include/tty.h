#ifndef TTY_H
#define TTY_H

#include <stdarg.h>
#include <stdint.h>
#include <types.h>
#include "keymap.h"
#include "pm.h"
#include "fs/types.h"
#include "util/list.h"

#define CON_MEM_START 0xB8000
#define CON_MEM_END   0xC0000
#define CON_MEM_SIZE  (CON_MEM_END - CON_MEM_START)
#define CON_MEM_CHARS (CON_MEM_SIZE / 2)

#define CON_MAX_START_LINE 175
#define CON_MAX_LINE       200

#define CGA_PORT_CMD  0x3D4
#define CGA_PORT_DATA 0x3D5

enum cga_register
{
	CGA_START_HI  = 12,
	CGA_START_LO  = 13,
	CGA_CURSOR_HI = 14,
	CGA_CURSOR_LO = 15,
};

#define NUM_TTYS 8

#define TTY_INBUF_SIZE 256
#define TTY_SCREEN_X    80
#define TTY_SCREEN_Y    25
#define TTY_SCREEN_SIZE (TTY_SCREEN_X * TTY_SCREEN_Y)

#define TTY_ECHO 0x01 // 00000001
#define TTY_RAW  0x02 // 00000010

typedef struct tty
{
	int   id;

	struct inode inode;

	char  inbuf[TTY_INBUF_SIZE];
	dword incount;
	dword eotcount;

	uint16_t outbuf[CON_MEM_CHARS];
	uint8_t outstart;
	uint8_t lastline;

	uint8_t x, y;
	uint8_t screenY;
	uint8_t status;

	uint8_t flags;

	list_t *requests;

	int    opencount;
	struct proc *owner;
} tty_t;

#define kout_id (NUM_TTYS-1)

void init_kout(void);
void kout_puts(const char *str);
void kout_putn(int num, int base);
void kout_aprintf(const char *fmt, va_list args);
void kout_printf(const char *fmt, ...);
byte kout_set_status(byte status);
void kout_select(void);
void kout_clear(void);

void init_tty(void);

void tty_set_cur_term(byte n);
byte tty_get_cur_term(void);

void tty_register_keymap(const char *name, keymap_t map);
int  tty_select_keymap(const char *name);

int tty_isatty(struct file *file);

void tty_puts(const char *str);
void tty_putn(int num, int base);

#endif /*TTY_H*/
