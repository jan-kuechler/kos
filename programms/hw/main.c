#include <stdio.h>
#include <fcntl.h>

extern int syscall(int, int, int, int);

int main(int argc, char **argv)
{
	//printf("Hello World!\n");

	printf("Hello World!\n");

	printf("What's your name? ");

	char name[20];
	scanf("%s", name);

	printf("Great job, %s!\n", name);

	char buffer[128];
	printf("Any last words? ");
	scanf("%s", buffer);
	printf("%s... ok.\n", buffer);

	fflush(stdout);
	return 0;
}

int _write(int f, char *p, int l)
{
	return 0;
}
