#include <string.h>
#include "console.h"
#include "ports.h"

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

static unsigned int cursor_x;
static unsigned int cursor_y;

static byte color;

static word *vmem = (word*)0xB8000;

/**
 * Scrolls one line down
 */
static void scroll_down(void)
{
	int i = SCREEN_WIDTH * (SCREEN_HEIGHT-1);

	memmove(vmem, vmem + SCREEN_WIDTH, 2 * SCREEN_WIDTH * (SCREEN_HEIGHT-1));

	for (; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
		vmem[i] = color << 8;
	}

	con_set_cursor_pos(cursor_x, cursor_y-1);
}

/**
 * Clears the screen
 */
void con_clear_screen(void)
{
	int i = 0;
	for (;i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
		vmem[i] = color << 8;
	}

	con_set_cursor_pos(0, 0);
}

/**
 * Clears the screen to the given column.
 * Returns the column.
 */
unsigned int con_clear_to(unsigned int x)
{
	int i = cursor_x;
	for (; i < x; ++i) {
		vmem[i + cursor_y * SCREEN_WIDTH] = color << 8;
	}

	return x;
}

/**
 * Sets the cursor position
 */
void con_set_cursor_pos(unsigned int x, unsigned int y)
{
	if (x >= SCREEN_WIDTH) {
		x = SCREEN_WIDTH - 1;
	}
	if (y >= SCREEN_HEIGHT) {
		y = SCREEN_HEIGHT - 1;
	}
	cursor_x = x;
	cursor_y = y;

	con_set_hw_cursor();
}

/**
 * Sets the hardware cursor to the current cursor position
 */
void con_set_hw_cursor()
{
	word pos = cursor_x + cursor_y * SCREEN_WIDTH;

	outb(0x3D4, 15);
	outb(0x3D5, pos);
	outb(0x3D4, 14);
	outb(0x3D5, pos >> 8);
}

/**
 * Sets the color
 */
byte con_set_color(byte clr)
{
	color = clr;
}

/**
 * Prints one character.
 */
void con_putc(const char c)
{
	switch (c) {
	case '\n':
		con_clear_to(SCREEN_WIDTH);
		cursor_x = 0;
		cursor_y++;
		break;

	case '\r':
		cursor_x = 0;
		break;

	case '\t':
		cursor_x = con_clear_to(cursor_x + (CON_TAB_WIDTH - (cursor_x % CON_TAB_WIDTH)));
		break;

	default:
		vmem[cursor_x + cursor_y * SCREEN_WIDTH] = c | (color << 8);
		cursor_x++;
		break;
	}

	if (cursor_x >= SCREEN_WIDTH) {
		cursor_y += cursor_x / SCREEN_WIDTH;
		cursor_x %= SCREEN_WIDTH;
	}

	while (cursor_y >= SCREEN_HEIGHT) {
		scroll_down();
	}

	con_set_hw_cursor();
}

/**
 * Prints a zero terminated string.
 */
void con_puts(const char *s)
{
	while (*s) {
		con_putc(*s++);
	}
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
		con_putc(padc);
	}
	con_puts(end + 1);
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
				con_putc('0');
				con_putc('x');
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
				con_putc('%');
				con_putc(*fmt);
				break;
			}
			fmt++;
		}
		else {
			con_putc(*fmt++);
		}
	}
}

/**
 * con_aprintf with varargs
 */
void con_printf(const char *fmt, ...)
{
	int *args = ((int*)&fmt) + 1;
	con_aprintf(fmt, &args);
}

/**
 * Initializes the console
 */
void init_console(void)
{
	color = 0x07;
	con_clear_screen();
}
