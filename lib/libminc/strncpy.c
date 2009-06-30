#include <string.h>

char *strncpy(char *s1, const char *s2, size_t n)
{
	char * rc = s1;
	while ((n > 0) && (*s1++ = *s2++)) {
		--n;
	}
	if (n > 0) {
		while (--n) {
			*s1++ = '\0';
		}
	}
	return rc;
}
