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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "platform.h"
#include "thread.h"

static thread_t *MAIN_THREAD;
void
platform_init_main_thread(
	thread_t *main_thread
)
{
	/* create main thread context */
	MAIN_THREAD = main_thread;
	memset(&main_thread->th_context, 0, sizeof(th_context_t));
}

void
platform_context_switch(
	thread_t *from_th,
	thread_t *to_th
)
{
	swapcontext(&from_th->th_context.p, &to_th->th_context.p);
}

void
platform_create_context(
	thread_t *th,
	int stacksize,
	pfunc_t func
)
{
	memset(&th->th_context, 0, sizeof(th_context_t));
    th->th_context.th_stack = (void *)malloc( stacksize );

	assert(th->th_context.th_stack != 0);
	bzero(th->th_context.th_stack, stacksize);

	getcontext(&th->th_context.p);

	th->th_context.p.uc_link = &MAIN_THREAD->th_context.p;
	th->th_context.p.uc_stack.ss_sp = th->th_context.th_stack;
	th->th_context.p.uc_stack.ss_size = stacksize;
	makecontext(&th->th_context.p, func, 0);
}

void
platform_free_context(
	thread_t *th
)
{
	free(th->th_context.th_stack);
}

