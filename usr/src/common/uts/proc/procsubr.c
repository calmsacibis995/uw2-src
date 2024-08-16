/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/procsubr.c	1.126"
#ident	"$Header: $"

#include <acc/audit/audit.h>
#include <acc/audit/auditrec.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <mem/as.h>
#include <mem/faultcatch.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <mem/ublock.h>
#include <proc/acct.h>
#include <proc/class.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/exec.h>
#include <proc/lwp.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <proc/resource.h>
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/uidquota.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/syscall.h>
#include <svc/systm.h>
#include <svc/reg.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/dl.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>
#include <util/var.h>

STATIC void proc_cleanup(proc_t *, int, int);
extern int proc_setup_f(proc_t *, int);
extern void proc_cleanup_f(proc_t *);
extern void lwp0resume(lwp_t *, vaddr_t, void (*funcp)(void));
extern void pgsess_fork(proc_t *, proc_t *);
extern void sess_init(proc_t *);
extern void sig_init(proc_t *, lwp_t *);
extern void fdtinit(proc_t *);
extern void main(void *); 
extern lkinfo_t sq_update_info;
extern void shmfork(proc_t *, proc_t *);
extern void xsd_cleanup(proc_t *);
extern int xsdfork(proc_t *, proc_t *);
extern void setup_newcontext(dupflag_t, boolean_t, struct user *,
			     void (*)(void *), void *);
extern void copy_ublock(caddr_t, caddr_t);
extern void systrap_cleanup(rval_t *, unsigned int, int);

extern void adt_allocaproc(struct aproc *, proc_t *);

/*
 * The following lockinfo structures are for the various synch objects 
 * in the proc and lwp structures.
 */

/*
 *+ Governs most of the state in the proc structure.
 */
STATIC LKINFO_DECL(prplockinfo, "PP::p_mutex", 0);

/*
 *+ Governs the session pointer in the proc structure.
 */
STATIC LKINFO_DECL(prsslockinfo, "PP::p_sess_mutex", 0);

/*
 *+ Governs the number of seized lwps field in the proc structure.
 */
STATIC LKINFO_DECL(prszlockinfo, "PP::p_seize_mutex", 0);

/*
 *+ Governs access to the p_cdir (current directory), p_rdir
 *+ (root directory), and l_rdir (LWP root directory) fields in the
 *+ proc structure and lwp structures.
 */
STATIC LKINFO_DECL(prcdlockinfo, "PP::p_dir_mutex", 0);

/*
 *+ The process read/write lock.
 */
STATIC LKINFO_DECL(prrdwrlockinfo, "PP::p_rdwrlock", 0);

/*
 *+ Controls profiling.
 */
LKINFO_DECL(proflkinfo, "PP::pr_lock", 0);

/*
 * Rollback flags for proc_cleanup et al.
 */
#define RB_UBLOCK	0x01
#define RB_FAMILY	0x02
#define RB_ASLOCKED	0x04

/*
 *
 * pid_t proc_setup(int cond, proc_t **newpp, int *errorp) 
 * 	This function assigns all the necessary data structures for use in a 
 * 	fork request. 
 *
 * Calling/Exit State:
 *	No locks are held on entry and no locks held on return.
 *	Returns the process-ID of the proto-process, if successful.
 *	Otherwise, -1 is returned.
 *
 * Remarks:
 *	This function replaces the SVR4.0 pid_assign() function.
 *	It checks to see that there is an empty slot in the proc table, that
 *	the requesting user does not have to many processes already active,
 *	and that the last slot in the proc table is not being allocated to
 *	anyone who should not use it.
 *
 *	After a proc slot is allocated, it will try to allocate a proc
 *	structure for the new process and all the other necessary data
 *	structures.  The following data structures are allocated by this
 *	function:
 *
 *	1) proc structure.
 *	2) LWP structure(s).
 *	3) pid structure.
 *	4) all the file descriptor related data structures (including XENIX
 *	   semaphores and shared memory).
 *	5) sigqueue_t object if the parent had done a fork.
 *	6) class specific LWP structure.
 *	7) u block(s) for the LWP(s) in the child.
 *	8) The profiling structure if process profiling is enabled.
 *	9) The address space for the child.
 *
 *	If all goes well, proc_setup() will return a new pid and set up the
 *	proc structure pointer for the child process.  The process is also
 *	entered into the process group and session of the parent. Further,
 *	references to all shared attributes are acquired here. All the LWP(s)
 *	in the child will be fully initialized.  The child process structure
 *	is fully initialized.
 */
