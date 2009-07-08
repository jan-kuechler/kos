#include "gdt.h"
#include "tss.h"

// the TSS used by the system for ring-changing-interrupts
tss_t tss = {
	.ss0   = GDT_SEL_DATA,
	.iomap = (word)sizeof(tss_t) + 1, // invalid iomap, as it is not used
};
