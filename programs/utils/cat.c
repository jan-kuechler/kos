#include <stdio.h>

void cat(FILE *f)
{
	char c;
	while ((c = fgetc(f)) != EOF) {
		fputc(c, stdout);
	}
}

int main(int argc, char **argv)
{
	int err = 0;

	if (argc < 2) {
		cat(stdin);
		return 0;
	}
	else {
		int i=1;
		for (; i < argc; ++i) {
			FILE *f = NULL;

			if (argv[i][0] == '-' && argv[i][1] == '\0') {
				f = stdin;
			}
			else {
				f = fopen(argv[i], "r");
			}

			if (!f) {
				fprintf(stderr, "%s: '%s': No such file or directory\n", argv[0], argv[i]);
				err++;
				continue;
			}

			cat(f);

			if (f != stdin) {
				fclose(f);
			}
		}
	}

	return err;
}
