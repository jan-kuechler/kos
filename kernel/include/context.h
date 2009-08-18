#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdbool.h>
#include <kos/config.h>

enum context
{
	CX_INIT_EARLY,
	CX_INIT,
	CX_INIT_DONE,
	CX_IRQ,
	CX_SYSCALL,
	CX_PROC,
	CX_PANIC,

	CX_END,
};

enum action
{
	A_CHANGE_IF,
	A_DELAY_EXEC,
	A_DYN_MEM,

	A_END,
};

void cx_set(enum context context);
enum context cx_get(void);
bool cx_allowed(enum action action);

#ifdef CONF_DEBUG
void cx_assert_allowed(enum action action, const char *file, const char *func, int line);
#define assert_allowed(act) cx_assert_allowed(act, __FILE__, __func__, __LINE__)
#else
#define assert_allowed(act)
#endif

#endif /*CONTEXT_H*/
