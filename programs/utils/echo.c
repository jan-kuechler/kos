#include <stdio.h>

int main(int argc, char **argv)
{
	int i=1;
	for (; i < argc; ++i) {
		printf("%s%c", argv[i], i==argc-1 ? '\n' : ' ');
	}

	return 0;
}
