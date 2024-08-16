/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/xnamfs/xsem.c	1.15"

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/*
 * THIS FILE CONTAINS CODE WHICH IS DESIGNED TO BE
 * PORTABLE BETWEEN DIFFERENT MACHINE ARCHITECHTURES
 * AND CONFIGURATIONS. IT SHOULD NOT REQUIRE ANY
 * MODIFICATIONS WHEN ADAPTING XENIX TO NEW HARDWARE.
 */


/*
 *  XENIX Semaphores are xnamnodes corresponding to vnodes
 *  of the special file type VXNAM.
 *  Semaphore xnamnodes contain the current count (x_scount) of the
 *  semaphore and pointers to the head and tail of the list of waiters for
 *  the semaphore (x_headw and x_tailw).
 *  When a process must wait for the resource, it puts its file
 *  structure on the waiting list. By convention, the first element on the
 *  waiting list is the one that currently "owns" the semaphore; i.e., it is
 *  the one using the resource governed by the semaphore.
 */


#include <util/types.h>
#include <util/sysmacros.h>
#include <util/param.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <proc/signal.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <util/metrics.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <fs/file.h>
#include <fs/pathname.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <fs/xnamfs/xnamnode.h>
#include <fs/xnamfs/xnamhier.h>
#include <util/var.h>
#include <io/conf.h>
#include <fs/fstyp.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <io/uio.h>
#include <acc/mac/mac.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#endif

#define SERROR  01              /* process controlling a sem terminated */

/*
 * Macros to lock/unlock the XENIX Semaphore mutex
 */
#define	XSEM_LOCK(xp)		LOCK(&(xp)->x_sem->xsem_mutex, FS_XSEMPL)
#define	XSEM_UNLOCK(xp, pl)	UNLOCK(&(xp)->x_sem->xsem_mutex, (pl))

/*
 * Macros to lock/unlock the XENIX Semaphore freelist
 */
#define XSFREE_LOCK()		LOCK(&xs_freelist_lock, FS_XFREEPL);
#define XSFREE_UNLOCK(pl)	UNLOCK(&xs_freelist_lock, (pl));

/*
 * XENIX Semaphore free list and lock
 */
static struct	xsem *xs_freelist;
lock_t		xs_freelist_lock;
lkinfo_t	xs_freelist_lkinfo;
LKINFO_DECL(xs_freelist_lkinfo, "FS:XNAMFS:xs_freelist lock", 0);

/*
 * XENIX Semaphore mutex lockinfo structure per semaphore
 */
LKINFO_DECL(xsem_lkinfo, "FS:XNAMFS:xsem_mutex lock per semaphore", 0);

extern struct	xsem xsem[]; 	/* XENIX semaphores */
extern int	nxsem;
extern void	xnammark(xnamnode_t *, int);
extern lkinfo_t	f_mutex_lkinfo;
extern fspin_t	file_list_mutex;


/*
 * int
 * xsem_alloc(xnamnode_t *xp)
 *	Allocates a XENIX semaphore structure.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	The semaphore free list lock is held while a new structure
 *	is allocated.
 */
STATIC int
xsem_alloc(xnamnode_t *xp)
{
	xsem_t	*pxsem;
	pl_t	pl;

	/*
	 * Check to see if a pointer exists to semaphores
	 */
	if (xp->x_sem)
		return (0);

	pl = XSFREE_LOCK();
	pxsem = xs_freelist;
	if (pxsem == NULL) {
		/*
		 *+ A table overflow occurred on the XENIX
		 *+ semaphore free list.
		 */
		cmn_err(CE_NOTE, "XENIX semaphore table overflow\n");
		XSFREE_UNLOCK(pl);
		return (ENFILE);
	}
	xs_freelist = (struct xsem *)pxsem->x_headw;
	xp->x_sem = pxsem;
	XSFREE_UNLOCK(pl);
	LOCK_INIT(&xp->x_sem->xsem_mutex, FS_XSEMHIER, FS_XSEMPL,
		  &xsem_lkinfo, KM_SLEEP);
	return 0;
}

