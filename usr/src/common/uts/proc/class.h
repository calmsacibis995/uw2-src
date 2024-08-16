/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CLASS_H	/* wrapper symbol for kernel use */
#define _PROC_CLASS_H	/* subject to change without notice */

#ident	"@(#)kern:proc/class.h	1.22"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS
/*
 * Needed for id_t and lwpstat_t.
 */
#include <util/types.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * NOTE: Developers making use of the scheduler class switch mechanism
 * to develop scheduling class modules should be aware that the
 * architecture is not frozen and the kernel interface for scheduling
 * class modules may change in future releases of System V.  Support
 * for the current interface is not guaranteed and class modules
 * developed to this interface may require changes in order to work
 * with future releases of the system.
 */

typedef struct class {
	char	*cl_name;	/* class name */
	void	(*cl_init)();	/* class specific initialization function */
	struct classfuncs *cl_funcs;	/* pointer to classfuncs structure */
} class_t;

extern struct class class[];	/* the class table */
extern int	nclass;		/* number of configured scheduling classes */
extern void (*sys_nudge)();     /* tuneable nudge for sysclass */

/*
 * Forward references for the parameters.
 */
struct pcparms;
struct qpcparms;
struct cred;
struct list;
struct lwp;
struct lwp_set;
struct sq;

typedef struct classfuncs {
	int		(*cl_admin)(void *, id_t, struct cred *);
	void		(*cl_enterclass)(long *, struct lwp *,
					 lwpstat_t *, int *, u_int *,
					 struct cred **, void **, void *);
	void		(*cl_exitclass)(struct lwp *);
	int		(*cl_fork)(void *, struct lwp *, lwpstat_t *,
				   int *, u_int *, struct cred **, void **);
	void		(*cl_unused4)();	/* used to be forkret */
	int		(*cl_getclinfo)();	/* unused */
	void		(*cl_unused3)();	/* used to be cl_getglobpri */
	void		(*cl_parmsget)(void *, long *);
	int		(*cl_parmsin)(long *);
	int		(*cl_parmsout)(long *);
	void		(*cl_parmsset)(long *, void *, void *);
	void		(*cl_preempt)(void *);
	int		(*cl_lwpcmp)(void *, long *);
	void		(*cl_setrun)(void *);
	void		(*cl_unused0)();	/* used to be cl_sleep */
	void		(*cl_stop)(void *);
	void		(*cl_unused1)();	/* used to be swapin */
	void 		(*cl_unused2)();	/* used to be swapout */
	void		(*cl_tick)(void *);
	void		(*cl_trapret)(void *);
	void		(*cl_wakeup)(void *, int);
	void		(*cl_insque)(struct lwp *, struct list *, int);
	int		(*cl_rdblock)(struct lwp *, struct list *);
	void		*(*cl_allocate)(void *);
	void		(*cl_deallocate)(void *);
	int		(*cl_changestart)(struct pcparms *, void **);
	int		(*cl_changeparms)(struct lwp *, int, id_t,
					  struct pcparms *, void *,
					  struct cred *, struct qpcparms **,
					  int *);
	int		(*cl_cancelchange)(struct lwp *, int, struct qpcparms *,
					   struct pcparms *, void *,
					   struct cred *, int *);
	int		(*cl_changeend)(struct pcparms *, void *);
	void		(*cl_newpri)(void *, int);
	int		(*cl_movetoqueue)(struct lwp_set *, struct lwp *);
	int		(*cl_prepblock)(struct lwp *, struct sq *, int);
	int		(*cl_block)(struct lwp *, struct sq *);
	int		(*cl_unblock)(struct lwp *, struct sq *, int *, int);
	void		(*cl_cancelblock)(struct lwp *, struct sq *, int *);
	int		(*cl_urdblock)(struct lwp *, struct sq *, int *);
	int		(*cl_yield)(void *, int);
	void		(*cl_audit)(int, int, int, void *, void *);
} classfuncs_t;


#endif  /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

#define	CL_ADMIN(clp, uaddr, reqpcid, reqpcredp) \
(*(clp)->cl_funcs->cl_admin)(uaddr, reqpcid, reqpcredp)

#define	CL_ENTERCLASS(clp, clparmsp, lwpp, lwp_statp, lwp_prip, lwp_flagp, \
		      lwp_credpp, cllwpp, classdata) \
(*(clp)->cl_funcs->cl_enterclass) \
  (clparmsp, lwpp, lwp_statp, lwp_prip, lwp_flagp, lwp_credpp, cllwpp, \
   classdata)

#define	CL_EXITCLASS(lwpp, cllwpp) (*(lwpp)->l_clfuncs->cl_exitclass)(cllwpp)

#define CL_FORK(lwpp, pcllwpp, clwpp, clwp_statp, clwp_prip, \
		clwp_flagp, clwp_credpp, cllwppp) \
(*(lwpp)->l_clfuncs->cl_fork)\
  (pcllwpp, clwpp, clwp_statp, clwp_prip, clwp_flagp, \
    clwp_credpp, cllwppp)

#define	CL_GETCLINFO(clp, clinfop, reqpcid, reqpcredp) \
    (*(clp)->cl_funcs->cl_getclinfo)(clinfop, reqpcid, reqpcredp)

#define	CL_PARMSGET(lwpp, cllwpp, clparmsp) \
    (*(lwpp)->l_clfuncs->cl_parmsget)(cllwpp, clparmsp)

