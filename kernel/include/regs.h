#ifndef REGS_H
#define REGS_H

#include "types.h"

/* the stack layout for interrupt handling containing all registers */
/* if you change something here, change int.s! */
typedef struct regs {
	dword    edi;
	dword    esi;
	dword    ebp;
	dword    esp;
	dword    ebx;
	dword    edx;
	dword    ecx;
	dword    eax;

	dword     ds;
	dword     es;
	dword     fs;
	dword     gs;

	dword   intr;
	dword   errc;

	dword    eip;
	dword     cs;
	dword eflags;
	dword  u_esp;
	dword   u_ss;
} regs_t;

#endif /*REGS_H*/
