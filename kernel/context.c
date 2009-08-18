#include "debug.h"
#include "context.h"
#include "kernel.h"

static enum context cur_context = CX_INIT_EARLY;

static bool allowed[CX_END][A_END] = {
	/*             CHANGE_IF DELAY_EXEC DYNAMIC_MEM */
	/* E_INIT  */{     false,     false,      false },
	/* INIT    */{     false,     false,       true },
	/* INITEND */{      true,     false,       true },
	/* IRQ     */{     false,     false,       true },
	/* SYSCALL */{      true,      true,       true },
	/* PROC    */{     false,      true,       true },
	/* PANIC   */{     false,     false,      false },
};

static const char *cx_name[CX_END] = {
	"Early Initialization",
	"Initialization",
	"End of Initialization",
	"Interrupt Request",
	"Syscall",
	"Process",
	"Panic",
};

static const char *a_name[A_END] = {
	"Change Interrupt Flag",
	"Delay Execution",
	"Dynamic Memory Handling",
};

void cx_set(enum context context)
{
	kassert(context < CX_END);
	cur_context = context;
}

enum context cx_get(void)
{
	return cur_context;
}

bool cx_allowed(enum action action)
{
	kassert(action < A_END);
	return allowed[cur_context][action];
}

void cx_assert_allowed(enum action action, const char *file, const char *func, int line)
{
	if (!cx_allowed(action)) {
		panic("%s is not allowed in %s context! %s:%s (%d)",
		      a_name[action], cx_name[cur_context], file, func, line);
	}
}
