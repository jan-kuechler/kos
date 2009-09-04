#include <stdio.h>

#define KMEM 0x100000
#define NO_PAGE 0x70000000

#define _U __attribute__((unused))

#define MEMREAD(name,where) \
	void name() { \
	  char *ptr = (char*)where; \
	  char _U test = *ptr; \
	}

#define MEMWRITE(name,where) \
	void name() { \
	  char *ptr = (char*)where; \
	  *ptr = 1; \
	}

MEMREAD(null_read, NULL)
MEMREAD(kmem_read, KMEM)
MEMREAD(nopage_read, NO_PAGE)

MEMWRITE(null_write, NULL)
MEMWRITE(kmem_write, KMEM)
MEMWRITE(nopage_write, NO_PAGE)

void sti()
{
	asm volatile("sti");
}

typedef struct cmd
{
	const char *name;
	void (*func)(void);
} cmd;

#define NCMD 7
cmd cmds[NCMD] = {
	{"NULL pointer read", null_read},
	{"NULL pointer write", null_write},
	{"Kernel memory read", kmem_read},
	{"Kernel memory write", kmem_write},
	{"Page N/A read", nopage_read},
	{"Page N/A write", nopage_write},
	{"asm(\"sti\")", sti},
};


int main(int argc, char **argv)
{
	int i=0;

	printf("Radau!\n\n");

	for (; i < NCMD; ++i) {
		printf("  %d - %s\n", i, cmds[i].name);
	}

	printf("\n");

	while (1) {
		printf("Was solls denn heute sein? ");
		scanf("%d", &i);

		if (i < 0 || i >= NCMD) {
			printf("???\n");
			continue;
		}

		cmds[i].func();
		printf("ARGH...");
	}

	return 42;
}
