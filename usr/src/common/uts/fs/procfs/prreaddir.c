/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/procfs/prreaddir.c	1.14"

#include <util/types.h>
#include <proc/cred.h>
#include <util/debug.h>
#include <fs/dirent.h>
#include <svc/errno.h>
#include <util/ksynch.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <proc/proc.h>
#include <util/sysmacros.h>
#include <io/uio.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/procfs/procfs.h>
#include <fs/procfs/prdata.h>
#include <proc/pid.h>
#include <svc/systm.h>
#include <util/cmn_err.h>

#define	dpnext(dp)	((dirent_t *)(void *)((char *)(dp) + (dp)->d_reclen))
#define	PRDIRECTSIZE	64
#define	PRDIRENTSIZE	64

STATIC int prread_files_dir(vnode_t *, uio_t *, int *, prntable_t *, int);
STATIC int prread_id_dir(vnode_t *, uio_t *, cred_t *, int *,
			 int (*)(), prnodetype_t);
STATIC dirent_t *praddtodir(dirent_t *, char *, ino_t, off_t);
STATIC int prreadobjectdir(vnode_t *, uio_t *, cred_t *, int *eofp);

/*
 * int prreaddir(vp, uiop, cr, eofp)
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
prreaddir(vnode_t *vp, uio_t *uiop, cred_t *cr, int *eofp)
{
	int error;

	switch (VTOP(vp)->pr_type) {
	case PR_PIDDIR:
		error = prread_files_dir(vp, uiop, eofp, pdtable, npdent);
		break;
	case PR_LWPIDDIR:
		error = prread_files_dir(vp, uiop, eofp, ldtable, nldent);
		break;
	case PR_LWPDIR:
		error = prread_id_dir(vp, uiop, cr, eofp,
		  lwpid_next_entries, PR_LWPIDDIR);
		break;
	case PR_PROCDIR:
		error = prread_id_dir(vp, uiop, cr, eofp,
		  pid_next_entries, PR_PIDDIR);
		break;
	case PR_OBJECTDIR:
		error = prreadobjectdir(vp, uiop, cr, eofp);
		break;
	default:
		error = ENOSYS;
		break;
	}

	return error;
}

#define	PRENTSIZE	16	/* Arbitrary */
#define	NENT		32

/*
 * int prread_files_dir(vnode_t *vp, uio_t *uiop, int *eofp,
 *			prntable_t *prtable, int prnent)
 *	Read a directory containing a fixed set of file names
 *	(/proc/pid or /proc/pid/lwp/lwpid).  prtable points to a table
 *	of file names and other information; prnent is the number of
 *	entries in the table.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC int
prread_files_dir(vnode_t *vp, uio_t *uiop, int *eofp,
		 prntable_t *prtable, int prnent)
{
	register int i;
	union {
		struct dirent d;	/* For alignment. */
		char bytes[NENT*16];	/* magic: big enough for NENT dirent structs */
	} dirbuf;
	char *start;
	struct dirent *dp, *dirend;
	struct prnode *pnp = VTOP(vp);
	struct prcommon *prcp = pnp->pr_common;
	int error, zombie, nas;
	unsigned int count, ncount;
	ino_t ino;
	off_t off;

	if (uiop->uio_offset < 0 || uiop->uio_resid <= 0 ||
	    uiop->uio_offset % PRENTSIZE != 0)
		return EINVAL;

	dp = &dirbuf.d;
	dp = praddtodir(dp, ".", pnp->pr_ino, PRENTSIZE);
	dp = praddtodir(dp, "..", VTOP(pnp->pr_parent)->pr_ino, 2*PRENTSIZE);

	if (!pr_p_mutex(prcp))
		/* The target process no longer exists. */
		return ENOENT;
	zombie = ((prcp->prc_flags & PRC_DESTROY) != 0);
	nas = ((prcp->prc_flags & PRC_SYS) != 0 ||
	       prcp->prc_proc->p_as == NULL);
	UNLOCK(&prcp->prc_proc->p_mutex, PLBASE);

	for (i = 0; i < prnent; i++) {
		if ((zombie && !prtable[i].prn_zvisible) ||
		    (nas && !prtable[i].prn_nasvisible))
			continue;
		ino = prino(prcp->prc_slot, prcp->prc_lwpid,
			    prtable[i].prn_ctype);
		off = (i+3)*PRENTSIZE;
		dp = praddtodir(dp, prtable[i].prn_comp, ino, off);
	}
	dirend = dp;
	/*
	 * Determine the user-requested portion and copy out.  Be sure
	 * not to return any partial dirent structures.  First locate
	 * the starting point for the copyout.
	 */
	off = 0;
	dp = &dirbuf.d;
	while (dp < dirend && off < uiop->uio_offset) {
		off = dp->d_off;
		dp = dpnext(dp);
	}
	start = (char *)dp;
	/*
         * Compute the amount to be copied out.
	 */
	count = ncount = 0;
	while (dp < dirend && (ncount += dp->d_reclen) <= uiop->uio_resid) {
		count = ncount;
		off = dp->d_off;
		dp = dpnext(dp);
	}
	error = 0;
	if (count == 0) {
		if (dp < dirend)	/* Not enough space */
			error = EINVAL;
	} else if (uiomove(start, count, UIO_READ, uiop))
		error = EFAULT;

	if (error == 0) {
		uiop->uio_offset = off;
		if (eofp)
			*eofp = (dp >= dirend);
	}
	return error;
}