/*
 * void
 * xsem_unalloc(xnamnode_t *xp);
 *	Puts a XENIX semphore structure back on freelist.
 *
 * Calling/Exit State:
 *      No locks are held on entry or exit.
 *
 * Description:
 *	Called from xnam_inactive(). The semaphore free list lock
 *	is held while the structure is put on the freelist.
 */
void
xsem_unalloc(xnamnode_t *xp)
{
	pl_t	pl;

	pl = XSFREE_LOCK();
	xp->x_sem->x_headw = (struct file *)xs_freelist;
	xs_freelist = xp->x_sem;
	XSFREE_UNLOCK(pl);

	LOCK_DEINIT(&xp->x_sem->xsem_mutex);
	xp->x_sem = NULL;
}

struct creatsema {
	char	*sem_name;	/* path name specifying the semaphore */
	int	mode;		/* protection of file */
};

/*
 * int
 * creatsem(struct creatsema *uap, rval_t *rvp)
 *	Creates an instance of a XENIX semaphore.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Creates an instance of a semaphore named sem_name.
 *	Semaphores are files of 0 length - the file name space
 *	is used to provide unique identifiers for semaphores.
 *	Fails if a file named sem_name already exists and has been
 *	opened by at least one process.
 */
int
creatsem(struct creatsema *uap, rval_t *rvp)
{
	vnode_t	   *vp;
	xnamnode_t *xp;
	file_t	   *fp;
	vattr_t	   vattr;
	int	   error;
	int	   fd;
	pl_t	   pl;

	ASSERT(KS_HOLD0LOCKS());
	error = lookupname(uap->sem_name, UIO_USERSPACE, FOLLOW, NULLVPP, &vp);
	if (error) {
		if (error != ENOENT)
			return (error);
		vattr.va_type = VXNAM;
		vattr.va_mode = (uap->mode & MODEMASK) & ~u.u_procp->p_cmask;
		vattr.va_mask = AT_TYPE|AT_MODE;
		vattr.va_rdev = XNAM_SEM;
		vattr.va_mask |= AT_RDEV;
		error = vn_create(uap->sem_name, UIO_USERSPACE, &vattr,
				  EXCL, 0, &vp, CRMKNOD);
		if (error) {
			return (error);
		}
	} else {
		error = MAC_VACCESS(vp, VREAD, CRED());
		if (error) {
			VN_RELE(vp);
			return (error);
		}
		if ((vp->v_type != VXNAM) || (vp->v_rdev != XNAM_SEM)) {
			VN_RELE(vp);
			return (ENOTNAM);
		}
		error = VOP_ACCESS(vp, VREAD, 0, CRED());
		if (error) {
			VN_RELE(vp);
			return (error);
		}
	}

	/* must be xnam type vnode */

	if (vp->v_op != &xnam_vnodeops) {
		VN_RELE(vp);
		return (EINVAL);
	}

	VN_LOCK(vp);
	if (vp->v_count != 1) {
		VN_UNLOCK(vp);
		VN_RELE(vp);
		return (EEXIST);
	}
	VN_UNLOCK(vp);

	ASSERT(vp->v_rdev == XNAM_SEM);
	xp = VTOXNAM(vp);
	error = xsem_alloc(xp);
	if (error) {
		VN_RELE(vp);
		return (error);
	}

	xp->x_sem->x_scount = 1;
	xp->x_sem->x_headw = (xp->x_sem->x_tailw = (struct file *)NULL);
	xp->x_sem->x_eflag = 0;
	SV_INIT(&xp->x_sem->xsem_sv);
	xnammark(xp, XNAMACC);

	/*
	 * Call falloc() with a null vnode pointer so that
	 * the fd-entry is not prematurely activated.
	 */
	error = falloc((vnode_t *)NULL, (uint_t)(uap->mode & FMASK), &fp, &fd);
	if (error) {
		VN_RELE(vp);
		return (error);
	}

	pl = FTE_LOCK(fp);
	fp->f_vnode = vp;	/* Must set before calling setf() */
	FTE_UNLOCK(fp, pl);
	setf(fd, fp);		/* Make fd available for use */

	rvp->r_val1 = fd;
	return (0);
}

