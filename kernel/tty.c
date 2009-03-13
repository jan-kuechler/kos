#include <bitop.h>
#include <ports.h>
#include <stdlib.h>
#include <kos/error.h>

#include "idt.h"
#include "kbc.h"
#include "keycode.h"
#include "keymap.h"
#include "tty.h"

#define CPOS(t) (t->x + t->y * TTY_SCREEN_X)

static char *names[NUM_TTYS] = {
	"tty0", "tty1",	"tty2",	"tty3",
	"tty4", "tty5", "tty6", "tty7"
};
static tty_t ttys[NUM_TTYS];
static tty_t *cur_tty;

typedef struct
{
	const char *name;
	keymap_t    map;
} keymap_holder_t;
static keymap_holder_t *keymaps;
static int              nummaps;
static keymap_t         cur_map;

static byte lshift;
static byte rshift;
static byte lalt;
static byte ralt;
static byte capslock;
static byte numlock;

static word *vmem = (word*)0xB8000;

static struct {
	byte shift;
	byte ctrl;
	byte alt;
	byte altgr;
} modifiers;

/**
 *  scroll(tty, lines)
 */
static void scroll(tty_t *tty, int lines)
{
	dword offs = lines * TTY_SCREEN_X * 2;
	dword count = (TTY_SCREEN_Y - lines) * TTY_SCREEN_X * 2;
	dword ncount = lines * TTY_SCREEN_X * 2;

	if (tty == cur_tty) {
		memmove(vmem, vmem + offs, count);
		memset(vmem + count, 0, ncount);
	}

	memmove(tty->outbuf, tty->outbuf + offs, count);
	memset(tty->outbuf + count, 0, ncount);
	tty->y -= lines;
}

/**
 *  flush(tty)
 */
static inline void flush(tty_t *tty)
{
	if (tty == cur_tty) {
		memcpy(vmem, tty->outbuf, TTY_SCREEN_SIZE * 2);
	}
}

/**
 *  clear(tty)
 */
static inline void clear(tty_t *tty)
{
	memset(tty->outbuf, 0, TTY_SCREEN_SIZE * 2); // TTY_SCREEN_SIZE is in words
}

/**
 *  update_cursor(tty)
 */
static inline void update_cursor(tty_t *tty)
{
	if (tty == cur_tty) {
		word pos = CPOS(tty);

		outb(0x3D4, 15);
		outb(0x3D4, bmask(pos, BMASK_BYTE));
		outb(0x3D4, 14);
		outb(0x3D4, bmask(pos >> 8, BMASK_BYTE));
	}
}

/**
 *  putc(tty, c)
 */
static void putc(tty_t *tty, char c)
{
	switch (c) {
	case '\n':
		tty->x = 0;
		tty->y++;
		break;

	case '\r':
		tty->x = 0;
		break;

	case '\b':
		if (tty->x >= 1) {
			tty->x--;
			tty->outbuf[CPOS(tty)] = ' ' | (tty->status << 8);
			if (tty == cur_tty)
				vmem[CPOS(tty)] = ' ' | (tty->status << 8);
		}
		break;

	case '\t':
		do {
			putc(tty, ' ');
		} while (tty->x % 7);
		break;

	default:
		tty->outbuf[CPOS(tty)] = c | (tty->status << 8);
		if (tty == cur_tty)
			vmem[CPOS(tty)] = c | (tty->status << 8);
		tty->x++;
	}

	if (tty->x >= TTY_SCREEN_X) {
		tty->y++;
		tty->x = 0;
	}

	if (tty->y >= TTY_SCREEN_Y) {
		scroll(tty, tty->y - TTY_SCREEN_Y + 1);
	}

	update_cursor(tty);
}

static void answer_rq(tty_t *tty, fs_request_t *rq)
{
	// Note: This function does not check anything.
	//       Be sure you know what you're doing!

	byte remove_rq = 0;

	if (!rq) {
		rq = tty->rqs[0];
		remove_rq = 1;
	}

	/* copy the data to the buffer */
	memcpy(rq->buf, tty->inbuf, rq->buflen);
	/* update the inbuffer */
	memmove(tty->inbuf, tty->inbuf + rq->buflen, tty->incount - rq->buflen);
	tty->incount -= rq->buflen;
	/* and finish the rq */
	rq->result = rq->buflen;
	fs_finish_rq(rq);

	if (remove_rq) {
		memmove(tty->rqs, tty->rqs + 1, (tty->rqcount - 1) * sizeof(fs_request_t*));
		tty->rqcount--;
		tty->rqs = realloc(tty->rqs, tty->rqcount * sizeof(fs_request_t*));
	}
}


