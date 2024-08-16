/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/inc/libthread.h	1.7.14.40"

#ifndef _LIBTHREAD_H
#define _LIBTHREAD_H

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * libthread.h:
 *	struct thread and struct lwp definitions.
 */
#include <synonyms.h>
#include <libcsupp.h>
#include <limits.h>
#include <signal.h>
#include <siginfo.h>
#include <sys/ucontext.h>
#include <sys/reg.h>
#include <sys/param.h>
#include <archdep.h>
#include <errno.h>
#include <thread.h>
#include <synch.h>
#include <sys/lwp.h>
#include <debug.h>
#include <sys/list.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <thrsig.h>
#include <sys/usync.h>

#ifndef STATIC
#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif /* DEBUG */
#endif /* STATIC */

/*
 * default stack allocation parameters
 *
 * BPW defines the number of bits per word
 * DEFAULTSTACK  the default stack size allocated to a thread
 * DEFAULTSTACKINCR the number of default stacks created as a group when no
 * stacks are available on the stack cache.
 * MAXSTACKS the maximum number of stacks allowed on the free list of
 * available stack cache.
 * THR_MIN_STACK the minimum size stack, thread local storage plus
 * one stack frame.
 */
#define BPW 			(NBBY*NBPW)
#define DEFAULTSTACK		(MAX ((2*PAGESIZE), 0x4000))
#define DEFAULTSTACKINCR	8
#define MAXSTACKS		16
#define THR_MIN_STACK		(SA(MINFRAME + (int)_thr_tls_size()))

/* 
 * thread priority range.
 */
#define THREAD_MIN_PRIORITY	0	/* min scheduling priority */
#define THREAD_MAX_PRIORITY	127	/* max scheduling priority */
#define DISPQ_SIZE (THREAD_MAX_PRIORITY - THREAD_MIN_PRIORITY + 1)
#define INVALID_PRIO            -1      /* invalid prio value for _thr_setrq */

/* following assumes THREAD_MIN_PRIORITY and THREAD_MAX_PRIORITY both > 0 */
#define HASH_PRIORITY(p,index)                                   \
        if (p >= THREAD_MIN_PRIORITY && p <= THREAD_MAX_PRIORITY)\
                index = p - THREAD_MIN_PRIORITY;                 \
        else                                                     \
                index = DISPQ_SIZE - 1

/*
 * default values for multiplexed threads for scheduling class and
 * priority. Inhereited when a multiplexed child thread is created by
 * a bound thread.
 */
#define DEFAULTMUXPRI	63		/* default scheduling prioirity */
#define DEFAULTMUXCID	1

/*
 * MAXRUNWORD is the number of words in the bit vector that used to
 * determine if runnable threads are on a queue. One bit is allocated per
 * queue to enable fast searching for runnable threads.
 */
#define MAXRUNWORD		(THREAD_MAX_PRIORITY/BPW)
#define IDLE_THREAD_PRI		-1		/* idle thread's priority */
#define IDLETHREADID		-1		/* thread id of idle threads */
#define HOUSEKEEPERID		-2		/* thread id of housekeeper  */
#define TIMERID			-3		/* thread id of timer service */

/*
 * Default values for RR time quantum.
 */

#define RR_DFLTSECS             0               /* default secs     */
#define RR_DFLTNSECS            100000000       /* default nsecs     */

/*
 * Increment a value protected by the counter lock. For those
 * architectures where atomic increment is supported and efficient
 * locking macros can be removed.
 */
#define ATOMIC_INC(x)	LOCK_COUNTER; \
			(x)++; \
			UNLOCK_COUNTER

/*
 * Find the ID of the lwp that the thread is currently running on.
 */
#define LWPID(t) ((lwppriv_block_t *)(t)->t_lwpp)->l_lwpdesc.lwp_id
			
/*
 * curthread is the pointer to the thread structure of the
 * currently exectuing thread.
 */
#define curthread (&_thr_thread)

extern lwp_mutex_t _thr_tidveclock;	/* lock that protects tidvec array    */
extern lwp_mutex_t _thr_preemptlock;	/* lock that protects idle thread list*/
extern lwp_mutex_t _thr_counterlock;	/* lock that protects global counters */
extern lwp_mutex_t _thr_runnableqlock;	/* lock that protects runnable queue  */
extern sema_t      _thr_house_sema;     /* housekeeping thread sleeps on this */


/*
 * Macros that define locking and unlocking operations on internal
 * thread locks within the thread library. 
 *
 */

#define TRYLOCK_THREAD(t)	_lwp_mutex_trylock(&((t)->t_lock))

#ifdef THR_DEBUG
#define LOCK_TIDVEC		_thr_lock_tidvec()
#define UNLOCK_TIDVEC		_thr_unlock_tidvec()

#define LOCK_COUNTER		_thr_lock_counter()
#define UNLOCK_COUNTER		_thr_unlock_counter()

#define LOCK_RUNQ		_thr_lock_runq()
#define UNLOCK_RUNQ		_thr_unlock_runq()

#define LOCK_THREAD(t)		_thr_lock_thread(t)
#define UNLOCK_THREAD(t)	_thr_unlock_thread(t)

#define LOCK_PREEMPTLOCK	_thr_lock_preemptlock()
#define UNLOCK_PREEMPTLOCK	_thr_unlock_preemptlock()
#else
#define LOCK_TIDVEC		_lwp_mutex_lock(&_thr_tidveclock)
#define UNLOCK_TIDVEC		_lwp_mutex_unlock(&_thr_tidveclock)

#define LOCK_COUNTER		_lwp_mutex_lock(&_thr_counterlock)
#define UNLOCK_COUNTER		_lwp_mutex_unlock(&_thr_counterlock)

