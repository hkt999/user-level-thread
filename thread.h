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
#ifndef _THREAD_H_
#define _THREAD_H_

#include <stdint.h>
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * constants
 */

#ifndef KB
#define	KB	1024
#endif /* KB */

#define THREAD_DEFAULT_STACK_SIZE   (64 * KB)

enum {
    THREAD_STATE_READY = 0,
    THREAD_STATE_SUSPEND,
	THREAD_STATE_SLEEP,
    THREAD_STATE_TERMINATE,
    THREAD_STATE_CLEAR,
	THREAD_NUM_STATE
};

#define DO_ALERT	1
#define NO_ALERT	0

/* forward references */
struct _thread;

/* required type definitions */
/* thread entry function */
typedef void (*thread_func_t) (void *param);

/* state alert function */
typedef void (*alert_func_t) (struct _thread *th, int old_state, int new_state);
typedef void (*kill_alert_func_t)(struct _thread *caller, 
	struct _thread *callee, int signal);

/* signal to thread */
typedef unsigned int	thread_signal_t;
typedef unsigned int	u_int;

/*
 * user-defined thread abstractions
 */
		
struct _thread {
	th_context_t th_context;

	const char 		*th_name;
    /* entry */
    thread_func_t	th_entry;
    void			*th_param;
	
	/* alert function */
	alert_func_t		th_alert;
	kill_alert_func_t	th_kill_alert;

    /* relationship */
    struct _thread  *th_parent;

    /* ipc */
    int				th_errno;
    thread_signal_t th_signal;

    /* management */
    u_int       th_suspcnt;
	uint64_t	expired_ms; 

    /* signature */
    uint32_t   th_signature;

    /* thread state */
    u_int       th_state;
	uint32_t 	th_accum;
	uint32_t	th_accumSwitch[THREAD_NUM_STATE];

    
    /* for memory allocate fail*/
    struct _thread	   *th_susplink;    
    
    struct _thread		*th_prev;
    struct _thread		*th_next;
};

typedef struct _thread thread_t;
/*
 * thread signal for the field "th_signal"
 */
#define THREAD_SIGHUP		0x00000001	/* hungup */
#define THREAD_SIGINT		0x00000002	/* terminal interrupt char */
#define THREAD_SIGQUIT		0x00000004	/* terminal quit char */
#define THREAD_SIGILL		0x00000008	/* illegal hardware inst. */
#define THREAD_SIGTRAP		0x00000010	/* hardware fault */
#define THREAD_SIGABRT		0x00000020	/* abnormal termination */
#define THREAD_SIGIOT		0x00000040	/* hardware fault */
#define THREAD_SIGBUS		0x00000080	/* bus error */
#define THREAD_SIGFPE		0x00000100	/* arithmetic exception */
#define THREAD_SIGKILL		0x00000200	/* termination */
#define THREAD_SIGUSR1		0x00000400	/* user-defined signal */
#define THREAD_SIGSEGV		0x00000800	/* invalid memory reference */
#define THREAD_SIGUSR2		0x00001000	/* user-defined signal */
#define THREAD_SIGPIPE		0x00002000	/* broken pipe */
#define THREAD_SIGALRM		0x00004000	/* timeout */
#define THREAD_SIGTERM		0x00008000	/* termination */
#define THREAD_SIGSTKFLT	0x00010000	/* ? */
#define THREAD_SIGCHLD		0x00020000	/* change in status of child */
#define THREAD_SIGCONT		0x00040000	/* continue stopped process */
#define THREAD_SIGSTOP		0x00080000	/* stop */
#define THREAD_SIGTSTP		0x00100000	/* terminal stop char */
#define THREAD_SIGTTIN		0x00200000	/* bkgrnd read from ctrl tty */
#define THREAD_SIGTTOU		0x00400000	/* bkgrnd write to ctrl tty */
#define THREAD_SIGURG		0x00800000	/* urgent condition */
#define THREAD_SIGXCPU		0x01000000	/* CPU limit exceeded */
#define THREAD_SIGXFSZ		0x02000000	/* file size limit exceeded */
#define THREAD_SIGVTALRM	0x04000000	/* virtual time alarm */
#define THREAD_SIGPROF		0x08000000	/* profiling time alarm */
#define THREAD_SIGWINCH		0x10000000	/* terminal window sz change */
#define THREAD_SIGIO		0x20000000	/* asynchronous I/O */
#define THREAD_SIGPOLL		THREAD_SIGIO	/* pollable event */


/*
 * macros
 */
#define thread_is_suspended(th)	    ((th)->th_suspcnt >= 1)
#define thread_is_ready(th)	    ((th)->th_suspcnt == 0)

/*
 * basic
 */
void			initial_thread_system();
thread_t		*thread_create(const char *name, thread_func_t func, void *param, int stacksize);
int				thread_set_alert(thread_t *th, alert_func_t alert);
int 			thread_set_kill_alert(thread_t *th, kill_alert_func_t alert);
alert_func_t	thread_get_alert(thread_t *th);

int				thread_terminate(thread_t *th);
thread_signal_t	thread_yield(thread_t *th);
thread_t		*thread_self(void);
int				thread_suspend(thread_t *th);
int				thread_resume(thread_t *th);
uint64_t		thread_get_min_expired(uint64_t now_tick);

/* alternative suspend, resume. */
int				thread_resume_force(thread_t *th);

/* sleep */
int				thread_sleep(u_int msecs);
int				thread_wake_up(thread_t *th); /* wake up the thread */

/* signal */
int				thread_kill(thread_t *th, thread_signal_t event);
thread_signal_t thread_poll_signal(void);
void			thread_reset_signal(void);

/* errno of the current thread */
int				thread_errno(void);

/*
 * debug routines
 */
void			thread_dump(void);

void     thread_main_loop(void);
int      thread_total(void);
thread_t *thread_main(void);

typedef void (*pfunc_t)();
void platform_context_switch(thread_t *from_th, thread_t *to_th);     
void platform_create_context(thread_t *th, int stacksize, pfunc_t f);
void platform_free_context(thread_t *th);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _THREAD_H_ */
