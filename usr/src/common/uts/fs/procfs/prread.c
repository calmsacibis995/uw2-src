/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/procfs/prread.c	1.17"
#ident	"$Header: $"

#include <util/types.h>
#include <svc/time.h>
#include <svc/clock.h>
#include <proc/cred.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <util/ksynch.h>
#include <mem/kmem.h>
#include <mem/as.h>
#include <mem/seg.h>
#include <mem/vmparam.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/class.h>
#include <proc/exec.h>
#include <proc/mman.h>
#include <proc/user.h>
#include <util/sysmacros.h>
#include <io/uio.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <proc/user.h>
#include <fs/procfs/procfs.h>
#include <fs/procfs/prdata.h>
#include <fs/procfs/prsystm.h>
#include <util/cmn_err.h>


STATIC int prreadas(vnode_t *, uio_t *, int, cred_t *);
STATIC int prreadstatus(vnode_t *, uio_t *, int, cred_t *);
STATIC int prreadpsinfo(vnode_t *, uio_t *, int, cred_t *);
STATIC int prreadmap(vnode_t *, uio_t *, int, cred_t *);
STATIC int prreadcred(vnode_t *, uio_t *, int, cred_t *);
STATIC int prreadsigact(vnode_t *, uio_t *, int, cred_t *);
STATIC int prreadlwpstatus(vnode_t *, uio_t *, int, cred_t *);
STATIC int prreadlwpsinfo(vnode_t *, uio_t *, int, cred_t *);


/*
 * int prread(vnode_t *vp, uio_t uiop, int ioflag, cred_t *cr)
 *
 * Calling/Exit State:
 *	VOP_RDWRLOCK must have been applied by the caller.
 */
int
prread(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	struct prnode *pnp = VTOP(vp);
	int error;

	switch (pnp->pr_type) {
	case PR_PROCDIR:
	case PR_PIDDIR:
	case PR_OBJECTDIR:
	case PR_LWPDIR:
	case PR_LWPIDDIR:
		error = EISDIR;
		break;
	case PR_CTL:
	case PR_LWPCTL:
		/*
		 * "Can't happen."
		 */
		error = 0;	/* NOP */
		break;
	case PR_AS:
		error = prreadas(vp, uiop, ioflag, cr);
		break;
	case PR_STATUS:
		error = prreadstatus(vp, uiop, ioflag, cr);
		break;
	case PR_PSINFO:
		error = prreadpsinfo(vp, uiop, ioflag, cr);
		break;
	case PR_MAP:
		error = prreadmap(vp, uiop, ioflag, cr);
		break;
	case PR_CRED:
		error = prreadcred(vp, uiop, ioflag, cr);
		break;
	case PR_SIGACT:
		error = prreadsigact(vp, uiop, ioflag, cr);
		break;
	case PR_LWPSTATUS:
		error = prreadlwpstatus(vp, uiop, ioflag, cr);
		break;
	case PR_LWPSINFO:
		error = prreadlwpsinfo(vp, uiop, ioflag, cr);
		break;
	default:
		error = EINVAL;		/* "Can't happen" */
		break;
	}

	return error;
}


/*
 * int prreadas(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 * 	Read the address-space of the target process.
 *
 * Calling/Exit State:
 *	VOP_RDWRLOCK must have been applied before entry.
 */
/* ARGSUSED */
STATIC int
prreadas(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	prnode_t *pnp = VTOP(vp);
	prcommon_t *prcp = pnp->pr_common;
	int error;

	if (!pr_p_rdwr(prcp, B_FALSE))
		/*
		 * Address space is going, or gone.	
		 */
		return ENOENT;
	if (pnp->pr_flags & PR_INVAL)
		error = EBADF;
	else
		error = prusrio(prcp->prc_proc, UIO_READ, uiop);
	pr_v_rdwr(prcp);
	return error;
}


