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

#ifdef DUMP_REGS
#include "console.h"
static void dump_regs(dword *esp)
{
	regs_t *regs = (regs_t*)*esp;

	con_printf("Stackdump @ %010x:\n", esp);
	con_printf("ds: %06x es: %06x fs: %06x gs: %06x\n", regs->ds, regs->es, regs->fs, regs->gs);
	con_printf("intr: %06d errc: %06d\n", regs->intr, regs->errc);
	con_printf("eip: %010x cs: %06x\n", regs->eip, regs->cs);
	con_printf("eflags: %x\n", regs->eflags);
	con_printf("u_esp: %010x u_ss: %06x\n", regs->u_esp, regs->u_ss);
}
#endif

#endif /*REGS_H*/
