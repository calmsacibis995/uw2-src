/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/procfs/prinactive.c	1.10"

#include <proc/proc.h>
#include <fs/vnode.h>
#include <fs/procfs/prdata.h>
#include <fs/procfs/procfs.h>
#include <io/poll.h>
#include <mem/kmem.h>
#include <util/debug.h>
#include <util/ksynch.h>

STATIC void pridinactive(vnode_t *);
STATIC void prfinactive(vnode_t *);


/*
 * prinactive(vp, cr)
 *	Give up the last reference to a /proc vnode.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/* ARGSUSED */
void
prinactive(vnode_t *vp, cred_t *cr)
{
	struct prnode *pnp = VTOP(vp);
	prnodetype_t parenttype;

	ASSERT(vp->v_type == VDIR || vp->v_type == VREG);

	if (pnp->pr_type == PR_PROCDIR) {
		/*
		 * The last reference to the root can only happen on
		 * an unmount.  Maintaining this reference count isn't
		 * strictly necessary but let's be clean about it.
		 */
		VN_LOCK(vp);
		vp->v_count--;
		VN_UNLOCK(vp);
		return;
	}

	parenttype = VTOP(pnp->pr_parent)->pr_type;
	if (parenttype == PR_PIDDIR || parenttype == PR_LWPIDDIR)
		prfinactive(vp);
	else
		pridinactive(vp);
}

/*
 * void pridinactive(vnode_t *)
 *	Give up the last reference to a /proc pid or lwpid directory.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
pridinactive(vnode_t *vp)
{
	prnode_t *pnp = VTOP(vp);
	prcommon_t *prcp;
	proc_t *p;
	lwp_t *lwp;
	vnode_t *dp, **prfiles;
	int n;

	prcp = pnp->pr_common;
	ASSERT(prcp != NULL);
	/*
	 * If this is the last /proc vnode associated with a process
	 * or LWP, clear the p_trace or l_trace pointer.  Watch out
	 * for races with new references being established.
	 */
	ASSERT(pnp->pr_type == PR_PIDDIR || pnp->pr_type == PR_LWPIDDIR);
	/*
	 * Note: lock p_mutex via the process common structure, since an
	 * LWP's common structure might be marked as PRC_DESTROY,
	 * preventing pr_p_mutex() from working even though the process
	 * still exists.
	 */
	if (pr_p_mutex(pnp->pr_pcommon)) {
		p = prcp->prc_proc;
		ASSERT(p != NULL);
		/*
		 * Now that we hold p_mutex, confirm that the reference
	       	 * count on the vnode is still 1.  If it's not, someone
		 * got in and established a new reference, in which case
		 * we give up our reference and return.  If the reference
		 * count is still 1, new references to the vnode will
		 * be blocked because we hold p_mutex.
		 */
		if (vp->v_count > 1) {
			VN_LOCK(vp);
			if (vp->v_count > 1) {
				vp->v_count--;
				VN_UNLOCK(vp);
				UNLOCK(&p->p_mutex, PLBASE);
				return;
			}
			/*
			 * We still hold the only remaining reference
			 * and can proceed to give it up.
			 */
			VN_UNLOCK(vp);
		}
		if (pnp->pr_type == PR_PIDDIR)
			p->p_trace = NULL;
		else if ((lwp = prcp->prc_lwp) != NULL)
			lwp->l_trace = NULL;
		UNLOCK(&p->p_mutex, PLBASE);
	} else {
		/*
		 * The only way the above pr_p_mutex() can fail is for the
		 * proc structure to be gone.
		 */
		ASSERT(pnp->pr_pcommon->prc_proc == NULL);

		/*
		 * Failing to acquire the p_mutex lock via pr_p_mutex() 
		 * does not guarantee that we hold the only remaining
		 * reference, it only means that the process is exiting
		 * or has already exited. Someone could have obtained 
		 * another reference after we last checked the v_count (in
		 * VN_RELE_CRED()) and before the process started to exit. 
		 */
		if (vp->v_count > 1) {
			VN_LOCK(vp);
			if (vp->v_count > 1) {
				vp->v_count--;
				VN_UNLOCK(vp);
				return;
			}
			/*
			 * We still hold the only remaining reference
			 * and can proceed to give it up.
			 */
			VN_UNLOCK(vp);
		}
	}

	/*
	 * Whether or not the process or LWP are still around, the
	 * associated data structures are now orphaned and we can
	 * free them without holding any locks.
	 */
	dp = pnp->pr_parent;
	LOCK_DEINIT(&pnp->pr_mutex);
	VN_DEINIT(vp);
	if ((prfiles = pnp->pr_files) != NULL) {
		n = (pnp->pr_type == PR_PIDDIR) ? npdent : nldent;
		ASSERT(prfilesempty(prfiles, n));
		kmem_free(prfiles, n*sizeof(vnode_t *));
	}
	kmem_free(pnp, sizeof *pnp);
	LOCK_DEINIT(&prcp->prc_mutex);
	if (prcp->prc_pollhead)
		phfree(prcp->prc_pollhead);
	kmem_free(prcp, sizeof *prcp);
	/*
	 * The release of the parent vnode should be last in this
	 * sequence because it may trigger another inactivation.
	 */
	VN_RELE(dp);
}

/*
 * prfinactive(vnode_t *)
 *	Give up the last reference to a /proc vnode that's not a
 *	pid or lwpid directory.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
prfinactive(vnode_t *vp)
{
	prnode_t *pnp = VTOP(vp);
	prnode_t *dpp;
	vnode_t *dp, **pp;

	dp = pnp->pr_parent;
	dpp = VTOP(dp);
	ASSERT(dpp->pr_type == PR_PIDDIR || dpp->pr_type == PR_LWPIDDIR);
	(void)LOCK(&dpp->pr_mutex, PLHI);
	/*
	 * Another reference may have been created before we acquired
	 * pr_mutex.  If so, give up our reference and return.
	 */
	if (vp->v_count > 1) {
		VN_LOCK(vp);
		if (vp->v_count > 1) {
			vp->v_count--;
			VN_UNLOCK(vp);
			UNLOCK(&dpp->pr_mutex, PLBASE);
			return;
		}
		/*
		 * We still hold the only reference; proceed.
		 */
		VN_UNLOCK(vp);
	}
	ASSERT(vp->v_count == 1);
	ASSERT(pnp->pr_files == NULL);

	/* Search the list of vnodes for the one being inactivated. */
	for (pp = &dpp->pr_files[pnp->pr_index];
	     VTOP(*pp) != pnp;
	     pp = &(VTOP(*pp)->pr_next)) {
		ASSERT(*pp);
	}
	*pp = pnp->pr_next;
	UNLOCK(&dpp->pr_mutex, PLBASE);
	/*
	 * Discard the vnode and give up the reference to the parent
	 * directory.
	 */
	VN_DEINIT(vp);
	kmem_free(pnp, sizeof *pnp);
	VN_RELE(dp);
}