/*
 * int prreadstatus(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 *	Read the process status file.
 *
 * Calling/Exit State:
 *	VOP_RDWRLOCK must have been applied before entry.
 */
/* ARGSUSED */
STATIC int
prreadstatus(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	prnode_t *pnp = VTOP(vp);
	prcommon_t *prcp = pnp->pr_common;
	pstatus_t ps;
	char *start;
	int count;
	int error;

	if (!pr_p_rdwr(prcp, B_FALSE))
		return ENOENT;
	if (pnp->pr_flags & PR_INVAL) {
		pr_v_rdwr(prcp);
		return EBADF;
	}
	if (uiop->uio_offset >= sizeof(pstatus_t)) {	/* EOF */
		pr_v_rdwr(prcp);
		return 0;
	}

	error = prgetpstatus(prcp->prc_proc, &ps);

	pr_v_rdwr(prcp);

	if (error)
		return error;

	start = (char *)&ps + uiop->uio_offset;
	count = sizeof ps - uiop->uio_offset;
	return uiomove(start, count, UIO_READ, uiop);
}


/*
 * int prreadpsinfo(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 *	Read the file of ps(1) information.
 *
 * Calling/Exit State:
 *	VOP_RDWRLOCK must have been applied before entry.
 */
/* ARGSUSED */
STATIC int
prreadpsinfo(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	prcommon_t *prcp = VTOP(vp)->pr_common;
	proc_t *p;
	struct psinfo psi;
	char *start;
	int count;

	bzero(&psi, sizeof psi);

	if (!pr_p_mutex(prcp))
		return ENOENT;
	p = prcp->prc_proc;
	ASSERT(p != NULL);
	if (VTOP(vp)->pr_flags & PR_INVAL) {
		UNLOCK(&p->p_mutex, PLBASE);
		return EBADF;
	}
	if (uiop->uio_offset >= sizeof psi) {	/* EOF */
		UNLOCK(&p->p_mutex, PLBASE);
		return 0;
	}

	prgetpsinfo(p, &psi);
	ASSERT(KS_HOLD0LOCKS());

	start = (char *)&psi + uiop->uio_offset;
	count = sizeof psi - uiop->uio_offset;
	return uiomove(start, count, UIO_READ, uiop);
}


/*
 * int prreadmap(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 *	Read the address-map file.
 *
 * Calling/Exit State:
 *	VOP_RDWRLOCK must have been applied before entry.
 */
