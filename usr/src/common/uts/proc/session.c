/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/session.c	1.17"
#ident	"$Header: $"

#include <util/types.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <proc/lwp.h>
#include <proc/session.h>
#include <svc/errno.h>
#include <util/ksynch.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <fs/fs_hier.h>
#include <fs/vnode.h>
#include <fs/specfs/snode.h>
#include <mem/kmem.h>
#include <io/conf.h>


/*
 * Session support routines.
 */


/*
 *+ Lock protecting the session structure.
 */
LKINFO_DECL(sess_lkinfo, "PP::s_mutex", 0);


/*
 *
 * void sess_init(proc_t *p)
 *	This function initializes the session management system for
 *	process 0.  This function should only be called once during
 *	system initialization.
 *	
 * Calling/Exit State:
 *	No locks held on entry and no locks held on exit.
 * 	Must be called after the pid structure is initialized
 *	via pid_alloc().
 *
 */
void
sess_init(proc_t *p)
{
	sess_t *sp;

	ASSERT(KS_HOLD0LOCKS());

	if (!(sp = kmem_alloc(sizeof *sp, KM_NOSLEEP))) {
		/*
		 *+ Out of memory during process management initialization.
		 *+ Corrective action: Check your configuration.
		 */
		cmn_err(CE_PANIC, "sess_init(): could not allocate space \n");
	}
	LOCK_INIT(&sp->s_mutex, PR_SSHIER, PR_SSMINIPL,
		  &sess_lkinfo, KM_NOSLEEP);
	sp->s_ctty = 0;
	sp->s_cttyref = 0;
	sp->s_ref = 1;
	sp->s_vp = NULL;
	sp->s_pgrps = p->p_pidp;
	sp->s_sidp = p->p_pidp;
	sp->s_cred = NULL;
	p->p_sessp = sp;
}


/*
 *
 * sess_t *sess_create(sess_t *sp)
 * 	Creates a new session.
 *
 * Calling/Exit State:
 *	The p_sess_mutex lock of the calling process and the s_mutex lock
 *	of the session in which the calling process resides must be held.
 *	Upon return, p_sess_mutex remains held, while s_mutex has been
 *	released.
 * 	This function returns a pointer to old session to be freed by the
 *	caller (after releasing a spin lock) if the caller was the last
 *	process in the session.  Otherwise, NULL is returned.
 *
 */
sess_t *
sess_create(sess_t *sp)
{
	proc_t *p;
	sess_t *osp;

	p = u.u_procp;

	ASSERT(LOCK_OWNED(&p->p_sess_mutex));
	ASSERT(LOCK_OWNED(&p->p_sessp->s_mutex));

	osp = p->p_sessp;
	ASSERT(osp->s_ref > 0);

	p->p_parentinsess = 0;
	if (--osp->s_ref > 0) {
		/*
		 * Clear the p_parentinsess flag for all children of the
		 * departing process (if there are any).
		 */
		if (p->p_child != NULL) {
			/*
			 * We take advantage of the fact that p_child cannot
			 * go from NULL to non-NULL, unless the session
			 * lock is held locked.
			 *
			 *  NOTE: We walk the processes in the session to
			 *	  find children rather than walk the parent-
			 *	  child-sibling list, due to the locking
			 *	  overhead to get a stable child list.
			 */
			proc_t *np;
			struct pid *pgidp;

			for (pgidp = osp->s_pgrps;
			     pgidp != NULL; pgidp = pgidp->pid_pgrpf) {
				np = pgidp->pid_pgprocs;
				do {
					if (np->p_parent != p)
						continue;
					/*
					 * For all processes in the group that
					 * have the calling process as the
					 * parent, clear p_parentinsess to
					 * prevent multiple calls to
					 * pgorphaned() against the same group.
					 */
					np->p_parentinsess = 0;
					while ((np = np->p_pglinkf) != NULL) {
						if (np->p_parent == p)
							np->p_parentinsess = 0;
					}
					if (pgidp != p->p_pgidp &&
					    (pgidp->pid_pgprocs->p_flag
							     & P_PGORPH) == 0) {
						/*
						 * Perform orphan check/action.
						 */
						(void)pgorphaned(pgidp,
								 B_FALSE);
					}
					break;
				} while ((np = np->p_pglinkf) != NULL);
			}
		}
		/*
		 * Depart from our process group (indicating our intention
		 * to leave the session as well).
		 */
		pgexit(p, B_TRUE);
		p->p_sessp = NULL;	/* interlock with setpgid() */
		UNLOCK(&osp->s_mutex, PL_SESS);
		osp = NULL;
	} else {
		int refcnt;
		p->p_sessp = NULL;	/* interlock with setpgid() */
		refcnt = osp->s_cttyref;
		UNLOCK(&osp->s_mutex, PL_SESS);
		pid_rele(p->p_pgidp);
		pid_rele(osp->s_sidp);
		if (refcnt == 0)
			LOCK_DEINIT(&osp->s_mutex);
		else
			osp = NULL;
	}

	/*
	 * Initialize new session.
	 */
	LOCK_INIT(&sp->s_mutex, PR_SSHIER, PR_SSMINIPL,
		  &sess_lkinfo, KM_NOSLEEP);
	sp->s_ctty = 0;
	sp->s_cttyref = 0;
	sp->s_ref = 1;
	sp->s_vp = NULL;
	sp->s_pgrps = NULL;
	sp->s_sidp = p->p_pidp;
	sp->s_cred = NULL;
	p->p_sessp = sp;
	p->p_cttydev = NODEV;

	/*
	 * Make pp the process group leader of a new process group
	 * (pgid == pid).
	 */
	pgjoin(p, p->p_pidp);

	/*
	 * Increment the reference count of the process group.
	 */
	pid_hold(sp->s_sidp);

	(void)LOCK(&p->p_mutex, PLHI);
	p->p_sid = p->p_pidp->pid_id;
	UNLOCK(&p->p_mutex, PL_SESS);
	return(osp);
}


