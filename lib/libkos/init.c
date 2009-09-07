#include <ctype.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include <kos/syscalln.h>
#include "helper.h"

extern int main(int argc, char **argv);

#define ESC ((char)27)

static int __esccpy(char *to, const char *from, int len)
{
	int n = 0;
	int i = 0;
	while(from[i] && len--) {
		char c = from[i++];
		if (c != ESC) {
			to[n++] = c;
		}
	}
	return n;
}

static char *__extract(const char *from, int len)
{
	char *str = malloc(len + 1);
	len = __esccpy(str, from, len);
	str[len] = '\0';
	return str;
}

static char **__split(char *input, int *count)
{
	if (!input)	return NULL;

	int len = strlen(input);
	int i = 0;
	int c = 0;
	int start = 0;

	int inquot = 0;
	int indquot = 0;
	int escaped = 0;
	int omitq = 0;

	char **argv = malloc(8 * sizeof(char*));
	int max = 8;

	for (; i < len; ++i) {
		char cur = input[i];
		if (isspace(cur) && !(inquot || indquot)) {
			int end = i;

			if (omitq) {
				end--;
				start++;
				omitq = 0;
			}
			if (c >= max) {
				max = (max * 2 > c) ? max * 2 : c;
				argv = realloc(argv, max * sizeof(char*));
			}
			argv[c++] = __extract(&input[start], end - start);

			start = i + 1;
		}
		else if (cur == '"' && !escaped) {
			indquot = !indquot;
			omitq++;
		}
		else if (cur == '\'' && !escaped) {
			inquot = !inquot;
			omitq++;
		}
		else if (cur == '\\') {
			if (!escaped) {
				input[i] = ESC;
			}
			escaped = !escaped;
		}
		else if (escaped) {
			escaped = 0;
		}
	}

	if (omitq) {
		if (omitq > 1)
			len--;
		start++;
	}
	if (c >= max) {
		max = c;
		argv = realloc(argv, max * sizeof(char*));
	}
	argv[c++] = __extract(&input[start], len - start);

	*count = c;
	return argv;
}

static void __freeargv(int argc, char **argv)
{
	int i=0;
	for (; i < argc; ++i) {
		free(argv[i]);
	}
	free(argv);
}

void __init()
{
	SYSCALL0(SC_OPEN_STD);

	char *cmdline = (char*)SYSCALL0(SC_GETCMDLINE);

	int argc = 0;
	char **argv = __split(cmdline, &argc);

	int n = main(argc, argv);

	__freeargv(argc, argv);

	exit(n);
}

#ifdef TEST
int main(int argc, char **argv)
{
	char teststring[] = "this is a \"test string\"";

	int n = 0;
	char **test = __split(teststring, &n);

	int i = 0;
	for (; i < n; ++i) {
		printf("%s\n", test[i]);
	}

	__freeargv(n, test);
}
#endif