/* ARGSUSED */
STATIC int
prreadmap(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	struct prnode *pnp = VTOP(vp);
	struct prcommon *prcp = pnp->pr_common;
	struct proc *p;
	struct prmap *map, *mp;
	struct seg *seg, *sseg, *brkseg;
	int count, error, nsegs, msize;
	vaddr_t saddr, addr, eaddr;
	u_int prot;
	ino_t ino;
	vnode_t *svp;
	struct as *as;
	char *start;
	char name[PRMAPSZ];

	if (!pr_p_rdwr(prcp, B_FALSE))
		return ENOENT;
	if (pnp->pr_flags & PR_INVAL) {
		pr_v_rdwr(prcp);
		return EBADF;
	}
	p = prcp->prc_proc;
	ASSERT(p != NULL);
	if ((as = p->p_as) == NULL) {
		/* User process became system process? */
		pr_v_rdwr(prcp);
		return EIO;
	}
	as_rdlock(as);
	nsegs = prnsegs(as);
	msize = nsegs*sizeof(struct prmap);
	if (uiop->uio_offset >= msize) {
		as_unlock(as);
		pr_v_rdwr(prcp);
		return 0;
	}
	map = kmem_zalloc(msize, KM_SLEEP);
	mp = map;
	/*
	 * Walk the segment list, constructing the map file entries.
	 */
	brkseg = as_segat(as, p->p_brkbase + p->p_brksize - 1);
	seg = sseg = as->a_segs;
	do {
		saddr = seg->s_base;
		eaddr = seg->s_base + seg->s_size;
		do { 
			mp->pr_vaddr = saddr;
			mp->pr_mflags = 0;
			prot = as_getprot(as, saddr, &addr);
			mp->pr_size = addr - saddr;
			if (prot & PROT_READ)
				mp->pr_mflags |= MA_READ;
			if (prot & PROT_WRITE)
				mp->pr_mflags |= MA_WRITE;
			if (prot & PROT_EXEC)
				mp->pr_mflags |= MA_EXEC;
			mp->pr_off = SOP_GETOFFSET(seg, saddr);
			if (SOP_GETTYPE(seg, saddr) == MAP_SHARED)
				mp->pr_mflags |= MA_SHARED;
			if (seg == brkseg)
				mp->pr_mflags |= MA_BREAK;
			else if (eaddr <= STK_HIGHADDR(p) &&
				 saddr >= STK_LOWADDR(p)) {
				mp->pr_mflags |= MA_STACK;
				as_memory(as, &saddr, (uint_t *)&mp->pr_size);
				mp->pr_vaddr = saddr;
			}
			if (SOP_GETVP(seg, saddr, &svp) == 0
			  && svp->v_type == VREG) {
				prmapname(svp, name, &ino, cr);
				strncpy(mp->pr_mapname, name, PRMAPSZ);
			}
			mp++;
		} while ((saddr = addr) != eaddr);
	} while ((seg = seg->s_next) != sseg);

	as_unlock(as);
	pr_v_rdwr(prcp);

	start = (char *)map + uiop->uio_offset;
	count = msize - uiop->uio_offset;
	error = uiomove(start, count, UIO_READ, uiop);
	kmem_free(map, msize);
	return error;
}


/*
 * int prreadcred(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 *	Read the process credentials.
 *
 * Calling/Exit State:
 *	VOP_RDWRLOCK must have been applied before entry.
 */
/* ARGSUSED */
STATIC int
prreadcred(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	prcommon_t *prcp = VTOP(vp)->pr_common;
	proc_t *p;
	struct prcred *prcredp;
	struct cred *tcr;
	int i, n, error, count;
	unsigned int size;
	char *start;

	if (!pr_p_mutex(prcp))
		return ENOENT;
	p = prcp->prc_proc;
	ASSERT(p != NULL);
	if (VTOP(vp)->pr_flags & PR_INVAL) {
		UNLOCK(&p->p_mutex, PLBASE);
		return EBADF;
	}
	/*
	 * Acquire a reference to the target process cred structure.
	 */
	tcr = p->p_cred;
	crhold(tcr);
	UNLOCK(&p->p_mutex, PLBASE);

	/*
	 * prcred_t has space for one supplementary group. Allocate space for
	 * sizeof(prcred_t) if n == 0 or 1, or exactly as many groups as we 
	 * need if n > 1.  This code makes
	 * assumptions about structure padding that are not strictly
	 * legitimate but are no problem in practice.
	 */
	n = tcr->cr_ngroups;
	size = sizeof(prcred_t);
	if (n > 1)
		size += (n-1)*sizeof(gid_t);

	if (uiop->uio_offset >= size) {	/* EOF */
		crfree(tcr);
		return 0;
	}

	prcredp = kmem_alloc(size, KM_SLEEP);
	/*
	 * Copy the credentials.
	 */
	prcredp->pr_euid = tcr->cr_uid;
	prcredp->pr_ruid = tcr->cr_ruid;
	prcredp->pr_suid = tcr->cr_suid;
	prcredp->pr_egid = tcr->cr_gid;
	prcredp->pr_rgid = tcr->cr_rgid;
	prcredp->pr_sgid = tcr->cr_sgid;
	prcredp->pr_ngroups = n;

	if (n == 0)
		prcredp->pr_groups[0] = 0;
	else for (i = 0; i < n; i++)
		prcredp->pr_groups[i] = tcr->cr_groups[i];
	crfree(tcr);

	start = (char *)prcredp + uiop->uio_offset;
	count = size - uiop->uio_offset;
	error = uiomove(start, count, UIO_READ, uiop);
	kmem_free(prcredp, size);
	return error;
}


