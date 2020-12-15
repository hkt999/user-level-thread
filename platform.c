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

