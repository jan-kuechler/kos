#include <string.h>
#include "console.h"
#include "idt.h"
#include "ports.h"

console_t vc[CON_NUM_VC];
static console_t *cur_vc;

static word *vmem = (word*)0xB8000;

/**
 * Scrolls one line down
 */
static void scroll_down(void)
{
	int i = CON_SCREEN_WIDTH * (CON_SCREEN_HEIGHT-1);

	memmove(cur_vc->screenbuffer, cur_vc->screenbuffer + CON_SCREEN_WIDTH,
	        2 * CON_SCREEN_WIDTH * (CON_SCREEN_HEIGHT-1));

	for (; i < CON_SCREEN_WIDTH * CON_SCREEN_HEIGHT; ++i) {
		cur_vc->screenbuffer[i] = cur_vc->color << 8;
	}

	con_set_cursor_pos(cur_vc->x, cur_vc->y-1);
}

/**
 * Clears the screen to the given column.
 * Returns the column.
 */
static unsigned int con_clear_to(unsigned int x)
{
	int i = cur_vc->x;
	for (; i < x; ++i) {
		cur_vc->screenbuffer[i + cur_vc->y * CON_SCREEN_WIDTH] = cur_vc->color << 8;
	}

	return x;
}

static void putc(char c)
{
	switch (c) {
	case '\n':
		con_clear_to(CON_SCREEN_WIDTH);
		cur_vc->x = 0;
		cur_vc->y++;
		break;

	case '\r':
		cur_vc->x = 0;
		break;

	case '\t':
		cur_vc->x = con_clear_to(cur_vc->x + (CON_TAB_WIDTH - (cur_vc->x % CON_TAB_WIDTH)));
		break;

	default:
		cur_vc->screenbuffer[cur_vc->x + cur_vc->y * CON_SCREEN_WIDTH] = c | (cur_vc->color << 8);
		cur_vc->x++;
		break;
	}

	if (cur_vc->x >= CON_SCREEN_WIDTH) {
		cur_vc->y += cur_vc->x / CON_SCREEN_WIDTH;
		cur_vc->x %= CON_SCREEN_WIDTH;
	}

	while (cur_vc->y >= CON_SCREEN_HEIGHT) {
		scroll_down();
	}

	con_set_hw_cursor();
}

/**
 * Clears the screen
 */
void con_clear_screen(void)
{
	int i = 0;
	for (;i < CON_SCREEN_WIDTH * CON_SCREEN_HEIGHT; ++i) {
		cur_vc->screenbuffer[i] = cur_vc->color << 8;
	}

	con_set_cursor_pos(0, 0);
	con_flush();
}

/**
 * Sets the cursor position
 */
void con_set_cursor_pos(unsigned int x, unsigned int y)
{
	if (x >= CON_SCREEN_WIDTH) {
		x = CON_SCREEN_WIDTH - 1;
	}
	if (y >= CON_SCREEN_HEIGHT) {
		y = CON_SCREEN_HEIGHT - 1;
	}
	cur_vc->x = x;
	cur_vc->y = y;

	con_set_hw_cursor();
}

/**
 * Sets the hardware cursor to the current cursor position
 */
void con_set_hw_cursor()
{
	disable_intr();

	word pos = cur_vc->x + cur_vc->y * CON_SCREEN_WIDTH;

	outb(0x3D4, 15);
	outb(0x3D5, pos);
	outb(0x3D4, 14);
	outb(0x3D5, pos >> 8);

	enable_intr();
}

/**
 * Sets the color
 */
byte con_set_color(byte clr)
{
	byte old = cur_vc->color;
	cur_vc->color = clr;
	return old;
}

/**
 * Prints one character.
 */
void con_putc(const char c)
{
	putc(c);
	con_flush();
}

/**
 * Prints a zero terminated string.
 */
void con_puts(const char *s)
{
	while (*s) {
		putc(*s++);
	}
	con_flush();
}

