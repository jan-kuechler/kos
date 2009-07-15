#include <string.h>
#include <stdint.h>

void *memset(void *s, int c, size_t n)
{
	char *p = (char*)s;
	while (n--) {
		*p++ = (char)c;
	}
	return s;
}

void *memsetw(void *s, int16_t c, size_t n)
{
	int16_t *p = (int16_t*)s;
	while (n--) {
		*p++ = c;
	}
	return s;
}
