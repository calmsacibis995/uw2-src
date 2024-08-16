/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_LWP_H	/* wrapper symbol for kernel use */
#define _PROC_LWP_H	/* subject to change without notice */

#ident	"@(#)kern:proc/lwp.h	1.90"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <mem/ublock.h>		/* REQUIRED */
#include <proc/lwp_f.h>		/* PORTABILITY */
#include <proc/signal.h>	/* REQUIRED */
#include <svc/time.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/list.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <vm/ublock.h>		/* REQUIRED */
#include <sys/lwp_f.h>		/* PORTABILITY */
#include <sys/signal.h>		/* REQUIRED */
#include <sys/time.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/list.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#else

#include <sys/time.h>	/* XXX - shouldn't be here */

#endif /* _KERNEL_HEADERS */

#if defined _KERNEL || defined _KMEMUSER

#ifdef _VNODE_HIST

/*
 * Special vnode history logging support for debugging.
 */
typedef struct lwp_vn_rec {
	char		*lvr_service;	/* service name */
	uint_t		lvr_count;	/* saved v_count */
	uint_t		lvr_softcnt;	/* saved v_softcnt */
	int		lvr_line;	/* line number */
	char		*lvr_file;	/* file name */
	struct vnode	*lvr_vp;	/* vnode itself */
	ulong_t		lvr_stamp;	/* time stamp */
} lwp_vn_rec_t;

#define	VN_LWP_HIST_SIZE	128

typedef struct lwp_vn_hist {
	int		lvi_cursor;	/* next avail rec */
	lwp_vn_rec_t	lvi_rec[VN_LWP_HIST_SIZE];
} lwp_vn_hist_t;

#endif /* _VNODE_HIST */

/*
 * LWP structure:
 */
typedef struct lwp {
	list_t l_qlink;		/* Forward and backward links.  The sleep */
				/* and run queues are doubly linked lists. */
				/* This structure must appear as the first */
				/* element of the LWP structure */
#define l_flink	l_qlink.flink	/* forward link */
#define l_rlink	l_qlink.rlink	/* backward link */
	lock_t	l_mutex;	/* spin lock for LWP state */
	lwpstat_t l_stat;	/* status of LWP */
	stype_t	l_stype;	/* sync object type that LWP is blocked upon */
	u_char	l_slptime;	/* time (secs) since block if SSLEEP or SSTOP */
	void	*l_syncp;	/* sync object for LWP when SSLEEP */
	u_int	l_trapevf;	/* LWP trap event flags */
	u_int	l_flag;		/* LWP flags */
	k_lwpid_t l_lwpid;	/* LWP-ID */
	struct cred *l_cred;	/* LWP's view of process credentials */
	struct vnode *l_rdir;	/* LWP's view of process root directory */
	struct lwp *l_prev;	/* previous LWP in proc (circular list) */
	struct lwp *l_next;	/* next LWP in proc (null terminated list) */
	struct proc *l_procp;	/* pointer to containing process */
	toid_t	l_artid;	/* timers: identifier of alarm callout */
	time_t	l_clktim;	/* timers: absolute seconds since epoch */
				/*	   at which the SIGALRM is to fire */
	toid_t	l_rtid;		/* timers: id of pending global callout entry */
				/*         for getitimer(2) and setitimer(2) */
	struct itimerval *l_realtimer;
				/* timers: current value of real-time timer */
	clock_t	l_utime;	/* LWP user time */
	clock_t	l_stime;	/* LWP system time */
	clock_t l_start;	/* start time for profiling */
	int	l_pri;		/* sched: priority */
	int	l_origpri;	/* sched: original global priority */
	id_t	l_cid;		/* sched: class ID */
	void	*l_cllwpp;	/* per LWP class specific data */
	struct classfuncs *l_clfuncs;
				/* sched: ptr to structure containing */
				/*	  class specific fn pointers */
	struct qpcparms *l_qparmsp;/* sched: queued scheduling class params */
	struct runque *l_rq;	/* sched: run queue to be added to */
	struct engine *l_kbind;	/* sched: kernel mode binding */
	struct engine *l_xbind;	/* sched: user-exclusive binding */
	struct engine *l_ubind;	/* sched: user-nonexclusive binding */
	struct engine *l_eng;	/* sched: engine currently running upon */
	clock_t l_lastran;	/* sched: LWP last ran */
	u_char	l_ublocktype;	/* usynch: class (reader/writer) of blocker */
	struct lwp *l_sfwd;	/* usynch: sync-queue forward link */
	struct lwp *l_sbak;	/* usynch: sync-queue backward link */
	struct sq *l_sq;	/* usynch: sync queue the LWP belongs on */
	struct sq *l_boost;	/* usynch: linked-list of sync-queues */
				/*	   applied against this LWP */
	char	*l_waiterp;	/* pointer to waiter flag in user space */
	event_t	l_pollevent;	/* poll(2) event object */
	char	l_pollflag;	/* flags for use during poll(2) */
	u_char	l_whystop;	/* PR_{REQUESTED, SIGNALLED, SYSENTRY, */
				/*     SYSEXIT, JOBCONTROL, FAULTED, */
				/*     SUSPENDED} */
	u_short	l_whatstop;	/* signal#, syscall#, fault type, or 0 */
	struct vnode *l_trace;	/* /proc: pointer to traced vnode */
	u_char	l_curflt;	/* /proc: current fault */
	u_char	l_sigwait;	/* signals: LWP is in sigwait(2) system call */
	u_char	l_cursig;	/* signals: current signal */
	sigstate_t l_cursigst;	/* signals: state information for current sig */
	k_sigset_t l_sigs;	/* signals: unmasked signals pending to LWP */
	k_sigset_t l_lwpsigs;	/* signals: signals directed to this LWP */
	k_sigset_t l_procsigs;	/* signals: unmasked signals accepted by this */
				/*	    LWP that were sent to the process */
	k_sigset_t l_sigheld;	/* signals: held signal bit mask */
	k_sigset_t l_sighasinfo;/* signals: siginfo exists for these signals */
	struct sigqueue *l_siginfo;/* signals: info list for pending signals */
	struct alwp *l_auditp;	/* auditing: LWP audit structure */
	struct strevent *l_sep;	/* linked list of SIGPOLL registrations */
	event_t	l_slpevent;	/* to implement the sleep() system call */
	struct ucontext *l_ucp;	/* pointer to the specified context for
				 * _lwp_create().
				 */
	struct user	*l_up;	/* pointer to the u area of this context */
	lwp_ubinfo_t l_ubinfo;	/* extended ublock support */
	uint_t		l_notrt; /* if non-zero the context is not reentrant */
	boolean_t	l_beingpt; /* B_TRUE: the context is being preempted */
	const char	*l_name; /* "ps" name for LWP, if non-NULL */
#ifdef _MPSTATS
	ulong_t		l_dispt; /* time when the context was made runnable */
#endif /* _MPSTATS */
	dl_t		l_waket; /* time when the context was last awakened */
#ifdef _VNODE_HIST
	lwp_vn_hist_t	l_vn_log; /* vnode log */
#endif	/* _VNODE_HIST */

	_LWP_F			/* family-specific LWP fields */
	struct cdevsw  *l_cdevswp;
	int		l_keepcnt;	/* don't swap unless desperate */
} lwp_t;

