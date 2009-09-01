#include <stdio.h>

#define KMEM 0x100000

int main(int argc, char **argv)
{
	int i=0;

	printf("Radau!\n\n");


	printf(" 01 - NULL pointer read\n");
	printf(" 02 - NULL pointer write\n");
	printf(" 03 - Kernel memory read\n");
	printf(" 04 - Kernel memory write\n");
	printf(" 02 - asm(\"sti\")\n");

	printf("\n");

	while (1) {
		printf("Was solls denn heute sein? ");
		scanf("%d", &i);

		switch (i) {
		case 1:
		{
			char *ptr = NULL;
			char test = *ptr;
			break;
		}

		case 2:
		{
			char *ptr = NULL;
			*ptr = 1;
			break;
		}

		case 3:
		{
			char *ptr = KMEM;
			char test = *ptr;
			break;
		}

		case 4:
		{
			char *ptr = KMEM;
			*ptr = 1;
			break;
		}

		case 5:
		{
			asm("sti");
			break;
		}

		default:
			printf("Bitte was??\n");
		}
	}

	printf("ARGH...");
}