#define LOCK_RUNQ		_lwp_mutex_lock(&(_thr_runnableqlock))
#define UNLOCK_RUNQ		_lwp_mutex_unlock(&(_thr_runnableqlock))

#define LOCK_THREAD(t)		_lwp_mutex_lock(&((t)->t_lock))
#define UNLOCK_THREAD(t)	_lwp_mutex_unlock(&((t)->t_lock))

#define LOCK_PREEMPTLOCK	_lwp_mutex_lock(&_thr_preemptlock)
#define UNLOCK_PREEMPTLOCK	_lwp_mutex_unlock(&_thr_preemptlock)
#endif /* THR_DEBUG */

#define IS_TIDVEC_LOCKED	LWP_MUTEX_ISLOCKED(&_thr_tidveclock)
#define IS_COUNTER_LOCKED  	LWP_MUTEX_ISLOCKED(&_thr_counterlock)
#define IS_PREEMPT_LOCKED	LWP_MUTEX_ISLOCKED(&_thr_preemptlock)
#define IS_RUNQ_LOCKED  	LWP_MUTEX_ISLOCKED(&_thr_runnableqlock)
#define IS_THREAD_LOCKED(t) 	LWP_MUTEX_ISLOCKED(&((t)->t_lock))
#define IS_CALLOUTQ_LOCKED      LWP_MUTEX_ISLOCKED(&_thr_callout_mutex)

#define LOCK_CALLOUTQ           _lwp_mutex_lock(&_thr_callout_mutex)
#define UNLOCK_CALLOUTQ         _lwp_mutex_unlock(&_thr_callout_mutex)

/*
 * Macros for operations on type thrq_elt_t defined in synch.h
 * THRQ_INIT initialized a thrq_elt_t structure.
 * THRQ_ISEMPTY tests queue empty condition.
 * THRQ_ONLY_MEMBER tests for a single element on the queue.
 */

#define THRQ_INIT(q)	{\
	((thrq_elt_t *)(q))->thrq_next =\
	((thrq_elt_t *)(q))->thrq_prev = NULL;\
}

#define THRQ_ISEMPTY(q)	(((thrq_elt_t *)(q))->thrq_next == NULL)

#define THRQ_ONLY_MEMBER(elt)	\
	(!(THRQ_ISEMPTY(elt)) && \
	(((thrq_elt_t *)(elt))->thrq_next == ((thrq_elt_t *)(elt))->thrq_prev))

#define _THRQ_ONLY_MEMBER(elt)  \
	(((thrq_elt_t *)(elt))->thrq_next == ((thrq_elt_t *)(elt))->thrq_prev)

/*
 * macros for suspending and continuing bound threads
 */
#define SUSPEND_BOUND_THREAD(tp) while (tp->t_state == TS_SUSPENDED) { \
	(void) _lwp_cond_wait(&(tp->t_suspcond), &(tp->t_lock)); }

#define CONTINUE_BOUND_THREAD(tp) _lwp_cond_signal(&(tp->t_suspcond))

/*
 * enumeration type defining differnt suspend states.
 */
enum	thr_sus  {TSUSP_CLEAR=0x0, TSUSP_PENDING=0x1};

/*
 * enumeration type defining different list types the thread is
 * currently on. No list, sleeping on a mutex, sleeping on a condition
 * variable, sleeping on a semaphore, sleeping on a libc synchronization
 * variable or on the run queue.
 */
enum	thr_list {TL_NONE, TL_MUTEX, TL_COND, TL_LIBC};

/*
 * enumeration type defining different thread flags. Used
 * to determine if the thread stack was provided by the thread library
 * or by the user, and if preemption is in progress.
 * T_SIGWAITUP flag is used to determine if sleep was interrupted by SIGWAITING.
 */

enum	thr_flags {T_ALLOCSTK=0x1, T_PREEMPT=0x2, T_SIGWAITUP=0x4};

/*
 * macros used in preemption; both PREEMPT_SELF and PREEMPT_THREAD
 * assume that the lock of the thread passed as the first argument is held
 * upon entry to the macro.
 */
#define PREEMPT_SELF(tp)                       \
		ASSERT(tp == curthread);       \
		ASSERT(IS_THREAD_LOCKED(tp));  \
		tp->t_flags |= T_PREEMPT;      \
		tp->t_state = TS_RUNNABLE;     \
		_thr_swtch(1, tp);	       \
		ASSERT(tp->t_state == TS_ONPROC);

#define PREEMPT_THREAD(tp)                                     \
		ASSERT(tp != curthread);                       \
		ASSERT(IS_THREAD_LOCKED(tp));                  \
		ASSERT(!(tp->t_flags & T_PREEMPT));            \
		if(tp->t_state == TS_ONPROC) {                 \
			_thr_sigsend(tp, SIGLWP);              \
			tp->t_flags |= T_PREEMPT;              \
		} else {                                       \
			if (tp->t_blockedonlwp == B_TRUE) {    \
				LOCK_RUNQ;                     \
				_lwp_cond_signal               \
				   (&tp->t_lazyswitch.condvar);\
				UNLOCK_RUNQ;                   \
			}                                      \
		}                                              \

/*
 * The following data structure is used for callout requests.
 */
typedef struct callo callo_t;

struct callo {
	thrq_elt_t	co_link;	/* forward/backward link */
	char		co_stat;	/* status*/
	timestruc_t	co_abstime;	/* absolute expiration time*/
	timestruc_t	co_interval;	/* interval time value */
	void		(*co_func)();	/* function */
	void		*co_arg;	/* argument to co_func */
};

