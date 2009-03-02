#ifndef GDT_H
#define GDT_H

#include "types.h"

#define GDT_NULL    0x00
#define GDT_SEGMENT 0x10
#define GDT_PRESENT 0x80

#define GDT_CODE 0x0A
#define GDT_DATA 0x02
#define GDT_TSS  0x09

#define GDT_BYTE_GRAN 0x40 /* 0100b */
#define GDT_PAGE_GRAN 0xC0 /* 1100b */

#define GDT_SIZE 6 /* Number of gdt entries */

/*
Change these whenever you change the order
of the gdt_set_desc calls in init_gdt!
If anything is changed edit int.s!
*/
#define GDT_SEL_NULL  0x00
#define GDT_SEL_CODE  0x08
#define GDT_SEL_DATA  0x10
#define GDT_SEL_UCODE 0x18
#define GDT_SEL_UDATA 0x20
#define GDT_SEL_TSS   0x28

typedef struct gdt_entry {
	word size;			/* contains lower 16 bits of the size */
	word base_low;  /* contains lower 16 bits of the base */
	byte base_mid;  /* contains middle 8 bits of the base */
	byte access;    /* contains the access bits */
	byte flags;     /* contains flags and upper 4 bits of the size */
	byte base_high; /* contains upper 8 bits of the base */
} __attribute__((packed)) gdt_entry_t;

void init_gdt(void);
void gdt_set_desc(int segment, dword size, dword base, byte access,
                  byte dpl, byte byte_gran);

#endif /*GDT_H*/
