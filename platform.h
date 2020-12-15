#ifndef __THREAD_PLATFORM_H_
#define __THREAD_PLATFORM_H_

#include <ucontext.h>

typedef struct th_context_t_
{
	ucontext_t p;
	void *th_stack;
} th_context_t;

#endif /* __THREAD_PLATFORM_H_ */

