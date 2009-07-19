#include <bitop.h>
#include <errno.h>
#include <minc.h>
#include <ports.h>
#include <stdlib.h>
#include <string.h>

#include "acpi.h"
#include "debug.h"
#include "idt.h"
#include "kbc.h"
#include "kernel.h"
#include "keycode.h"
#include "keymap.h"
#include "tty.h"
#include "fs/devfs.h"
#include "fs/request.h"
#include "mm/kmalloc.h"
#include "util/list.h"

#define CPOS(t) (t->x + t->y * TTY_SCREEN_X)

#define CHECK_TTY(file) ((int)(file)->inode->impl >= 0 && (int)(file)->inode->impl < NUM_TTYS)
#define GET_TTY(file) (&ttys[file->inode->impl])

static char *names[NUM_TTYS] = {
	"tty0", "tty1",	"tty2",	"tty3",
	"tty4", "tty5", "tty6", "tty7"
};
static tty_t ttys[NUM_TTYS];
static tty_t *cur_tty;

static tty_t _kout_tty;
static tty_t *kout_tty = &_kout_tty;

typedef struct
{
	const char *name;
	keymap_t    map;
} keymap_holder_t;
static keymap_holder_t *keymaps;
static int              nummaps;
static keymap_t         cur_map;

static word *vmem = (word*)0xB8000;

static struct {
	byte shift;
	byte ctrl;
	byte alt;
	byte altgr;
	byte capslock;
	byte numlock;
} modifiers;

static int tty_open(struct inode *ino, struct file *file, dword flags);

static int tty_close(struct file *file);
static int tty_read(struct file *file, void *buffer, dword count, dword offset);
static int tty_write(struct file *file, void *buffer, dword count, dword offset);
static int tty_read_async(struct request *rq);
static int tty_write_async(struct request *rq);
static int tty_seek(struct file *file, dword offset, dword index);

static struct inode_ops tty_ino_ops = {
	.open = tty_open,
};

static struct file_ops tty_file_ops = {
	.close = tty_close,
	.read  = tty_read,
	.write = tty_write,
	.read_async = tty_read_async,
	.write_async = tty_write_async,
	.seek = tty_seek,
};

/**
 *  scroll(tty, lines)
 */
static void scroll(tty_t *tty, int lines)
{
	dword offs = lines * TTY_SCREEN_X * 2;
	dword count = (TTY_SCREEN_Y - lines) * TTY_SCREEN_X * 2;
	dword ncount = lines * TTY_SCREEN_X; // * 2;

	if (tty == cur_tty) {
		memmove(((byte*)vmem), ((byte*)vmem) + offs, count);
		memsetw(((int8_t*)vmem) + count, ' ' | (tty->status << 8), ncount);
	}

	memmove(((byte*)tty->outbuf), ((byte*)tty->outbuf) + offs, count);
	memsetw(((int8_t*)tty->outbuf) + count, ' ' | (tty->status << 8), ncount);
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
	memsetw(tty->outbuf, ' ' | (tty->status << 8), TTY_SCREEN_SIZE);
}

/**
 *  update_cursor(tty)
 */