struct opensema {
	caddr_t sem_name;       /* path name specifying the semaphore */
};

/*
 * int
 * opensem(struct opensema *uap, rval_t *rvp)
 *	Opens a XENIX semaphore.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Opens a semaphore named sem_name and returns that semaphore's unique
 *	id number. Fails if the semaphore doesn't exist, or if a process that 
 *	controlled the sem terminated without relinquishing control leaving 
 *	the resource governed by the sem in an inconsistent state (ENAVAIL).
 */

int
opensem(struct opensema *uap, rval_t *rvp)
{
	vnode_t	   *vp;
	xnamnode_t *xp;
	file_t	   *fp;
	int	   error;
	int	   fd;
	pl_t	   pl;

	ASSERT(KS_HOLD0LOCKS());
	error = lookupname(uap->sem_name, UIO_USERSPACE, FOLLOW, NULLVPP, &vp);
	if (error) {
		return (error);
	}

	error = MAC_VACCESS(vp, VREAD, CRED());
	if (error) {
		VN_RELE(vp);
		return error;
	}

	if (vp->v_type != VXNAM || vp->v_rdev != XNAM_SEM) {
		VN_RELE(vp);
		return ENOTNAM;
	}

	/* must be associated with an xnamnode */ 
	if (vp->v_op != &xnam_vnodeops) {
		VN_RELE(vp);
		return EINVAL;
	}

	xp = VTOXNAM(vp);
	/*
	 * Make sure x_sem is allocated
	 */
	error = xsem_alloc(xp);
	if (error) {
		VN_RELE(vp);
		return error;
	}

	pl = XSEM_LOCK(xp);
	if (xp->x_sem->x_eflag == SERROR) {
		XSEM_UNLOCK(xp, pl);
		VN_RELE(vp);
		return (ENAVAIL);
	}
	XSEM_UNLOCK(xp, pl);

	error = VOP_ACCESS(vp, VREAD, 0, CRED());
	if (error) {
		VN_RELE(vp);
		return (error);
	}

	/*
	 * Call falloc() with a null vnode pointer so that
	 * the fd-entry is not prematurely activated.
	 */
	error = falloc((vnode_t *)NULL, 0, &fp, &fd);
	if (error) {
		VN_RELE(vp);
		return (error);
	}

	pl = FTE_LOCK(fp);
	fp->f_vnode = vp;	/* Must set before calling setf() */
	FTE_UNLOCK(fp, pl);
	setf(fd, fp);		/* Make fd available for use */

	rvp->r_val1 = fd;
	return (0);
}

struct sigsema {
	int fdes;     /* sem # to signal (special file desc.) */
};

/*
 * int
 * sigsem(struct sigsema *uap, rval_t *rvp)
 *	Signals an LWP waiting on a semaphore that it may proceed
 *	to use the resource governed by a semaphore.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	The semaphore spin lock is held while checking and updating the
 *	current semaphore count. Signals the first waiting LWP on the
 *	semaphore's queue (FIFO order) to be rescheduled for execution.
 */
