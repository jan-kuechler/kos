#ifndef DEBUG_H
#define DEBUG_H

#include <kos/config.h>

#define DBG_LOAD      'l'
#define DBG_GDT       'g'
#define DBG_IDT       'i'
#define DBG_TESTTASKS 't'
#define DBG_MODULE    'm'
#define DBG_ELF       'e'
#define DBG_TTY       'c' // console
#define DBG_VM        'v'
#define DBG_MM        'r' // RAM
#define DBG_PM        'p'
#define DBG_SC        's'

#ifdef CONF_DEBUG
#include "kernel.h"
#define kassert(exp)                                 \
	do {                                               \
		if (!(exp))                                      \
			panic("Assertion " #exp " failed! %s:%s (%d)", \
			      __FILE__, __func__, __LINE__);           \
	} while (0);
#else
#  define kassert(exp) do {} while (0);
#endif

void init_debug(void);
int dbg_check(char flag);
int dbg_verbose(char flag);
void dbg_printf(char flag, const char *msg, ...);
void dbg_vprintf(char flag, const char *msg, ...);

void init_stack_backtrace(void);
void dbg_stack_backtrace(void);

#endif /*DEBUG_H*/