/*
 * The status of a callout request can be one of the following.
 */

#define CO_TIMER_OFF    0
#define CO_TIMER_ON     1
#define CO_TIMEDOUT     2

/*
 * LWP private data block, one allocated for each LWP.
 */

typedef struct lwppriv_block lwppriv_block_t; /* lwp private data block */

struct lwppriv_block {
	__lwp_desc_t	l_lwpdesc;	/* lwp private shared with libc   */
	lwppriv_block_t *l_next;	/* link to the next lwppriv_block */
};

/*
 * Each thread descriptor contains a _thr_binding structure.  This structure
 * holds a pointer to an array of values bound to keys, and the number of
 * elements in the array.  The pointer is initially NULL, and the counter 0.
 */
struct _thr_binding {
	unsigned int    tb_count;
	void            **tb_data;
};

/*
 * Thread structure, allocated one per thread, contains most of the
 * important information that the thread library needs.
 */

#define t_ucontext	t_map.thr_ucontext
#define t_tid		t_map.thr_tid
#define t_lwpp		t_map.thr_lwpp
#define t_state		t_map.thr_state
#define t_psig		t_map.thr_psig
#define t_next		t_map.thr_next
#define t_usropts       t_map.thr_usropts
#define t_dbg_busy	t_map.thr_dbg_busy
#define t_dbg_cancel	t_map.thr_dbg_cancel
#define t_dbg_set	t_map.thr_dbg_set
#define t_dbg_startup   t_map.thr_dbg_startup
#define t_start_addr    t_map.thr_start_addr
#define t_dbg_switch    t_map.thr_dbg_switch

#define TNEXT(T)	((thread_desc_t *)(T)->t_map.thr_next)

/*
 * Is thread permanently bound to a LWP?
 */
#define ISBOUND(t)	(((t)->t_usropts & THR_BOUND))


typedef struct lazyblock lazyblock_t;

struct lazyblock {
	thrq_elt_t	thrq_lazy_elt;
	lwp_cond_t	condvar;
};

typedef struct thread thread_desc_t;

struct thread {
        /* list stuff */
        thrq_elt_t 	t_thrq_elt;     /* circular sleepq queue            */
	/*
	 * the following block is also needed by debug; the elements must be 
	 * kept in the same order as those in thread_map in thread.h
	 */
	struct thread_map	t_map;
	/*
	 * end of block for debug 
	 */
        thread_desc_t  *t_prev;         /* and _thr_deathrow pointers       */
        void           *t_sync_addr;    /* addr of object to sleep on       */
        enum thr_list   t_sync_type;    /* type of object being slept on    */
	lazyblock_t	t_lazyswitch;	/* queue and cond var for lazy switch*/

        /* thread context and characteristics */
	boolean_t	t_blockedonlwp; /* TRUE if mux thread lazy switched */
        char           *t_tls;          /* pointer to thread local storage  */
        enum thr_flags  t_flags;        /* flags, (T_ALLOCSTK, ...)         */
        int             t_pri;          /* scheduling priority              */
        int             t_cid;          /* scheduling class                 */
        boolean_t       t_usingfpu;     /* TRUE, if thread is using FPU	    */
        char            t_suspend;      /* suspend thread when set          */
	void           *t_exitval;      /* thr_exit value of this thread    */
	thread_desc_t  *t_idle;         /* pointer to idle thread           */
#define	t_suspcond   t_lazyswitch.condvar /* condvar for bound thread suspend*/
#define t_stk        t_ucontext.uc_stack.ss_sp     /* stack base of thread  */
#define t_stksize    t_ucontext.uc_stack.ss_size   /* size of thread's stack*/
#define t_sp         t_ucontext.uc_mcontext.gregs[R_ESP] /* stack pointer   */
#define t_pc         t_ucontext.uc_mcontext.gregs[R_EIP] /* program counter */

        /* sync stuff */
        lwp_mutex_t     t_lock;         /* mutex held to change thread      */
        cond_t          t_join;         /* cond variable for suspend/join   */
	int		t_joincount;	/* count of users to t_join         */

        /* signal stuff */

        char            t_nosig;        /* indicator to block signal handlers */
        char            t_sig;          /* signal received at critical region */
        sigset_t        t_hold;         /* per thread signal mask             */
        sigset_t        t_oldmask;      /* saved signal mask                  */

        /* timer stuff  */
        callo_t         t_callo_alarm;  /* alarm clock callout request      */
        callo_t         t_callo_realit; /* real interval timer callout req  */
        callo_t         t_callo_cv; 	/* cond var timedwait callout req   */
	struct _thr_binding t_binding;	/* TSD values */
};

/*
 * tls.h must be included at this point after the definition of thread_desc_t.
 * tls.h uses thread_desc_t. DO NOT MOVE this, anywhere in this file.
 */
#include <tls.h>

/*
 * Maximum number of terminated threads on the _thr_reaplist before
 * the reaper is called to reclaim thread resources.
 */
#define REAP_THRESHOLD		8

/* 
 * External thread ID representation is of thread_t (id_t type) and is
 * interpreted as follows:
 * the low order THR_ID_SIZE (set to 16) bits represent thread's ID number.
 * the high order THR_GEN_SIZE (set to 16) bits represent ID generation 
 * number.
 *
 * External thread ID is converted internally into thread descriptor 
 * as follows:
 * 
 * The low order THR_ID_SIZE bits is used as index into array of thread IDs
 * (_thr_tidvec). The the high order THR_GEN_SIZE bits is used to compare 
 * ID generation numbers.
 *
 * This scheme assures thread IDs that are unique with
 * upto 64K threads present at any time in a single process and
 * supports upto 64K generation numbers for a single ID.
 * Internal management of thread IDs is implemented by two functions
 * _thr_alloc_tid and _thr_free_tid located in thread.c.
 */

