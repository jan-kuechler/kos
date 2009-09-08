#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>

int main(int argc, char **argv)
{
	bool machine = false;  /* -m */
	bool nodename = false; /* -n */
	bool release = false;  /* -r */
	bool sysname = false;  /* -s */
	bool version = false;  /* -v */

	while (optind < argc) {
		int res = getopt(argc, argv, "amnrsv");
		if (res == -1) break;

		switch (res) {
		case '?':
			fprintf(stderr, "Unknown option.\n");
			return 1;

		case 'a':
			machine = true;
			nodename = true;
			release = true;
			sysname = true;
			version = true;
			break;

		case 'm':
			machine = true;
			break;

		case 'n':
			nodename = true;
			break;

		case 'r':
			release = true;
			break;

		case 's':
			sysname = true;
			break;

		case 'v':
			version = true;
			break;

		default:
			break;
		}
	}

	struct utsname n;
	int err = uname(&n);

	if (err) {
		fprintf(stderr, "Error: 'uname' failed.");
		return 1;
	}

	if (sysname)
		printf("%s ", n.sysname);

	if (version)
		printf("%s ", n.version);

	if (release)
		printf("%s ", n.release);

	if (machine)
		printf("%s ", n.machine);

	if (nodename)
		printf("%s ", n.nodename);

	printf("\n");

	return 0;
}
