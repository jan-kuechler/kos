#ifndef TTY_H
#define TTY_H

#include <types.h>
#include "keymap.h"
#include "pm.h"
#include "fs/devfs.h"
#include "fs/fs.h"
#include "fs/request.h"

#define NUM_TTYS 8

#define TTY_INBUF_SIZE 256
#define TTY_SCREEN_X    80
#define TTY_SCREEN_Y    25
#define TTY_SCREEN_SIZE (TTY_SCREEN_X * TTY_SCREEN_Y)

#define TTY_ECHO 0x01 // 00000001
#define TTY_RAW  0x02 // 00000010

typedef struct tty
{
	fs_devfile_t file; // this _must_ be the first entry in the struct! (see query in tty.c)

	int   id;

	char  inbuf[TTY_INBUF_SIZE];
	dword incount;
	dword eotcount;

	word  outbuf[TTY_SCREEN_SIZE];

	byte  x, y;
	byte  status;

	byte  flags;

	fs_request_t **rqs;
	int            rqcount;

	int    opencount;
	proc_t *owner;
} tty_t;

void init_tty(void);

void tty_set_cur_term(byte n);
byte tty_get_cur_term(void);

void tty_register_keymap(const char *name, keymap_t map);
int  tty_select_keymap(const char *name);

#endif /*TTY_H*/
