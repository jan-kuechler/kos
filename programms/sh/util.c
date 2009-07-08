#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include "util.h"

static char *trim_left(char *str)
{
	int n = strlen(str);
	int i = 0;

	for (; i < n && isspace(*str); i++, str++)
		;

	return str;
}

static char *trim_right(char *str)
{
	int n = strlen(str);

	for (; n > 0 && isspace(str[n-1]); n--)
		;

	str[n] = '\0';
	return str;
}

char *prepare(char *input)
{
	input = trim_left(input);
	input = trim_right(input);

	if (strlen(input)) {
		return input;
	}
	return NULL;
}

#define ESC ((char)27)

static int esccpy(char *to, char *from, int len)
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

static char *extract(char *from, int len)
{
	char *str = malloc(len + 1);
	len = esccpy(str, from, len);
	str[len] = '\0';
	return str;
}

int split_cmd(char *input, struct cmd *cmd)
{
	int n = strlen(input);
	int i = 0;

	int start = 0;

	int inquot = 0;
	int indquot = 0;
	int escaped = 0;

	int omitq = 0;

	char *args[256];

	cmd->argc = 0;

	for (; i < n; ++i) {
		char cur = input[i];
		if (isspace(cur) && !(inquot || indquot)) {
			int end = i;

			if (omitq) {
				end--;
				start++;
				omitq = 0;
			}
			args[cmd->argc++] = extract(&input[start], end - start);

			start = i + 1;
		}
		else if (cur == '"' && !escaped) {
			indquot = !indquot;
			omitq = 1;
		}
		else if (cur == '\'' && !escaped) {
			inquot = !inquot;
			omitq = 1;
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
		n--;
		start++;
	}
	args[cmd->argc++] = extract(&input[start], n - start);

	cmd->argv = malloc(cmd->argc * sizeof(char*));
	for (i = 0; i < cmd->argc; ++i) {
		cmd->argv[i] = args[i];
	}

	return 0;
}

void free_argv(struct cmd *cmd)
{
	int i=0;
	for (; i < cmd->argc; ++i) {
		free(cmd->argv[i]);
	}
	free(cmd->argv);
}

#ifdef TEST

#include <stdio.h>

int main(int argc, char **argv)
{
	char test1[] = " Hallo  \n";

	printf("Before: '%s'\n", test1);
	char *after = prepare(test1);
	printf("After: '%s'\n", after ? after : "<NULL>");

	char test2[] = "  \t\t \n";
	printf("Before: '%s'\n", test2);
	after = prepare(test2);
	printf("After: '%s'\n", after ? after : "<NULL>");

	struct cmd cmd = {0, 0};
	char line[] = "echo \"test bla\" 'blub\\' diedup'";
	split_cmd(line, &cmd);
	printf("Cmd: \n Argc: %d\n", cmd.argc);

	int i=0;
	for (; i < cmd.argc; ++i) {
		printf(" ->%s\n", cmd.argv[i]);
	}

	return 0;
}

#endif