/**
 *  open(tty, rq)
 */
static void open(tty_t *tty, fs_request_t *rq)
{
	if (!tty->owner && !tty->opencount)
		tty->owner = rq->proc;

	if (tty->owner == rq->proc)
		tty->opencount++;

	fs_finish_rq(rq);
}

/**
 *  close(tty, rq)
 */
static void close(tty_t *tty, fs_request_t *rq)
{
	if (tty->owner == rq->proc)
		tty->opencount--;

	if (!tty->opencount)
		tty->owner = 0;

	fs_finish_rq(rq);
}

/**
 *  read(tty, rq)
 */
static void read(tty_t *tty, fs_request_t *rq)
{
	if ((!tty->rqcount) && (rq->buflen <= tty->incount)) { /* fullfill request */
		answer_rq(tty, rq);
	}
	else { /* store request */
		tty->rqs = realloc(tty->rqs, sizeof(fs_request_t*) * (++tty->rqcount));
		tty->rqs[tty->rqcount-1] == rq;
	}
}

/**
 *  write(tty, rq)
 */
static void write(tty_t *tty, fs_request_t *rq)
{
	int i=0;
	char *buffer = rq->buf;
	for (; i < rq->buflen; ++i) {
		putc(tty, buffer[i]);
	}

	rq->result = rq->buflen;
	fs_finish_rq(rq);
}

/**
 *  query(file, rq)
 */
static int query(fs_devfile_t *file, fs_request_t *rq)
{
	tty_t *tty = (tty_t*)file; // the file is just the first position in the tty_t struct

	switch (rq->type) {
	case RQ_OPEN:
		open(tty, rq);
		break;

	case RQ_CLOSE:
		close(tty, rq);
		break;

	case RQ_READ:
		read(tty, rq);
		break;

	case RQ_WRITE:
		write(tty, rq);
		break;

	default:
		fs_finish_rq(rq);
		return E_NOT_SUPPORTED;
	}

	return OK;
}

static void puts(tty_t *tty, const char *str)
{
	while (*str)
		putc(tty, *str++);
}

static void tty_dbg_info(tty_t *tty)
{
	putc(tty, '\n');
	puts(tty, "== TTY Debug Info ==\n");
	puts(tty, "   Id: ");
	putc(tty, '0' + tty->id);
	putc(tty, '\n');

	puts(tty, "   Modifiers: ");
	if (modifiers.shift)
		puts(tty, "shift ");
	if (modifiers.alt)
		puts(tty, "alt ");
	if (modifiers.altgr)
		puts(tty, "altgr ");
	if (modifiers.ctrl)
		puts(tty, "ctrl ");
	putc(tty, '\n');

}

/** following code handles the keyboard irq **/
static inline byte get_keycode(byte *brk)
{
	static byte e0 = 0; // set to 1 if the prev was 0xE0
	static byte e1 = 0; // set to 1 if the prev was 0xE1 or to 2 when the prev-prev was 0xE1
	static word e1_data = 0;

	byte data = kbc_getkey();

	if ((data & 0x80) &&
	    (e1 || (data != 0xE1)) &&
	    (e0 || (data != 0xE0)))
	{
		*brk = 1;
		data &= ~0x80;
	}
	else {
		*brk = 0;
	}

	if (e0) {
		e0 = 0;
		return kbc_scan_to_keycode(KBC_E0, data);
	}
	else if (e1 == 2) {
		e1_data |= ((word)data << 8);
		e1 = 0;
		return kbc_scan_to_keycode(KBC_E1, e1_data);
	}
	else if (e1 == 1) {
		e1_data = data;
		e1++;
	}
	else if (data == 0xE0) {
		e0 = 1;
	}
	else if (data == 0xE1) {
		e1 = 1;
	}
	else {
		return kbc_scan_to_keycode(KBC_NORMAL, data);
	}
}