/*
 *
 * void alloctty(vnode_t *vp)
 * 	This function establishes the given vnode as the common vnode of the
 * 	terminal device that is to be the controlling terminal of the session
 * 	for the calling process.
 *
 * Calling/Exit State:
 *	The stream associated with the given vnode is held locked by the
 *	caller.  The session (s_mutex) and p_sess_mutex lock of the calling
 *	process must also be held locked by the calling process.
 *	All locks remain held upon return.
 */
void
alloctty(vnode_t *vp)
{
	dev_t rdev;		/* rdev of controlling terminal */
	struct pid *pgidp;	/* used for non-zombie pgrp walking */
	sess_t *sp;		/* session of calling process */

	ASSERT(LOCK_OWNED(&u.u_procp->p_sess_mutex));
	ASSERT(LOCK_OWNED(&u.u_procp->p_sessp->s_mutex));

	/*
	 * Establish the necessary references to the common vnode.
	 * Note that the snode portion of a common vnode references
	 * itself, hence the double increment of s_count.
	 */

	VN_HOLD(vp);
	opencount(vp, 2);

	/*
	 * Assign values for new terminal to session.
	 */
	sp = u.u_procp->p_sessp;
	sp->s_ctty = 1;
	sp->s_vp = vp;
	crhold(u.u_lwpp->l_cred);
	sp->s_cred = u.u_lwpp->l_cred;

	/*
	 * Update each non-zombie process in the session to be aware of the
	 * new controlling tty.
	 */
	rdev = vp->v_rdev;
	pgidp = sp->s_pgrps;
	do {
		proc_t *p;
		p = pgidp->pid_pgprocs;
		do {
			p->p_cttydev = rdev;
		} while ((p = p->p_pglinkf) != NULL);
	} while ((pgidp = pgidp->pid_pgrpf) != NULL);
}


