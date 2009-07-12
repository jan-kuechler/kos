#include <stdio.h>


int main(int argc, char **argv)
{
	int i=0;

	printf("Radau!\n\n");


	printf(" 01 - NULL pointer access\n");
	printf(" 02 - asm(\"sti\")\n");

	printf("\n");

	while (1) {
		printf("Was solls heute sein? ");
		scanf("%d", &i);

		switch (i) {
		case 1:
		{
			char *ptr = NULL;
			*ptr = 1;
			break;
		}

		case 2:
		{
			asm("sti");
			break;
		}

		default:
			printf("Bitte was??\n");
		}
	}

}
