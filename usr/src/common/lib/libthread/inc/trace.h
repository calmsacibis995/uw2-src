/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_TRACE_H
#define _SYS_TRACE_H

#ident	"@(#)libthread:common/lib/libthread/inc/trace.h	1.3.2.2"

#include <synch.h>
#include <thread.h>
#include <stdio.h>
#include <libthread.h>
#include <sys/usync.h>

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * this file defines structures, functions, and macros used by trace
 */

/*
 * Following are macros defining the names used in calls to TRACE*()
 * These macros correspond to the category and event being traced.
 */
/* trace categories */
#define TR_CAT_THREAD		1
#define TR_CAT_MUTEX		2
#define TR_CAT_COND		3
#define TR_CAT_SEMA		4
#define TR_CAT_RWLOCK		5
#define TR_CAT_RMUTEX		6
#define TR_CAT_BARRIER		7
#define TR_CAT_BARRIER_SPIN	8
#define TR_CAT_SPIN		9

/* events in TR_CAT_THREAD category */
#define TR_EV_THR_CREATE	1
#define TR_EV_THR_EXIT		2
#define TR_EV_THR_JOIN		3
#define TR_EV_THR_SELF		4
#define TR_EV_THR_MINSTACK	5
#define TR_EV_THR_CONTINUE	6
#define TR_EV_THR_SUSPEND	7
#define TR_EV_THR_SETCONC	8
#define TR_EV_THR_GETCONC	9
#define TR_EV_THR_KILL		10
#define TR_EV_THR_SIGSETMASK	11
#define TR_EV_THR_SETSCHED	12
#define TR_EV_THR_GETSCHED	13
#define TR_EV_THR_SETPRIO	14
#define TR_EV_THR_GETPRIO	15
#define TR_EV_THR_YIELD		16
#define TR_EV_THR_GET_RR	17
#define TR_EV_THR_KEYCREATE	18
#define TR_EV_THR_KEYDELETE	19
#define TR_EV_THR_SETSPECIFIC	20
#define TR_EV_THR_GETSPECIFIC	21

/* events in TR_CAT_MUTEX category */
#define TR_EV_MINIT		1
#define TR_EV_MLOCK		2
#define TR_EV_MTRYLOCK		3
#define TR_EV_MUNLOCK		4
#define TR_EV_MDESTROY		5

/* events in TR_CAT_COND category */
#define TR_EV_CINIT		1
#define TR_EV_CSIGNAL		2
#define TR_EV_CBROADCAST	3
#define TR_EV_CWAIT		4
#define TR_EV_CTIMEDWAIT	5
#define TR_EV_CDESTROY		6

/* events in TR_CAT_SEMA category */
#define TR_EV_SINIT		1
#define TR_EV_SWAIT		2
#define TR_EV_STRYWAIT		3
#define TR_EV_SPOST		4
#define TR_EV_SDESTROY		5

/* events in TR_CAT_RWLOCK category */
#define TR_EV_RWINIT		1
#define TR_EV_RW_RDLOCK		2
#define TR_EV_RW_WRLOCK		3
#define TR_EV_RW_UNLOCK		4
#define TR_EV_RW_TRYRD		5
#define TR_EV_RW_TRYWR		6
#define TR_EV_RWDESTROY		7

/* events in TR_CAT_RMUTEX category */
#define TR_EV_RMINIT		1
#define TR_EV_RMLOCK		2
#define TR_EV_RMTRYLOCK		3
#define TR_EV_RMUNLOCK		4
#define TR_EV_RMDESTROY		5

/* events in TR_CAT_BARRIER category */
#define TR_EV_BINIT		1
#define TR_EV_BWAIT		2
#define TR_EV_BDESTROY		3

/* events in TR_CAT_BARRIER_SPIN category */
#define TR_EV_BSINIT		1
#define TR_EV_BSWAIT		2
#define TR_EV_BSDESTROY		3

/* events in TR_CAT_SPIN category */
#define TR_EV_SPINIT		1
#define TR_EV_SPLOCK		2
#define TR_EV_SPTRYLOCK		3
#define TR_EV_SPUNLOCK		4
#define TR_EV_SPDESTROY		5

/* following values are for the which field */
#define TR_CALL_ONLY		0
#define TR_CALL_FIRST		1
#define TR_CALL_SECOND		2

/* following values are for certain arg fields */
#define TR_DATA_DONTKNOW	0
#define TR_DATA_BLOCK		1
#define TR_DATA_NOBLOCK		2
#define TR_DATA_WAITER		3
#define TR_DATA_NOWAITER	4