/*ARGSUSED*/
int
sigsem(struct sigsema *uap, rval_t *rvp)
{
	vnode_t	   *vp;
	xnamnode_t *xp;
	file_t	   *fp;
	int	   error;
	pl_t	   pl;


	ASSERT(KS_HOLD0LOCKS());
	error = getf(uap->fdes, &fp);
	if (error) {
		return (error);
	}

	vp = fp->f_vnode;
	/* must be sem type vnode */
	if ((vp->v_type != VXNAM) || (vp->v_rdev != XNAM_SEM)) {
		error = ENOTNAM;
		goto out;
	}

	if (vp->v_op != &xnam_vnodeops) {
		error = EINVAL;
		goto out;
	}

	/* ensure sem owner is signalling */
	xp = VTOXNAM(vp);
	if (xp->x_sem->x_headw != fp) {
		error = ENAVAIL;
		goto out;
	}

	pl = XSEM_LOCK(xp);
	/* remove self from head of list as owner */
	xp->x_sem->x_headw = fp->f_slnk;

	/* signal 1st waiting process, if there is one */
	if ((xp->x_sem->x_scount)++ < 0 && SV_BLKD(&xp->x_sem->xsem_sv)) {
		XSEM_UNLOCK(xp, pl);
		SV_SIGNAL(&xp->x_sem->xsem_sv, 0);
	} else {
		XSEM_UNLOCK(xp, pl);
	}
	xnammark(xp, XNAMACC);

out:
	FTE_RELE(fp);
	return (error);
}

struct waitsema {
	int fdes;
};

/*
 * int
 * cwaitsem(int fdes, int nowait)
 *	Common code for waitsem and nbwaitsem.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	If the resource is in use by another LWP, waitsem() will block
 *	until it becomes available; nbwaitsem() returns EINVAL instead
 *	of blocking. The file table mutex lock is held while ensuring
 *	that this LWP neither owns the semaphore nor waits for it; this
 *	is to prevent two calls without an intervening sigsem() call
 *	using the same file descriptor. The semaphore spin lock is held
 *	while checking/decrementing the current semaphore count. The
 *	file table mutex lock is held while blocking on the file table
 *	synchronization variable.
 */
STATIC int
cwaitsem(int fdes, int nowait)
{
	vnode_t	   *vp;
	xnamnode_t *xp;
	file_t	   *fp;
	file_t	   *wp;
	int	   error;
	pl_t	   xpl;

	ASSERT(KS_HOLD0LOCKS());
	error = getf(fdes, &fp);
	if (error) {
		return error;
	}
	vp = fp->f_vnode;

	/* must be sem type vnode */
	if (vp->v_type != VXNAM || vp->v_rdev != XNAM_SEM) {
		error = ENOTNAM;
		goto out;
	}

	/* must be associated with an xnamnode */
	if (vp->v_op != &xnam_vnodeops) {
		error = EINVAL;
		goto out;
	}
	xp = VTOXNAM(vp);
	if (xp->x_sem->x_eflag == SERROR) {
		error = ENAVAIL;
		goto out;
	}
	/* 
	 * ensure that this process neither owns the semaphore nor waits
	 * for it (i.e, prevent 2 waitsem calls without intervening sigsem 
	 * by same process using the same file descriptor).
 	 */
	for (wp = xp->x_sem->x_headw; wp != NULL; wp = wp->f_slnk) {
		if (wp == fp) {
			/* instead of pending */
	    		error = EINVAL;
			goto out;
		}
	}
	xpl = XSEM_LOCK(xp);
	if (nowait && xp->x_sem->x_scount <= 0) {
		XSEM_UNLOCK(xp, xpl);
		error = ENAVAIL;
		goto out;
	}
	XSEM_UNLOCK(xp, xpl);
	xnammark(xp, XNAMACC);

	fp->f_slnk = (struct file *)NULL;
	xpl = XSEM_LOCK(xp);
	if (--(xp->x_sem->x_scount) < 0) {   /* sem busy, must wait */
		/* insert at tail of waiter list */
		xp->x_sem->x_tailw->f_slnk = fp;
		xp->x_sem->x_tailw = fp;
		while(fp != xp->x_sem->x_headw) {
			if (SV_WAIT_SIG(&xp->x_sem->xsem_sv, PRIMED,
				&xp->x_sem->xsem_mutex) == B_FALSE) {
				error = EINTR;
				goto out;
			}
			xpl = XSEM_LOCK(xp);

			/* check if process controlling the sem has */
			/* ceased to use it before giving up control*/
			if (xp->x_sem->x_eflag == SERROR) {
				error = ENAVAIL;
				break;
			}
		}
	} else {
		/* insert on list as sem "owner" */
		xp->x_sem->x_headw = (xp->x_sem->x_tailw = fp);
	}
	XSEM_UNLOCK(xp, xpl);
out:
	FTE_RELE(fp);
	return (error);
}

