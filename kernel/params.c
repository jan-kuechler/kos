#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"

static int exec(char *cmd, char *val, struct parameter *params, unsigned int num)
{
	int i=0;

	for (; i < num; i++) {
		if (strcmp(cmd, params[i].name) == 0) {
			return params[i].func(val);
		}
	}

	return -ENOENT;
}

static char *next(char *cmdline, char **cmd, char **val)
{
	int i=0;
	int val_delim = 0;

	for (; cmdline[i]; i++) {
		if (cmdline[i] == ' ') {
			break;
		}
		if (!val_delim && cmdline[i] == '=') {
			val_delim = i;
		}
	}

	*cmd = cmdline;
	if (!val_delim) {
		*val = NULL;
	}
	else {
		cmdline[val_delim] = '\0';

		*val = &cmdline[val_delim + 1];
	}

	char *next = &cmdline[i];
	if (cmdline[i]) {
		cmdline[i] = '\0';
		next++;
	}

	while (*next == ' ') {
		next++;
	}

	return next;
}

/**
 *  parse_params(cmdline, params, num)
 *
 * Parses a cmdline.
 */
int parse_params(char *cmdline, struct parameter *params, unsigned int num, bool noerror)
{
	while (*cmdline == ' ') {
		cmdline++;
	}

	while (*cmdline) {
		char *cmd, *val;

		cmdline = next(cmdline, &cmd, &val);
		int err = exec(cmd, val, params, num);

		if (err && noerror) return err;
	}

	return 0;
}

void print_params(struct parameter *params, unsigned int num)
{
	int i=0;
	for (; i < num; ++i) {
		kout_printf("%02d - '%s'\n", i, params[i].name);
	}
}
