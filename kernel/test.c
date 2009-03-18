#include "console.h"
#include <kos/error.h>
#include <kos/syscall.h>

//#define print(a)

//#define print con_puts   /* direct */
#define print kos_puts  /* using syscalls */
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
	kos_exit(0);

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

	wait();
	wait();

	msg_t msg;
	msg.cmd    = 42;
	msg.subcmd = 1337;
	msg.param1 = 0xDEADBEEF;
	msg.param2 = 0xD00F;
	kos_send((pid_t)3, &msg);

	for (;;) {
		print("Task 2\n");
		wait();
	}
}

void task3(void)
{
	print("Hello from task 3\n");
	wait();

	msg_t msg;

	print("Task 3 waiting for a message...\n");
	byte status = kos_receive(&msg, 1);

	if (status == OK) {
		print("GOT A MESSAGE! CHEER!\n");

		if (msg.cmd == kos_get_answer())
			print("The answer to life, the universe and everything.\n");
		if (msg.subcmd == 1337)
			print("LEET!\n");
		if (msg.param1 == 0xDEADBEEF)
			print("Armes Rind )-:\n");
		if (msg.param2 == 0xD00F)
			print("Selber!\n");
	}

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

	print("5: Sleeping for 10 secs.\n");
	kos_sleep(10000);
	print("5: ZzzZzzzZzz... Huh?\n");

	for (;;) {
		print("Task 5\n");
		wait();
	}
}
