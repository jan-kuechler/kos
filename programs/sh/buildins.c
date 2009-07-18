#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

static int cmd_exit(int argc, char **argv)
{
	int status = 0;
	if (argc > 1) {
		status = atoi(argv[1]);
	}
	exit(status);

	return status; // dummy to keep compiler happy
}

static int cmd_echo(int argc, char **argv)
{
	int i=1;
	for (; i < argc; ++i) {
		printf("%s%c", argv[i], i == argc-1 ? '\n' : ' ');
	}
	return 0;
}

typedef int (*bi_func_t)(int, char**);

struct bi_cmd
{
	const char *cmd;
	bi_func_t func;
};

#define CMD(n) {#n, cmd_##n}

static struct bi_cmd commands[] = {
	CMD(exit),
//	CMD(echo),
	{0, 0}
};

int handle_buildin(struct cmd *cmd, int *status)
{
	int i = 0;
	while (commands[i].cmd) {
		if (strcmp(commands[i].cmd, cmd->argv[0]) == 0) {

			*status = commands[i].func(cmd->argc, cmd->argv);

			return 1;
		}
		i++;
	}

	return 0;
}


