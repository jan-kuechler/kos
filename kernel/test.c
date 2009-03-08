#include "console.h"
#include <kos/syscall.h>

//#define print(a)

//#define print con_puts   /* direct */
#define print kos_print  /* using syscalls */
//#define print(x)         /* silent */

static void wait(void)
{
	unsigned int i=0, j=0;
	for (; i < 0xFFFFFL; ++i) {
		for (j=0; j < 25; ++j) {
			asm volatile("nop");
		}
	}
}

void task1(void)
{
	print("Hello from task 1\n");

	wait();

	int i=0;
	for (; i < 2 ; ++i) {
			print("Task 1\n");
			wait();
	}

	print("Task1 exiting!\n");
	kos_exit();

	print("EPIC FAIL!\n");

	for (;;) {
			print("Task 1\n");
			wait();
	}
}

void task2(void)
{
	print("Hello from task 2\n");
	wait();

	for (;;) {
		print("Task 2\n");
		wait();
	}
}

void task3(void)
{
	print("Hello from task 3\n");
	wait();

	for (;;) {
		print("Task 3\n");
		wait();
	}
}

void task4(void)
{
	print("Hello from task 4\n");
	wait();

	for (;;) {
		print("Task 4\n");
		wait();
	}
}

void task5(void)
{
	print("Hello from task 5\n");
	wait();

	for (;;) {
		print("Task 5\n");
		wait();
	}
}