struct tidvec {
	thread_desc_t *thr_ptr;	/* ptr to a thread descriptor */
				/*  or a next free tid */
	int id_gen;		/* ID's generation number */
};

typedef struct tidvec tidvec_t;
extern tidvec_t *_thr_tidvec;		/* list of thread IDs	*/
extern tidvec_t *_thr_freetid;		/* last tid on the free list */
extern tidvec_t *_thr_last_tid_index;	/* last tid in the pool of tids */
extern int _thr_total_tids;		/* current size of tidvec in tids */

#ifndef THR_GEN_SIZE
#define THR_GEN_SIZE 16		/* Size of generation number in bits */
#endif
#ifndef THR_ID_SIZE
#define THR_ID_SIZE 16		/* Size of thread's ID in bits	*/
#endif
#define THR_TID_MASK ~(~0 << THR_ID_SIZE)
#define THR_GEN_MASK ~(~0 << THR_GEN_SIZE)

/*
 * Macro THREAD converts thread ID (thread_t) to a pointer to thread
 * descriptor (thread_desc_t)
 */
 
#define THREAD(x) \
	(((((x) & THR_TID_MASK) <= 0 || ((x) & THR_TID_MASK) > _thr_total_tids) \
	|| ((((x) >> THR_ID_SIZE) & THR_GEN_MASK) != \
	_thr_tidvec[(x) & THR_TID_MASK].id_gen)\
	|| (_thr_tidvec[(x) & THR_TID_MASK].thr_ptr >= \
	(thread_desc_t *) _thr_tidvec &&\
	_thr_tidvec[(x) & THR_TID_MASK].thr_ptr <= \
	(thread_desc_t *) _thr_last_tid_index)) ? \
	NULL : _thr_tidvec[(x) & THR_TID_MASK].thr_ptr)

/*
 * Macros and Declarations for managing _thr_userpriarray[]
 */


        /*
         * REM_FROM_PRIO_ARRAY decrements the counter of the indicated
         * priority in the priority array.  If the counter being
         * incremented is being set to 0, it decrements _thr_userpricnt
         * and resets _thr_userminpri if the value being decremented
         * was the current _thr_userminpri.
	 * If _thr_userpricnt is decremented to 1 and _thr_getminpri is
	 * not THREAD_MAX_PRIORITY, the flag _thr_preempt_ok is set to
	 * false, indicating that preemption need not be attempted
	 * except for the housekeeping thread.
	 *
	 * _thr_counterlock should be held on entry to this macro.
	 *
	 * NOTE: this assumes that the priority value has already been
	 * hashed via the macro HASH_PRIORITY.
         */

#define REM_FROM_PRIO_ARRAY(oldval)                                \
	ASSERT ((oldval < (DISPQ_SIZE)) && (oldval >= 0));	   \
	ASSERT (_thr_userpriarray[(oldval)] > 0);                  \
	ASSERT (_thr_userpricnt > 1);                              \
	_thr_userpriarray[(oldval)]--;                             \
	if (_thr_userpriarray[(oldval)] == 0) {                    \
		_thr_userpricnt--;                                 \
		if (_thr_userminpri == (oldval))                   \
			_thr_userminpri = _thr_getminpri((oldval));\
		if ((_thr_userpricnt == 1) &&                      \
		   (_thr_userminpri < THREAD_MAX_PRIORITY))        \
			_thr_preempt_ok = B_FALSE;                 \
	}

        /*
         * ADD_TO_PRIO_ARRAY increments the counter of the indicated
         * priority in the priority array.  If the counter being
         * incremented is being set to 1, it increments _thr_userpricnt
         * and resets _thr_userminpri if the new value represents a
         * lowering of the minimum priority in the process.
	 *
	 * _thr_counterlock should be held on entry to this macro.
	 *
	 * NOTE: this assumes that the priority value has already been
	 * hashed via the macro HASH_PRIORITY.
         */

#define ADD_TO_PRIO_ARRAY(newval)                         \
	ASSERT(newval < DISPQ_SIZE && newval >= 0);       \
	_thr_userpriarray[(newval)]++;                    \
	if (_thr_userpriarray[(newval)] == 1) {           \
		_thr_userpricnt++;                        \
		if ((newval) < _thr_userminpri)           \
			_thr_userminpri = (newval);       \
        }

extern thread_desc_t *_thr_allthreads; /* doubly linked list of all threads */
extern thread_desc_t *_thr_housethread; /* pointer to housekeeping thread   */
extern thread_desc_t *_thr_calloutserver; /* pointer to callout server thread */
extern int _thr_totalthreads;		/* total number of threads created */
extern int _thr_nidle;			/* number of idle lwps in this pool. */
extern int _thr_minlwps;		/* min number of idle lwps */
extern int _thr_nlwps;			/* number of lwps in this pool. */
extern int _thr_nrunnable;		/* number of threads on the run queue */
extern int _thr_nthreads;		/* number of unbound threads */
extern int _thr_nage;			/* number of aging lwps */
extern int _thr_dontkill_lwp;		/* number of aging lwps to age again */
extern int _thr_maxpriq;		/* highest pri runnable mux thread*/
extern int _thr_reapable;               /* number of threads on _thr_reaplist */
extern int _thr_joinable;               /* number of unjoined joinable threads*/
extern int _thr_lwpswanted;    /* tells how many LWPs housekeeper should make */
extern int _thr_userpricnt;     /* number of user priorities in process */
extern int _thr_userminpri;     /* lowest priority of any user thread in proc */
extern int _thr_userpriarray[]; /* array mapping thread count to priorities */
extern boolean_t _thr_preempt_off;      /* if user turned off preemption */
extern boolean_t _thr_preempt_ok;       /* if preemption conditions satisfied */
extern thrq_elt_t _thr_runnable[];/* the queue of runnable threads defined in disp.c */

