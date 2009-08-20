#ifndef _JPRINTF_H
#define _JPRINTF_H

#include "stdarg.h"

typedef int (*pfn_putc)(void *, char);
typedef int (*pfn_putsn)(void *, const char *, int);

// Rückgabewerte von putc und putsn:
//      >0: Erfolg. Wert wird zu bytes_written addiert, das von jvprintf am Ende zurückgegeben wird.
//      =0 (putc) oder !=num (putsn): jvprintf terminiert und gibt bytes_written + diesen Wert zurück.
//      <0: jvprintf terminiert und gibt -1 zurück.

struct jprintf_args
{
	pfn_putc putc_fct;
	pfn_putsn putsn_fct;
	void * arg;
};

int jvprintf(struct jprintf_args * args, const char * format, va_list ap);

#endif /* ndef _JPRINTF_H */