/*
 * int prread_id_dir(vnode_t *vp, uio_t *uiop, cred_t *cr, int *eofp,
 *   int efunc(), prnodetype_t ptype)
 *	Read a directory containing ids (/proc, or /proc/pid/lwp).
 *	efunc is a pointer to a function that generates the "idbuf"
 *	structures used in constructing the directory entries.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/* ARGSUSED */
STATIC int
prread_id_dir(vnode_t *vp, uio_t *uiop, cred_t *cr, int *eofp,
	      int efunc(), prnodetype_t ptype)
{
	struct prnode *pnp = VTOP(vp);
	struct prcommon *prcp = pnp->pr_common;
	int oresid, start, next, eof = 0, error = 0, slot;
	register int i, n;
	unsigned int count, ncount;
	off_t off;
	ino_t ino;
	idbuf_t idbuf[NENT];	
	union {
		struct dirent d;	/* For alignment. */
		char bytes[NENT*16];	/* magic: big enough for NENT dirent structs */
	} dirbuf;
	char name[20];
	struct dirent *dp, *dirend;

	if (uiop->uio_offset < 0 || uiop->uio_resid <= 0 ||
	    (uiop->uio_offset % PRENTSIZE) != 0)
		return EINVAL;
	oresid = uiop->uio_resid;

	while (uiop->uio_resid > 0) {
		dp = &dirbuf.d;
		if (uiop->uio_offset < 2*PRENTSIZE) {
			if (uiop->uio_offset == 0)
				dp = praddtodir(dp, ".", pnp->pr_ino,
						PRENTSIZE);
			dp = praddtodir(dp, "..", VTOP(pnp->pr_parent)->pr_ino,
					2*PRENTSIZE);
			uiop->uio_offset = 2*PRENTSIZE;
		}
		start = (uiop->uio_offset-2*PRENTSIZE)/PRENTSIZE;
		if ((n = (*efunc)(prcp, start, NENT, idbuf, &next)) == 0) {
			eof = 1;
			break;
		}
		/*
		 * Convert a block of (id, node-id) pairs into dirent
		 * structures.
		 */
		for (i = 0; i < n; i++) {
			ino = (idbuf[i].id_nodeid << 5) | ptype + 2;
			slot = idbuf[i].id_slot;
			numtos((u_long)idbuf[i].id_id, name);
			dp = praddtodir(dp, name, ino, (slot+3)*PRENTSIZE);
		}
		dirend = dp;
		/*
		 * Compute number of bytes to transfer.  Don't return
		 * any partial dirent structures.
		 */
		count = ncount = 0;
		dp = &dirbuf.d;
		while (dp < dirend &&
		       (ncount += dp->d_reclen) <= uiop->uio_resid) {
			count = ncount;
			off = dp->d_off;
			dp = dpnext(dp);
		}
		if (count == 0) {
			if (oresid == uiop->uio_resid)
				/* Not enough space */
				error = EINVAL;
			break;
		}
		if (uiomove(&dirbuf, count, UIO_READ, uiop)) {
			error = EFAULT;
			break;
		}
		uiop->uio_offset = off;
	}
	if (error == 0 && eofp)
		*eofp = eof;
	return error;
}	

/*
 * int prreadobjectdir(vnode_t *vp, uio_t *uiop, cred_t *cr, int *eofp)
 *	Read the /proc/pid/object directory.  Construct names
 *	corresponding to the mapped objects in the target process
 *	address space.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/* ARGSUSED */