extern lwp_cond_t _thr_aging;	/* condition that unused lwps block on */

extern thread_desc_t *_thr_reaplist;	/* list of unreaped threads */
extern thread_desc_t *_thr_joinablelist;/* list of unjoined threads */
extern thrq_elt_t    _thr_lazyq;        /* list of lazy-switched threads */
extern thread_desc_t *_thr_idlethreads;	/* list of idle threads */
extern boolean_t _thr_sigwaiting_ok;    /* if ok to enable SIGWAITING hndlr */


extern int _thr_setrq(thread_desc_t *, int);
extern thread_desc_t *_thr_disp(thread_desc_t *);
extern void _thr_swtch(boolean_t, thread_desc_t *);
extern void _thr_fix_runq(void);
extern void *_thr_age(void);
extern void _thr_enq_thread_to(thread_desc_t **, thread_desc_t *);
extern int _thr_deq_thread_from(thread_desc_t **, thread_desc_t *);
extern thread_desc_t *_thr_deq_any_from(thread_desc_t **);
extern void _thr_deq_from_idlethreads(thread_desc_t *);
extern int _thr_remove_from_runq(thread_desc_t *);

extern int _thr_get_qlength(thrq_elt_t *);

extern int _thr_new_lwp(thread_desc_t *, void *(*func)(), void *);

extern int _thr_alloc_stack(size_t, caddr_t *);
extern void _thr_free_stack(caddr_t , int);
extern int _thr_alloc_chunk(caddr_t, int, caddr_t *);
extern void _thr_free_chunk(caddr_t, int);

extern void _thrq_elt_ins_after(thrq_elt_t *, thrq_elt_t *);
extern void _thrq_elt_add_end(thrq_elt_t *, thrq_elt_t *);
extern thrq_elt_t *_thrq_elt_rem_first(thrq_elt_t *);
extern void _thrq_elt_rem_from_q(thrq_elt_t *);

extern boolean_t _thrq_is_on_q(thrq_elt_t *, thread_desc_t *);
extern void _thrq_prio_ins(thrq_elt_t *, thread_desc_t *);

extern boolean_t _thr_key_exit(thread_desc_t *thr);

#ifdef DEBUG
extern void _thr_showlazyq(void);
#endif /* DEBUG */

/*
 * queue macros used for thread descriptors
 */

/*
 *  _thrq_ins_after(thrq_elt_t *after, thread_desc_t *new)
 *	This macro adds a thread after the list element specified
 *	 by after on the list after is on.
 *
 *  Parameter/Calling State:
 *	On entry, the thread lock of new and the queue lock of the list
 *	after is on must be held.
 *
 *	after - Pointer to the current list element.
 *	new - Pointer to the thread to be put on the queue.
 *
 *  Return Values/Exit State:
 *	Returns no value. 
 */

#define _thrq_ins_after(after, new) _thrq_elt_ins_after((after), (thrq_elt_t *)(new))

/*
 *  _thr_callout_ins_after(callo_t *after, callo_t *new)
 *	This macro adds a callout to the callout queue after the list element
 *	specified by after.
 *
 *  Parameter/Calling State:
 *	On entry, the callout queue lock must be held.
 *
 *	after - Pointer to the current list element.
 *	new - Pointer to the thread to be put on the callout queue.
 *
 *  Return Values/Exit State:
 *	Returns no value. 
 */

#define _thr_callout_ins_after(after, new) _thrq_elt_ins_after((thrq_elt_t *) (after), (thrq_elt_t *) (new))

/*
 *  _thrq_add_end(thrq_elt_t *old, thread_desc_t *new)
 *	This macro adds a thread after the list element specified
 *	 by old on the list old is on.
 *
 *  Parameter/Calling State:
 *      On entry, the thread lock of new and the queue lock of the list
 *      old is on must be held.
 *
 *	after - Pointer to the current list element.
 *	new - Pointer to the thread to be put on the queue.
 *
 *  Return Values/Exit State:
 *	Returns no value. 
 */

#define _thrq_add_end(old, new) _thrq_elt_add_end((old), (thrq_elt_t *) (new))

/*
 *  _thr_callout_add_end(callo_t *old, callo_t *new)
 *	This macro adds a callout to the end of the callout queue specified
 *	 by old. 
 *
 *  Parameter/Calling State:
 *      On entry, the callout queue lock must be held.
 *
 *	old - Pointer to the current list element.
 *	new - Pointer to the callout to be put on the callout queue.
 *
 *  Return Values/Exit State:
 *	Returns no value. 
 */

#define _thr_callout_add_end(old, new) _thrq_elt_add_end((thrq_elt_t *) (old), (thrq_elt_t *) (new))

/*
 *  _thrq_rem_first(thrq_elt_t *qp)
 *	This macro removes the first thread from the list element specified
 *	by qp.
 *
 *  Parameter/Calling State:
 *	On entry, the queue lock of qp must be held.
 *
 *	qp - Pointer to the current list element.
 *
 *  Return Values/Exit State:
 *	Returns a pointer to the first thread on the list, if the list is 
 *	is empty NULL is returned.
 */

#define _thrq_rem_first(qp) (thread_desc_t *) _thrq_elt_rem_first((qp))

