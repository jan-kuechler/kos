#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>

int vasprintf(char ** buffer, const char * format, va_list ap);
int asprintf(char ** buffer, const char * format, ...);

#endif /*STDIO_H*/