/* Poll (l_pollflag) flag definitions */
#define	SPOLLTIME	0x1	/* timer active */

/* Trap event (l_trapevf) flag definitions: */
#define EVF_PL_CRED	0x00000001 /* credentials changed */
#define EVF_PL_RDIR	0x00000002 /* root directory changed */
#define EVF_PL_RLIMIT	0x00000004 /* process resource limits changed */
#define EVF_PL_JOBSTOP	0x00000008 /* defaulted job control sig delivered */
#define EVF_PL_PRSTOP	0x00000010 /* /proc requested stop pending */
#define EVF_PL_SEIZE	0x00000020 /* VM seize request pending */
#define EVF_PL_RENDEZV	0x00000040 /* rendezvous request pending */
#define EVF_PL_DESTROY	0x00000080 /* LWP is to be destroyed */
#define EVF_PL_SIG	0x00000100 /* LWP has unmasked pending sigs / cur sig */
#define EVF_PL_SYSENTRY	0x00000200 /* system call tracing on entry is enabled */
#define EVF_PL_SYSEXIT	0x00000400 /* system call tracing on exit is enabled */
#define EVF_PL_SYSABORT	0x00000800 /* system call aborted by /proc */
#define EVF_PL_PROF	0x00001000 /* profiling is enabled */
#define EVF_PL_PROF_EVT	0x00002000 /* a profiling event has been posted */ 
#define EVF_L_VFORKW	0x00004000 /* vfork wait required */
#define EVF_L_SCHEDPARM	0x00008000 /* scheduling parameters change pending */
#define EVF_L_ASAGE	0x00010000 /* age the address space */
#define EVF_L_UBIND	0x00020000 /* user binding pending */
#define EVF_PL_SUSPEND	0x00040000 /* suspend this context */
#define EVF_PL_STEP	0x00080000 /* single step this context */
#define EVF_PL_AUDIT	0x00100000 /* auditing is enabled */
#define EVF_PL_AEXEMPT	0x00200000 /* LWP is exempt from auditing */
#define EVF_PL_AEMASK	0x00400000 /* LWP's audit event mask has changed */
#define EVF_PL_DISAUDIT	0x00800000 /* auditing is off for the LWP */
#define EVF_F_1		0x01000000 /* reserved for family use (lwp_f.h) */
#define EVF_F_2		0x02000000 /* reserved for family use (lwp_f.h) */
#define EVF_F_3		0x04000000 /* reserved for family use (lwp_f.h) */
#define EVF_F_4		0x08000000 /* reserved for family use (lwp_f.h) */
#define EVF_PL_SWAPWAIT	0x10000000 /* lwp should join swapout barrier */

