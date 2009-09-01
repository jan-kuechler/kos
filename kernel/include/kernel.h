#ifndef KERNEL_H
#define KERNEL_H

#include <multiboot.h>
#include "linker.h"

extern multiboot_info_t multiboot_info;

linker_symbol(kernel_phys_start);
linker_symbol(kernel_phys_end);
linker_symbol(kernel_start);
linker_symbol(kernel_end);
linker_symbol(kernel_size);

struct regs;
void print_state(struct regs *regs);

void shutdown();
void panic(const char *fmt, ...);

#endif /*KERNEL_H*/