pid_t
proc_setup(int cond, 		/* Allow assignment of last slot & other info */
	proc_t **newpp,		/* Child process proc structure pointer */ 
	int *errorp)		/* to return additional error info */
{
	static uidres_t uidres;
	proc_t		*prp;
	proc_t		* const p = u.u_procp;	/* calling process */
	lwp_t		*lwp;			/* lwp in the calling proc */
	lwp_t		*clwp;			/* lwp in the child proc */
	uidquo_t	*uidquotap;		/* ptr to real user-ID quotas */
	cred_t		*credp;			/* credentials for uidquotap */
	struct pid 	*pidp;
	pl_t		pl;
	uint_t		*map;
	uint_t		holdcnt;
	int		rollback_flags;
	int		*bufp = NULL;		/* used by Audit subsystem */

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(!(cond & NP_SYSPROC) || (p->p_flag & P_SYS));

	/* 
	 * Set up for an additional process reference count of 1.
	 */
	uidres.ur_prcnt = 1;

	/*
	 * Do all the resource quota checks and credentials holds first.
	 *
	 * Compute the number of LWPs for the child in uidres.ur_lwpcnt
	 * and the number of holds on the credentials in holdcnt; the
	 * number of holds will be one for the process and one per LWP.
	 *
	 * Note: I. The child inherits p_cred rather than l_cred, since in
	 *	    the case of forkall/vfork with multiple LWPs, any changes
	 *	    made to p_cred by another LWP before it reached the
	 *	    barrier must be seen in the child process; otherwise
	 *	    that LWP (in the child) would lose the credentials change
	 *	    it had done (in the parent).  For all other cases, it
	 *	    wouldn't matter which we use, since it's just a snapshot,
	 * 	    except for the p_uidquotap atomicity requirement described
	 *	    below.
	 *
	 *      II. For the forkall case, we will charge the user with the 
	 *	    total number of LWPs in the parent which includes all the 
	 *	    active LWPs in the parent and all the LWPs in the parent
	 *	    that have exited but have not been waited for. We do this 
	 * 	    since the child after a forkall() is a true clone of the 
	 *	    parent - the IDs of the waitable LWPs in the parent that 
	 *	    have exited	 are also cloned.
	 */
	if ((cond & (NP_FORKALL|NP_VFORK)) || (SINGLE_THREADED())) {
		/*
		 * Forkall or vfork case, or else fork1 with a single LWP.
		 *
		 * NOTE: There is no need to hold p_mutex here since all
		 *	 other LWPs in the process are held at rendezvous
		 *	 for NP_FORKALL.  Otherwise, there is but a single
		 *	 LWP and hence no need to lock.
		 */

		if (cond & NP_FORKALL)
			uidres.ur_lwpcnt = p->p_ntotallwp; 
		else
			uidres.ur_lwpcnt = 1;

		uidquotap = p->p_uidquotap;
		if (!(uidquota_incr(uidquotap, &uidres, B_FALSE))) {
			*errorp = EAGAIN;
			return (-1);
		}
		crholdn(credp = p->p_cred, holdcnt = uidres.ur_lwpcnt + 1);
	} else {
		/*
		 * Fork1 with multiple LWPs.
		 *
		 * The hold on p_cred and the snapshot of p_uidquotap
		 * must be obtained atomically, to ensure that they have
		 * the same real-UID.  Also, holds must be obtained on
		 * both structures (uidquota_incr for the uidquota struct)
		 * before dropping p_mutex, to ensure that they are not
		 * deallocated.
		 */
		ASSERT(cond & NP_FORK);

		uidres.ur_lwpcnt = 1;

		pl = LOCK(&p->p_mutex, PLHI);
		uidquotap = p->p_uidquotap;
		if (!(uidquota_incr(uidquotap, &uidres, B_FALSE))) {
			UNLOCK(&p->p_mutex, pl);
			*errorp = EAGAIN;
			return (-1);
		}
		crholdn(credp = p->p_cred, holdcnt = 2);
		UNLOCK(&p->p_mutex, pl);
	}

	/*
	 * Normally the parent's UID quota structure is inherited by the
	 * child.  However, if the parent is a system process it will not
	 * have a uidquota struct, since system processes are not subject
	 * to user quotas.  If the child of a system process is not a system
	 * process, we need to get a new uidquota struct.
	 */
	if (uidquotap == NULL) {
		ASSERT(p->p_flag & P_SYS);
		if (!(cond & NP_SYSPROC)) {
			/*
			 * A system process is spawning a user process.
			 * We need a new uidquota structure for the child.
			 */
			uidquotap = uidquota_get(credp->cr_ruid);
			/* uidquota_get returns w/1 prcnt already */
			uidres.ur_prcnt = 0;

			/*
			 * The uidquota_incr() done above would have had
			 * no effect, since uidquotap was NULL.  Do it again
			 * here, now that we have a uidquota struct.
			 */
			if (!(uidquota_incr(uidquotap, &uidres, B_FALSE))) {
				crfreen(credp, holdcnt);
				*errorp = EAGAIN;
				return (-1);
			}
		}
	}

	/* check to see if we will be using up the last slot */

	pl = RW_WRLOCK(&proc_list_mutex, PL_PROCLIST);

	if (MET_PROC_CNT >= v.v_proc - 1) {
		if (MET_PROC_CNT == v.v_proc || pm_denied(credp, P_SYSOPS)) {
			if (MET_PROC_CNT == v.v_proc) {
				MET_PROC_FAIL();
			}
			RW_UNLOCK(&proc_list_mutex, pl);
			/*
			 * Note that since we have a reference on the uidquota
			 * object pointed to by uidquotap, we do not need to
			 * hold p_mutex lock while invoking the uidquota_decr()
			 * function.
			 */
			uidquota_decr(uidquotap, &uidres);
			crfreen(credp, holdcnt);
			*errorp = EAGAIN;
			return (-1);
		}
	}

	/*
	 * Call MET_PROC_INUSE() to increment MET_PROC_CNT and any
	 * other process inuse metrics while we still have the lock.
	 */
	MET_PROC_INUSE(1);
	RW_UNLOCK(&proc_list_mutex, pl);

	/* Allocate the data structures */

	prp = kmem_zalloc(sizeof(proc_t), KM_SLEEP);

	prp->p_cred = credp;
	prp->p_uidquotap = uidquotap;
        prp->p_parent = p;

	/* Init all the proc structure synch objects */
	LOCK_INIT(&prp->p_mutex, PR_PHIER, PR_PMINIPL, &prplockinfo, KM_SLEEP);
	LOCK_INIT(&prp->p_sess_mutex, PR_PSSHIER, PR_SSMINIPL, &prsslockinfo,
		 KM_SLEEP);
	LOCK_INIT(&prp->p_seize_mutex, PR_SZHIER, PR_SZMINIPL, &prszlockinfo,
		 KM_SLEEP);
	LOCK_INIT(&prp->p_dir_mutex, PR_CDHIER, PR_CDMINIPL, &prcdlockinfo,
		 KM_SLEEP);
	LOCK_INIT(&prp->p_squpdate, SQ_HIER, SQ_MINIPL, &sq_update_info,
		 KM_SLEEP);
	SV_INIT(&prp->p_pwsv);
	SV_INIT(&prp->p_vfork);
	SV_INIT(&prp->p_wait2wait);
	SV_INIT(&prp->p_waitsv);
	SV_INIT(&prp->p_lwpwaitsv);
	SV_INIT(&prp->p_wait2seize);
	EVENT_INIT(&prp->p_seized);
	SV_INIT(&prp->p_rendezvous);
	SV_INIT(&prp->p_rendezvoused);
	SV_INIT(&prp->p_stopsv);
	SV_INIT(&prp->p_destroy);
	RWSLEEP_INIT(&prp->p_rdwrlock, 0, &prrdwrlockinfo, KM_SLEEP);
	FSPIN_INIT(&prp->p_niblk_mutex);
	SV_INIT(&prp->p_suspsv);     

	if (ublock_proc_init(prp) != 0) {
		proc_cleanup(prp, cond, 0);
		*errorp = EAGAIN;
		return (-1);
	}

#ifdef DEBUG
	ASSERT(ublock_lock(prp, UB_NOSWAP) == 0);
#else
	(void) ublock_lock(prp, UB_NOSWAP);
#endif

	/*
	 * Do family-specific setup.
	 */

	if (proc_setup_f(prp, cond) == -1) {
		proc_cleanup(prp, cond, RB_UBLOCK);
		*errorp = EAGAIN;
		return (-1);
	}

	rollback_flags = RB_UBLOCK|RB_FAMILY;

	/*
	 * Allocate and initialize the LWP ID map and directory of the
	 * newly created proc structure.
	 *
	 * The LWP in the child corresponding to the creating LWP will
	 * have the the same ID as the creating LWP.  For the forkall()
	 * case we will simply clone all the directory information.
	 */

	if (cond & (NP_FORK | NP_VFORK)) {
		prp->p_nlwp = 1;
		prp->p_ntotallwp = 1;
		prp->p_nlwpdir = p->p_nlwpdir;
		/* 
		 * Allocate the same LWP ID to the 
		 * child LWP as the creating LWP.
		 */
		if (prp->p_nlwpdir > NBITPW) {	/* large ID map */
			uint_t nwords = BITMASK_NWORDS((uint_t)prp->p_nlwpdir);

			map = kmem_alloc(nwords * sizeof(uint_t), KM_SLEEP);
			BITMASKN_INIT(map, nwords,
				      (uint_t)u.u_lwpp->l_lwpid - 1);
			prp->p_large_lwpidmap = map;
		} else {				/* small ID map */
			BITMASK1_INIT(&prp->p_small_lwpidmap,
				      (uint_t)u.u_lwpp->l_lwpid - 1);
		}

	} else {			/* forkall case; clone ID map */
		size_t size;

		prp->p_nlwp = p->p_nlwp;
		prp->p_ntotallwp = p->p_ntotallwp;
		prp->p_nlwpdir = p->p_nlwpdir;
		if ((size = prp->p_nlwpdir) > NBITPW) {		/* large map */
			size = BITMASK_NWORDS((uint_t)size) * sizeof(uint_t);
			map = kmem_alloc(size, KM_SLEEP);
			/*
			 * Clone the ID map state. 
			 */
			bcopy(p->p_large_lwpidmap, map, size);
			prp->p_large_lwpidmap = map;
		} else {					/* small map */
			prp->p_small_lwpidmap = p->p_small_lwpidmap;
		}
	}

	if (prp->p_nlwpdir == 1)
		prp->p_lwpdir = &prp->p_lwpp;
	else {
		prp->p_lwpdir = (lwp_t **)
				kmem_zalloc(prp->p_nlwpdir * sizeof(lwp_t *),
					    KM_SLEEP);
	}
		
        prp->p_flag = P_LOAD;	/* Other flags will be set in pgsess_fork() */
        if (cond & NP_VFORK)
                prp->p_flag |= P_VFORK;

	/*
	 * If inherit-on-fork, copy /proc tracing flags and fields to child.
	 * NOTE: New system processes never inherit tracing flags.
	 */
	if (cond & NP_SYSPROC) {
		prp->p_flag |= P_SYS;
#ifdef DEBUG
		ASSERT(ublock_lock(prp, UB_NOSWAP) == 0);
#else
		(void) ublock_lock(prp, UB_NOSWAP);
#endif
	} else {
		if ((p->p_flag & (P_PROCTR|P_PRFORK)) == (P_PROCTR|P_PRFORK)) {
			ASSERT(!(cond & NP_INIT));
			prp->p_flag |= (P_PROCTR|P_PRFORK);
			if (p->p_entrymask != NULL) {
				prp->p_entrymask =
				    kmem_alloc(sizeof(k_sysset_t), KM_SLEEP);
				bcopy(p->p_entrymask, prp->p_entrymask,
				      sizeof(k_sysset_t));
			}
			if (p->p_exitmask != NULL) {
				prp->p_exitmask =
				    kmem_alloc(sizeof(k_sysset_t), KM_SLEEP);
				bcopy(p->p_exitmask, prp->p_exitmask,
				      sizeof(k_sysset_t));
			}
			prp->p_fltmask = p->p_fltmask;
			prp->p_sigtrmask = p->p_sigtrmask;
		}
#ifdef DEBUG
		ASSERT(ublock_lock(prp, UB_SWAPPABLE) == 0);
#else
		(void) ublock_lock(prp, UB_SWAPPABLE);
#endif
	}

	/*
	 * Allocate the address space for the new process.
	 */
	if (p->p_as != NULL) {
		if (!SINGLE_THREADED()) {
			as_rdlock(p->p_as);
			rollback_flags |= RB_ASLOCKED;
		}

		/*
		 * Call the shared memory and xenix fork entry points.
		 * The assumption here is that these entry points don't need
		 * the child's address space.
		 */
		if (p->p_nshmseg)
			shmfork(p, prp);

		if (p->p_sdp) {
			int error;
			if ((error = xsdfork(prp, p)) != 0) {
				proc_cleanup(prp, cond, rollback_flags);
				*errorp = error;
				return (-1);
			}
		}

		if (cond & NP_VFORK)
			prp->p_as = p->p_as;	/* borrow the parents 'as' */
		else {
			prp->p_as = as_dup(p->p_as);	 
			if (prp->p_as == NULL) {
				proc_cleanup(prp, cond, rollback_flags);
				*errorp = EAGAIN;
				return (-1);
			}
		}

		prp->p_brkbase = p->p_brkbase;
		prp->p_brksize = p->p_brksize;
		prp->p_stkbase = p->p_stkbase;
		prp->p_stksize = p->p_stksize;
	}

	if (cond & (NP_FORK | NP_VFORK)) {
		ADT_FORKINIT(u.u_lwpp->l_auditp, 1, bufp);
		ADT_FORK_LWPID(u.u_lwpp->l_auditp, bufp, u.u_lwpp->l_lwpid);
		if ((clwp = lwp_setup(u.u_lwpp, cond, prp)) == NULL) {
			proc_cleanup(prp, cond, rollback_flags);
			*errorp = EAGAIN;
			return (-1);
		}

		/* Link up the new LWP with the process */

		if (prp->p_lwpdir != &prp->p_lwpp)
			prp->p_lwpdir[u.u_lwpp->l_lwpid - 1] = clwp;
		prp->p_lwpp = clwp;
		clwp->l_next = NULL;
		clwp->l_prev = clwp;
	} else {
		/* The mother of all forks: forkall(). */
		lwp = p->p_lwpp;	 
		ADT_FORKINIT(u.u_lwpp->l_auditp, p->p_nlwp, bufp);
		while (lwp != NULL) {
			ADT_FORK_LWPID(u.u_lwpp->l_auditp, bufp, lwp->l_lwpid);
			if ((clwp = lwp_setup(lwp, cond, prp)) == NULL) {
				proc_cleanup(prp, cond, rollback_flags);
				*errorp = EAGAIN;
				return (-1);
			}

			if (prp->p_lwpdir != &prp->p_lwpp)
				prp->p_lwpdir[lwp->l_lwpid - 1] = clwp;

			/* Link up the new beast! */
			if (prp->p_lwpp == NULL) {
				prp->p_lwpp = clwp;
				clwp->l_next = NULL;
				clwp->l_prev = clwp;
			} else {
				prp->p_lwpp->l_prev->l_next = clwp;
				clwp->l_next = NULL;
				clwp->l_prev = prp->p_lwpp->l_prev;
				prp->p_lwpp->l_prev = clwp;
			}
			lwp = lwp->l_next;
		}
	}

	/*
	 * If this is the first fork() done by the parent process, allocate
	 * the sigqueue_t object to be used for posting SIGCHLD.
	 */
	if (p->p_sigcldinfo == NULL) 	{	/* This is the first fork */
		sigqueue_t *sqp;

		sqp = siginfo_get(KM_SLEEP, 1);
		if ((cond & NP_FORK) && p->p_nlwp > 1) {	/* could race */
			(void)LOCK(&p->p_mutex, PLHI);
			if (p->p_sigcldinfo == NULL) {		/* won race */
				p->p_sigcldinfo = sqp;
				UNLOCK(&p->p_mutex, PLBASE);
			} else {				/* lost race */
				UNLOCK(&p->p_mutex, PLBASE);
				kmem_free(sqp, sizeof(sigqueue_t));
			}
		} else {			/* FORKALL or VFORK: no race */
			p->p_sigcldinfo = sqp;
		}
	}

	/*
	 * If profiling is enabled, allocate the prof structure.
	 */
	if (u.u_lwpp->l_trapevf & EVF_PL_PROF) {	/* fast check */
		struct prof *profp;

		profp = kmem_alloc(sizeof(struct prof), KM_SLEEP);
		if ((cond & NP_FORK) && p->p_nlwp > 1) {	/* could race */
			(void)LOCK(&p->p_mutex, PLHI);
			if (u.u_lwpp->l_trapevf & EVF_PL_PROF) {  /* won race */
				profp->pr_base = p->p_profp->pr_base;
				profp->pr_size = p->p_profp->pr_size;
				profp->pr_off = p->p_profp->pr_off;
				profp->pr_scale = p->p_profp->pr_scale;
				UNLOCK(&p->p_mutex, PLBASE);
				prp->p_profp = profp;
				SLEEP_INIT(&profp->pr_lock, 0, &proflkinfo,
							  KM_SLEEP);
			} else {				/* lost race */
				UNLOCK(&p->p_mutex, PLBASE);
				kmem_free(profp, sizeof(struct prof));
			}
		} else {		
			/* forkall, vfork, or singly threaded fork: no race */
			profp->pr_base = p->p_profp->pr_base;
			profp->pr_size = p->p_profp->pr_size;
			profp->pr_off = p->p_profp->pr_off;
			profp->pr_scale = p->p_profp->pr_scale;
			prp->p_profp = profp;
			SLEEP_INIT(&profp->pr_lock, 0, &proflkinfo, KM_SLEEP);
		}
	}

	/* Allocate the pid structure */
	pidp = kmem_alloc(sizeof(struct pid), KM_SLEEP);

	/* Get references on the fd namespace */
	fdtfork(prp);

	/*
	 * Include the xenix hooks. need to recode the xenix hooks based
	 * on the new way of handling fd name-space.
	 */

	if (xsemfork(prp)) {
		/* 
		 * Roll back the state.  A sigcldinfo structure may have
		 * been allocated for the forking (parent) process, but if
		 * so, it is harmless to let it exist.  The sigcldinfo
		 * structure is allocated the first time a process forks,
		 * rather than when it is created, as an optimization.
		 * It is OK for the process to have this structure even
		 * if its fork attempt fails.
		 */
		if (prp->p_profp) {
			SLEEP_DEINIT(&prp->p_profp->pr_lock);
			kmem_free(prp->p_profp, sizeof(struct prof));
		}
		kmem_free(pidp, sizeof(struct pid));

		/* Rollback fdtfork() */
		closeall(prp);
		FDT_DEINIT(prp);

		proc_cleanup(prp, cond, rollback_flags);
		*errorp = EAGAIN;
		return (-1);
	}

	/*
	 * Establish necessary references and state for current directory,
	 * root directory, resource limits, and execinfo objects.
	 *
	 * NOTES:
	 *	We are careful to establish valid current and root directory
	 *	pointers _prior_ to adding the LWP into the process list.
	 *	Otherwise, a race with dofusers() would be possible.
	 *
	 *	If auditing is installed, allocate and initialize the
	 *	process audit structure for the child process.  A reference
	 *	to the current working directory is obtained from within
	 *	adt_allocaproc(), so no call to CDIR_HOLD() is required.
	 *
	 *	CDIR_HOLD() acquires a reference to the current directory
	 *	of the calling process, and initializes the child's p_cdir
	 *	field.
	 */ 
	if (p->p_auditp)
		adt_allocaproc(p->p_auditp, prp);
	else
		CDIR_HOLD(prp->p_cdir);
	if (u.u_lwpp->l_rdir != NULL) {
		prp->p_rdir = u.u_lwpp->l_rdir;
		VN_HOLDN(prp->p_rdir, prp->p_nlwp + 1);
	}
	prp->p_rlimits = u.u_rlimits;
	rlholdn(prp->p_rlimits, prp->p_nlwp + 1);

	/*
	 * Applying a hold to the execinfo object implicitly applies
	 * a hold to the a.out vnode associated with the execinfo object.
	 */
	prp->p_execinfo = p->p_execinfo;
	eihold(prp->p_execinfo);

	/*
	 * Initialize other fields of the child's proc structure.
	 * The following fields may require that p_mutex of the parent be
	 * locked to get a stable view.
	 */
	if ((cond & (NP_FORKALL)) || (SINGLE_THREADED())) {
		/* p_mutex need not be held in this case */

		sigfork(prp, cond);	/* Copy signal disposition */
		prp->p_nshmseg = p->p_nshmseg;
		prp->p_cmask = p->p_cmask;
		prp->p_sigwait = p->p_sigwait;
		prp->p_mem = 0L;
		(void)LOCK(&p->p_mutex, PLHI);
	} else {
		/* Need to hold parent's p_mutex to get a stable view. */
		ASSERT(cond & NP_FORK);
		(void)LOCK(&p->p_mutex, PLHI);
		sigfork(prp, cond);
		prp->p_nshmseg = p->p_nshmseg;
		prp->p_cmask = p->p_cmask;
		prp->p_sigwait = p->p_sigwait;
		prp->p_mem = 0L;
	}
	/*
	 * first wait for other LWPs to finish, then set P_CLDWAIT so we can
	 * drop p_mutex and later link it up to parent-child_sibling chain 
	 * without checking P_CLDWAIT again.
	 * (this is because we must do the following while/SV_WAIT-LOCK
	 * before pid_alloc(); otherwise it could deadlock on proc_list_mutex
	 * and/or s_mutex.  see comments below on pid_alloc().)
	 */
	while(p->p_flag & P_CLDWAIT) {
		SV_WAIT(&p->p_wait2wait, PRIWAIT, &p->p_mutex);
		(void)LOCK(&p->p_mutex, PLHI);
	}
	p->p_flag |= P_CLDWAIT;
	UNLOCK(&p->p_mutex, PLBASE);
	/*
	 * Perform other initialization of the child's state that can be
	 * done without holding any locks.
	 */
        prp->p_acflag = AFORK;
	GET_HRESTIME(&prp->p_start);

        prp->p_ppid = p->p_pidp->pid_id;;
        prp->p_nextofkin = p;
        prp->p_sysid = p->p_sysid;      /* RFS HOOK */

	prp->p_notblocked = 1;
	prp->p_active = lbolt;
	prp->p_incoretime = lbolt;
	SV_INIT(&prp->p_swdefer_sv);

	/*
	 * Initialize proto-process. pid_alloc() manipulates the global 
	 * variable procentfree. This function also initializes the proc 
	 * directory and the p_slot field of the created process.
	 * Note that pid_alloc() returns with the s_mutex lock of the
	 * calling context's session held and with the proc_list_mutex
	 * held in write mode.
	 * When pid_alloc() returns, the new process is globally visible
	 * via the normal lookup channels.  /proc must not touch us yet
	 * as we are SIDL, and the caller of this function still has
	 * more initialization to do.
	 */
	prp->p_pidp = pidp; 
	pid_alloc(pidp, prp);

	/* Initialize child process group and session of its parent */  
	pgsess_fork(prp, p);

	(void)LOCK(&p->p_mutex, PLHI);

	/*
	 * Link up to parent-child-sibling chain.
	 * We rely on p_prevsib and p_nextsib being initialized
	 * to NULL, by the initial allocation of the proc structure.
	 */
	if (p->p_child != NULL) {
		p->p_child->p_prevsib = prp;
		prp->p_nextsib = p->p_child;
	}
	p->p_child = prp;

	/*
	 * now turn off the P_CLDWAIT flag, and wake up any LWPs still waiting.
	 * we should also freeprocs() all zombies (resulted while P_CLDWAIT
	 * was set) but cannot do it here since freeprocs() releases p_mutex
	 * lock; do it later when we release the p_mutex lock.
	 */
	p->p_flag &= ~P_CLDWAIT;
	if (SV_BLKD(&p->p_wait2wait))
		SV_SIGNAL(&p->p_wait2wait, 0);
	/*
	 * Link up to nextofkin-orphan-nextorphan chain.
	 * Note we rely on p_prevorph and p_nextorph being initialized
	 * to NULL, by the initial allocation of the proc structure.
	 */
	if (p->p_orphan != NULL) {
		p->p_orphan->p_prevorph = prp;
		prp->p_nextorph = p->p_orphan;
	}
	p->p_orphan = prp;

	/*
	 * Inherit the l_trapevf and l_flag atomically
	 * w.r.t. making the new LWPs globally visible.
	 *
	 * Note: If /proc mucks with l_trapevf or l_flag and
	 *	 expects the changes to be carried over to a
	 *	 child process which is being created then the
	 *	 target process must either be stopped (will not
	 *	 be executing this code then), or /proc must be
	 *	 holding the p_mutex of the containing process.
	 */
	lwp = p->p_lwpp;	/* Calling process's LWP chain */
	clwp = prp->p_lwpp;	/* Child process's LWP chain */
	while (clwp != NULL && lwp != NULL) {
		ASSERT((cond & NP_FORKALL) ? lwp->l_lwpid == clwp->l_lwpid : 1);
		if (cond & NP_FORKALL) 
			clwp->l_trapevf = lwp->l_trapevf &
					(INHERIT_FLAGS|EVF_PL_SUSPEND);
		else
			clwp->l_trapevf = (lwp->l_trapevf & INHERIT_FLAGS);
		clwp->l_flag = (lwp->l_flag & ~L_INITLWP);
		if (prp->p_entrymask)
			clwp->l_trapevf |= EVF_PL_SYSENTRY;
		if (prp->p_exitmask)
			clwp->l_trapevf |= EVF_PL_SYSEXIT;
		clwp = clwp->l_next;
		lwp = lwp->l_next;
	}

	/*
	 * We initialized the trap event flags for the child LWPs
	 * immediately above.  The profiling data structure for the
	 * child was previously allocated (or not).  For forkall()
	 * and vfork() this is not a problem, since the state sampled
	 * above cannot change.
	 * In the case of fork1() with multiple LWPs in the parent it
	 * is possible for the trap event flags to have changed (i.e.
	 * EVF_PL_PROF is now on, but was not previously) since the
	 * sibling LWPs are not quiesced for fork1().
	 * The bottom line is that if we did not allocate a profiling
	 * structure to the child, the LWP in the child should not have
	 * the EVF_PL_PROF flag set.
	 */
	if (prp->p_profp == NULL)
		prp->p_lwpp->l_trapevf &= ~EVF_PL_PROF;

	/* inherit parent's audit flags */
	if (p->p_auditp)
		prp->p_auditp->a_flags = p->p_auditp->a_flags;

	if (p->p_zombies) {
		/*
		 * Drop the proc list mutex lock and the sess mutex lock
		 * before calling freeprocs(). We are still holding the
		 * p_mutex lock.
		 */
		RW_UNLOCK(&proc_list_mutex, PLHI);
		UNLOCK(&p->p_sessp->s_mutex, PLHI);
		freeprocs();
	} else {
		/* Drop all locks */
		UNLOCK(&p->p_mutex, PLHI);
		RW_UNLOCK(&proc_list_mutex, PL_SESS);
		UNLOCK(&p->p_sessp->s_mutex, PLBASE);
	}
	prp->p_epid = prp->p_pidp->pid_id;

	*newpp = prp;
	*errorp = 0;

	/*
	 * Now that the child is visible to the swapper, we can safely load
	 * up translations for it without worrying about memory deadlocks.
	 */
	if (p->p_as != NULL && !(cond & NP_VFORK)) {
		ASSERT(p->p_as != prp->p_as);
		as_childload(p->p_as, prp->p_as);	 
	}

	/* 
	 * Note that we only lock the parent's address space.
	 */
	if (rollback_flags & RB_ASLOCKED)
		as_unlock(p->p_as);	 

	return (prp->p_pidp->pid_id);
}


