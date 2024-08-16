/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_DISP_H	/* wrapper symbol for kernel use */
#define _PROC_DISP_H	/* subject to change without notice */

#ident	"@(#)kern:proc/disp.h	1.30"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>
#include <util/list.h>
#include <util/ksynch.h>
#include <proc/class.h>
#include <util/plocal.h>
#include <proc/user.h>
#include <proc/lwp.h>
#include <proc/disp_p.h> /* PORTABILITY */
#include <util/param.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>
#include <sys/list.h>
#include <sys/ksynch.h>
#include <sys/class.h>
#include <sys/plocal.h>
#include <sys/user.h>
#include <sys/lwp.h>
#include <sys/disp_p.h> /* PORTABILITY */
#include <sys/param.h>

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * This is the basic structure onto which an lwp is placed when it becomes
 * runnable.
 */
typedef struct runque {
	int rq_maxrunpri;		/* max run priority of this queue */
	int rq_srunlwps;		/* number of runnable lwp's on this run-queue */
	uint_t *rq_dqactmap;		/* dispatcher summary information */
	list_t *rq_dispq;		/* points to the dispatcher queues proper */
	int rq_dispcnt;			/* fairness */
#ifdef UNIPROC
	struct engine *rq_nudgelist; /* engine scheduling from this
					run queue */
#else
	struct engine **rq_lastnudge;	/* last engine nudged, used to allow
					   preemptions to be spread equally
					   among engines */
	struct engine *rq_nudgelist[MAXNUMCPU+1]; /* engines scheduling from this
						 run queue (NULL terminated
						 array) */
#endif /* UNIPROC */
} runque_t;

#endif	/* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

/*
 * Operations for locking the run-queue.
 */
extern fspin_t runque_mutex;

#define	RUNQUE_LOCK()		FSPIN_LOCK(&runque_mutex)
#define	RUNQUE_UNLOCK()		FSPIN_UNLOCK(&runque_mutex)
#define	RUNQUE_INIT()		FSPIN_INIT(&runque_mutex)
#define	RUNQUE_OWNED()		FSPIN_OWNED(&runque_mutex)

/*
 * Return the lwp currently running on this engine, or NULL if there isn't
 * any.
 */
#define	CURRENT_LWP()	u.u_lwpp

/*
 * Clear the preemption flag due to a deferred preemption caused by a
 * "don't preempt" flag.  Must be called with all interrupts disabled.
 */
#define	dispclrpreempt()	\
			(l.eventflags &= ~(EVT_KPRUNRUN|EVT_RUNRUN), \
			l.prmpt_state.s_prmpt_state.s_noprmpt = 1, \
			l.eng->e_npri = -1)


/*
 * Set the runrun flag.  Must be called with all interrupts disabled.
 */
#define SETRUNRUN()	(l.eventflags |= EVT_RUNRUN)

/*
 * Set the kprunrun and the runrun flag.  Must be called with all
 * interrupts disabled.
 */
#define SETKPRUNRUN()	(l.prmpt_state.s_prmpt_state.s_noprmpt = 0,\
				l.eventflags |= EVT_KPRUNRUN|EVT_RUNRUN)


/*
 * Interfaces to machine specific context switch support routines.
 */
struct lwp;
struct engine;

extern void resume(struct lwp *, struct lwp *);
extern void use_private(struct lwp *, void (*funcp)(struct lwp *), struct lwp *);
extern void idle(void);
extern int save(struct lwp *);
extern void nudge(int, struct engine *);
extern void kpnudge(int, struct engine *);
extern void preemption(struct lwp *, void (*)(), int prmpt, engine_t *engp);
extern void ipreemption(struct lwp *, void (*)());
extern void block_preemption(void);
extern void unblock_preemption(void);
extern void dispinit(void);
extern void offline_self(void);
extern void swtch(struct lwp *);
extern void qswtch(struct lwp *);
extern void setfrontrq(struct lwp *);
extern void setbackrq(struct lwp *);
extern boolean_t dispdeq(struct lwp *);
extern int getcid(char *, id_t *);
extern void dispmdepinit(int);
extern void dispmdepnewpri(int);
extern void dispp0init(struct lwp *);
extern void dispnolwp(void);
extern lwpstat_t dispnewpri(struct lwp *, int);
extern void dispexclusive(struct engine *);
extern void dispunexclusive(struct engine *);
extern int dispexbindok(struct engine *);
extern int dispofflineok(struct engine *);
struct pcparms;
extern int parmsstart(struct pcparms *, void **);
extern void parmsend(struct pcparms *, void *);
extern int parmsset(void *, struct pcparms *, struct lwp *, struct lwp *);
extern void parmsget(struct lwp *, struct pcparms *);
extern int parmsin(struct pcparms *);
extern int parmsout(struct pcparms *);
extern void parmsret(void);
extern void parmsprop(struct lwp *);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_DISP_H */