static inline void update_cursor(tty_t *tty)
{
	if (tty == cur_tty) {
		word pos = TTY_SCREEN_SIZE + 1;
		if (list_size(tty->requests) > 0) {
			pos = CPOS(tty);
		}
		outb(0x3D4, 15);
		outb(0x3D5, pos);
		outb(0x3D4, 14);
		outb(0x3D5, pos >> 8);
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

	while (tty->y >= TTY_SCREEN_Y) {
		scroll(tty, 1);
	}

	update_cursor(tty);
}

/**
 *  puts(tty, str)
 */
static inline void puts(tty_t *tty, const char *str)
{
	while (*str)
		putc(tty, *str++);
}

/**
 *  putn(tty, num, base)
 */
static void putn(tty_t *tty, int num, int base, int pad, char pc)
{
	static char buffer[256];
	numfmt(buffer, num, base, pad, pc);
	puts(tty, buffer);
}

/**
 *  enough_data(tty, wanted)
 *
 * Returns true if the tty has enough data.
 */
static int enough_data(tty_t *tty, dword wanted)
{
	if (tty->flags & TTY_RAW) {
		return (tty->incount >= wanted);
	}
	else {
		return (tty->eotcount > 0);
	}
}

/**
 *  can_answer_rq(tty, gotrq)
 *
 * Returns 1 if there is a request that can be answered
 */
static byte can_answer_rq(tty_t *tty)
{
	if (list_size(tty->requests) <= 0) {
		return 0;
	}

	return enough_data(tty, ((struct request*)list_front(tty->requests))->buflen);
}

/**
 *  copy_data(tty, buffer, size)
 *
 * Copy up to size bytes of data from the tty to the buffer.
 */
static inline dword copy_data(tty_t *tty, void *buffer, dword size)
{
	if (bisset(tty->flags, TTY_RAW)) {
		memcpy(buffer, tty->inbuf, size);
		return size;
	}
	else {
		strcpy(buffer, tty->inbuf);
		return strlen(tty->inbuf); // do not include the trailing 0
	}
}

/**
 *  remove_data(tty, count)
 *
 * Remove count bytes from the tty inbuffer.
 */
static inline void remove_data(tty_t *tty, dword count)
{
	if (bnotset(tty->flags, TTY_RAW))
		count++; // include the EOF marker

	memmove(tty->inbuf, tty->inbuf + count, TTY_INBUF_SIZE - count);
	tty->incount -= count;

	if (bnotset(tty->flags, TTY_RAW))
		tty->eotcount--;
}

/**
 *  answer_rq(tty, rq)
 *
 * Answers the given request or the first request in the tty's queue.
 *
 * Note: This function does not check anything.
 *       Be sure you know what you're doing!
 */
static void answer_rq(tty_t *tty)
{
	dbg_vprintf(DBG_TTY, "answer_rq for %d\n", tty->id);
	struct request *rq = list_del_front(tty->requests);

	dbg_vprintf(DBG_TTY, "  copy %d bytes to %p\n", rq->buflen, rq->buffer);
	dword count = copy_data(tty, rq->buffer, rq->buflen);
	dbg_vprintf(DBG_TTY, "  %d bytes copied.\n", count);
	remove_data(tty, count);
	dbg_vprintf(DBG_TTY, "  old data removed.\n");

	rq->result = count;

	dbg_vprintf(DBG_TTY, "  rq-proc blocked? %s\n", rq->blocked ? "yes" : "no");
	rq_finish(rq);
	dbg_vprintf(DBG_TTY, "  finished request.\n");

	update_cursor(tty);
}

static int tty_open(struct inode *ino, struct file *file, dword flags)
{
	file->pos = 0;
	file->fops = &tty_file_ops;

	return 0;
}

static int tty_close(struct file *file)
{
	if (!CHECK_TTY(file)) return -1;
	return 0;
}

static int tty_read(struct file *file, void *buffer, dword count, dword offset)
{
	if (!CHECK_TTY(file)) return -1;
	tty_t *tty = GET_TTY(file);

	dbg_vprintf(DBG_TTY, "read-request for %d bytes\n", count);

	if (enough_data(tty, count)) {
		dword num = copy_data(tty, buffer, count);
		remove_data(tty, num);
		return num;
	}
	else {
		struct request *rq = rq_create(file, buffer, count);
		rq->free_buffer = 1;
		list_add_back(tty->requests, rq);
		rq_block(rq);

		update_cursor(tty);

		return -EAGAIN;
	}
}

static int tty_write(struct file *file, void *buffer, dword count, dword offset)
{
	if (!CHECK_TTY(file)) return -1;
	tty_t *tty = GET_TTY(file);

	char *buf = buffer;

	int i=0;
	for (; i < count; ++i) {
		putc(tty, buf[i]);
	}

	return count;
}

static int tty_read_async(struct request *rq)
{
	return -ENOSYS;
}

static int tty_write_async(struct request *rq)
{
	return -ENOSYS;
}

static int tty_seek(struct file *file, dword offset, dword index)
{
	return -ENOSYS;
}

int tty_isatty(struct file *file)
{
	return CHECK_TTY(file);
}

static inline void select_tty(tty_t *tty)
{
	cur_tty = tty;
	flush(cur_tty);
	update_cursor(cur_tty);
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

	return 0;
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
		if (modifiers.shift) {
			if (code == KEYC_DEL) {
				kbc_reset_cpu();
			}
			else if (code == KEYC_END) {
				// weniger steuern, mehr alten und komplett entfernen!
				acpi_poweroff();
				puts(cur_tty, "\nCould not shutdown. Sorry.\n");
			}
		}

		if (code >= KEYC_F1 && code <= KEYC_F8) {
			tty_set_cur_term(code - KEYC_F1);
			return 1;
		}
	}

	if (code == KEYC_F12) {
		select_tty(kout_tty);
		return 1;
	}

	return 0;
}

static inline byte handle_cbreak_input(byte c)
{
	switch (c) {

	case '\n':
		putc(cur_tty, '\n');
		cur_tty->inbuf[cur_tty->incount++] = '\n';
		/* fallthrough */
	case EOT:
		cur_tty->eotcount++;
		cur_tty->inbuf[cur_tty->incount++] = 0;
		return 1;

	case '\b':
		if (cur_tty->incount > 0) {
			putc(cur_tty, '\b');
			if (cur_tty->inbuf[cur_tty->incount-1] == 0)
				cur_tty->eotcount--;
			cur_tty->incount--;
		}
		return 1;
	}

	return 0;
}