/*
 * int prreadsigact(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 *	Read the file of signal dispositions.
 *
 * Calling/Exit State:
 *	VOP_RDWRLOCK must have been applied before entry.
 */
/* ARGSUSED */
STATIC int
prreadsigact(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	prcommon_t *prcp = VTOP(vp)->pr_common;
	proc_t *p;
	struct sigaction sigact[MAXSIG];
	char *start;
	int count;

	if (!pr_p_mutex(prcp))
		return ENOENT;
	p = prcp->prc_proc;
	ASSERT(p != NULL);
	if (VTOP(vp)->pr_flags & PR_INVAL) {
		UNLOCK(&p->p_mutex, PLBASE);
		return EBADF;
	}
	if (uiop->uio_offset >= sizeof sigact) {
		UNLOCK(&p->p_mutex, PLBASE);
		return 0;
	}
	prgetsigact(p, sigact);
	UNLOCK(&p->p_mutex, PLBASE);

	start = (char *)sigact + uiop->uio_offset;
	count = sizeof sigact - uiop->uio_offset;
	return uiomove(start, count, UIO_READ, uiop);
}


/*
 * int prreadlwpstatus(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 *	Read the LWP status file.
 *
 * Calling/Exit State:
 *	VOP_RDWRLOCK must have been applied before entry.
 */
/* ARGSUSED */
STATIC int
prreadlwpstatus(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	prnode_t *pnp = VTOP(vp);
	prcommon_t *prcp = pnp->pr_common;
	lwpstatus_t ls;
	char *start;
	int count;
	int error;

	ASSERT(prcp->prc_flags & PRC_LWP);
	if (!pr_p_rdwr(prcp, B_FALSE))
		return ENOENT;
	if (pnp->pr_flags & PR_INVAL) {
		pr_v_rdwr(prcp);
		return EBADF;
	}
	if (uiop->uio_offset >= sizeof(lwpstatus_t)) {	/* EOF */
		pr_v_rdwr(prcp);
		return 0;
	}

	error = prgetlwpstatus(prcp->prc_lwp, &ls);

	pr_v_rdwr(prcp);

	if (error)
		return error;

	start = (char *)&ls + uiop->uio_offset;
	count = sizeof(ls) - uiop->uio_offset;
	return uiomove(start, count, UIO_READ, uiop);
}


/*
 * int prreadlwpsinfo(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 *	Read the file of LWP ps(1) information.
 *
 * Calling/Exit State:
 *	VOP_RDWRLOCK must have been applied before entry.
 */
/* ARGSUSED */
STATIC int
prreadlwpsinfo(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	prcommon_t *prcp = VTOP(vp)->pr_common;
	proc_t *p;
	lwp_t *lwp;
	struct lwpsinfo lsi;
	char *start;
	int count;

	bzero(&lsi, sizeof lsi);

	if (!pr_p_mutex(prcp))
		return ENOENT;
	p = prcp->prc_proc;
	ASSERT(p != NULL);
	if (VTOP(vp)->pr_flags & PR_INVAL) {
		UNLOCK(&p->p_mutex, PLBASE);
		return EBADF;
	}
	lwp = prcp->prc_lwp;
	ASSERT(lwp != NULL);
	if (uiop->uio_offset >= sizeof lsi) {	/* EOF */
		UNLOCK(&p->p_mutex, PLBASE);
		return 0;
	}

	prgetlwpsinfo(lwp, &lsi);
	ASSERT(KS_HOLD0LOCKS());

	start = (char *)&lsi + uiop->uio_offset;
	count = sizeof lsi - uiop->uio_offset;
	return uiomove(start, count, UIO_READ, uiop);
}
