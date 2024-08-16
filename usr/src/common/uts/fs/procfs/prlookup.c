/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/procfs/prlookup.c	1.24"
#ident	"$Header: $"

#include <acc/mac/mac.h>
#include <util/types.h>
#include <proc/cred.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <util/ksynch.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <fs/vnode.h>
#include <fs/pathname.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <proc/exec.h>
#include <fs/procfs/prdata.h>
#include <fs/fs_hier.h>


LKINFO_DECL(prc_mutex_lkinfo, "PP::prc_mutex", 0);
LKINFO_DECL(pr_mutex_lkinfo, "PP::pr_mutex", 0);

STATIC int prprocdir(vnode_t *, char *, vnode_t **);
struct vnode *prpget(vnode_t *, pid_t);
STATIC int prlwpdir(vnode_t *, char *, vnode_t **);
STATIC struct vnode *prlget(vnode_t *, u_int);
STATIC int prmakenode(vnode_t *, char *, vnode_t **, prntable_t *, int);
STATIC int probjectdir(vnode_t *, char *, vnode_t **, cred_t *);


/*
 * int prlookup(vnode_t *dp, char *comp, vnode_t **vpp, pathname_t *pnp,
 *	        int lookup_flags, vnode_t *rootvp, cred_t *cr)
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/* ARGSUSED */
int
prlookup(vnode_t *dp, char *comp, vnode_t **vpp, pathname_t *pnp,
	 int lookup_flags, vnode_t *rootvp, cred_t *cr)
{
	int error;
	struct prnode *dpp = VTOP(dp);

	/*
	 * Handle aliases of the parent directory, and also ".." entries.
	 */
	if (comp[0] == 0 || strcmp(comp, ".") == 0) {
		VN_HOLD(dp);
		*vpp = dp;
		return 0;
	}
	if (strcmp(comp, "..") == 0) {
		VN_HOLD(dpp->pr_parent);
		*vpp = dpp->pr_parent;
		return 0;
	}

	switch (dpp->pr_type) {
	case PR_PROCDIR:
		error = prprocdir(dp, comp, vpp);
		break;
	case PR_PIDDIR:
		error = prmakenode(dp, comp, vpp, pdtable, npdent);
		break;
	case PR_LWPDIR:
		error = prlwpdir(dp, comp, vpp);
		break;
	case PR_LWPIDDIR:
		error = prmakenode(dp, comp, vpp, ldtable, nldent);
		break;
	case PR_OBJECTDIR:
		error = probjectdir(dp, comp, vpp, cr);
		break;
	default:
		/*
		 * Can't happen.  Just say no.
		 */
		error = EINVAL;
		break;
	}

	if (!error)
		/*
		 * The check is place here only to
		 * return ENOENT if calling process has no (MAC)
		 * read access to object.  Otherwise, EACCES
		 * is returned from the independent level.
		 */
		if (MAC_VACCESS(*vpp, VREAD, CRED())) {
			VN_RELE(*vpp);
			*vpp = NULL;
			error = ENOENT;
		}
	return error;
}


/*
 * int prprocdir(dp, comp, vpp)
 *	Lookup a name in the top-level /proc directory.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC int
prprocdir(vnode_t *dp, char *comp, vnode_t **vpp)
{
	register pid_t n = 0;

	/*
	 * Top-level /proc directory.  Entries can only be process ids.
	 */
	while (*comp) {
		if (*comp < '0' || *comp > '9')
			return ENOENT;
		n = 10*n + *comp++ - '0';
		if (n >= MAXPID  || n < 0)
			return ENOENT;
	}

	*vpp = prpget(dp, n);
	return (*vpp == NULL) ? ENOENT : 0;
}


/*
 * struct vnode *prpget(vnode_t *dp, pid_t pid)
 * 	Return a directory vnode for the pid specified.  If necessary,
 *	construct a vnode and associated prcommon structure and attach
 *	them to the process.
 *
 * Calling/Exit State:
 * 	No locks are held on entry, and none are held on exit.
 */
