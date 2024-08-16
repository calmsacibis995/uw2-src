/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head.usr:thread.h	1.16"
#ifndef _THREAD_H
#define _THREAD_H
#ident	"$Header: $"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/priocntl.h>
#include <sys/time.h> 
#ifndef _UCONTEXT_H 
#include <ucontext.h>
#endif /* _UCONTEXT_H */

typedef	id_t	thread_t;		/* thread id */

/*
 * These types are presented here as a reasonable default, but they
 * may be extended, either through the XX_reserved member, or by imbedding
 * these types at the beginning of some larger object.  __thr_init() chains
 * these together.
 */
typedef struct __lwp_desc {
	lwpid_t lwp_id;			/* this lwp's lwpid           */
	struct __thread_desc *lwp_thread;	/* current thread descriptor */
	FILE    *tracefile;             /* pointer to trace file      */
	char    *buf;                   /* pointer to trace buffer    */
	pid_t   curpid;                 /* pid that opened tracefile  */
	void *lwp_reserved;		/* for thread library writers */
} __lwp_desc_t;

typedef struct __thread_desc {
	thread_t thr_id;		/* this thread's thread-id */
	int *thr_errp;			/* pointer to errno */
	void *thr_priv_datap;		/* per-thread private data */
	void *thr_reserved;		/* for thread library writers */
	int thr_errno;			/* per-thread errno */
} __thread_desc_t;

/*
 * per-thread information for debuggers;
 * this must be identical to the portion in thread_desc_t
 */
struct thread_map {
	ucontext_t	thr_ucontext;
	thread_t	thr_tid;
	void		*thr_lwpp;
	int		thr_state;
	long		thr_usropts;  /* usr options, (THR_BOUND, ...)  */
	sigset_t	thr_psig;
	sigset_t	thr_dbg_set;
	unsigned char	thr_dbg_cancel;
	unsigned char	thr_dbg_busy;
	unsigned char	thr_dbg_startup;
	unsigned char	thr_dbg_switch;
	void		*(*thr_start_addr)(void *);
	void		*thr_next;
} ;

/*
 * macros defining different scheduling states of a thread.
 * Sleeping, currently runnable but not executing on an lwp, executing
 * on an lwp, dead but the threads resources have not been cleaned up,
 * and suspended.
 */

#define TS_ONPROC       0
#define TS_SLEEP        1
#define TS_RUNNABLE     2
#define TS_ZOMBIE       3
#define TS_SUSPENDED    4

/* macros for use of debuggers - debuggers should not rely directly
 * on values of the thread states
 */

#define THR_MAP(T)		((struct thread_map *)(((char *)(T)) +\
					sizeof(thrq_elt_t)))
#define	THR_MAP_ON_LWP(T)	((T)->thr_state == TS_ONPROC)
#define THR_MAP_IS_SUSPENDED(T) ((T)->thr_state == TS_SUSPENDED)
#define THR_MAP_IS_ZOMBIE(T)    ((T)->thr_state == TS_ZOMBIE)

/* values for 2nd argument to debugger notification function */

enum	thread_change {
	tc_invalid,
	tc_thread_create,
	tc_thread_exit,
	tc_switch_begin,
	tc_switch_complete,
	tc_cancel_complete,
	tc_thread_suspend,
	tc_thread_suspend_pending,
	tc_thread_continue
} ;

/* per-process information for debuggers */

struct thread_debug {
	void **thr_map;	/* address of ptr to first thread on list */
	void (*thr_brk)(struct thread_map *, enum thread_change);
			/* address of notification function */
	int  thr_debug_on;
                /* set by debugger to indicate its presence, 0 if
                 * process is not being debugged
                 */
} ;

extern struct thread_debug	_thr_debug;

/*
 * flags for thr_create
 */

#define THR_SUSPENDED 0x1
#define THR_BOUND     0x2 
#define THR_NEW_LWP   0x4
#define THR_INCR_CONC 0x4
#define THR_DETACHED  0x8 
#define THR_DAEMON    0x10

/*
 * timer defines which can be used in conversion between timeval and timestruc_t
 */

#define MICROSEC	1000000
#define NANOSEC         1000000000

/*
 * Scheduling policies supported by the library threads.
 */

#define SCHED_UNKNOWN		0		/* Unknown Policy */
#define SCHED_TS		1		/* Time-Sharing	*/
#define SCHED_OTHER             1               /* POSIX defined Multiplexed threads Policy */
#define SCHED_FIFO		2		/* First-In-First-Out */
#define SCHED_RR		3               /* Round-Robin	*/

#define POLICY_PARAM_SZ		PC_CLPARMSZ	/* (32/sizeof(long)) */

typedef struct {
	id_t	policy;
	long	policy_params[POLICY_PARAM_SZ];
} sched_param_t;

struct ts_param {
	int	prio;
};

struct fifo_param {
	int     prio;
};

struct rr_param {
	int     prio;
};

/* Keys for thread-specific data */
typedef unsigned int thread_key_t;

#if defined(__STDC__)
int	 thr_create(void *, size_t, void *(*)(void *), void *, long, 
								thread_t *);
int	 thr_join(thread_t, thread_t *, void **);
void	 thr_exit(void *);
thread_t thr_self(void);
int	 thr_setconcurrency(int);
int	 thr_getconcurrency(void);
int	 thr_setscheduler(thread_t, const sched_param_t *);
int	 thr_getscheduler(thread_t, sched_param_t *);
void	 thr_get_rr_interval(timestruc_t *);
int	 thr_setprio(thread_t, int);
int	 thr_getprio(thread_t, int *);
void	 thr_yield(void);
int	 thr_suspend(thread_t);
int	 thr_continue(thread_t);
int	 thr_kill(thread_t, int);
int	 thr_sigsetmask(int, const sigset_t *, sigset_t *);
size_t	 thr_minstack(void);
int	 thr_keycreate(thread_key_t *key, void (*destructor)(void *));
int	 thr_keydelete(thread_key_t key);
int	 thr_setspecific(thread_key_t key, void *value);
int	 thr_getspecific(thread_key_t key, void **retval);
#else
int	 thr_create();
int	 thr_join();
void	 thr_exit();
thread_t thr_self();
int	 thr_setconcurrency();
int	 thr_getconcurrency();
int	 thr_setscheduler();
int	 thr_getscheduler();
void	 thr_get_rr_interval();
int	 thr_setprio();
int	 thr_getprio();
void	 thr_yield();
int	 thr_suspend();
int	 thr_continue();
int	 thr_kill();
int	 thr_sigsetmask();
size_t	 thr_minstack();
int	 thr_keycreate();
int	 thr_keydelete();
int	 thr_setspecific();
int	 thr_getspecific();	/* XXX char * */
#endif /* defined(__STDC__) */

#ifdef __cplusplus
}
#endif

#endif /* _THREAD_H */
