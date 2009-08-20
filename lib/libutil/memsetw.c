#include <stdlib.h>
#include <stdint.h>

void *memsetw(void *s, int16_t c, size_t n)
{
	int16_t *p = (int16_t*)s;
	while (n--) {
		*p++ = c;
	}
	return s;
}
