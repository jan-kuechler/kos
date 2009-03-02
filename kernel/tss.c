#include "gdt.h"
#include "tss.h"

tss_t tss = {
	.ss0   = GDT_SEL_DATA,
	.iomap = (word)sizeof(tss_t),
};