struct vnode *
prpget(vnode_t *dp, pid_t pid)
{
	register proc_t *p;
	register struct prnode *pnp;
	register struct vnode *vp;
	register prcommon_t *prcp;

	/*
	 * Allocate a prcommon structure and a prnode because we'll
	 * probably need them.  If we don't, we just release them.
	 * We should probably use kmem_alloc rather than kmem_zalloc,
	 * since we're explicitly initializing every field anyway;
	 * this is just paranoia.
	 *
	 * Much of this initialization could be done without holding
	 * p_mutex (i.e. before prfind is called); the code should be
	 * along these lines.
	 */
	pnp = kmem_zalloc(sizeof *pnp, KM_SLEEP);
	prcp = kmem_zalloc(sizeof *prcp, KM_SLEEP);

	if ((p = prfind(pid)) == NULL) {
		kmem_free(pnp, sizeof *pnp);
		kmem_free(prcp, sizeof *prcp);
		return NULL;
	}
	if ((vp = p->p_trace) != NULL) {
		/*
		 * This is not the first /proc reference to the process.
		 * Release the structures we allocated and return a
		 * reference to the associated vnode.
		 */
		VN_HOLD(vp);
		UNLOCK(&p->p_mutex, PLBASE);
		kmem_free(pnp, sizeof *pnp);
		kmem_free(prcp, sizeof *prcp);
		return vp;
	}
	if (!(p->p_flag & P_DESTROY) && prchoose(p) == NULL) {
		/* SIDL: Process is not quite yet created */
		UNLOCK(&p->p_mutex, PLBASE);
		kmem_free(pnp, sizeof *pnp);
		kmem_free(prcp, sizeof *prcp);
		return NULL;
	}

	/*
	 * Allocate and initialize a new prcommon structure and vnode.
	 */
	LOCK_INIT(&prcp->prc_mutex, PRC_HIER, PRC_MINIPL,
		  &prc_mutex_lkinfo, KM_NOSLEEP);
	prcp->prc_flags = (p->p_flag & P_DESTROY) ? PRC_DESTROY : 0;
	if (p->p_flag & P_SYS)
		prcp->prc_flags |= PRC_SYS;
	prcp->prc_rdwriters = 0;
	SV_INIT(&prcp->prc_rdwrdone);
	SV_INIT(&prcp->prc_stopsv);
	prcp->prc_pollhead = NULL;
	prcp->prc_pid = p->p_pidp->pid_id;
	prcp->prc_opens = 0;
	prcp->prc_writers = 0;
	prcp->prc_proc = p;
	prcp->prc_slot = p->p_slot;

	vp = &pnp->pr_vnode;
	VN_INIT(vp, procvfs, VDIR, 0, VNOMAP|VNOMOUNT|VNOSWAP, KM_NOSLEEP);
	vp->v_op = &prvnodeops;
	vp->v_lid = p->p_cred->cr_lid;
	vp->v_macflag = VMAC_SUPPORT;
	vp->v_data = pnp;

	pnp->pr_type = PR_PIDDIR;
	pnp->pr_mode = 0555;
	pnp->pr_ino = prino(p->p_slot, 0, PR_PIDDIR);
	pnp->pr_flags = 0;
	pnp->pr_common = prcp;
	pnp->pr_pcommon = prcp;
	pnp->pr_parent = dp;
	pnp->pr_index = 0;
	pnp->pr_files = NULL;
	LOCK_INIT(&pnp->pr_mutex, PRN_HIER, PRN_MINIPL,
	  &pr_mutex_lkinfo, KM_NOSLEEP);
	pnp->pr_next = NULL;
	p->p_trace = vp;
	VN_HOLD(dp);
	UNLOCK(&p->p_mutex, PLBASE);
	return vp;
}


/*
 * int prlwpdir(vnode_t *dp, char *comp, vnode_t **vpp)
 *	Lookup a vnode in /proc/pid/lwp.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC int
prlwpdir(vnode_t *dp, char *comp, vnode_t **vpp)
{
	long int n = 0;

	/*
	 * /proc/pid/lwp directory.  Entries can only be lwp ids.
	 */
	while (*comp) {
		if (*comp <'0' || *comp > '9')
			return ENOENT;
		n = 10 * n + *comp++ - '0';
	}
	if (n <= 0)	/* Illegal LWP id */
		return ENOENT;
	*vpp = prlget(dp, n);
	return (*vpp == NULL) ? ENOENT : 0;
}