/*
 *
 * void proc_cleanup(proc_t *prp, int cond, int rollback_flags)
 * 	Clean up from a failed fork operation begun by proc_setup().
 * 	This function performs the basic clean-up operations, and gets
 * 	rid of all the data structures allocated and resources allocated
 * 	by proc_setup().
 *
 * Calling/Exit State:
 * 	No locks held on entry and no locks held on exit.
 *
 */

void 
proc_cleanup(proc_t *prp,	/* Pointer to the proc structure to cleanup */
	     int cond,		/* Conditions passed to proc_setup() */
	     int rollback_flags) /* Initializations to roll back */
{
	lwp_t		*walkp;
	lwp_t		*tmp;
	pl_t		s1;
	uidres_t	uidres;	
	
	ASSERT(KS_HOLD0LOCKS());

	/* 
	 * Note that we only lock the parent's address space.
	 */
	if (rollback_flags & RB_ASLOCKED) {
		as_unlock(prp->p_parent->p_as);	
	}

	if (prp->p_nlwpdir > NBITPW) {
		kmem_free(prp->p_large_lwpidmap,
			  BITMASK_NWORDS((uint_t)prp->p_nlwpdir) *
						sizeof(uint_t));
	}
	if (prp->p_lwpdir != &prp->p_lwpp) {
		kmem_free(prp->p_lwpdir,
			  (size_t)prp->p_nlwpdir * sizeof(lwp_t *));
	}
	if (prp->p_entrymask != NULL)
		kmem_free(prp->p_entrymask, sizeof(k_sysset_t));

	if (prp->p_exitmask != NULL)
		kmem_free(prp->p_exitmask, sizeof(k_sysset_t));

	walkp = prp->p_lwpp;
	while (walkp != NULL) {
		tmp = walkp;
		walkp = walkp->l_next;
		lwp_cleanup(tmp);
	}

	if (!(cond & NP_VFORK)) {
		if (prp->p_as != NULL)
			as_free(prp->p_as);
	}

	/* XXX: need to roll back from the XENIX hooks: MORE HERE */

	if (prp->p_sdp) {
		xsd_cleanup(prp);
	}

	/*
	 * Do family-specific cleanup.
	 */

	if (rollback_flags & RB_FAMILY)
		proc_cleanup_f(prp);

	if (rollback_flags & RB_UBLOCK)
		ublock_proc_deinit(prp);

	LOCK_DEINIT(&prp->p_mutex);
	LOCK_DEINIT(&prp->p_sess_mutex);
	LOCK_DEINIT(&prp->p_seize_mutex);
	LOCK_DEINIT(&prp->p_dir_mutex);
	LOCK_DEINIT(&prp->p_squpdate);

	RWSLEEP_DEINIT(&prp->p_rdwrlock);

	/*
	 * Reset the uid quotas, release holds on credentials.
	 */
	struct_zero(&uidres, sizeof(uidres_t));
	uidres.ur_prcnt = 1;
	if (cond & NP_FORKALL) {
		uidres.ur_lwpcnt = prp->p_ntotallwp;
		uidquota_decr(prp->p_uidquotap, &uidres);
		crfreen(prp->p_cred, prp->p_nlwp + 1);
	} else {
		uidres.ur_lwpcnt = 1;
		uidquota_decr(prp->p_uidquotap, &uidres);
		crfreen(prp->p_cred, 2);	/* 1 for process + 1 for LWP */
	}

	/* Free up the proc structure. */
	kmem_free(prp, sizeof(proc_t));

	s1 = RW_WRLOCK(&proc_list_mutex, PL_PROCLIST);
	/*
	 * call MET_PROC_INUSE() to decrement MET_PROC_CNT
	 * and update any other process release metrics. 
	 * This needs the proc_list_mutex lock.
	 */
	MET_PROC_INUSE(-1);
	RW_UNLOCK(&proc_list_mutex, s1);
}