/*
 * int
 * waitsem(struct waitsema *uap, rval_t *rval)
 *	Awaits and checks access to a resource governed by a semaphore.
 *	If resource is in use, block until the resource becomes available.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 */
/*ARGSUSED*/
int
waitsem(struct waitsema *uap, rval_t *rval)
{
	return cwaitsem(uap->fdes, 0);
}
	
/*
 * int
 * nbwaitsem(struct waitsema *uap, rval_t *rval)
 *	Awaits and checks access to a resource governed by a semaphore.
 *	If resource is in use, do not block.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 */
/*ARGSUSED*/
int
nbwaitsem(struct waitsema *uap, rval_t *rval)
{
	return cwaitsem(uap->fdes, 1);
}

/*
 * void
 * closesem(file_t *fp, vnode_t *vp)
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Called from closef() to cleanup in case a terminating process is
 *	the current "owner" of a semaphore protected resource or is waiting
 *	on a semaphore. The f_mutex lock has been dropped at this point
 *	since this is the last reference to the file table entry.
 */

void
closesem(file_t *fp, vnode_t *vp)
{
	file_t	   *lfp;
	file_t	   *tfp;
	xnamnode_t *xp;
	pl_t	   pl;

	/* an assert that this is correct type of file system */
	ASSERT(vp->v_rdev == XNAM_SEM);

#ifdef CC_PARTIAL
        MAC_GIVESVAL(vp->v_op, &xnam_vnodeops);
#endif

	xp = VTOXNAM(vp);
	lfp = (struct file *)NULL;
	for (tfp = xp->x_sem->x_headw; tfp != NULL; lfp=tfp, tfp=tfp->f_slnk) {
		if (tfp == fp) {
			pl = XSEM_LOCK(xp);
	   		if (lfp == NULL) {     /* process is semaphore owner */
				xp->x_sem->x_eflag = SERROR;
				xp->x_sem->x_scount++;
				/* cause all processes waiting to error return */
				for (tfp = tfp->f_slnk; tfp != NULL;
							tfp = tfp->f_slnk) {
					xp->x_sem->x_scount++;
				}
				if (SV_BLKD(&xp->x_sem->xsem_sv)) {
					XSEM_UNLOCK(xp, pl);
					SV_BROADCAST(&xp->x_sem->xsem_sv, 0);
					pl = XSEM_LOCK(xp);
				}
				xp->x_sem->x_headw = (struct file *)NULL;
				XSEM_UNLOCK(xp, pl);
				xnammark(xp, XNAMCHG);
				return;
	    		}
			/* remove self from waiting list */
			xp->x_sem->x_scount++;
			lfp->f_slnk = tfp->f_slnk;
			if (tfp->f_slnk == NULL)
				xp->x_sem->x_tailw = lfp;
			XSEM_UNLOCK(xp, pl);
	    		xnammark(xp, XNAMCHG);
		}
	}
}


/*
 * int
 * xsemfork()
 *	Handles inheritance of XENIX semaphores over forks.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Each child is given its own copy of a file structure
 *	referencing the semaphore.
 */
