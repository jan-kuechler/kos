#include <stdarg.h>

static int bufns(char *buffer, const char *str, int n)
{
	int x = 0;
	while (n--) {
		buffer[x++] = *str++;
	}
	return x;
}

static int bufs(char *buffer, const char *str)
{
	int n = 0;
	while (*str) {
		buffer[n++] = *str++;
	}
	return n;
}

int numfmt(char *buffer, int num, int base, int pad, char pc)
{
	static char digits[] = "0123456789ABCDEFGHIJKLMOPQRSTUVWXYZ";

	int n = 0;
	char tmp[65];
	char *end = tmp + 64;

	if (base < 2 || base > 36)
		return 0;

	*end-- = 0;

	do {
		int rem = num % base;
		num = num / base;
		*end-- = digits[rem];
		pad--;
	} while (num > 0);

	while (pad-- > 0) {
		buffer[n++] = pc;
	}

	while (*(++end)) {
		buffer[n++] = *end;
	}

	buffer[n] = '\0';
	return n;
}

int strafmt(char *buffer, const char *fmt, va_list args)
{
	int  n = 0;
	long val = 0;
	int  pad = 0;
	char padc = ' ';

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
				val = va_arg(args, int);
				if (val < 0) {
					buffer[n++] = '-';
					pad--;
					val = -val;
				}
			}
			else if (*fmt == 'i' || *fmt == 'o' ||
			         *fmt == 'p' || *fmt == 'x' ||
			         *fmt == 'b')
			{
				val = va_arg(args, int);
				val = val  & 0xffffffff;
			}


			switch (*fmt) {
			case 'c':
				buffer[n++] = (char)va_arg(args, int);
				break;

			case 'b':
				n += numfmt(&buffer[n], val, 2, pad, padc);
				break;

			case 'd':
			case 'i':
			case 'u':
				n += numfmt(&buffer[n], val, 10, pad, padc);
				break;

			case 'o':
				n += numfmt(&buffer[n], val, 8, pad, padc);
				break;

			case 'p':
				padc = '0';
				pad  = 8;
				n += bufs(&buffer[n], "0x");
			case 'x':
				n += numfmt(&buffer[n], val, 16, pad, padc);
				break;

			case 's':
				n += bufs(&buffer[n], va_arg(args, char*));
				break;

			case 'S':
				n += bufns(&buffer[n], va_arg(args, char*), pad);
				break;

			case '%':
				buffer[n++] = '%';
				break;

			default:
				buffer[n++] = '%';
				buffer[n++] = *fmt;
				break;
			}
			fmt++;
		}
		else {
			buffer[n++] = *fmt++;
		}
	}

	buffer[n] = '\0';
	return n;
}

int strfmt(char *buffer, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int n = strafmt(buffer, fmt, args);
	va_end(args);
	return n;
}


