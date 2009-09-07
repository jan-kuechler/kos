#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define debug(...) ((void)0)

void list(const char *dir)
{
	fprintf(stderr, "opedir");
	DIR *d = opendir(dir);
	fprintf(stderr, "\tdone!\n");

	if (!d) {
		fprintf(stderr, "Could not open %s.\n", dir);
		exit(1);
	}

	struct dirent *entry = readdir(d);
	while (entry) {
		printf("%s  ", entry->d_name);
		entry = readdir(d);
	}

	printf("\n\n");
	closedir(d);
}

int main(int argc, char **argv)
{
	if (argc > 1) {
		int i=1;
		for (; i< argc; ++i) {
			if (argc > 2) {
				printf("%s: \n", argv[i]);
			}
			list(argv[i]);
		}
	}
	else {
		char cwd[256];
		if (getcwd(cwd, 256) == 0) {
			fprintf(stderr, "Could net get working directory.\n");
			exit(1);
		}

		list(cwd);
	}

	return 0;
}