int
xsemfork(proc_t *cproc)
{
	boolean_t  locked;
	file_t	   *fp;
	file_t	   *ofp;
	int	   i;
	int	   error;
	pl_t	   pl;
	fd_table_t *fdtp;
	fd_table_t *nfdtp;
	fd_entry_t *fdep;

	if (nxsem == 0) {
		return (0);
	}

	error = 0;
	nfdtp = GET_FDT(cproc);
	/*
	 * No need to lock the child's file descriptor table 
	 * here, since is it a copy of the parent's but the 
	 * child is not yet runnable.
	 */
	for (i = 0; i < nfdtp->fdt_sizeused; i++) {
		/*
		 * We don't have to worry about recursive locking of
		 * the file descriptor table in getf, since we locked
		 * the child's table above, and getf will lock the
		 * parent.  The tables match at this point, since
		 * fdtfork() was called just prior to this routine,
		 * and the child process has not run yet.
		 */
		error = getf(i, &ofp);
		if (error == 0 &&
		   ofp->f_vnode->v_type == VXNAM &&
		   ofp->f_vnode->v_rdev == XNAM_SEM) {
			/* if an open semaphore was inherited */
			/* replace the inherited file structure */
			/* with a new one not shared with parent */
			/*
			 * First do the work of "setf(i, NULLFP);"
			 * for the child process.
			 */
			fdep = &nfdtp->fdt_entrytab[i];
			fdep->fd_status = FD_UNUSED;
			/*
	 		 * Call xsem_falloc() to set up the new file
	 		 * table entry.
	 		 */
			error = xsem_falloc(ofp->f_vnode, &fp);
			if (error) {
				FTE_RELE(ofp);
				return (error);
			}
			/*
			 * Do the work of "setf(fd, fp);"
			 * for the child process.
			 */
			fdep->fd_file = fp;
			fdep->fd_status = FD_INUSE;

			unfalloc(ofp);
			VN_HOLD(fp->f_vnode);
		}
		if (!error)
			FTE_RELE(ofp);
	}
	return (error);
}

/*
 * int
 * xsem_falloc()
 *	Allocates and initializes a file table entry
 *	XENIX semaphores over forks.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Duplicates some of the work of falloc().
 *	Since fdtfork() has already been called, the
 *	file descriptor entries are already allocated.
 *	This routine must only allocate a new file table 
 *	entry.  Upon return, xsemfork will install this
 *	entry in the child's file descriptor table instead
 *	of the parent's.  Called only from xsemfork.
 */
int
xsem_falloc(vnode_t *vp, file_t **fpp)
{
	file_t		*fp;
	int		nfd;
	int		error;

	/*
	 * Do the work of
	 * "falloc((vnode_t *)vp, 0, &fp, &fd);"
	 * for the child process.  Note that the
	 * file descriptor entry was already allocated
	 * by fdtfork, and we therefore do not need to
	 * call fdalloc_l() as falloc() does.
	 */
	fp = (file_t *)kmem_zalloc(sizeof(file_t), KM_SLEEP);

	LOCK_INIT(&fp->f_mutex, FILE_HIER, FILE_MINIPL,
		&f_mutex_lkinfo, KM_SLEEP);
	fp->f_flag = 0;			/* redundant because of kmem_zalloc() */
	fp->f_count = 1;
	fp->f_vnode = vp;
	fp->f_offset = 0;		/* redundant because of kmem_zalloc() */
	crhold(u.u_lwpp->l_cred);
	fp->f_cred = u.u_lwpp->l_cred;

	*fpp = fp;

	/* Link the file table entry into the file table entry list. */
	FSPIN_LOCK(&file_list_mutex);
	MET_FILE_INUSE(1);
	FSPIN_UNLOCK(&file_list_mutex);

	return (0);
}

/*
 * void
 * xsem_init()
 *	Initialize XENIX semaphore free list.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */
void
xsem_init()
{
	xsem_t	*pxsem;

	/*
	 * Check to see if XENIX semaphores are configured in
	 */
	if (nxsem <= 0) {
		return;
	}

	LOCK_INIT(&xs_freelist_lock, FS_XFREEHIER, FS_XFREEPL,
		  &xs_freelist_lkinfo, KM_SLEEP);

	/* last one in list has headw set to NULL */
	for (xs_freelist = pxsem = &xsem[0]; pxsem < &xsem[nxsem-1]; pxsem++)
		pxsem->x_headw = (file_t *)(pxsem+1);
}
