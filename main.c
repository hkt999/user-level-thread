#include <stdio.h>
#include "thread.h"

void worker_func(void *param)
{
	intptr_t interval = (intptr_t)param;
	int i;

	for (i=0; i<10; i++) {
		printf("worker (%s) -- %d\n", thread_self()->th_name, i);
		thread_sleep(interval);
	}
	printf("end of workder (%s)\n", thread_self()->th_name);
}

int
main(int argc, char **argv)
{
	initial_thread_system();

	thread_create("thread1", worker_func, (void *)300, 64*KB);
	thread_create("thread2", worker_func, (void *)500, 64*KB);
	thread_create("thread3", worker_func, (void *)800, 64*KB);

	/* dump thread system status every 5s */
	while (1) {
		thread_sleep(10000);
		thread_dump();
	}

	return 0;
}
