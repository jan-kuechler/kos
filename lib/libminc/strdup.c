#include <string.h>

extern void *malloc(size_t);

char *strdup(const char *s)
{
	char *n = malloc(strlen(s) + 1);
	strcpy(n, s);
	return n;
}
