#include <malloc.h>
#include <string.h>
//#include <syscall.h>
#include "util.h"

int run(struct cmd *cmd, int *status)
{
	char *cmdline = build_cmd(cmd);

	int pid = execute(cmd->argv[0], cmdline);

	if (!pid) {
		char *path = malloc(strlen(cmd->argv[0]) + 6); /* strlen("/bin/") */
		strcpy(path, "/bin/");
		strcat(path, cmd->argv[0]);

		pid = execute(path, cmdline);

		free(path);
	}


	free(cmdline);

	if (pid != 0) {
		*status = waitpid(pid, (void*)0, 0);
		return 1;
	}

	return 0;
}
