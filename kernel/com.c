#include <bitop.h>
#include <types.h>
#include <ports.h>
#include "com.h"

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
	/* TODO: Get real base ports from BIOS */

	init_port(0, COM_BAUD, COM_PARITY, COM_BITS);
}

static int transmit_empty(int p)
{
	return inb(ports[p] + LINE_STATUS);
}

void com_putc(int port, char c)
{
	while (!transmit_empty(port))
		;

	outb(ports[port] + TRANSMIT, c);
}