/*
 * callo_t *
 *  _thr_callout_rem_first(callo_t *qp)
 *	This macro removes the first callout from the callout queue qp.
 *
 *  Parameter/Calling State:
 *	On entry, the callout queue lock must be held.
 *
 *	qp - Pointer to the header of the callout queue.
 *
 *  Return Values/Exit State:
 *	Returns a pointer to the first callout on the list, if the list is 
 *	is empty NULL is returned.
 */

#define _thr_callout_rem_first(qp) (callo_t *) _thrq_elt_rem_first((thrq_elt_t *)(qp))

/*
 *  _thrq_rem_from_q(thread_desc_t *target)
 *      This macros removes a thread from the list it is currently on.
 *
 *  Parameter/Calling State:
 *      On entry, the thread lock of target must be held and the
 *      queue lock of the queue that target is on must be held.
 *
 *      target - Pointer to the thread to be removed from the queue.
 *
 *  Return Values/Exit State:
 *      Returns no value.
 */

#define _thrq_rem_from_q(target) _thrq_elt_rem_from_q((thrq_elt_t *)(target))

/*
 *  _thr_callout_rem_from_q(callo_t *target)
 *      This macros removes a callout from the callout list.
 *
 *  Parameter/Calling State:
 *      On entry, the queue lock of the callout queue must be held.
 *
 *      target - Pointer to the callout to be removed from the queue.
 *
 *  Return Values/Exit State:
 *      Returns no value.
 */

#define _thr_callout_rem_from_q(target) _thrq_elt_rem_from_q((thrq_elt_t *)(target))

extern void __thr_sigwaitinghndlr(void);
extern int _thr_hibit(ulong);
extern void _thr_sigt0init(thread_desc_t *);
extern void _thr_timer_threxit(thread_desc_t *);

extern int _thr_setrun(thread_desc_t *);

extern void _thr_panic(char *);
extern int _thr_assfail(char *, char *, int);

extern thread_desc_t *_thr_idle_thread_create(void);
extern void _thr_reaper(void);
extern boolean_t _thr_put_on_reaplist(thread_desc_t *);
extern void _thr_wakehousekeeper(void);
extern void * _thr_housekeeper(void *);
extern void _thr_activate_lwp(int);
extern void _thr_prioq_init(int, int);
extern int  _thr_getminpri(int);

extern int _thr_alloc_tid(thread_desc_t *);
extern void _thr_free_tid(thread_t);
extern thread_desc_t *_thr_get_thread(thread_t);

extern void _thr_start(void *(*)(void *), void *);
extern void _thr_makecontext(thread_desc_t *, void *(*func)(void *), void *);
extern void _thr_make_idle_context(thread_desc_t *);
extern void _thr_lwp_terminate(thread_desc_t *);

extern __lwp_desc_t *_thr_lwp_allocprivate(void);
extern void _thr_lwp_freeprivate(__lwp_desc_t *);
extern int _thr_alloc_tls(caddr_t, size_t, thread_desc_t **);
extern void _thr_t0init(void);

extern int _thr_lwpcreate(thread_desc_t *, long, caddr_t, void *(*func)(void *), void *, int);
extern void _thr_init(void);
extern void _init(void);
extern void _thr_resume(thread_desc_t *, thread_desc_t *, boolean_t, 
	boolean_t, boolean_t);
extern int _thr_mutex_lock_sigoff(mutex_t *, thread_desc_t *);
extern int _thr_lwp_mutex_lock_sigoff(lwp_mutex_t *, thread_desc_t *);

/*
 * debug (currently) routine that indicate whether it is ok to
 * disable preemption.
 */
extern void _thr_preempt_disable(void);

/*
 * debug (currently) routines that indicate whether it is ok to
 * disable or enable the SIGWAITING handler.
 */
extern void _thr_sigwaiting_disable(void);
extern void _thr_sigwaiting_enable(void);

/*
 * debug routines provided to dump fields within a thread or a list
 * of current threads within the library.
 */
extern void _thr_dumpreaplist(void);
extern void _thr_dumpjoinlist(void);
extern void _thr_dump_thread(thread_desc_t *);
extern void _thr_dump_allthreads(void);

/*
 * signal related externs
 */
extern int (*_sys_sigprocmask) (int, const sigset_t *, sigset_t *);
extern int (*_sys__sigaction) (int, const struct sigaction *, struct sigaction *, 
		void (*)(int, siginfo_t *, ucontext_t *, void (*)()));
extern int (*_sys_sigsuspend) (const sigset_t *);
extern int (*_sys_sigpause) (int);
extern int (*_sys_sigpending) (sigset_t *);
extern int (*_sys_sigwait) (sigset_t *);
extern int (*_sys_setcontext) (ucontext_t *);
extern unsigned (*_sys_alarm) (unsigned);
extern unsigned (*_sys_sleep) (unsigned);
extern int (*_sys_setitimer) (int, struct itimerval *, struct itimerval *);
extern int (*_sys_getitimer) (int, struct itimerval *);
extern pid_t (*_sys_forkall) (void);
extern pid_t (*_sys_fork) (void);

extern int (*_sys_close) (int);
extern void _thr_sigacthandler(int, siginfo_t *, ucontext_t *, void (*)());
extern sigset_t _thr_sig_programerror;	/* signals caused by program error */
extern sigset_t _thr_sig_allmask;              /* mask of all signals  */

extern void _thr_sigx(thread_desc_t *, boolean_t);
extern int _thr_sigsend(thread_desc_t *, int);

/*
 * internal condition variable routines
 */