static inline void handle_input(byte code)
{
	if (!cur_map) {
		return;
	}

	byte c = 0;
	if (modifiers.shift || modifiers.capslock)
		c = cur_map[code].shift;
	else if (modifiers.altgr)
		c = cur_map[code].altgr;
	else if (modifiers.ctrl)
		c = cur_map[code].ctrl;
	else
		c = cur_map[code].normal;

	if (c == 0) {
		return;
	}

	if (cur_tty->incount == TTY_INBUF_SIZE)
		return;

	if (!(cur_tty->flags & TTY_RAW)) {
		if (handle_cbreak_input(c))
			goto input_end;
	}
	else if (c == EOT) {
		return;
	}

	if ((cur_tty->flags & TTY_ECHO) && c != EOT)
		putc(cur_tty, c);

	cur_tty->inbuf[cur_tty->incount++] = c;

input_end:
	while (can_answer_rq(cur_tty)) {
		dbg_vprintf(DBG_TTY, "can_answer_rq!\n");
		answer_rq(cur_tty);
	}

}

/**
 *  tty_irq_handler(irq, esp)
 */
static void tty_irq_handler(int irq, dword *esp)
{
	byte brk = 0;
	byte keyc = get_keycode(&brk);

	if (!keyc)
		return;

	if (handle_modifiers(keyc, brk)) {
		return;
	}

	if (brk) {
		return; // no need for keydowns anymore
	}

	if (handle_raw(keyc)) {
		return;
	}

	handle_input(keyc);
}

/** some tty service functions **/

/**
 *  tty_set_cur_term(n)
 */
void tty_set_cur_term(byte n)
{
	if (n < NUM_TTYS) {
		select_tty(&ttys[n]);
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
	keymaps = krealloc(keymaps, (++nummaps) * sizeof(keymap_holder_t));
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
			return 0;
		}
	}
	return -ENOENT;
}

/**
 *  tty_puts(str)
 *
 * Prints the given string on the current terminal
 */
void tty_puts(const char *str)
{
	puts(cur_tty, str);
}

/**
 *  tty_putn(num, base)
 *
 * Prints the given number on the current terminal
 */
void tty_putn(int num, int base)
{
	putn(cur_tty, num, base, 0, ' ');
}

static inline void init(tty_t *tty, int id, int early)
{
	tty->id = id;

	memset(&tty->inode, 0, sizeof(tty->inode));
	tty->inode.name   = names[id];
	tty->inode.flags  = FS_CHARDEV;
	tty->inode.impl   = id;
	tty->inode.ops    = &tty_ino_ops;

	tty->incount = 0;
	tty->status  = 0x07;
	tty->flags   = TTY_ECHO;

	if (!early)
		tty->requests = list_create();

	clear(tty);
}

/**
 *  init_tty()
 */
void init_tty(void)
{
	int i=0;
	for (; i < NUM_TTYS; ++i) { // spare the kout_tty
		tty_t *tty = &ttys[i];

		init(tty, i, 0);

		devfs_register(&tty->inode);
	}

	// anything else for kout_tty is done in init_kout
	kout_tty->requests = list_create();
//	devfs_register(&kout_tty->inode);

	modifiers.shift = 0;
	modifiers.ctrl  = 0;
	modifiers.alt   = 0;
	modifiers.altgr = 0;

	nummaps = 0;
	cur_map = NULL;

	idt_set_irq_handler(1, tty_irq_handler);

	tty_set_cur_term(0);
}

/** kout **/

void init_kout(void)
{
	init(kout_tty, kout_id, 1);

	cur_tty = kout_tty;
}

void kout_puts(const char *str)
{
	puts(kout_tty, str);
}

void kout_putn(int num, int base)
{
	putn(kout_tty, num, base, 0, ' ');
}

void kout_aprintf(const char *fmt, va_list args)
{
	static char buffer[1024];

	strafmt(buffer, fmt, args);
	kout_puts(buffer);
}

void kout_printf(const char *fmt, ...)
{
	static char buffer[1024];

	va_list args;
	va_start(args, fmt);
	strafmt(buffer, fmt, args);
	kout_puts(buffer);
	va_end(args);
}

byte kout_set_status(byte status)
{
	byte old = kout_tty->status;
	kout_tty->status = status;
	return old;
}

void kout_select(void)
{
	tty_set_cur_term(kout_id);
}

void kout_clear()
{
	clear(kout_tty);
	flush(kout_tty);
}