#ifndef TRACE

#define TRACE5(curtp, ev1, ev2, which, arg1, arg2, arg3, arg4, arg5)
#define TRACE4(curtp, ev1, ev2, which, arg1, arg2, arg3, arg4)
#define TRACE3(curtp, ev1, ev2, which, arg1, arg2, arg3)
#define TRACE2(curtp, ev1, ev2, which, arg1, arg2)
#define TRACE1(curtp, ev1, ev2, which, arg1)
#define TRACE0(curtp, ev1, ev2, which)

#else /* TRACE is defined */

#define TRACE5(curtp, ev1, ev2, which, arg1, arg2, arg3, arg4, arg5)           \
	((void) (((1 << ((ev1) - 1)) & _thr_trace_categories) &&               \
	 (_thr_trace_event((thread_desc_t *)curtp, (short)(ev1), (short)(ev2), \
	 (short)which, (long)arg1, (long)arg2, (long)arg3, (long)arg4,         \
	 (long)arg5), 1)))

#define TRACE4(curtp, ev1, ev2, which, arg1, arg2, arg3, arg4)                 \
	((void) (((1 << ((ev1) - 1)) & _thr_trace_categories) &&               \
	 (_thr_trace_event((thread_desc_t *)curtp, (short)(ev1), (short)(ev2), \
	 (short)which, (long)arg1, (long)arg2, (long)arg3, (long)arg4, 0), 1)))

#define TRACE3(curtp, ev1, ev2, which, arg1, arg2, arg3)                       \
	((void) (((1 << ((ev1) - 1)) & _thr_trace_categories) &&               \
	 (_thr_trace_event((thread_desc_t *)curtp, (short)(ev1), (short)(ev2), \
	 (short)which, (long)arg1, (long)arg2, (long)arg3, 0, 0), 1)))

#define TRACE2(curtp, ev1, ev2, which, arg1, arg2)                             \
	((void) (((1 << ((ev1) - 1)) & _thr_trace_categories) &&               \
	 (_thr_trace_event((thread_desc_t *)curtp, (short)(ev1), (short)(ev2), \
	 (short)which, (long)arg1, (long)arg2, 0, 0, 0), 1)))

#define TRACE1(curtp, ev1, ev2, which, arg1)                                   \
	((void) (((1 << ((ev1) - 1)) & _thr_trace_categories) &&               \
	 (_thr_trace_event((thread_desc_t *)curtp, (short)(ev1), (short)(ev2), \
	 (short)which, (long)arg1, 0, 0, 0, 0), 1)))

#define TRACE0(curtp, ev1, ev2, which)                                         \
	((void) (((1 << ((ev1) - 1)) & _thr_trace_categories) &&               \
	 (_thr_trace_event((thread_desc_t *)curtp, (short)(ev1), (short)(ev2), \
	 (short)which, 0, 0, 0, 0, 0), 1)))

#define	LOCK_TRACE		_lwp_mutex_lock(&_thr_trace_lock)
#define UNLOCK_TRACE		_lwp_mutex_unlock(&_thr_trace_lock)
#define IS_TRACE_LOCKED          LWP_MUTEX_ISLOCKED(&_thr_trace_lock)
#define TRACE_COND_WAIT		_lwp_cond_wait(&_thr_trace_cond,&_thr_trace_lock);

extern	const int       _thr_trace_category_count; /* trace category count    */
extern	boolean_t	_thr_trace_initialized; /* trace initialized?         */
extern	pid_t		_thr_trace_pid;		/* current process ID         */
extern	int		_thr_trace_forkcount;	/* count of pending fork()s   */
extern	lwp_mutex_t	_thr_trace_lock;	/* protects global trace vars */
extern	lwp_cond_t	_thr_trace_cond;	/* used during fork()s        */

extern	long		_thr_trace_categories;	/* trace category bit pattern */
extern	char		_thr_trace_dir[MAXPATHLEN]; /* trace file directory   */
extern	int		_thr_trace_buf;		/* 'type' arg to setvbuf      */

/* the following routines are used for tracing */
extern	void	_thr_open_tracefile(thread_desc_t *);
extern	void	_thr_trace_event(thread_desc_t *, short, short,
	            short, long, long, long, long, long);
extern	void	_thr_trace_init(void);

#endif /* if !defined(TRACE) */
#endif	/* _SYS_TRACE_H */
