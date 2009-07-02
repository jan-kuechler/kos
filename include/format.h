#ifndef FORMAT_H
#define FORMAT_H

#include <stdarg.h>

/* see lib/libminc/format.c */

int numfmt(char *buffer, int num, int base, int pad, char pc);

int strfmt(char *buffer, const char *fmt, ...);
int strafmt(char *buffer, const char *fmt, va_list args);

#endif /*FORMAT_H*/