/*
 *
 * void relectty(struct vnode *vp, cred_t *crp )
 *	Release the given vnode from identifying a controlling terminal.
 *
 * Calling/Exit State:
 *	No locks can be held on entry, as this function invokes VOP_CLOSE()
 *	which may block.  No locks are held upon return either.
 *
 * Remarks:
 *	This function is called either from freectty() or from the various
 *	sy_XXXX() functions in the gentty.c (/dev/tty) driver.
 *
 *	In the freectty() function case, the logic is as follows:
 *
 *		pl = LOCK(&sessp->s_mutex, PL_SESS);
 *		cttyref = sessp->s_cttyref;
 *		sessp->s_vp = NULL;
 *		UNLOCK(&sessp->s_mutex, pl);
 *		if (cttyref == 0) {
 *			relectty(vp, sessp->s_cred);
 *		}
 *
 *	In the gentty.c case, the logic is:
 *
 *		ASSERT(KS_HOLD0LOCKS());
 *
 *		procp = u.u_procp;
 *		if (procp->p_nlwp > 1) {	-- multiple LWPs, guard sessp
 *			(void)LOCK(&procp->p_sess_mutex, PL_SESS);
 *			sessp = procp->p_sessp;
 *			(void)LOCK(&sessp->s_mutex, PL_SESS);
 *			UNLOCK(&procp->p_sess_mutex, PL_SESS);
 *		} else {			-- single LWP
 *			sessp = procp->p_sessp;
 *			(void)LOCK(&sessp->s_mutex, PL_SESS);
 *		}
 *
 *		if (!sessp->s_ctty) {
 *			UNLOCK(&sessp->s_mutex, PLBASE);
 *			return (ENXIO);
 *		}
 *		if ((ttyvp = sessp->s_vp) == NULL) {
 *			UNLOCK(&sessp->s_mutex, PLBASE);
 *			return (EIO);
 *		}
 *
 *		SP_CTTYHOLD(sessp);		-- hold execution reference
 *		UNLOCK(&sessp->s_mutex, PLBASE);
 *
 *		<do the appropriate op>
 *
 *		-- Release the execution reference.
 *		-- NOTE: It is _very_ important to not again fetch p_sessp,
 *		-- as another LWP in the process could have performed a
 *		-- setsid(2) or setpgrp(2) operation causing us to enter
 *		-- a new session as a session leader process.  It is critical
 *		-- that we continue to refer to the original session: sessp
 *		-- here in this code.  Otherwise, we could decrement the
 *		-- execution reference counter in the wrong session.
 *
 *		(void)LOCK(&sessp->s_mutex, PL_SESS);
 *		SP_CTTYREL(sessp, ttyvp);
 *
 */
void
relectty(struct vnode *vp, cred_t *crp)
{

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(crp != NULL);
	ASSERT(vp->v_stream != NULL);

	/*
	 * Release our references to the common vnode of the device
	 * (making it look like last-close).
	 */
	VOP_CLOSE(vp, FREAD|FWRITE, B_TRUE, (off_t)0, crp);
	VN_RELE(vp);
	crfree(crp);
}


/*
 *
 * void freectty()
 * 	Release the controlling terminal referenced by the session
 *	of the  calling process.
 *
 * Calling/Exit State:
 *	No locks must be held upon entry and none held upon return.
 *
 * Remarks
 *	This function is called from exit when the controlling process
 *	exit.  The process is already single threded at this point.
 */
void
freectty(void)
{
	sess_t *sp;
	proc_t *p;
	struct pid *pgidp;		/* used for non-zombie pgrp walking */
	vnode_t *vp;
	ulong_t cttyref;
	major_t ttymaj;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(u.u_procp->p_nlwp == 1);

	sp = u.u_procp->p_sessp;
	vp = sp->s_vp;

	ttymaj = getmajor(u.u_procp->p_cttydev);
	if (strcmp("tpath", cdevsw[ttymaj].d_name) == 0) {
		/* 
		 * Sever the TP from the actual tty device so that anybody who
		 * maintains an open reference to the device (and had been
		 * working through a TP) will lose all access to it.
		 */
		tp_disconnect(u.u_procp->p_cttydev);
	}

	ASSERT(vp != NULL);
	(void)LOCK(&sp->s_mutex, PL_SESS);
	cttyref = sp->s_cttyref;
	sp->s_vp = NULL;
	/*
	 * Update each non-zombie process in the session to be aware that
	 * the controlling tty has been discarded.
	 */
	pgidp = sp->s_pgrps;
	do {
		p = pgidp->pid_pgprocs;
		do
			p->p_cttydev = NODEV;
		while ((p = p->p_pglinkf) != NULL);
	} while ((pgidp = pgidp->pid_pgrpf) != NULL);

	if (vp->v_stream) {
		strfreectty(vp->v_stream); /* s_mutex is returned unlocked */
	} else
		UNLOCK(&sp->s_mutex, PL0);

	if (cttyref == 0)
		relectty(vp, sp->s_cred);
}