/*
 * struct vnode *prlget(vnode_t *dp, u_int lwpid)
 *	Return a directory vnode for the lwpid specified.  If necessary,
 *	construct a vnode and associated prcommon structure and attach
 *	them to the lwp.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC struct vnode *
prlget(vnode_t *dp, u_int lwpid)
{
	register proc_t *p;
	register lwp_t *lwp;
	register struct prnode *pnp;
	register struct vnode *vp;
	register prcommon_t *prcp, *pprcp;

	/*
	 * Again, we could use kmem_alloc instead of kmem_zalloc here,
	 * and much of the initialization could be done without holding
	 * p_mutex.
	 */
	pnp = kmem_zalloc(sizeof *pnp, KM_SLEEP);
	prcp = kmem_zalloc(sizeof *prcp, KM_SLEEP);
	pprcp = VTOP(dp)->pr_common;
	if (!pr_p_mutex(pprcp)) {
		kmem_free(pnp, sizeof *pnp);
		kmem_free(prcp, sizeof *prcp);
		return NULL;
	}
	/*
	 * Process exists and won't go away while we hold p_mutex.
	 */
	p = pprcp->prc_proc;
	/*
	 * Locate the target LWP structure and establish a reference
	 * to a /proc vnode (if necessary).
	 */
	if (lwpid > p->p_nlwpdir ||
	    (lwp = p->p_lwpdir[lwpid-1]) == NULL ||
	    lwp->l_stat == SIDL) {
		UNLOCK(&p->p_mutex, PLBASE);
		kmem_free(pnp, sizeof *pnp);
		kmem_free(prcp, sizeof *prcp);
		return NULL;
	}
	if ((vp = lwp->l_trace) != NULL) {
		/*
		 * This is not the first /proc reference to the lwp.
		 * Release the structures we allocated and return a
		 * reference to the associated vnode.
		 */
		VN_HOLD(vp);
		UNLOCK(&p->p_mutex, PLBASE);
		kmem_free(pnp, sizeof *pnp);
		kmem_free(prcp, sizeof *prcp);
		return vp;
	}
	/*
	 * Initialize the new prnode and prcommon structure.
	 */
	LOCK_INIT(&prcp->prc_mutex, PRC_HIER, PRC_MINIPL,
		  &prc_mutex_lkinfo, KM_NOSLEEP);
	prcp->prc_flags = (p->p_flag & P_SYS) ? PRC_LWP|PRC_SYS : PRC_LWP;
	prcp->prc_rdwriters = 0;
	SV_INIT(&prcp->prc_rdwrdone);
	SV_INIT(&prcp->prc_stopsv);
	prcp->prc_pollhead = NULL;
	prcp->prc_pid = p->p_pidp->pid_id;
	prcp->prc_opens = 0;		/* not used for LWPs */
	prcp->prc_writers = 0;		/* not used for LWPs */
	prcp->prc_proc = p;
	prcp->prc_slot = p->p_slot;
	prcp->prc_lwp = lwp;
	prcp->prc_lwpid = lwp->l_lwpid;

	vp = &pnp->pr_vnode;
	VN_INIT(vp, procvfs, VDIR, 0, VNOMAP|VNOMOUNT|VNOSWAP, KM_NOSLEEP);
	vp->v_op = &prvnodeops;
	vp->v_lid = p->p_cred->cr_lid;
	vp->v_macflag = VMAC_SUPPORT;
	vp->v_data = pnp;

	pnp->pr_type = PR_LWPIDDIR;
	pnp->pr_mode = 0555;
	pnp->pr_ino = prino(p->p_slot, lwpid, PR_LWPIDDIR);
	pnp->pr_flags = 0;
	pnp->pr_common = prcp;
	pnp->pr_pcommon = pprcp;
	pnp->pr_parent = dp;
	pnp->pr_index = 0;
	pnp->pr_files = NULL;
	pnp->pr_next = NULL;
	LOCK_INIT(&pnp->pr_mutex, PRN_HIER, PRN_MINIPL,
		  &pr_mutex_lkinfo, KM_NOSLEEP);

	lwp->l_trace = vp;
	VN_HOLD(dp);

	UNLOCK(&p->p_mutex, PLBASE);
	return vp;
}


