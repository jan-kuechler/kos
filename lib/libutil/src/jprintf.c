/*
 * Copyright (c) 2006-2007  The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Burkhard Weseloh.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Die hier definierte jvprintf Funktion wird von allen anderen *printfs
 * für die Formatierung genutzt. Die funktionsspezifische Ausgabe erfolgt
 * über callbacks. Eine simple Anwendung dieser Funktion gibt es in printf.c.
 */

#include <stdlib.h>
#include <string.h>

#include "jprintf.h"

#define malloc  kmalloc
#define callc   kcalloc
#define realloc krealloc
#define free    kfree

/** Dividiert ein uint64 durch einen uint32 und gibt das Ergebnis zurück
 *
 * @param dividend
 * @param divisor
 * @param remainder Wenn ungleich NULL, wird in *remainder der Rest zurückgegeben
 *
 * @return dividend/divisor
 */
unsigned long long divmod(unsigned long long dividend, unsigned int divisor, unsigned int * remainder)
{
    unsigned long long quotient;
    unsigned int rem;

    quotient = dividend / divisor;
    rem = dividend % divisor;

    if(remainder)
    {
        *remainder = rem;
    }

    return quotient;
}

/**
 * Konvertiert eine vorzeichenlose 64-Bit-Zahl in eine Zeichenkette.
 *
 * @param value Die Zahl
 * @param buf Der Puffer. strlen(buf) = 64*log(2)/log(radix)+1
 * @param radix Die Basis der auszugebenen Zeichenkette. 2 <= radix <= 36
 * @param uppercase Wenn ungleich 0, werden Großbuchstaben für Ziffern > 9 benutzt.
 *
 * @return buf
 */
char * ulltoa(unsigned long long value, char * buf, unsigned int radix, unsigned int uppercase)
{
    char * p = buf;
    const char * const chars = uppercase ? "0123456789ABCDEFGHIJKLMOPQRSTUVWXYZ" : "0123456789abcdefghijklmopqrstuvwxyz";
    unsigned long long temp;
    unsigned int digits;
    unsigned int remainder;

    // Es werden nur Basen zwischen 2 und 36 unterstützt
    if(radix < 2 || radix > 36)
    {
        return buf;
    }

    // Anzahl der Ziffern zählen
    temp = value;
    digits = 0;
    do
    {
        digits++;
        temp = divmod(temp, radix, 0);
    }
    while(temp > 0);

    // Zeiger auf das Ende der Zahl setzen und Nullterminierung einfügen
    p += digits;
    *p = 0;

    // Ziffern rückwärts in den Puffer schreiben
    temp = value;
    do
    {
        temp = divmod(temp, radix, &remainder);
        *--p = chars[remainder];
    }
    while(--digits);

    return buf;
}

/**
 * Ruft die dem Kontext entsprechende putc Funktion auf.
 *
 * @param args Der Kontext
 * @param c Das Zeichen
 *
 * @return Anzahl der ausgegebenen Bytes
 *
 * @see jvprintf()
 */
int jprintf_putc(struct jprintf_args * args, char c)
{
	if(args->putc_fct != NULL)
	{
		return args->putc_fct(args->arg, c);
	}

	return 1;
}

/**
 * Ruft die dem Kontext entsprechende putsn Funktion auf.
 *
 * @param args Der Kontext
 * @param string Die Zeichenkette
 * @param n Die Anzahl der Zeichen oder -1 für Ausgabe bis zur Nullterminierung
 *
 * @return Anzahl der ausgegebenen Bytes. Negative Zahl im Fehlerfall.
 *
 * @see jvprintf()
 */
int jprintf_putsn(struct jprintf_args * args, const char * string, int n)
{
	if(args->putsn_fct != NULL)
	{
		return args->putsn_fct(args->arg, string, n);
	}
	else
	{
	    // Wenn keine putsn Funktion definiert wurde, auf putc zurückfallen
		int i = 0;
		int bytes_written = 0;

		while(string[i] && (i < n || n == -1))
		{
			bytes_written += jprintf_putc(args, string[i]);
			i++;
		}

		return bytes_written;
	}
}

// WRITE_STRING und WRITE_CHAR geben einen String bzw. einen char aus, und
// verlassen gegebenen Falls die aufrufende Funktion. Nur in jprintf zu verwenden.
#define WRITE_STRING(string, num) \
do { \
	int ret = jprintf_putsn(args, string, num); \
	if(ret < 0) \
	{ \
		return ret; \
	} \
	else if(num != -1 && ret != num) \
	{ \
        return bytes_written + ret;\
	} \
	bytes_written += ret; \
} while(0)

#define WRITE_CHAR(c) \
do { \
	int ret = jprintf_putc(args, c); \
	if(ret < 0) \
	{ \
		return ret; \
	} \
	else if(ret == 0) \
	{ \
        return bytes_written;\
	} \
	bytes_written += ret; \
} while(0)


/**
 * vprintf mit Callbacks. Gibt nicht das terminierende Nullbyte aus.
 *
 * @param args Der Kontext bestehend aus Zeiger auf die putc und putsn-Funktionen sowie auf einen benutzerdefinierten Parameter
 * @param format der format string
 * @param ap die Argumente
 */
