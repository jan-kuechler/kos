#ifndef MINC_H
#define MINC_H

#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>

/* see lib/libminc/format.c */

int numfmt(char *buffer, int num, int base, int pad, char pc);

int strfmt(char *buffer, const char *fmt, ...);
int strafmt(char *buffer, const char *fmt, va_list args);

void *memsetw(void *s, int16_t c, size_t n);

#endif /*MINC_H*/
