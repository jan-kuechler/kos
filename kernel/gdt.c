#include <bitop.h>

#include "debug.h"
#include "gdt.h"
#include "tss.h"

gdt_entry_t gdt[GDT_SIZE];

/**
 *  init_gdt()
 *
 * Initializes all GDT entries.
 */
void init_gdt(void)
{
	/* NOTE: The order of the descriptors is important! */

	/* NULL descriptor */
	gdt_set_desc(0, 0x00, 0x00, GDT_NULL, 0, 0);

	/* Kernel code and data segments, both 4GB with base addr 0 */
	gdt_set_desc(1, 0xFFFFFFFF, 0x00000000,
	             GDT_PRESENT | GDT_SEGMENT | GDT_CODE, 0, 0);
	gdt_set_desc(2, 0xFFFFFFFF, 0x00000000,
	             GDT_PRESENT | GDT_SEGMENT | GDT_DATA, 0, 0);

	/* User code and data segments, both 4GB with base addr 0 */
	gdt_set_desc(3, 0xFFFFFFFF, 0x00000000,
	             GDT_PRESENT | GDT_SEGMENT | GDT_CODE, 3, 0);
	gdt_set_desc(4, 0xFFFFFFFF, 0x00000000,
               GDT_PRESENT | GDT_SEGMENT | GDT_DATA, 3, 0);

	/* TSS segment */
	gdt_set_desc(5, sizeof(tss_t)-1, (dword)&tss,
	             GDT_PRESENT | GDT_TSS, 3, 1);

	struct
	{
		word  size;
		dword base;
	} __attribute__((packed)) gdt_ptr = {
		.size = GDT_SIZE*8-1,
		.base = (dword)gdt,
	};

	dbg_printf(DBG_GDT, "Loading GDT\n");
	asm volatile(
		"lgdtl %0           \n\t" /* load the gdt */
		"ljmpl $0x08, $1f   \n\t" /* jump to new code segment (loads cs)
		                             (0x08 => 2. entry in the gdt) */
		"1:                 \n\t"
		"mov   $0x10, %%eax \n\t" /* 0x10 is the new data selector
		                            (3. entry in the gdt) */
		"mov   %%eax, %%ds  \n\t" /* reload all segment registers */
		"mov   %%eax, %%es  \n\t"
		"mov   %%eax, %%fs  \n\t"
		"mov   %%eax, %%gs  \n\t"
		"mov   %%eax, %%ss  \n\t"
		: : "m"(gdt_ptr) : "eax");

	dbg_printf(DBG_GDT, "Loading TSS\n");
	asm volatile("ltr %%ax \n\t" : : "a"(GDT_SEL_TSS));
}

/**
 *  gdt_set_desc(segment, size, base, access, dpl, byte_gran)
 *
 * Sets a GDT gate.
 */
void gdt_set_desc(int segment, dword size, dword base, byte access,
                  byte dpl, byte byte_gran)
{
	dbg_vprintf(DBG_GDT, "Setting GDT Entry: #%d Base: 0x%x Size: 0x%x CPL: %d\n", segment, base, size, (dword)dpl);

	gdt[segment].size      = bmask(size, BMASK_WORD);
	gdt[segment].base_low  = bmask(base, BMASK_WORD);
	gdt[segment].base_mid  = bmask(base >> 16, BMASK_BYTE);
	gdt[segment].base_high = bmask(base >> 24, BMASK_BYTE);
	gdt[segment].access    = access | (bmask(dpl, BMASK_2BIT) << 5);
	gdt[segment].flags     = bmask(size >> 16, BMASK_4BIT);

	if (byte_gran) {
		gdt[segment].flags |= GDT_BYTE_GRAN; /* => 0100xxxx */
	}
	else {
		gdt[segment].flags |= GDT_PAGE_GRAN; /* => 1100xxxx */
	}
}
