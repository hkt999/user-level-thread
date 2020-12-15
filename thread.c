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
#include "thread.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_THREAD  256
#define THREAD_SIGNATURE	0x82

typedef struct _th_system_t {
	thread_t *main_thread;
	thread_t *active_thread;
	thread_t active_thread_slot[MAX_THREAD];
	thread_t *free_thread_list;
	int count;
	int ready_count;
	int sleep_count;
	uint64_t start;
} th_system_t;

static th_system_t THREAD;

uint64_t tick_ms() {
	struct timeval  tv;
	gettimeofday(&tv, NULL);

	return (uint64_t)((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 - THREAD.start);
}

static thread_t * ALLOC_THREAD_SLOT()
{
    thread_t *ret = THREAD.free_thread_list;
    THREAD.free_thread_list = THREAD.free_thread_list->th_next;
	
	memset( ret, 0, sizeof( thread_t) );

	THREAD.count++;
    return ret;
}

static void free_thread_slot(
	thread_t *th)
{
    memset( th, 0, sizeof( thread_t ) );
	th->th_next = THREAD.free_thread_list;
	THREAD.free_thread_list = th;
	THREAD.count--;
}


static void thread_free(
	thread_t *th)
{
    platform_free_context( th ); /* free thread stack */
    free_thread_slot( th );
}


static void thread_change_state(
	thread_t *th,
	int state,
	int alert)
{
	if ((int)th->th_state==state)
		return; /* nothing change */

	/* chage the state of a given thread */
	int old_state = th->th_state;
	th->th_state = state;
	if (old_state == THREAD_STATE_READY) {
		if (state != THREAD_STATE_READY)
			THREAD.ready_count--;
	} else if (state == THREAD_STATE_READY) {
		THREAD.ready_count++;
	}

	if (old_state == THREAD_STATE_SLEEP) {
		if (state != THREAD_STATE_SLEEP)
			THREAD.sleep_count--;
	} else if (state == THREAD_STATE_SLEEP) {
		THREAD.sleep_count++;
	}

	th->th_accumSwitch[state]++;

	if (alert&&th->th_alert) {
		if (th->th_alert)
			th->th_alert(th, old_state, state);
	}
}

static thread_t *pick_thread(thread_t *th, uint64_t now_ms)
{
    if (th) {
		if (th->th_signature == THREAD_SIGNATURE) {
			if (th->th_state == THREAD_STATE_SLEEP) {
				if (now_ms >= th->expired_ms) {
					thread_change_state( th, THREAD_STATE_READY, DO_ALERT );
					th->expired_ms = 0; /* reset expired timer */
				}
			}

			if ((th->th_state != THREAD_STATE_READY))
            	th = NULL;
		} else {
			printf("Fatal error: the thread is corrupted\n");
			exit(1);
		}
    } else { /* if no thread is selected, choose a thread by scheduler (round-robin)*/
		/* if the current thread is in sleeping, then check if it is expired or not */
		th = THREAD.active_thread;
		if (th->th_state == THREAD_STATE_SLEEP) {
			if (now_ms >= th->expired_ms) {
				/* wakeup switch state to ready */
				thread_change_state( th, THREAD_STATE_READY, DO_ALERT );
				th->expired_ms = 0; /* clear sleep timeout */
				return th;
			}
		} 
		th = THREAD.active_thread->th_next;
		while (th!=THREAD.active_thread) {
			switch (th->th_state) {
				case THREAD_STATE_READY:
					break;

				case THREAD_STATE_SLEEP:
					if (now_ms >= th->expired_ms) {
						/* wakeup switch state to ready */
						thread_change_state( th, THREAD_STATE_READY, DO_ALERT );
						th->expired_ms = 0; /* clear sleep timeout */
					}
					break;

				case THREAD_STATE_CLEAR: {
					thread_t *new_th = th->th_next;
					th->th_prev->th_next = th->th_next;
					th->th_next->th_prev = th->th_prev;
					thread_free( th );
					th = new_th;
					continue; /* continue the while loop */
					}

				case THREAD_STATE_TERMINATE:
					thread_change_state( th, THREAD_STATE_CLEAR, DO_ALERT );
					break;
			} /* switch */

			if (th->th_state==THREAD_STATE_READY)
				break; /* break the while loop */

			th = th->th_next;
		}

		/* if pick an active thread, then select nothing */
        if (th==THREAD.active_thread)
            th = NULL;
    }

    return th;
}

static void thread_stub()
{
    thread_t *th = (thread_t *)thread_self();

    th->th_entry( th->th_param );
	thread_change_state( th, THREAD_STATE_TERMINATE, DO_ALERT );

    // remove active thread, terminate the active thread
    // and resume the next thread

    thread_yield ( NULL );
}


void initial_thread_system()
{
	int i;
	thread_t *th;
	memset( &THREAD, 0, sizeof(th_system_t));
	THREAD.start = tick_ms();

	/* chain the free thread */
    for (i=0; i<MAX_THREAD-1; i++) {
		THREAD.active_thread_slot[i].th_next = &THREAD.active_thread_slot[i+1];
    }
    THREAD.active_thread_slot[i].th_next = NULL;
    THREAD.free_thread_list = THREAD.active_thread_slot;

    th = ALLOC_THREAD_SLOT(); // initial main thread data structure

    th->th_next = th;
	th->th_prev = th;

	THREAD.active_thread = th;
	THREAD.active_thread->th_signature = THREAD_SIGNATURE;
	THREAD.active_thread->th_state = THREAD_STATE_READY;
	THREAD.active_thread->th_name = "main thread";
	THREAD.main_thread = THREAD.active_thread;
	
	THREAD.ready_count = 1; /* current thread */
}


thread_t * thread_create(
	const char *name,
	thread_func_t func,
	void *param,
	int stacksize)
{
    thread_t *th;

    if (!THREAD.active_thread) { // initialize mapping thread management
        initial_thread_system();
    }

    if ((th=ALLOC_THREAD_SLOT())==NULL)
        return th;

    if (stacksize < 128*KB)
        stacksize = 128*KB;

	th->th_name = name;
    th->th_entry = func;
    th->th_param = param;
	th->th_alert = NULL; /* clear the alert function */
	th->th_kill_alert = NULL;
    th->th_parent = thread_self();
    th->th_signature = THREAD_SIGNATURE;
    th->th_signal = 0;
    th->th_errno = 0;
    th->th_suspcnt = 0;
    th->th_susplink = (thread_t *)NULL;

	/* create a platform dependent thread context */
	platform_create_context(th, stacksize, thread_stub);

	/* chain the thread structure */
    th->th_next = THREAD.active_thread;
    th->th_prev = THREAD.active_thread->th_prev;
    THREAD.active_thread->th_prev = th;
    th->th_prev->th_next = th;
	THREAD.ready_count ++;

    return th;
}


int	thread_set_alert(
	thread_t *th,
	alert_func_t alert)
{
	th->th_alert = alert;
	return 0;
}

int thread_set_kill_alert(thread_t *th, kill_alert_func_t alert)
{
	th->th_kill_alert = alert;
	return 0;
}


alert_func_t	thread_get_alert(thread_t *th)
{
	return th->th_alert;
}


int thread_terminate(thread_t *th)
{
    if (th==NULL)
        return -1;

    if (th->th_signature!=THREAD_SIGNATURE)
        return -1;

	thread_change_state(th, THREAD_STATE_TERMINATE, DO_ALERT);
    return 0;
}

static int thread_compare(const void *a, const void *b)
{
	thread_t **ta = (thread_t **)a;
	thread_t **tb = (thread_t **)b;
	if ((*ta)->th_accum == (*tb)->th_accum)
		return 0;

	return ((*tb)->th_accum - (*ta)->th_accum) > 0 ? 1 : -1;
}

void thread_dump()
{
	thread_t *thread_slot[MAX_THREAD];
	char buf[256];
	int i, num = 0;

	thread_t *th = THREAD.active_thread;
	do {
		thread_slot[num++] = th;
		th = th->th_next;
	} while (th != THREAD.active_thread);

	qsort(thread_slot, num, sizeof(thread_t *), thread_compare);

	printf("---- thread information ----\n");
	printf("thread ready count: %d\n", THREAD.ready_count);
	printf("thread sleep count: %d\n", THREAD.sleep_count);

	for (i=0; i<num; i++) {
		sprintf(buf, "[%s]", thread_slot[i]->th_name);
		int len = strlen(buf);
		int diff = 40 - len;
		if (diff > 0) {
			while (diff-->0) {
				buf[len++] = ' ';
			}
			buf[len] = 0;
		}
		printf("%s ", buf);
			
		printf("%8d(C) %5d(R), %5d(S), %5d(SL) time(%lu)\n",
			thread_slot[i]->th_accum,
			thread_slot[i]->th_accumSwitch[THREAD_STATE_READY],
			thread_slot[i]->th_accumSwitch[THREAD_STATE_SUSPEND],
			thread_slot[i]->th_accumSwitch[THREAD_STATE_SLEEP],
			thread_slot[i]->expired_ms);

		thread_slot[i]->th_accum = 0;
	}
	printf("---------------------------\n");
}

thread_signal_t	thread_yield(thread_t *th)
{
	thread_t *cur_thread = thread_self();
	uint64_t now_ms;
	
	while (1) {
		if (THREAD.ready_count == 0) {
			/* there is no ready thread, host sleep for a while */
			usleep(10000); /* 10ms */
		}
		now_ms = tick_ms();

    	if ((th = pick_thread(th, now_ms)) != NULL) {
        	if (th == THREAD.active_thread)
				break; /* no other thread to yield to */

			/*
			 * switch context to DEST_THREAD 
			 * context switch may jump to stub call, so we need set 
			 * ACTIVE_THREAD here
			 */
			THREAD.active_thread = th;
        	platform_context_switch( cur_thread, th ); 
			THREAD.active_thread = cur_thread;
			break; /* return to destination thread */
    	} else {
			/* no other thread to schedule to, so check the current active thread */
			if (THREAD.active_thread->th_state == THREAD_STATE_READY) {
				break; /* no other ready thread */
			} else {
				continue; /* check next time slot */
			}
		}
	}
	THREAD.active_thread->th_accum++;
    return THREAD.active_thread->th_signal;
}

thread_t * thread_self(void)
{
	return THREAD.active_thread;;
}


int thread_suspend(thread_t *th)
{
	if (!th)
		th = THREAD.active_thread;;

	if (th->th_signature!=THREAD_SIGNATURE)
		return -1; // fail

	if (++th->th_suspcnt>0) {
		thread_change_state( th, THREAD_STATE_SUSPEND, DO_ALERT );
		th->expired_ms = 0; /* clear the expired time out */
	}

	if (th==THREAD.active_thread) {
		thread_yield( NULL );
	}

	return 0;
}

int thread_resume(thread_t *th)
{
	if (th&&th->th_signature!=THREAD_SIGNATURE)
		return -1; // fail

	if (--th->th_suspcnt<=0) { 
		thread_change_state(th, THREAD_STATE_READY, DO_ALERT);
		th->th_suspcnt = 0;
		th->expired_ms = 0; /* clear the expired time out */
	}

	return 0;
}

int thread_resume_force(thread_t *th)
{
	if (th&&th->th_signature!=THREAD_SIGNATURE)
		return -1; // fail

	th->th_suspcnt = 0;
	if (th->th_state != THREAD_STATE_READY)
		thread_change_state( th, THREAD_STATE_READY, DO_ALERT);
		
	th->expired_ms = 0; /* clear the expired time out */

	return 0;
}

int thread_resume_from_interrupt(thread_t *th)
{
	if (th&&th->th_signature!=THREAD_SIGNATURE)
		return -1; // fail

	th->th_suspcnt = 0;
	if (th->th_state != THREAD_STATE_READY)
		thread_change_state( th, THREAD_STATE_READY, NO_ALERT);
		
	th->expired_ms = 0; /* clear the expired time out */

	return 0;

}

int thread_sleep(u_int msecs)
{
	thread_signal_t	sig;
	thread_t *th = THREAD.active_thread;

	th->expired_ms = msecs + tick_ms();
	thread_change_state(th, THREAD_STATE_SLEEP, DO_ALERT);

	return thread_yield(NULL); /* yield to other thread */
}

int thread_wake_up(thread_t *th)
{
	if (th==NULL) 
		return -1;

	if (th->th_signature!=THREAD_SIGNATURE) {
		return -2;
	}

	if (th->th_state==THREAD_STATE_SLEEP) {
		thread_change_state(th, THREAD_STATE_READY, DO_ALERT);
		th->expired_ms = 0;
		return 0;
	}

	return -3; /* the thread is not sleeping */
}


int thread_kill(thread_t *th, thread_signal_t event)
{
    if (th&&th->th_signature==THREAD_SIGNATURE) {
        th->th_signal = event;
		if (th->th_state==THREAD_STATE_SLEEP) { 
			/* WAKE UP !! */
			thread_change_state(th, THREAD_STATE_READY, DO_ALERT);
			th->expired_ms = 0;
		}

		if (th->th_kill_alert)
			th->th_kill_alert( thread_self(), th, event );
	}

    return 0;
}

thread_signal_t thread_poll_signal(void)
{
    return THREAD.active_thread->th_signal;
}


void thread_reset_signal(void)
{
    THREAD.active_thread->th_signal = 0;
}

int thread_errno()
{
	return THREAD.active_thread->th_errno;
}
