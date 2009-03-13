#include <string.h>

int strncmp(const char *s1, const char *s2, size_t n)
{
	while (*s1 && n && (*s1 == *s2)) {
		++s1;
		++s2;
		--n;
	}
	if (!n) {
		return 0;
	}
	else {
		return ( *s1 - *s2 );
	}
}