static inline byte handle_modifiers(byte code, byte brk)
{
	switch (code) {
	case KEYC_SHIFT_LEFT:
	case KEYC_SHIFT_RIGHT:
		modifiers.shift = !brk;
		return 1;

	case KEYC_CTRL_LEFT:
	case KEYC_CTRL_RIGHT:
		modifiers.ctrl = !brk;
		return 1;

	case KEYC_ALT:
		modifiers.alt = !brk;
		return 1;

	case KEYC_ALTGR:
		modifiers.altgr = !brk;
		return 1;

	default:
		return 0;
	}
}

static inline byte handle_raw(byte code)
{
	if (modifiers.ctrl) {
		switch (code) {
		case KEYC_F1: tty_set_cur_term(0); return 1;
		case KEYC_F2: tty_set_cur_term(1); return 1;
		case KEYC_F3: tty_set_cur_term(2); return 1;
		case KEYC_F4: tty_set_cur_term(3); return 1;
		case KEYC_F5: tty_set_cur_term(4); return 1;
		case KEYC_F6: tty_set_cur_term(5); return 1;
		case KEYC_F7: tty_set_cur_term(6); return 1;
		case KEYC_F8: tty_set_cur_term(7); return 1;
		}
	}

	if (code == KEYC_F12) {
		tty_dbg_info(cur_tty);
		return 1;
	}

	return 0;
}

static void handle_input(byte code)
{
	if (!cur_map)
		return;

	char c = 0;
	if (modifiers.shift)
		c = cur_map[code].shift;
	else if (modifiers.altgr)
		c = cur_map[code].altgr;
	else
		c = cur_map[code].normal;

	if (c == 0)
		return;

	if (cur_tty->incount == TTY_INBUF_SIZE)
		return;

	cur_tty->inbuf[cur_tty->incount++] = c;

	while (cur_tty->rqcount && (cur_tty->incount >= cur_tty->rqs[0]->buflen))
		answer_rq(cur_tty, 0);

	if (cur_tty->flags & TTY_ECHO)
		putc(cur_tty, c);
}

static void tty_irq_handler(int irq, dword *esp)
{
	byte brk = 0;
	byte keyc = get_keycode(&brk);

	if (handle_modifiers(keyc, brk))
		return;

	if (brk)
		return; // no need for keydowns anymore

	if (handle_raw(keyc))
		return;

	handle_input(keyc);
}

/** some tty service functions **/

/**
 *  tty_set_cur_term(n)
 */
void tty_set_cur_term(byte n)
{
	if (n < NUM_TTYS) {
		cur_tty = &ttys[n];
		flush(cur_tty);
	}
}

/**
 *  tty_get_cur_term()
 */
byte tty_get_cur_term()
{
	return cur_tty->id;
}

/**
 *  tty_register_keymap(name, map)
 */
void tty_register_keymap(const char *name, keymap_t map)
{
	keymaps = realloc(keymaps, (++nummaps) * sizeof(keymap_holder_t));
	keymaps[nummaps-1].name = name;
	keymaps[nummaps-1].map  = map;

	if (!cur_map)
		cur_map = map;
}

/**
 *  tty_select_keymap(name)
 */
int tty_select_keymap(const char *name)
{
	int i=0;
	for (; i < nummaps; ++i) {
		if (strcmp(keymaps[i].name, name) == 0) {
			cur_map = keymaps[i].map;
			return OK;
		}
	}
	return E_NOT_FOUND;
}

/**
 *  init_tty()
 */
void init_tty(void)
{
	int i=0;
	for (; i < NUM_TTYS; ++i) {
		tty_t *tty = &ttys[i];

		tty->id = i;
		tty->file.path  = names[i];
		tty->file.query = query;

		tty->incount = 0;
		tty->status  = 0x07;
		tty->flags   = TTY_ECHO;

		clear(tty);

		fs_create_dev(&tty->file);
	}

	modifiers.shift = 0;
	modifiers.ctrl  = 0;
	modifiers.alt   = 0;
	modifiers.altgr = 0;

	nummaps = 0;
	cur_map = NULL;

	idt_set_irq_handler(1, tty_irq_handler);

	cur_tty = &ttys[0];
	flush(cur_tty);
}