static unsigned long long divmod(unsigned long long dividend,
                                 unsigned int divisor,
                                 unsigned int *remainder)
{
    unsigned int highword = dividend >> 32;
    unsigned int lowword = dividend & 0xffffffff;
    unsigned long long quotient;
    unsigned int rem;

    asm(
			"div  %%ecx        \n\t"
      "xchg %%ebx, %%eax \n\t"
      "div  %%ecx        \n\t"
      "xchg %%edx, %%ebx \n\t"
       : "=A"(quotient), "=b"(rem)
       : "a"(highword), "b"(lowword), "c"(divisor), "d"(0)
    );

    if(remainder) {
        *remainder = rem;
    }

    return quotient;
}

/**
 * Prints an unsigned number.
 */
void con_putn(unsigned long long n, int b, int pad, char padc)
{
	static char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	char tmp[65];
	char *end = tmp + 64;
	unsigned int rem;

	if (b < 2 || b > 36)
		return;

	*end-- = 0;

	do {
		n = divmod(n, b, &rem);
		*end-- = digits[rem];
		pad--;
	} while (n > 0);

	while (pad-- > 0) {
		putc(padc);
	}


	while (*(++end)) {
		putc(*end);
	}
	con_flush();
}

/**
 * A very simple printf
 */
void con_aprintf(const char *fmt, int **args)
{
	long long val = 0;
	int pad;
	char padc;

	while (*fmt) {
		if (*fmt == '%') {
			fmt++;

			pad = 0;
			if (*fmt == '0') {
				padc = '0';
				fmt++;
			}
			else {
				padc = ' ';
			}

			while (*fmt >= '0' && *fmt <= '9') {
				pad = pad * 10 + *fmt++ - '0';
			}

			if (*fmt == 'd' || *fmt == 'u') {
				val = *(*args)++;
				if (val < 0) {
					con_putc('-');
					pad--;
					val = -val;
				}
			}
			else if (*fmt == 'i' || *fmt == 'o' || *fmt == 'p' || *fmt == 'x') {
				val = *(*args)++;
				val = val  & 0xffffffff;
			}


			switch (*fmt) {
			case 'c':
				con_putc(*(*args)++);
				break;

			case 'd':
			case 'i':
			case 'u':
				con_putn(val, 10, pad, padc);
				break;

			case 'o':
				con_putn(val, 8, pad, padc);
				break;

			case 'p':
			case 'x':
#ifdef CON_PTRINF_HEX_0X
				putc('0');
				putc('x');
				if (pad >= 2)
					pad -= 2;
				else
					pad = 0;
#endif
				con_putn(val, 16, pad, padc);
				break;

			case 's':
				con_puts((char*)*(*args)++);
				break;

			case '%':
				con_putc('%');
				break;

			default:
				putc('%');
				con_putc(*fmt);
				break;
			}
			fmt++;
		}
		else {
			con_putc(*fmt++);
		}
	}
	/* no need for a con_flush(), as only con_* is used here */
}

/**
 * con_aprintf with varargs
 */
void con_printf(const char *fmt, ...)
{
	int *args = ((int*)&fmt) + 1;
	con_aprintf(fmt, &args);
}

void con_flush()
{
	int i = 0;

	disable_intr();
	for (; i < CON_SCREENBUFFER_SIZE; ++i) {
		vmem[i] = cur_vc->screenbuffer[i];
	}
	enable_intr();
}

void con_select(dword id)
{
	cur_vc = &vc[id];
	con_flush();
	con_set_hw_cursor();
}

/**
 * Initializes the console
 */
void init_console(void)
{
	int i=0;
	for (; i < CON_NUM_VC; ++i) {
		vc[i].id = i;
		vc[i].in_avail = 0;
		vc[i].x = 0;
		vc[i].y = 0;
		vc[i].color    = 0x07;

		memset(vc[i].inbuffer, 0, CON_INBUFFER_SIZE);

	}
	cur_vc = &vc[0];
	con_clear_screen();
}
