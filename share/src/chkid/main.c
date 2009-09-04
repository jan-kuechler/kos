#include <stdio.h>

#include "initrd.h"

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: chkid <file> [log]\n");
		return 1;
	}

	id_check(argv[1], argc > 2 ? argv[2] : "chkid.log");

	return 0;
}