/*
 *
 * void p0init()
 * 	Allocate the proc structure, lwp structure, and the uarea for proc0. 
 * 	We then handcraft process0.  This function should be called after 
 * 	selfinit() during the sysinit process. This function also 
 *	initializes the various PM subsystems.
 *
 * Calling/Exit State:
 *	No locks held on entry and no locks held on exit.
 *	This function DOES NOT return to the caller.
 *
 * Remarks:
 *	This function is called on the per-engine stack. After handcrafting
 *	the first process, we switch to the stack of the first LWP in 
 *	process 0. Since U areas may not be mapped at the same virtual 
 *	address, we cannot return from this function. Instead, we execute 
 *	main() which completes the system initialization.
 *
 */
void
p0init(void)
{
	proc_t		*pp;
	lwp_t		*lwpp;
	vaddr_t		ubaddr;
	user_t		*newup;
	extern struct rlimit sysdef_rlimits[];		/* proc.cf/Space.c */
	extern void lwp_setup_f(lwp_t *);

	/*
	 * Initialize the PM system.
	 */
	cred_init();
	uidquota_init();
	acctinit();

	/* 
	 * Allocate all the data structures for proc 0.
	 */
	pp = kmem_zalloc(sizeof(proc_t), KM_NOSLEEP);
	lwpp = kmem_zalloc(sizeof(lwp_t), KM_NOSLEEP);

	if ((pp == NULL) || (lwpp == NULL)) {
		/*
		 *+ Process 0 creation failed. The user cannot take any
		 *+ corrective action.
		 */
		cmn_err(CE_PANIC, "Process 0 - creation failed\n");
	}

	/* Begin ublock initialization */

	ublock_init();

	if (ublock_proc_init(pp) != 0) {
		/*
		 *+ Ublock initialization for process 0 failed.
		 */
		cmn_err(CE_PANIC, "p0init: ublock initialization failed");
	}

#ifdef DEBUG
	ASSERT(ublock_lock(pp, UB_NOSWAP) == 0);
#else
	(void) ublock_lock(pp, UB_NOSWAP);
#endif

	/* Initialize process data */
	LOCK_INIT(&pp->p_mutex, PR_PHIER, PLHI, &prplockinfo, KM_NOSLEEP);
	LOCK_INIT(&pp->p_sess_mutex, PR_PSSHIER, PR_SSMINIPL, &prsslockinfo,
		  KM_NOSLEEP);
	LOCK_INIT(&pp->p_seize_mutex, PR_SZHIER, PR_SZMINIPL, &prszlockinfo,
		  KM_NOSLEEP);
	LOCK_INIT(&pp->p_dir_mutex, PR_CDHIER, PR_CDMINIPL, &prcdlockinfo,
		  KM_NOSLEEP);
	LOCK_INIT(&pp->p_squpdate, SQ_HIER, SQ_MINIPL, &sq_update_info,
		  KM_NOSLEEP);

	SV_INIT(&pp->p_pwsv);
	SV_INIT(&pp->p_vfork);
	SV_INIT(&pp->p_wait2wait);
	SV_INIT(&pp->p_waitsv);
	SV_INIT(&pp->p_lwpwaitsv);
	SV_INIT(&pp->p_wait2seize);
	EVENT_INIT(&pp->p_seized);
	SV_INIT(&pp->p_rendezvous);
	SV_INIT(&pp->p_rendezvoused);
	SV_INIT(&pp->p_stopsv);
	SV_INIT(&pp->p_destroy);
	RWSLEEP_INIT(&pp->p_rdwrlock, 0, &prrdwrlockinfo, KM_NOSLEEP);
	FSPIN_INIT(&pp->p_niblk_mutex);
	SV_INIT(&pp->p_suspsv);

	pp->p_flag = P_SYS | P_LOAD | P_PGORPH; 
	pp->p_cttydev = NODEV;
	pp->p_sid = 0;

	/*
	 * p_cdir is initialized to 'rootdir' in vfs_mountroot()
	 * when the root directory is known.
	 */
	pp->p_cdir = NULL;
	pp->p_rdir = lwpp->l_rdir = NULL;

	pp->p_lwpp = lwpp;
	pp->p_nlwp = 1;
	pp->p_ntotallwp = 1;
        pp->p_niblked = 0;
	pp->p_sigwait = B_FALSE;

	pp->p_nlwpdir = 1;
	pp->p_lwpdir = &pp->p_lwpp;
	BITMASK1_INIT(&pp->p_small_lwpidmap, 0);	/* ID 1 == bit 0 */

	pp->p_profp = NULL;

	/* Allocate and initialize the pid0 structure */
	pid_init(pp);

	/* Allocate and initialize the sess0 structure */
	sess_init(pp);

	/* Allocate and initialize the audit structure for pid0 */
	if (adt_installed())
		pp->p_auditp = adt_p0aproc();

	/*
	 * Create and initialize the credentials for process 0.
	 * Note that we hold 2 references (one for the process,
	 * and one for its LWP).
	 */
	sys_cred = crget();
	lwpp->l_cred = sys_cred;
	crhold(sys_cred);
	pp->p_cred = sys_cred;

	/*
	 * Create system resource limits object for process 0.
	 * Note that we hold 2 references (one for the process,
	 * and one for its LWP).
	 */
	pp->p_rlimits = sys_rlimits = rlget();
	u.u_rlimits = sys_rlimits;
	rlhold(sys_rlimits);
	bcopy(sysdef_rlimits, &sys_rlimits->rl_limits[0],
	      sizeof(sys_rlimits->rl_limits));

	/*
	 * Initialize child and resource orphan state.
	 */
	pp->p_child = NULL;
	pp->p_prevsib = NULL;
	pp->p_nextsib = NULL;
	pp->p_zombies = NULL;
	pp->p_nextzomb = NULL;
	pp->p_orphan = NULL;
	pp->p_prevorph = NULL;
	pp->p_nextorph = NULL;

	/*
	 * No tracing by /proc.
	 */
	pp->p_entrymask = NULL;
	pp->p_exitmask = NULL;

	/*
	 * Initialize the LWP structure.
	 * Note that ublock_lwp_alloc will take care of l_ubinfo.
	 */
	LOCK_INIT(&lwpp->l_mutex, LWP_HIER, PLHI, &lwplockinfo, KM_NOSLEEP);
	EVENT_INIT(&lwpp->l_pollevent);
	EVENT_INIT(&lwpp->l_slpevent);
	lwpp->l_stat = SONPROC;
	lwpp->l_trapevf = 0;
	lwpp->l_flag = 0;
	lwpp->l_lwpid = 1;	/* Note that we don't use the ID 0 */
	lwpp->l_prev = lwpp;
	lwpp->l_next = NULL;
	lwpp->l_procp = pp;
	lwpp->l_sep = NULL;
	lwpp->l_pri = v.v_maxsyspri;

	/* Allocate the ublock for proc0 */

	if ((ubaddr = ublock_lwp_alloc(lwpp)) == 0) {
		/*
		 *+ Ublock allocation for process 0 lwp 1 failed.
		 *+ The user cannot take any corrective action.
		 */
		cmn_err(CE_PANIC, "p0init: ublock allocation failed");
	}
        newup = lwpp->l_up = UBLOCK_TO_UAREA(ubaddr);

	lwp_setup_f(lwpp);

	/*
	 * Do the scheduling related initializations.
	 */
	dispp0init(lwpp);

	lwpp->l_utime = 0;
	lwpp->l_stime = 0;

	/*
	 * Process 0 will become the system daemon process.
	 * This LWP is not named, since it will exit at the end of main().
	 */
	proc_sys = pp;
	pp->p_execinfo = eiget();
	strncpy(pp->p_execinfo->ei_comm, "sysproc", PSCOMSIZ);
	strncpy(pp->p_execinfo->ei_psargs, "sysproc", PSARGSZ);

	/*
	 * Initialize the signals subsystem and the process 0 signal state.
	 */
	sig_init(pp, lwpp);


	/*
	 * Initialize file descriptor table for process 0.
	 */
	fdtinit(pp);

	/*
	 * Initialize start time for process 0.  This early in the
	 * life of the system it should be safe to refer to these
	 * things without locks. 
	 */
	pp->p_start = hrestime;

	/*
	 * Step on to our private stack. We use the normal context
	 * switch mechanism to effect the stack switch. First, clone the 
	 * u area state. 
	 */
	copy_ublock((caddr_t)&u, (caddr_t)newup);

	newup->u_rlimits = pp->p_rlimits;
	newup->u_procp = pp;
	newup->u_lwpp = lwpp;

	/* Initialize pagefault error handling mechanism. */
	newup->u_fault_catch.fc_func = fc_jmpjmp;

	/*
	 * Initialize the context in the new u area.
	 */
	setup_newcontext(DUP_FORK1, B_TRUE, newup, main, NULL);

	/*
	 * Acquire l_mutex to make the context switch code happy.
	 */
	(void)LOCK(&lwpp->l_mutex, PLHI);
	resume(lwpp, NULL);
	/* NOTREACHED */	
}