extern int _thr_remove_from_cond_queue(thread_desc_t *);
extern int _thr_remove_from_mutex_queue(thread_desc_t *);
extern int _thr_condwait_lwpmutex(cond_t *, lwp_mutex_t *);
extern int _thr_cond_wait(cond_t *, mutex_t *, lwp_mutex_t *,
		      const timestruc_t *, boolean_t, boolean_t);
extern int _thr_remove_from_sema_queue(thread_desc_t *);
extern int _thr_remove_from_libc_queue(thread_desc_t *);
extern int rwlock_init(rwlock_t *, int, void *);
extern int rw_rdlock(rwlock_t *);
extern int rw_wrlock(rwlock_t *);
extern int rw_unlock(rwlock_t *);
extern int rw_tryrdlock(rwlock_t *);
extern int rw_trywrlock(rwlock_t *);
extern int rwlock_destroy(rwlock_t *);

/*
 * Internal routines to reposition a thread on a sleep queue.
 */

extern int  _thr_requeue(thread_desc_t *, int);
extern void _thr_requeue_mutex(thread_desc_t *, int);
extern void _thr_requeue_cond(thread_desc_t *, int);
extern void _thr_requeue_sema(thread_desc_t *, int);
extern int  _thr_requeue_runq(thread_desc_t *, int);
extern void _thr_requeue_libc(thread_desc_t *, int);

extern void _thr_debug_notify(void *, enum thread_change);

extern int _thr_setcallout(callo_t *cop, const timestruc_t *abstime,
			   const timestruc_t *interval, void (*func)(void *arg),
			   void *arg);
extern int _thr_rmcallout(callo_t *cop);
extern int _thr_lwpcreate(thread_desc_t *t, long retpc, caddr_t sp,
			void *(*func)(void *), void *arg, int flags);
extern void _lwp_make_context(ucontext_t *ucp, void (*func)(void *arg),
			      void *arg, void *private, thread_desc_t *tp);
extern void _thr_libcsync_init(void);
extern int hrestime(timestruc_t *timestrucp);

/*
 * Internal untraced synchronization routines
 */
#ifdef TRACE
int             _thr_notrace_mutex_init(mutex_t *, int, void *);
int             _thr_notrace_mutex_lock(mutex_t *);
int             _thr_notrace_mutex_unlock(mutex_t *);
int             _thr_notrace_mutex_trylock(mutex_t *);
int             _thr_notrace_mutex_destroy(mutex_t *);
int             _thr_notrace_cond_init(cond_t *, int, void *);
int             _thr_notrace_cond_signal(cond_t *);
int             _thr_notrace_cond_broadcast(cond_t *);
int             _thr_notrace_cond_wait(cond_t *,mutex_t *);
int             _thr_notrace_cond_timedwait(cond_t *,mutex_t *,timestruc_t *);
int             _thr_notrace_cond_destroy(cond_t *);
int             _thr_notrace_sema_init(sema_t *, int, int, void *);
int             _thr_notrace_sema_wait(sema_t *);
int             _thr_notrace_sema_trywait(sema_t *);
int             _thr_notrace_sema_post(sema_t *);
int             _thr_notrace_sema_destroy(sema_t *);
int             _thr_notrace_rwlock_init(rwlock_t *, int, void *);
int             _thr_notrace_rw_rdlock(rwlock_t *);
int             _thr_notrace_rw_wrlock(rwlock_t *);
int             _thr_notrace_rw_unlock(rwlock_t *);
int             _thr_notrace_rw_tryrdlock(rwlock_t *);
int             _thr_notrace_rw_trywrlock(rwlock_t *);
int             _thr_notrace_rwlock_destroy(rwlock_t *);
int             _thr_notrace__spin_init(spin_t *, void *);
void            _thr_notrace__spin_unlock(spin_t *);
void            _thr_notrace__spin_lock(spin_t *);
int             _thr_notrace__spin_trylock(spin_t *);
int             _thr_notrace__spin_destroy(spin_t *);
#endif /* TRACE */


#ifdef DEBUG
/*
 * lock statistics for internal data gathering.
 */

typedef struct thr_lckstat thr_lckstat_t;

struct thr_lckstat {
	int	lckstat_locks;
	int	lckstat_unlocks;
	int	lckstat_waits;
};

extern void _thr_lock_tidvec(void);
extern void _thr_unlock_tidvec(void);
extern void _thr_unlock_counter(void);
extern void _thr_unlock_counter(void);
extern void _thr_lock_runq(void);
extern void _thr_unlock_runq(void);
extern void _thr_lock_thread(thread_desc_t *);
extern void _thr_unlock_thread(thread_desc_t *);
extern void _thr_unlock_preemptlock(void);
extern void _thr_unlock_preemptlock(void);

/*
 * lock statistics initialize and print routines. 
 */
extern void _thr_lckstats_init(void);
extern void _thr_lckstats_print(void);

#endif /* DEBUG */

#ifdef TRACE
/*
 * macros to ensure that internal library calls to library interfaces
 * are not logged in trace files
 */

/* MUTEX INTERFACES */
#define _THR_MUTEX_INIT(mutex, type, arg) _thr_notrace_mutex_init(mutex, type, arg)
#define _THR_MUTEX_LOCK(mutex)            _thr_notrace_mutex_lock(mutex)
#define _THR_MUTEX_UNLOCK(mutex)          _thr_notrace_mutex_unlock(mutex)
#define _THR_MUTEX_TRYLOCK(mutex)         _thr_notrace_mutex_trylock(mutex)
#define _THR_MUTEX_DESTROY(mutex)         _thr_notrace_mutex_destroy(mutex)

