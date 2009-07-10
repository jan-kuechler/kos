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

	return 0;
}
