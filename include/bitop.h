#ifndef BITOP_H
#define BITOP_H

#define bset(map,bit) (map |= bit)
#define bclr(map,bit) (map &= ~bit)

#define bsetn(map,n)  (map |= (1<<n))
#define bclrn(map,n)  (map &= ~(1<<n))

#define bisset(map,bit) ((map & bit) == bit)
#define bissetn(map,n)  ((map & (1<<n)) == (1<<n))

#define BMASK_1BIT         0x1
#define BMASK_2BIT         0x3
#define BMASK_3BIT         0x7
#define BMASK_4BIT         0xF
#define BMASK_BYTE        0xFF
#define BMASK_WORD      0xFFFF
#define BMASK_DWORD 0xFFFFFFFF

#define bmask(var,mask) (var & mask)

/* There is no invalid return value to mark an error, so make
   sure that at least one bit is set, when you call this! */
static inline unsigned char bscanfwd(unsigned int map)
{
	int i = 0;
	for (; i < (sizeof(map) * 8); ++i) {
		if (bissetn(map,i))
			return i;
	}
	/* this should never be reached, send a debug interrupt */
	asm volatile("int $0x03");
}

#endif /*BITOP_H*/
