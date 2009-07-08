/**
 *  sh - The kOS shell
 *
 * Version: 0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFLEN 1024

#define DEBUG printf

static void die(const char *msg)
{
	fprintf(stderr, "Fatal error occured: %s\n", msg);
	exit(1);
}

static size_t prompt(char *buf, size_t max)
{
	char *str = NULL;
	printf("> ");
	if (!(str = fgets(buf, max, stdin))) {
		die("Cannot read input.");
	}

	size_t len = strlen(buf);

	if (len && buf[len-1] == '\n') {
		len--;
		buf[len] = '\0';
	}

	return len;
}

int main(int argc, char **argv)
{
	char buffer[BUFLEN];

	while (1) {
		prompt(buffer, BUFLEN);

		if (strcmp(buffer, "exit") == 0) {
			break;
		}
	}

	return 0;
}
