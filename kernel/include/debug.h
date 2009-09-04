#ifndef DEBUG_H
#define DEBUG_H

#include <stdarg.h>
#include <types.h>
#include <kos/config.h>

#define DBG_LOAD      'l'
#define DBG_PANICBT   'b'
#define DBG_GDT       'g'
#define DBG_IDT       'i'
#define DBG_TESTTASKS 't'
#define DBG_MODULE    'm'
#define DBG_LOADER    'x'
#define DBG_TTY       'c' // console
#define DBG_VM        'v'
#define DBG_MM        'r' // RAM
#define DBG_PM        'p'
#define DBG_SC        's'
#define DBG_FS        'f'
#define DBG_CDI       'd'

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

#ifdef CONF_DEBUG
extern uint32_t dbg_lsc_calln;
extern uint32_t dbg_lsc_arg0;
extern uint32_t dbg_lsc_arg1;
extern uint32_t dbg_lsc_arg2;
extern uint32_t dbg_lsc_proc;

#  define dbg_set_last_syscall(n,a0,a1,a2) \
	do {                                     \
		dbg_lsc_calln = n;                     \
		dbg_lsc_arg0  = a0;                    \
		dbg_lsc_arg1  = a1;                    \
		dbg_lsc_arg2  = a2;                    \
		dbg_lsc_proc  = cur_proc->pid;         \
	} while (0);

#  define dbg_print_last_syscall()      \
	extern const char *dbg_lsc_name[];    \
	dbg_error("Last syscall: %s (%d) "    \
	          "(0x%08x, 0x%08x, 0x%08x) " \
	          "by %s (%d)\n",             \
	          dbg_lsc_name[dbg_lsc_calln],\
	          dbg_lsc_calln, dbg_lsc_arg0,\
	          dbg_lsc_arg1, dbg_lsc_arg2, \
	          pm_get_proc(dbg_lsc_proc)->cmdline, \
	          dbg_lsc_proc);
#else
#  define dbg_set_last_syscall(n,a0,a1,a2)
#  define dbg_print_last_syscall()
#endif

struct proc;

void init_debug(void);
int dbg_check(char flag);
int dbg_verbose(char flag);
void dbg_error(const char *msg, ...);
void dbg_aerror(const char *msg, va_list args);
void dbg_printf(char flag, const char *msg, ...);
void dbg_vprintf(char flag, const char *msg, ...);

void init_stack_backtrace(void);
void dbg_stack_backtrace(void);
void dbg_stack_backtrace_ex(dword ebp, dword eip);
void dbg_proc_backtrace(struct proc *proc);

#endif /*DEBUG_H*/