/* LWP (l_flag) flag definitions */
#define L_JOBSTOPPED	0x0000001 /* LWP stopped by a defaulted */
				  /* job control stop signal */
#define L_PRSTOPPED	0x0000002 /* LWP stopped for /proc */
#define L_NWAKE		0x0000004 /* LWP is not sleeping interruptibly */
#define L_SIGWOKE	0x0000008 /* LWP awakened from sleep by signal */
#define L_NOSTOP	0x0000010 /* LWP cannot be stopped */
#define L_DETACHED	0x0000040 /* LWP cannot be waited for */
#define L_SUSPENDED	0x0000080 /* LWP is suspended */
#define L_BOOSTED	0x0000100 /* LWP benefited from a priority boost */
#define L_CLOCK		0x0000200 /* LWP was caught by clock when profiling */
#define L_AGEDISP	0x0000400 /* LWP has aged, don't reset dispwait */
#define	L_SQTIMEDOUT	0x0000800 /* LWP timed out while waiting on
				   * a sync queue.
				   */
#define L_ONSQ		0x0001000 /* indicates whether LWP is on
				   * a sync queue.
				   */
#define	L_ONGLOBSQ	0x0002000 /* whether queued on global/local sync
				   * queue. 1 => global, 0 => local.
				   */
#define L_ONSEMAQ	0x0004000 /* queued waiting for a semaphore */
#define L_INITLWP	0x0008000 /* first LWP in the process */

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

/* Macro to get the credentials of the current context (LWP). */
#define CRED()	(u.u_lwpp->l_cred)

/*
 * Macro for an LWP to post an aging event against self. It is assumed
 * that this macro will always be called at base ipl.
 */
#define POST_AGING_EVENT_SELF() { \
	ASSERT(getpl() == PLBASE); \
	(void)LOCK(&u.u_lwpp->l_mutex, PLHI); \
	u.u_lwpp->l_trapevf |= EVF_L_ASAGE; \
	UNLOCK(&u.u_lwpp->l_mutex, PLBASE); \
}

/*
 * Macro for an LWP to unpost an aging event posted against itself.
 * It is assumed that this macro will always be called at base ipl.
 */
#define UNPOST_AGING_EVENT_SELF() { \
	ASSERT(getpl() == PLBASE); \
	(void)LOCK(&u.u_lwpp->l_mutex, PLHI); \
	u.u_lwpp->l_trapevf &= ~EVF_L_ASAGE; \
	UNLOCK(&u.u_lwpp->l_mutex, PLBASE); \
}

/*
 * Macro for an LWP to test whether an aging event has been posted.
 * The information returned is stale unless l_mutex is held.
 */
#define AGING_EVENT_POSTED()	(u.u_lwpp->l_trapevf & EVF_L_ASAGE)

/*
 * Attributes of an LWP, abstracted from the flags and bits used to
 * indicate these attributes.
 */
#define LWP_SEIZED(lwp)	((lwp)->l_trapevf & EVF_PL_SEIZE) /* LWP is frozen */
#define LWP_LOADED(lwp)	((lwp)->l_procp->p_flag & P_LOAD)/* LWP is ready to run
							    in a VM sense */
/*
 * Define various flag sets for inheritance on _lwp_create(2) and fork(2),
 * process/LWP attribute updates, queued signal flags, and flags for trap
 * entry and exit.
 */
#define UPDATE_FLAGS	(EVF_PL_CRED|EVF_PL_RDIR|EVF_PL_RLIMIT|EVF_L_SCHEDPARM)

#define INHERIT_FLAGS	(UPDATE_FLAGS|EVF_PL_JOBSTOP|EVF_PL_DESTROY| \
			 EVF_PL_PROF|EVF_PL_AUDIT)

#define QUEUEDSIG_FLAGS	(EVF_PL_JOBSTOP|EVF_PL_PRSTOP|EVF_PL_RENDEZV| \
			 EVF_PL_DESTROY|EVF_PL_SIG|EVF_PL_SUSPEND)

