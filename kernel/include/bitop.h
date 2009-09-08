#ifndef BITOP_H
#define BITOP_H

#include <stdint.h>

#define bset(map,bit) (map |= bit)
#define bclr(map,bit) (map &= ~bit)

#define bsetn(map,n)  (map |= (1<<n))
#define bclrn(map,n)  (map &= ~(1<<n))

#define bisset(map,bit) (map & bit)
#define bissetn(map,n)  (map & (1<<n))

#define bnotset(map,bit) (bisset(map,bit) == 0)
#define bnotsetn(map,n)  (bissetn(map,n) == 0)

#define BMASK_1BIT         0x1
#define BMASK_2BIT         0x3
#define BMASK_3BIT         0x7
#define BMASK_4BIT         0xF
#define BMASK_BYTE        0xFF
#define BMASK_12BIT      0xFFF
#define BMASK_WORD      0xFFFF
#define BMASK_DWORD 0xFFFFFFFF

#define bmask(var,mask) (var & mask)

#include <kos/config.h>

/* There is no invalid return value to mark an error, so make
   sure that at least one bit is set, when you call this! */
static inline unsigned char bscanfwd(unsigned int map)
{
#ifdef CONF_DEBUG
	int i = 0;
	for (; i < (sizeof(map) * 8); ++i) {
		if (bissetn(map,i))
			return i;
	}
	/* this should never be reached, send a debug interrupt */
	asm volatile("int $0x03");
	/* but stop warnings about missing return values */
	return 0;
#else
	unsigned int ret = 0;
	asm volatile("bsfl %1, %0 \n"
	            : "=r"(ret)
	            : "r"(map));
	return ret;
#endif
}

static inline void bswap(const void *src, void *dst, uint8_t size)
{
	size -= size%2;
	if (size) {
		const uint8_t *bsrc = src;
		uint8_t *bdst = dst;

		uint8_t i=0;
		for (; i < size; ++i) {
			bdst[size - i - 1] = bsrc[i];
		}
	}
}

static inline uint16_t bigendian_word(uint16_t in)
{
	uint16_t out;
	bswap(&in, &out, 2);
	return out;
}

static inline uint32_t bigendian_dword(uint32_t in)
{
	uint32_t out;
	bswap(&in, &out, 4);
	return out;
}

#endif /*BITOP_H*/
