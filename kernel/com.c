#include <bitop.h>
#include <types.h>
#include <ports.h>
#include "bios.h"
#include "com.h"
#include "fs/types.h"
#include "fs/devfs.h"

#define

#define TRANSMIT     0
#define INTR_ENABLE  1
#define INTR_IDENT   2
#define FIFO_CTRL    2
#define LINE_CTRL    3
#define MODEM_CTRL   4
#define LINE_STATUS  5
#define MODEM_STATUS 6
#define SCRATCH      7

/* default ports */
static int ports[4] = {
	0x3F8,
	0x2F8,
	0x3E8,
	0x2E8,
};

static struct inode inodes[COM_MAX_PORTS];

static char *names[COM_MAX_PORTS] = {
	"com0", "com1", "com2", "com3",
};

static int com_open(struct inode *ino, struct file *file, dword flags);

static int com_close(struct file *file);
static int com_read(struct file *file, void *bufer, dword count, dword offset);
static int com_write(struct file *file, void *buffer, dword count, dword offset);

static struct inode_ops com_ino_ops = {
	.open = com_open,
};

static struct file_ops com_file_ops = {
	.close = com_close,
	.read  = com_read,
	.write = com_write,
};

#define USABLE(p) (ports[p] != 0)

static void init_port(int p, int baud, int parity, int bits)
{
	int base = ports[p];

	/* disable interrupts */
	outb(base + INTR_ENABLE, 0);

	/* set LDAB */
	outb(base + LINE_CTRL, 0x80);

	/* set baud rate */
	word divisor = 115200 / baud; /* Who doesn't like magic numbers? */
	outb(base + 0, bmask(divisor, BMASK_BYTE));
	outb(base + 1, bmask(divisor >> 8, BMASK_BYTE));

	/* reset LDAB and set parity & bits */
	outb(base + LINE_CTRL, ((parity & 0x07) << 3) | ((bits - 5) & 0x03));

	/* enable FIFO */
	outb(base + FIFO_CTRL, 0xC7);

	/* enable IRQ */
	outb(base + MODEM_CTRL, 0x0B);
}

void init_com(void)
{
	struct bios_data_area *info = (struct bios_data_area*)BIOS_DATA_ADDR;

	int i=0;
	for (; i < 4; ++i) {
		ports[i] = info->com_io[i];

		if (USABLE(i)) {
			init_port(i, COM_BAUD, COM_PARITY, COM_BITS);

			memset(inodes[i], 0, sizeof(struct inode));
			inodes[i].name  = names[i];
			inodes[i].flags = FS_CHARDEV;
			inodes[i].impl  = i;
			inodes[i].ops   = &com_ino_ops;

			devfs_register(&inodes[i]);
		}
	}
}


static int transmit_empty(int p)
{
	return USABLE(p) ? inb(ports[p] + LINE_STATUS) : 0;
}

void com_putc(int port, char c)
{
	if (!USABLE(port))
		return;

	while (!transmit_empty(port))
		;

	outb(ports[port] + TRANSMIT, c);
}

void com_puts(int port, const char *str)
{
	if (!USABLE(port))
		return;

	while (*str) {
		com_putc(port, *str++);
	}
}

static int com_open(struct inode *ino, struct file *file, dword flags)
{
	file->pos = 0;
	file->fops = &com_file_ops;

	return 0;
}

static int com_close(struct file *file)
{
	return 0;
}

static int com_read(struct file *file, void *buffer, dword count, dword offset)
{
	return 0;
}

static int com_write(struct file *file, void *buffer, dword count, dword offset)
{
	int id = file->impl;

	char *buf = buffer;
	int i=0;
	for (; i < count; ++i) {
		com_putc(id, buf[i]);
	}

	return count;
}