STATIC int
prreadobjectdir(vnode_t *vp, uio_t *uiop, cred_t *cr, int *eofp)
{
	prnode_t *pnp = VTOP(vp);
	prcommon_t *prcp = pnp->pr_common;
	proc_t *p;
	vnode_t **vpp;
	register struct as *as;
	vattr_t vattr;
	char name[64], *start;
	int i, nsegs, nvp, error, dbsize;
	u_int count, ncount;
	ino_t ino;
	off_t off, maxoff;
	dirent_t *dp, *dirbuf, *dirend;

	if (uiop->uio_offset < 0 || uiop->uio_resid <= 0 ||
	    (uiop->uio_offset % PRDIRECTSIZE) != 0)
		return EINVAL;

	if (pr_p_rdwr(prcp, B_FALSE) == B_FALSE) {
		/*
		 * Process is gone; show no entries in the directory.
		 */
		*eofp = 1;
		return 0;
	}
	p = prcp->prc_proc;
	if ((as = p->p_as) == NULL) {
		pr_v_rdwr(prcp);
		*eofp = 1;
		return 0;
	}
	/*
	 * Lock the address space, find all the mapped vnodes, and
	 * construct directory entries for them.
	 */
	as_rdlock(as);
	/*
	 * Determine the number of segments, which is an upper bound
	 * on the number of mapped vnodes.  Allocate space for that
	 * many vnode pointers.
	 */
	nsegs = prcountsegs(as);
	vpp = kmem_alloc(nsegs*sizeof(vnode_t *), KM_SLEEP);
	/*
	 * Construct a list of unique vnode pointers appearing in
	 * the address space.
	 */
	nvp = prvpsegs(p, vpp);
	as_unlock(as);

	/*
	 * Allocate space for a buffer to hold the directory entries
	 * as we construct them.  We need enough space for the names
	 * derived from the vnodes, plus "a.out", plus "." and "..".
	 */
	dbsize = (nvp+3) * PRDIRENTSIZE;
	dirbuf = kmem_alloc(dbsize, KM_SLEEP);
	dp = dirbuf;
	dp = praddtodir(dp, ".", pnp->pr_ino, PRDIRECTSIZE);
	dp = praddtodir(dp, "..", VTOP(pnp->pr_parent)->pr_ino, 2*PRDIRECTSIZE);
	vattr.va_mask = AT_NODEID;
	ino = (VOP_GETATTR(vpp[0], &vattr, 0, cr) == 0) ? vattr.va_nodeid : 0;
	dp = praddtodir(dp, "a.out", ino, 3*PRDIRECTSIZE);

	/*
	 * Walk the list of vnode pointers, constructing names and
	 * directory entries.
	 */
	for (i = 0; i < nvp; i++) {
		prmapname(vpp[i], name, &ino, cr);
		dp = praddtodir(dp, name, ino, (i+4)*PRDIRECTSIZE);
	}
	dirend = dp;
	maxoff = (nvp+3)*PRDIRECTSIZE;
	/*
	 * We have all the data now, so release remaining locks.
	 */
	pr_v_rdwr(prcp);
	kmem_free(vpp, nsegs*sizeof(vnode_t *));

	error = 0;
	if (uiop->uio_offset >= maxoff) {	/* EOF */
		*eofp = 1;
		goto out;
	}
	/*
	 * Determine the user-requested portion and copy out.  Be sure
	 * not to return any partial dirent structures.  First locate
	 * the starting point for the copyout.
	 */
	off = 0;
	dp = dirbuf;
	while (dp < dirend && off < uiop->uio_offset) {
		off = dp->d_off;
		dp = dpnext(dp);
	}
	start = (char *)dp;
	/*
	 * Compute the amount to be copied out.
	 */
	count = ncount = 0;
	while (dp < dirend && (ncount += dp->d_reclen) <= uiop->uio_resid) {
		count = ncount;
		off = dp->d_off;
		dp = dpnext(dp);
	}
	if (count == 0)		/* Not enough space */
		error = EINVAL;
	else if (uiomove(start, count, UIO_READ, uiop))
		error = EFAULT;
	else {
		uiop->uio_offset = off;
		*eofp = (dp >= dirend);
	}
out:
	kmem_free(dirbuf, dbsize);
	return error;
}

#define	DSIZE		((char *)((struct dirent *)0)->d_name - (char *)0)

/*
 * dirent_t *praddtodir(dirent_t *dp, char *name, ino_t ino, off_t off)
 *	Construct a directory entry with the given parameters and return
 * 	a pointer to the next one.  The caller must have allocated
 *	sufficient space to hold the entry.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC dirent_t *
praddtodir(dirent_t *dp, char *name, ino_t ino, off_t off)
{
	register int reclen;

	dp->d_ino = ino;
	dp->d_off = off;
	reclen = roundup(DSIZE+strlen(name)+1, sizeof(int));
	strncpy(dp->d_name, name, reclen-DSIZE);
	dp->d_reclen = (short) reclen;
	return dpnext(dp);
}
