#ifndef BITOP_H
#define BITOP_H

#define bset(map,bit) (map |= bit)
#define bclr(map,bit) (map &= ~bit)

#define bsetn(map,n)  (map |= (1<<n))
#define bclrn(map,n)  (map &= ~(1<<n))

#define bisset(map,bit) ((map & bit) != 0)
#define bissetn(map,n)  ((map & (1<<n)) != 0)

#define BMASK_1BIT         0x1
#define BMASK_2BIT         0x3
#define BMASK_3BIT         0x7
#define BMASK_4BIT         0xF
#define BMASK_BYTE        0xFF
#define BMASK_WORD      0xFFFF
#define BMASK_DWORD 0xFFFFFFFF

#define bmask(var,mask) (var & mask)

#endif /*BITOP_H*/
