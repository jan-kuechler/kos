#include <stdio.h>
#include <stdlib.h>

#define MAGIC1 42
#define MAGIC2 13

#define LARGE   0x54321 /* large and not a page size multiple */
#define MEDIUM      512
#define SMALL        16

static void large()
{
	printf("Allocating a large block: ");
	char *large = malloc(LARGE);
	printf("done\n");

	printf("Accessing middle and last member: ");
	large[LARGE/2] = MAGIC1;
	large[LARGE-1] = MAGIC2;

	if (large[LARGE/2] != MAGIC1) {
		printf("error (middle)\n");
		goto end;
	}
	if (large[LARGE-1] != MAGIC2) {
		printf("error (last)\n");
		goto end;
	}
	printf("done\n");

end:
	printf("Freeing the block: ");
	free(large);
	printf("done\n");
}

static void medium()
{

}

static void small()
{

}

int main(int argc, char **argv)
{
	printf("Memtest\n");

	large();
	medium();
	small();
}