prntable_t pdtable[] = {
	"as",		PR_AS,		0, 0, VREG, 0600,
	"ctl",		PR_CTL,		0, 1, VREG, 0200,
	"status",	PR_STATUS,	0, 1, VREG, 0400,
	"psinfo",	PR_PSINFO,	1, 1, VREG, 0444,
	"map",		PR_MAP,		0, 0, VREG, 0400,
	"cred",		PR_CRED,	0, 1, VREG, 0400,
	"sigact",	PR_SIGACT,	0, 1, VREG, 0400,
	"object",	PR_OBJECTDIR,	0, 0, VDIR, 0500,
	"lwp",		PR_LWPDIR,	0, 1, VDIR, 0555,
};

prntable_t ldtable[] = {
	"lwpctl",	PR_LWPCTL,	0, 1, VREG, 0200,
	"lwpstatus",	PR_LWPSTATUS,	0, 1, VREG, 0400,
	"lwpsinfo",	PR_LWPSINFO,	1, 1, VREG, 0444,
};

int	npdent = sizeof pdtable/sizeof pdtable[0];
int	nldent = sizeof ldtable/sizeof ldtable[0];


/*
 * int prmakenode(vnode_t *dp, char *comp, vnode_t **vpp,
 *		  prntable_t *tp, int n)
 * 	Make a /proc vnode for the given <directory, name> pair if one
 * 	does not already exist, or return a new reference if it does
 * 	already exist.  Return a vnode pointer in *vpp.  Function returns
 * 	zero on success or an errno on failure.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC int
prmakenode(vnode_t *dp, char *comp, vnode_t **vpp, prntable_t *tp, int n)
{
	prnode_t *pnp, *opnp, *dpp = VTOP(dp);
	prcommon_t *prcp = dpp->pr_common;
	vnode_t *vp, *ovp, **prfiles;
	int i, zombie, nas;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(dp->v_type == VDIR);
	ASSERT(dpp->pr_type == PR_PIDDIR || dpp->pr_type == PR_LWPIDDIR);

	if (!pr_p_mutex(prcp)) {
		zombie = nas = B_TRUE;
	} else {
		zombie = ((prcp->prc_flags & PRC_DESTROY) != 0);
		nas = ((prcp->prc_flags & PRC_SYS) != 0 ||
		       prcp->prc_proc->p_as == NULL);
		UNLOCK(&prcp->prc_proc->p_mutex, PLBASE);
	}
	for (i = 0; i < n; i++, tp++)
		if (strcmp(comp, tp->prn_comp) == 0 &&
		    (!zombie || tp->prn_zvisible) &&
		    (!nas || tp->prn_nasvisible))
			break;
	if (i >= n)
		return ENOENT;
	/*
	 * Found a match.  Return a reference to the vnode if it
	 * already exists; create it if it doesn't.  Build data
	 * structures as necessary.
	 *
	 * Preallocate a prnode in case we need it.  Our best guess
	 * is that most of the time we will.  We're also guessing that
	 * much of the time we WON'T need to preallocate the pr_files
	 * array in the parent prnode.
	 */
	pnp = kmem_alloc(sizeof *pnp, KM_SLEEP);

	(void) LOCK(&dpp->pr_mutex, PLHI);
	if (dpp->pr_files == NULL) {
		UNLOCK(&dpp->pr_mutex, PLBASE);
		prfiles = kmem_zalloc(n*sizeof (vnode_t *), KM_SLEEP);
		(void) LOCK(&dpp->pr_mutex, PLHI);
		if (dpp->pr_files == NULL)
			dpp->pr_files = prfiles;
		else
			kmem_free(prfiles, n*sizeof (vnode_t *));
	}
	if ((ovp = dpp->pr_files[i]) != NULL) {
		/*
		 * A vnode already exists, but it might be stale.
		 * Note that pr_flags is stable as long as the parent
		 * pr_mutex is held.
		 */
		opnp = VTOP(ovp);
		if ((opnp->pr_flags & PR_INVAL) == 0) {
			VN_HOLD(ovp);
			UNLOCK(&dpp->pr_mutex, PLBASE);
			kmem_free(pnp, sizeof *pnp);
			*vpp = ovp;
			return 0;
		}
		/*
		 * Vnode is stale; initialize new one and link old one into
		 * chain of invalid vnodes.
		 */
	}
	/*
	 * Initialize and return the new vnode.
	 */
	pnp->pr_type = tp->prn_ctype;
	pnp->pr_mode = tp->prn_mode;
	pnp->pr_ino = prino(prcp->prc_slot, prcp->prc_lwpid, tp->prn_ctype);
	pnp->pr_flags = dpp->pr_flags;
	pnp->pr_common = dpp->pr_common;
	pnp->pr_pcommon = dpp->pr_pcommon;
	VN_HOLD(dp);
	pnp->pr_parent = dp;
	pnp->pr_index = i;
	pnp->pr_files = NULL;
	pnp->pr_next = ovp;

	vp = PTOV(pnp);
	VN_INIT(vp, procvfs, tp->prn_ftype, 0,
		VNOMAP|VNOMOUNT|VNOSWAP, KM_NOSLEEP);
	vp->v_op = &prvnodeops;
	vp->v_lid = dp->v_lid;
	vp->v_macflag = VMAC_SUPPORT;
	vp->v_data = pnp;

	dpp->pr_files[i] = vp;
	UNLOCK(&dpp->pr_mutex, PLBASE);
	*vpp = vp;

	return 0;
}


