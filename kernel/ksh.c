#include <string.h>
#include <kos/syscall.h>
#include "acpi.h"
#include "kbc.h"

#define PROMPT "ksh# "

typedef struct
{
	const char *cmd;
	int (*func)();
} cmd_t;

static int stdin;
static int stdout;
static int stderr;

static void print(int fd, const char *str)
{
	kos_write(fd, str, strlen(str));
}

static int shutdown()
{
	acpi_poweroff();

	print(stderr, "Shutdown failed. Sorry.\n");
	return 1;
}

static int restart()
{
	kbc_reset_cpu();

	print(stderr, "Restart failed. Sorry.\n");
	return 1;
}

#define NUM_CMDS 2
static cmd_t cmds[NUM_CMDS] = {
	{"shutdown", shutdown},
	{"restart",  restart},
};

static void run_cmd(const char *cmd)
{
	int i=0;
	for (; i < NUM_CMDS; ++i) {
		if (strcmp(cmd, cmds[i].cmd) == 0) {
			cmds[i].func();
			return;
		}
	}
	print(stderr, "unknown command: '");
	print(stderr, cmd);
	print(stderr, "'\n");
}

void ksh(void)
{
	stdin  = kos_open("/dev/tty7", 0, 0);
	stdout = kos_open("/dev/tty7", 0, 0);
	stderr = kos_open("/dev/tty7", 0, 0);

	char buffer[512] = {0};
	int len = 0;

	print(stdout, PROMPT);
	while ((len = kos_read(stdin, buffer, 512)) > 0) {
		buffer[len] = 0;
		run_cmd(buffer);
		print(stdout, PROMPT);
	}

	kos_exit(0);
}