/*
 * void complete_fork(void)
 *	Do the final processing for a user level process creation.
 *
 * Calling/Exit State:
 *	No locks can be held on entry. 
 */
void
complete_fork(void)
{
	lwp_t	*lwpp = u.u_lwpp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (lwpp->l_flag & L_INITLWP) {
		/* 
		 * This is the LWP corresponding to the
		 * LWP in the parent that initiated the
		 * process cloning operation. Setup the
		 * return values (as seen by the user level
		 * code) for this LWP.
		 */
		rval_t rval;
		uint_t scall = u.u_syscall; /* system call number */
		ASSERT(scall == SYS_fork || scall == SYS_vfork ||
		       scall == SYS_fork1 || scall == SYS_forkall);

		rval.r_val1 = u.u_procp->p_ppid;
		rval.r_val2 = 1;
		systrap_cleanup(&rval, scall, 0);
	}

	/*
	 * If we have inherited the L_SUSPENDED flag, we
	 * should suspend ourselves. Set the EVF_PL_SUSPEND flag
	 * and let issig()/psig() sort it all out.
	 */
	if (lwpp->l_flag & L_SUSPENDED) {
		(void)LOCK(&lwpp->l_procp->p_mutex, PLHI);
		(void)LOCK(&lwpp->l_mutex, PLHI);
		lwpp->l_trapevf |= EVF_PL_SUSPEND;
		UNLOCK(&lwpp->l_mutex, PLHI);
		UNLOCK(&lwpp->l_procp->p_mutex, PLBASE);
	}
}

