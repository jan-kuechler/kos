#include <stdio.h>

int main(int argc, char **argv)
{
#if defined(V3)
	char buffer[1024];

	printf("$ ");

#elif defined(V2)
	printf("%s\n", argv[0]);
#else
	int i=1;
	for (; i < argc; ++i) {
		printf("%s%c", argv[i], i==argc-1 ? '\n' : ' ');
	}
#endif

	return 0;
}