#define QUEUEDSIG(lwpp)	((lwpp)->l_trapevf & QUEUEDSIG_FLAGS)

#define ADT_FLAGS	(EVF_PL_AUDIT|EVF_PL_AEXEMPT|EVF_PL_AEMASK| \
			 EVF_PL_DISAUDIT)

#define ADTENTRY_FLAGS	(EVF_PL_DISAUDIT|EVF_PL_AEXEMPT|EVF_PL_AEMASK)

#define ADTEXIT_FLAGS	ADTENTRY_FLAGS

#define TRAPENTRY_FLAGS	(UPDATE_FLAGS|EVF_PL_SYSENTRY|ADT_FLAGS)

#define TRAPEXIT_FLAGS	(F_TRAPEXIT_FLAGS|UPDATE_FLAGS|QUEUEDSIG_FLAGS| \
			 EVF_PL_PROF_EVT|EVF_L_ASAGE|EVF_PL_STEP| \
			 ADTEXIT_FLAGS)


/* export lwplockinfo; it is defined in lwpsubr.c and is needed in p0init() */

extern lkinfo_t	lwplockinfo;

/* Some defines for use by the lwp_wait() function */

#define	DIR_ACTIVE	1
#define	DIR_ESEARCH	2
#define	DIR_EXITED	3
#define	DIR_INVAL	4

/*
 * Flags passed to the lwp_dup() function.
 */
typedef enum {DUP_VFORK, DUP_FORK1, DUP_FORKALL, DUP_LWPCR} dupflag_t;

extern  void    	lwp_copy(lwp_t *, lwp_t *, int, boolean_t);
extern  struct 	lwp 	*lwp_setup(lwp_t *, int, struct proc *);
extern	void		lwp_dup(lwp_t *, lwp_t *, dupflag_t,
				   void (*)(void *), void *);
extern 	k_lwpid_t	lwp_dirslot(struct proc *, lwp_t *);
extern	int		lwp_reapid(struct proc *, lwpid_t);
extern 	int		lwp_reapany(struct proc *, lwpid_t *);
extern  int		spawn_lwp(int, k_lwpid_t *, u_long, struct ucontext *, 
				  void (*)(void *), void *);
extern  int		spawn_sys_lwp(k_lwpid_t *, u_long, void (*)(void *),
					void *);
extern  int		wait_sys_lwp(k_lwpid_t);
extern 	void		lwp_cleanup(lwp_t *);
extern	void		lwp_exit(void);
extern	void		freelwp(struct proc *);
extern	void		lwp_attrupdate(void);
extern	void		complete_lwpcreate(void);
extern	void		lwp_swapout_wait(void);

#endif /* _KERNEL */

/*
 * Various defines to be used by user programs.
 */
#define LWP_DETACHED	(1 << 0)	/* LWP cannot be waited for */
#define LWP_SUSPENDED	(1 << 1)	/* Create the LWP suspended */

/*
 * lwpinfo structure.
 */
typedef struct lwpinfo {
        timestruc_t	lwp_utime;
        timestruc_t	lwp_stime;
        long		lwp_filler[10];
} lwpinfo_t;

#ifndef _KERNEL
#if defined(__STDC__)
/*
 * LWP related system call prototypes.
 */
#include <sys/ucontext.h>

extern	int	_lwp_create(struct ucontext *, unsigned long, lwpid_t *);
extern	void	_lwp_makecontext(struct ucontext *ucp, void (*)(void *),
			void *, void *, caddr_t, size_t);
extern	int	_lwp_suspend(lwpid_t);
extern	int	_lwp_continue(lwpid_t);
extern	lwpid_t	_lwp_self(void);
extern	void	_lwp_exit(void);
extern	int	_lwp_kill(lwpid_t, int);
extern	int	_lwp_wait(lwpid_t, lwpid_t *);
extern	int	_lwp_info(struct lwpinfo *);
extern	void	_lwp_setprivate(void *);
extern	void	*_lwp_getprivate(void);

#else	/* __STDC__ */

extern	lwpid_t	_lwp_create();
extern	void	_lwp_makecontext();
extern	int	_lwp_suspend();
extern	int	_lwp_continue();
extern	lwpid_t	_lwp_self();
extern	void	_lwp_exit();
extern	int	_lwp_kill();
extern	int	_lwp_wait();
extern	int	_lwp_info();
extern	void	_lwp_setprivate();
extern	void	*_lwp_getprivate();

#endif /* __STDC__ */

#endif /* !_KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_LWP_H */
