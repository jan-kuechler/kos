#include <string.h>
#include <kos/syscall.h>
#include "acpi.h"
#include "kbc.h"
#include "loader.h"
#include "module.h"
#include "tty.h"

#define PROMPT "ksh# "

typedef struct
{
	const char *cmd;
	const char *desc;
	void (*func)();
} cmd_t;

static int stdin;
static int stdout;
static int stderr;

static void print(int fd, const char *str)
{
	write(fd, str, strlen(str));
}

/***************************/
static void shutdown();
static void restart();
static void help();
static void test();
static void file();
static void int3();

static cmd_t cmds[] = {
	{"shutdown", "Turns the computer off.", shutdown},
	{"restart",  "Restarts the computer.", restart},
	{"int3", "Generates a debug interrupt.", int3},
	{"test", "Starts the test module.", test},
	{"file", "Print /test's content.", file},
	{"help", "Prints this list.", help},
};
static int num_cmds = sizeof(cmds) / sizeof(cmd_t);

static void shutdown()
{
	acpi_poweroff();

	print(stderr, "Shutdown failed. Sorry.\n");
}

static void restart()
{
	kbc_reset_cpu();

	print(stderr, "Restart failed. Sorry.\n");
}

static void int3()
{
	asm volatile("int $0x03");
}

static void test()
{
	//mod_exec(1);
	if (!exec_file("/bin/test", "/bin/test testarg", 2))
		print(stderr, "Cannot execute /bin/test\n");
}

static void file()
{
	int f = open("/file", 0, 0);

	if (f < 0) {
		print(stderr, "could not open /file\n");
		return;
	}

	char buf[256] = {0};
	int len = 0;
	while ((len = read(f, buf, 256))> 0) {
		buf[len] = 0;
		print(stdout, buf);
	}

	print(stderr, "== EOF ==\n");

	close(f);
}

static void help()
{
	int i=0;
	for (; i < num_cmds; ++i) {
		print(stdout, cmds[i].cmd);
		print(stdout, " - ");
		print(stdout, cmds[i].desc);
		print(stdout, "\n");
	}
}

/***************************/

static void run_cmd(const char *cmd)
{
	int i=0;
	for (; i < num_cmds; ++i) {
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
	stdin = open("/dev/tty7", 0, 0);
	if (stdin < 0) {
		kout_printf("Cannot open stdin.\n");
		exit(0);
	}

	stdout = open("/dev/tty7", 0, 0);
	if (stdout < 0) {
		kout_printf("Cannot open stdout.\n");
		exit(0);
	}
	stderr = open("/dev/tty7", 0, 0);
	if (stderr < 0) {
		kout_printf("Cannot open stderr.\n");
		exit(0);
	}

	char buffer[512] = {0};
	int len = 0;

	print(stdout, PROMPT);
	while ((len = read(stdin, buffer, 512)) > 0) {
		buffer[len] = 0;
		run_cmd(buffer);
		print(stdout, PROMPT);
	}

	exit(0);
}