/*
 * int probjectdir(vnode_t *dp, char *comp, vnode_t **vpp, cred_t *cr)
 *	Lookup a vnode in /proc/pid/object.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC int
probjectdir(vnode_t *dp, char *comp, vnode_t **vpp, cred_t *cr)
{
	prnode_t *dpp = VTOP(dp);
	prcommon_t *prcp = dpp->pr_common;
	proc_t *p;
	vnode_t *vp, **vplist;
	struct as *as;
	register int i, error = 0, nvp, nsegs;
	ino_t ino;
	char name[64];

	/*
	 * Stabilize the address-space before proceeding.  Acquire the
	 * process reader-writer lock and the address-space lock.
	 */
	if (!pr_p_rdwr(prcp, B_FALSE))
		return ENOENT;
	p = prcp->prc_proc;
	ASSERT(p != NULL);
	if ((as = p->p_as) == NULL) {
		/*
		 * Highly unlikely, but a user process might become a system
		 * process and race with us here.
		 */
		pr_v_rdwr(prcp);
		return 0;
	}
	as_rdlock(as);

	if (strcmp(comp, "a.out") == 0) {
		if (p->p_execinfo == NULL
		  || (vp = p->p_execinfo->ei_execvp) == NULL)
			error = ENOENT;
		else {
			VN_HOLD(vp);
			*vpp = vp;
		}
		as_unlock(as);
		pr_v_rdwr(prcp);
		return error;
	}
	/*
	 * Locate all vnodes in the address space, construct names for
	 * them, and see if they match the supplied name.
	 */
	nsegs = prcountsegs(as);
	vplist = kmem_alloc(nsegs*sizeof (vnode_t *), KM_SLEEP);
	nvp = prvpsegs(p, vplist);
	error = ENOENT;
	for (i = 0; i < nvp; i++) {
		prmapname(vplist[i], name, &ino, cr);
		if (strcmp(name, comp) == 0) {
			vp = vplist[i];
			VN_HOLD(vp);
			*vpp = vp;
			error = 0;
			break;
		}
	}
	kmem_free(vplist, nsegs*sizeof (vnode_t *));
	as_unlock(as);
	pr_v_rdwr(prcp);
	return error;
}