/*
 * void syscontext_returned(void)
 *	This function is executed if a system context returns from the 
 *	initial function specified during context creation.
 *
 * Calling/Exit State:
 *	None.
 */
void
syscontext_returned(void)
{
	/*
	 *+ System context returned - kernel software problem.
	 */
	cmn_err(CE_PANIC, "System context returned");
	/* NOTREACHED */
}


/*
 * void proc_kmadv(void)
 *	Call kmem_advise() for process management data structures.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single threaded.
 */
void
proc_kmadv(void)
{
	kmem_advise(sizeof(proc_t));
	kmem_advise(sizeof(struct pid));
	kmem_advise(sizeof(lwp_t));
	kmem_advise(sizeof(execinfo_t));
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_proc(const proc_t *p)
 * 	Formatted dump of the proc structure
 *
 * Calling/Exit State:
 * 	None. I'll try to dump whatever you pass to me.
 */
void
print_proc(const proc_t *p)
{
	int i;

	debug_printf("proc structure dump at 0x%x\n\n", p);

	debug_printf("p_mutex=%x p_sess_mutex=%x p_seize_mutex=%x\n",
		     p->p_mutex, p->p_sess_mutex, p->p_seize_mutex);

	debug_printf("p_dir_muxtex=%x p_squpdate=%x p_niblk_mutex=%x\n",
		     p->p_dir_mutex, p->p_squpdate, p->p_niblk_mutex);

	debug_printf("p_flag=%x p_sysid=%x p_epid=%x p_slot=%x\n",
		     p->p_flag, p->p_sysid, p->p_epid, p->p_slot);

	debug_printf("p_holdcnt=%x p_pidp=%x p_pgid=%x *p_pgidp=%x\n",
		     p->p_holdcnt, p->p_pidp, p->p_pgid, p->p_pgidp);
	
	debug_printf("*p_sqhashp=%x", p->p_sqhashp);

	debug_printf("*p_pglinkf=%x *p_pglinkb=%x\n",
		     p->p_pglinkf, p->p_pglinkb);

	debug_printf("p_cttydev=%x p_sid=%x *p_sessp=%x p_ppid=%x\n",
		     p->p_cttydev, p->p_sid, p->p_sessp, p->p_ppid);

	debug_printf("*p_parent=%x *p_child=%d\n", p->p_parent, p->p_child);

	debug_printf("*p_prevsib=%x *p_nextsib=%x\n",
		     p->p_prevsib, p->p_nextsib);

	debug_printf("*p_zombies=%x *p_nextzomb=%x\n",
		     p->p_zombies, p->p_nextzomb);

	debug_printf("*p_nextofkin=%d *p_orphan=%x\n",
		     p->p_nextofkin, p->p_orphan);

	debug_printf("*p_prevorph=%x *p_nextorph=% *p_prev=%x *p_next=%x\n",
		     p->p_prevorph, p->p_nextorph, p->p_prev, p->p_next);

	debug_printf("*p_uidquotap=%x *p_cred=%x *p_cdir=%x *p_rdir=%x\n",
		     p->p_uidquotap, p->p_cred, p->p_cdir, p->p_rdir);

	debug_printf("*p_rlimits=%x *p_lwpp=%x p_nlwp=%x\n",
		     p->p_rlimits, p->p_lwpp, p->p_nlwp);

	debug_printf("p_nstopped=%x p_nprstopped=%x\n",
		     p->p_nstopped, p->p_nprstopped);

	debug_printf("p_nreqstopped=%x p_niblked=%x\n",
		     p->p_nreqstopped, p->p_niblked);

	debug_printf("p_nlwpdir=%x \n", p->p_nlwpdir);

	debug_printf("**p_lwpdir=%x p_lwpidmap=%x\n",
		     p->p_lwpdir, p->p_lwpidmap.idmap_small);


	for (i = 0; i < NSIGWORDS; ) {
		debug_printf("p_sigs.ks_sigbits[%d]=%x ",
			     i, p->p_sigs.ks_sigbits[i]);
		if ((++i % 2) == 0)
			debug_printf("\n");
	}
	if ((--i % 2) == 0)
		debug_printf("\n");

	for (i = 0; i < NSIGWORDS; ) {
		debug_printf("p_sigaccept.ks_sigbits[%d]=%x ",
			     i, p->p_sigaccept.ks_sigbits[i]);

		if ((++i % 2) == 0)
			debug_printf("\n");
	}
	if ((--i % 2) == 0)
		debug_printf("\n");

	for (i = 0; i < NSIGWORDS; ) {
		debug_printf("p_sigtrmask.ks_sigbits[%d]=%x ",
			     i, p->p_sigtrmask.ks_sigbits[i]);

		if ((++i % 2) == 0)
			debug_printf("\n");
	}
	if ((--i % 2) == 0)
		debug_printf("\n");

	for (i = 0; i < NSIGWORDS; ) {
		debug_printf("p_sigignore.ks_sigbits[%d]=%x ",
			     i, p->p_sigignore.ks_sigbits[i]);

		if ((++i % 2) == 0)
			debug_printf("\n");
	}
	if ((--i % 2) == 0)
		debug_printf("\n");

	debug_printf(
	    "(*p_sigreturn)()=%x (*p_sigactret)()=%x p_sigcldinfo=%x\n",
		     p->p_sigreturn, p->p_sigactret, p->p_sigcldinfo);
	
	for (i = 0; i < MAXSIG;) {
		debug_printf("p_sigstate[%d]=%x ", i, p->p_sigstate[i]); 
		if ((++i % 3) == 0)
			debug_printf("\n");
	}
	if ((--i % 3) == 0)
		debug_printf("\n");

	debug_printf("p_sigjobstop=%x p_parentinsess=%x p_maxlwppri=%x\n",
		     p->p_sigjobstop, p->p_parentinsess, p->p_maxlwppri);

	debug_printf("*p_profp=%x\n", p->p_profp);

	debug_printf("p_brkbase=%x p_brksize=%x\n",
		     p->p_brkbase, p->p_brksize);

	debug_printf("p_stkbase=%x p_stksize=%x\n",
		     p->p_stkbase, p->p_stksize);

	debug_printf("&p_vfork=%x &p_wait2wait=%x &p_waitsv=%x\n",
		     &p->p_vfork, &p->p_wait2wait, &p->p_waitsv);

	debug_printf("&p_lwpwaitsv=%x &p_wait2seize=%x &p_seized=%x\n",
		     &p->p_lwpwaitsv, &p->p_wait2seize, &p->p_seized);

	debug_printf("p_nseized=%x p_nrendezvoused=%x &p_rendezvous=%x\n",
		     p->p_nseized, p->p_nrendezvoused, &p->p_rendezvous);

	debug_printf("&p_destroy=%x &p_suspsv=%x &p_rdwrlock=%x\n",
		     &p->p_destroy, &p->p_suspsv, &p->p_rdwrlock);

	debug_printf("p_wdata=%x p_wcode=%x p_nshmseg=%x p_plock=%x\n",
		     p->p_wdata, p->p_wcode, p->p_nshmseg, p->p_plock);

	debug_printf("p_acflag=%x p_sigwait=%x  p_swrss=%x\n",
		     p->p_acflag, p->p_sigwait, p->p_swrss);

	debug_printf("*p_as=%x *p_segacct=%x *p_semundo=%x\n",
		     p->p_as, p->p_segacct, p->p_semundo);

	debug_printf("*p_sdp=%x &p_start=%x\n", p->p_sdp, &p->p_start);

	debug_printf("p_mem=%x &p_ior=%x &p_iow=%x &p_ioch=%x\n",
		     p->p_mem, &p->p_ior, &p->p_iow, &p->p_ioch);

	debug_printf("&p_cmask=%x &p_utime=%x &p_stime=%x\n",
		     &p->p_cmask, &p->p_utime, &p->p_stime);

	debug_printf("&p_cutime=%x &p_cstime=%x\n",
		     &p->p_cutime, &p->p_cstime);

	debug_printf("*p_trace=%x &p_fltmask=%x &p_mw=%x\n",
		     p->p_trace, &p->p_fltmask, &p->p_mw);

	debug_printf("&p_fdtab=%x *p_execinfo=%x *p_auditp=%x\n",
		     &p->p_fdtab, p->p_execinfo, p->p_auditp);

	debug_printf("&p_sw1=%x &p_sw2=%x &p_sw3=%x\n",
		     &p->p_sw1, &p->p_sw2, &p->p_sw3);

	debug_printf("p_notblocked=%x p_slptime=%x\n",
		     p->p_notblocked, p->p_slptime);
}

#endif /* DEBUG || DEBUG_TOOLS */