#define CL_PARMSIN(clp, clparmsp) \
    (*(clp)->cl_funcs->cl_parmsin)(clparmsp)

#define CL_PARMSOUT(clp, clparmsp) \
    (*(clp)->cl_funcs->cl_parmsout)(clparmsp)

#define	CL_PARMSSET(lwpp, cllwpp, clparmsp, argp) \
    (*(lwpp)->l_clfuncs->cl_parmsset)(clparmsp, cllwpp, argp)

#define CL_PREEMPT(lwpp, cllwpp) (*(lwpp)->l_clfuncs->cl_preempt)(cllwpp)

#define CL_LWPCMP(lwpp, cllwpp1, lwpparms2) \
    (*(lwpp)->l_clfuncs->cl_lwpcmp)(cllwpp1, lwpparms2)

#define CL_SETRUN(lwpp, cllwpp) (*(lwpp)->l_clfuncs->cl_setrun)(cllwpp)

#define CL_STOP(lwpp, cllwpp) \
    (*(lwpp)->l_clfuncs->cl_stop)(cllwpp)

#define CL_TICK(lwpp, cllwpp) (*(lwpp)->l_clfuncs->cl_tick)(cllwpp)

#define CL_TRAPRET(lwpp, cllwpp) (*(lwpp)->l_clfuncs->cl_trapret)(cllwpp)

#define CL_WAKEUP(lwpp, cllwpp, preemptflg) \
    (*(lwpp)->l_clfuncs->cl_wakeup) (cllwpp, preemptflg)

#define	CL_INSQUE(lwpp, q, priority) \
    (*(lwpp)->l_clfuncs->cl_insque)(lwpp, q, priority)

#define	CL_RDBLOCK(lwpp, q) \
    (*(lwpp)->l_clfuncs->cl_rdblock)(lwpp, q)

#define	CL_ALLOCATE(clp, clwp) (*(clp)->cl_funcs->cl_allocate)(clwp)

#define	CL_DEALLOCATE(clp, cprivatep) \
	(*(clp)->cl_funcs->cl_deallocate)(cprivatep)

#define	CL_CHANGESTART(clp, pcparmsp, argpp) \
	(*(clp)->cl_funcs->cl_changestart)(pcparmsp, argpp)

#define	CL_CHANGEPARMS(clp, lwpp, cmd, cid, pcparmsp, classdata, \
		      credp, qpcparmspp, donep) \
	(*(clp)->cl_funcs->cl_changeparms)(lwpp, cmd, cid, pcparmsp, classdata, \
					   credp, qpcparmspp, donep)

#define	CL_PREPBLOCK(lwpp, sq, rdwr) \
	(*(lwpp)->l_clfuncs->cl_prepblock)(lwpp, sq, rdwr)

#define	CL_BLOCK(lwpp, sq) \
	(*(lwpp)->l_clfuncs->cl_block)(lwpp, sq)

#define	CL_URDBLOCK(lwpp, sq, retp) \
	(*(lwpp)->l_clfuncs->cl_urdblock)(lwpp, sq, retp)

#define	CL_UNBLOCK(lwpp, sq, count, flag) \
	(*(lwpp)->l_clfuncs->cl_unblock)(lwpp, sq, count, flag)

#define	CL_CANCELBLOCK(lwpp, sq, deqp) \
	(*(lwpp)->l_clfuncs->cl_cancelblock)(lwpp, sq, deqp)

#define	CL_YIELD(lwpp, cllwpp, flag) \
	(*(lwpp)->l_clfuncs->cl_yield)(cllwpp, flag)

#define CL_AUDIT(clp, flag, cmd, error, arg, classdata) \
    (*(clp)->cl_funcs->cl_audit) (flag, cmd, error, arg, classdata)



/*
 * These commands describe the circumstances were CL_CHANGEPARMS was called.
 */
#define	CHANGEPARMS_MAYAPPLY		1 /* class may apply change sequentially */
#define	CHANGEPARMS_CHECKONLY		2 /* class must check credentials only
					     i.e. new change is about to cancel
					     a queued change of a different class */
#define	CHANGEPARMS_FORK		3 /* lwp is child of lwp with queued change */

#define	CL_CANCELCHANGE(clp, lwpp, cmd, qpcparmsp, pcparmsp, classdata, credp, \
			combinedp) \
	(*(clp)->cl_funcs->cl_cancelchange)(lwpp, cmd, qpcparmsp, pcparmsp, \
					    classdata, credp, combinedp)

/*
 * These commands describe the circumstances were CL_CANCELCHANGE was called.
 */
#define	CANCELCHANGE_TRYCOMBINE		0 /* class should try to combine changes */
#define	CANCELCHANGE_MUSTCANCEL		1 /* class should not try to combine changes */
#define	CANCELCHANGE_EXIT		2 /* lwp called exit before applying queued change */

#define	CL_CHANGEEND(clp, parmsp, argp) \
	(*(clp)->cl_funcs->cl_changeend)(parmsp, argp)

#define	CL_NEWPRI(lwpp, cllwpp, pri) (*(lwpp)->l_clfuncs->cl_newpri)(cllwpp, pri)

#define	CL_MOVETOQUEUE(lwpsp, lwpp) \
	(*(lwpp)->l_clfuncs->cl_movetoqueue)(lwpsp, lwpp)
#endif /* _KERNEL */
#if defined(__cplusplus)
	}
#endif

#endif	/* _PROC_CLASS_H */