/* CONDITION VARIABLE INTERFACES */
#define _THR_COND_INIT(cond, type, arg)   _thr_notrace_cond_init(cond, type, arg)
#define _THR_COND_DESTROY(cond)           _thr_notrace_cond_destroy(cond)
#define _THR_COND_TIMEDWAIT(cond, mutex, abstime) \
                               _thr_notrace_cond_timedwait(cond, mutex, abstime)
#define _THR_COND_WAIT(cond, mutex)      _thr_notrace_cond_wait(cond, mutex)
#define _THR_COND_SIGNAL(cond)           _thr_notrace_cond_signal(cond)
#define _THR_COND_BROADCAST(cond)        _thr_notrace_cond_broadcast(cond)

/* SEMAPHORE INTERFACES */
#define _THR_SEMA_INIT(sema, count, type, arg) \
                               _thr_notrace_sema_init(sema, count, type, arg)
#define _THR_SEMA_DESTROY(sema)          _thr_notrace_sema_destroy(sema)
#define _THR_SEMA_WAIT(sema)             _thr_notrace_sema_wait(sema)
#define _THR_SEMA_TRYWAIT(sema)          _thr_notrace_sema_trywait(sema)
#define _THR_SEMA_POST(sema)             _thr_notrace_sema_post(sema)

/* READER-WRITER LOCK INTERFACES */
#define _THR_RWLOCK_INIT(rwlock, type, arg) _thr_notrace_rwlock_init(rwlock, type, arg)
#define _THR_RW_RDLOCK(rwlock)              _thr_notrace_rw_rdlock(rwlock)
#define _THR_RW_WRLOCK(rwlock)              _thr_notrace_rw_wrlock(rwlock)
#define _THR_RW_UNLOCK(rwlock)              _thr_notrace_rw_unlock(rwlock)
#define _THR_RW_TRYRDLOCK(rwlock)           _thr_notrace_rw_tryrdlock(rwlock)
#define _THR_RW_TRYWRLOCK(rwlock)           _thr_notrace_rw_trywrlock(rwlock)
#define _THR_RWLOCK_DESTROY(rwlock)         _thr_notrace_rwlock_destroy(rwlock)

/* SPIN LOCK INTERFACES */
#define _THR_SPIN_INIT(sp_lock, arg)    _thr_notrace_spin_init(sp_lock, arg)
#define _THR_SPIN_DESTROY(sp_lock)      _thr_notrace_spin_destroy(sp_lock)
#define _THR_SPIN_UNLOCK(sp_lock)       _thr_notrace_spin_unlock(sp_lock)
#define _THR_SPIN_LOCK(sp_lock)         _thr_notrace_spin_lock(sp_lock)
#define _THR_SPIN_TRYLOCK(sp_lock)      _thr_notrace_spin_trylock(sp_lock)





#else  /* TRACE is not defined */
/* MUTEX INTERFACES */
#define _THR_MUTEX_INIT(mutex, type, arg) mutex_init(mutex, type, arg)
#define _THR_MUTEX_LOCK(mutex)            mutex_lock(mutex)
#define _THR_MUTEX_UNLOCK(mutex)          mutex_unlock(mutex)
#define _THR_MUTEX_TRYLOCK(mutex)         mutex_trylock(mutex)
#define _THR_MUTEX_DESTROY(mutex)         mutex_destroy(mutex)

/* CONDITION VARIABLE INTERFACES */
#define _THR_COND_INIT(cond, type, arg)   cond_init(cond, type, arg)
#define _THR_COND_DESTROY(cond)           cond_destroy(cond)
#define _THR_COND_TIMEDWAIT(cond, mutex, abstime) \
                                          cond_timedwait(cond, mutex, abstime)
#define _THR_COND_WAIT(cond, mutex)       cond_wait(cond, mutex)
#define _THR_COND_SIGNAL(cond)            cond_signal(cond)
#define _THR_COND_BROADCAST(cond)         cond_broadcast(cond)

/* SEMAPHORE INTERFACES */
#define _THR_SEMA_INIT(sema, count, type, arg) \
                                         sema_init(sema, count, type, arg)
#define _THR_SEMA_DESTROY(sema)          sema_destroy(sema)
#define _THR_SEMA_WAIT(sema)             sema_wait(sema)
#define _THR_SEMA_TRYWAIT(sema)          sema_trywait(sema)
#define _THR_SEMA_POST(sema)             sema_post(sema)

/* READER-WRITER LOCK INTERFACES */
#define _THR_RWLOCK_INIT(rwlock, type, arg) rwlock_init(rwlock, type, arg)
#define _THR_RW_RDLOCK(rwlock)              rw_rdlock(rwlock)
#define _THR_RW_WRLOCK(rwlock)              rw_wrlock(rwlock)
#define _THR_RW_UNLOCK(rwlock)              rw_unlock(rwlock)
#define _THR_RW_TRYRDLOCK(rwlock)           rw_tryrdlock(rwlock)
#define _THR_RW_TRYWRLOCK(rwlock)           rw_trywrlock(rwlock)
#define _THR_RWLOCK_DESTROY(rwlock)         rwlock_destroy(rwlock)

/* SPIN LOCK INTERFACES */
#define _THR_SPIN_INIT(sp_lock, arg)    _spin_init(sp_lock, arg)
#define _THR_SPIN_DESTROY(sp_lock)      _spin_destroy(sp_lock)
#define _THR_SPIN_UNLOCK(sp_lock)       _spin_unlock(sp_lock)
#define _THR_SPIN_LOCK(sp_lock)         _spin_lock(sp_lock)
#define _THR_SPIN_TRYLOCK(sp_lock)      _spin_trylock(sp_lock)


#endif /* TRACE */
#endif /* _LIBTHREAD_H */
