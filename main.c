/*
MIT License

Copyright (c) 2020 Kevin Huang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
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
		thread_sleep(5000);
		thread_dump();
	}

	return 0;
}
