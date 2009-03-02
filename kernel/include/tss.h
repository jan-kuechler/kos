#ifndef TSS_H
#define TSS_H

#include "types.h"

typedef struct tss {
	dword   link;
	dword   esp0;
	dword    ss0;
	dword   esp1;
	dword    ss1;
	dword   esp2;
	dword    ss2;
	dword    cr3;
	dword    eip;
	dword eflags;
	dword    eax;
	dword    ecx;
	dword    edx;
	dword    ebx;
	dword    esp;
	dword    ebp;
	dword    esi;
	dword    edi;
	dword     es;
	dword     cs;
	dword     ss;
	dword     ds;
	dword     fs;
	dword     gs;
	dword   ldtr;
	word    trap;
	word   iomap;
} __attribute__((packed)) tss_t;

extern tss_t tss;

#endif /*TSS_H*/