int jvprintf(struct jprintf_args * args, const char * format, va_list ap)
{
	const char * t;
	int bytes_written;

	char flags;
	unsigned int width;
	unsigned int precision;
	unsigned int length;

	bytes_written = 0;

	while(*format)
	{
		t = format;
		while(*format != '%' && *format)
		{
			format++;
		}

		WRITE_STRING(t, format - t);

		if(!*format)
		{
			break;
		}

		format++;

		if(*format == '%')
		{
			WRITE_CHAR(*format);
			format++;
			continue;
		}

		// default values
		flags = 0;
		width = 0;
		precision = -1;
		length = 32;

		// flags
		switch(*format)
		{
		case '+':
		case '-':
		case ' ':
		case '#':
		case '0':
			flags = *format;
			format++;
		}

		// width
		switch(*format)
		{
		case '0' ... '9':
			width = strtol(format, (char**)&format, 10);
			break;
		case '*':
			width = va_arg(ap, unsigned int);
			format++;
			break;
		}

		// precision
		if(*format == '.')
		{
			format++;
			switch(*format)
			{
			case '0' ... '9':
				precision = strtol(format, (char**)&format, 10);
				break;
			case '*':
				precision = va_arg(ap, unsigned int);
				format++;
				break;
			}
		}

		// length
		switch(*format)
		{
		case 'h':
			// short
			length = 16;
			format++;
            // char
			if(*format == 'h')
			{
                length = 8;
                format++;
			}
			break;
		case 'l':
			// long
            length = 32;
            format++;
            // long long
			if(*format == 'l')
			{
                length = 64;
                format++;
			}
			break;
		case 'L':
			// long double
			format++;
			break;
		}

		// format specifier
		switch(*format)
		{
		    {
				char buf[67];
				int sign;

		case 'o':
        case 'u':
        case 'x':
        case 'p':
        case 'X':
                {
                    unsigned long long value = 0;
                    switch(length)
                    {
                        case 8:
                        case 16:
                        case 32:
                            value = va_arg(ap, unsigned int);
                            break;
                        case 64:
                            value = va_arg(ap, unsigned long long);
                            break;
                    }
                    ulltoa(value, buf, (*format == 'p' || *format == 'x' || *format == 'X') ? 16 : ((*format == 'o') ? 8 : 10) , (*format == 'X' ? 1 : 0));
                    sign = 1;
                }

                if(0) // fall through und den nächsten {}-block nicht ausführen
		case 'd':
		case 'i':
                {
                    signed long long value = 0;

                    switch(length)
                    {
                        case 8:
                        case 16:
                        case 32:
                            value = va_arg(ap, signed int);
                            break;
                        case 64:
                            value = va_arg(ap, signed long long);
                            break;
                    }

                    if(value < 0)
                    {
                        buf[0] = '-';
                        ulltoa(-value, buf+1, 10, 0);
                    }
                    else
                    {
                        ulltoa(value, buf, 10, 0);
                    }

                    sign = (value < 0) ? -1 : (value > 0) ? 1 : 0;
                }

				if(width != 0)
				{
					int len = strlen(buf);

					switch(flags)
					{
					case ' ':
					case 0: // keine flags angegeben
						for( ; len < width; len++)
						{
							WRITE_CHAR(' ');
						}
						WRITE_STRING(buf, -1);
						break;
					case '-':
						WRITE_STRING(buf, -1);
						for( ; len < width; len++)
						{
							WRITE_CHAR(' ');
						}
						break;
					case '0':
						if(sign < 0)
						{
							WRITE_CHAR('-');
						}
						for( ; len < width; len++)
						{
							WRITE_CHAR('0');
						}
						if(sign < 0)
						{
							WRITE_STRING(buf + 1, -1); // - überspringen
						}
						else
						{
							WRITE_STRING(buf, -1);
						}
						break;
					case '+':
						if(sign >= 0)
						{
							len++;
						}
						for( ; len < width; len++)
						{
							WRITE_CHAR(' ');
						}
						if(sign >= 0)
						{
							WRITE_CHAR('+');
						}
						WRITE_STRING(buf, -1);
						break;
					default:
						WRITE_STRING(buf, -1);
						break;
					}
				}
				else
				{
					switch(flags)
					{
					case '+':
						if(sign >= 0)
						{
							WRITE_CHAR('+');
						}
						break;
					case ' ':
						if(sign >= 0)
						{
							WRITE_CHAR(' ');
						}
						break;
					}

					WRITE_STRING(buf, -1);
				}
			}
			format++;
			break;
        case 'c':
            {
				char c = (char)va_arg(ap, int);
                WRITE_CHAR(c);
            }
            format++;
            break;
		case 's':
			{
				char * string = va_arg(ap, char *);
				if(width != 0)
				{
					int len = strlen(string), pad;

					switch(flags)
					{
					case '-':
						WRITE_STRING(string, len);
						for(pad = len ; pad < width; pad++)
						{
							WRITE_CHAR(' ');
						}
						break;
					case '0':
						for(pad = len ; pad < width; pad++)
						{
							WRITE_CHAR('0');
						}
						WRITE_STRING(string, len);
						break;
					case ' ':
					case 0: // keine flags angegeben
						for(pad = len ; pad < width; pad++)
						{
							WRITE_CHAR(' ');
						}
						WRITE_STRING(string, len);
						break;
					default:
						WRITE_STRING(string, len);
						break;
					}
				}
				else
				{
					WRITE_STRING(string, -1);
				}
			}
			format++;
			break;
		}
	}

	return bytes_written;
}
