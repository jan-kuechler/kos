#include "console.h"

static void wait(void)
{
	unsigned int i=0, j=0;
	for (; i < 0xFFFFFL; ++i) {
		for (j=0; j < 2; ++j) {

		}
	}
}

void task1(void)
{
	con_puts("Hello from task 1\n");
	wait();

	for (;;) {
		con_puts("Task 1\n");
		wait();
	}
}

void task2(void)
{
	con_puts("Hello from task 2\n");
	wait();

	for (;;) {
		con_puts("Task 2\n");
		wait();
	}
}

void task3(void)
{
	con_puts("Hello from task 3\n");
	wait();

	for (;;) {
		con_puts("Task 3\n");
		wait();
	}
}

void task4(void)
{
	con_puts("Hello from task 4\n");
	wait();

	for (;;) {
		con_puts("Task 4\n");
		wait();
	}
}

void task5(void)
{
	con_puts("Hello from task 5\n");
	wait();

	for (;;) {
		con_puts("Task 5\n");
		wait();
	}
}
