/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/mac/vnmac.c	1.10"
#ident	"$Header: $"

/*
 * This file contains FILE SYSTEM relevant routines for MAC.
 */

#include <acc/audit/audit.h>
#include <acc/audit/auditrec.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/pathname.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/dnlc.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/ksynch.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/types.h>

STATIC int	cdevstat(struct vnode *, int, struct devstat *, struct cred *);

/*
 *
 * int mac_rootlid(lid_t lid)
 * 	assigns a LID to the root vnode of the root file system.
 * 	and sets also the floor level of the root file system
 * 	This routine is called from secsys().
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, mac_rootlid returns zero.  On failure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 * Remarks:
 *	This is a one shot call, i.e., once assigned, the level
 *	of the root vnode can never be changed.
 */

int
mac_rootlid(lid_t lid)
{

	ASSERT(getpl() == PLBASE);

	if (mac_valid(lid))
		return EINVAL;
	
	if (!rootdir->v_lid) {
		rootdir->v_lid = lid;
		rootdir->v_vfsp->vfs_macfloor = lid;
	} else
		return EBUSY;
	return 0;
}

/*
 *
 * int
 * mld_deflect(vnode_t *vp, char *eff_dirname, vnode_t **tvpp, pathname_t *pnp,
 *	       int lookup_flags, vnode_t *rdir, cred_t *crp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    Function mld_deflect() performs the deflection to the appropriate
 *    effective directory.  If the effective directory does not
 *    exist, it is created.  The P_MACWRITE privilege allows the
 *    creation of an effective directory at a level which is
 *    different from the MLD.  Note that search access to the MLD
 *    itself has already been done.  Before allowing the deflection,
 *    the effective directory must be a directory and must be at the
 *    level of the calling process.
 *
 */
int
mld_deflect(vnode_t *vp, char *eff_dirname, vnode_t **tvpp, pathname_t *pnp,
	    int lookup_flags, vnode_t *rdir, cred_t *crp)
{
	vattr_t	*vattr;		/* vattr to get mode of MLD */
	cred_t	*lcred;		/* cred with MAC override */
	int	error;
	vnode_t	*tvp = *tvpp;

	vattr = kmem_alloc(sizeof(vattr_t), KM_SLEEP);

retry:
	if ((error = VOP_LOOKUP(vp, eff_dirname, &tvp, pnp, lookup_flags, rdir,
	     crp)) == ENOENT &&
	    (error = VOP_GETATTR(vp, vattr, 0, crp)) == 0) {
		lid_t ceiling = vp->v_vfsp->vfs_macceiling;
		lid_t floor = vp->v_vfsp->vfs_macfloor;

		if (MAC_ACCESS(MACEQUAL, floor, crp->cr_lid)
		    &&  MAC_ACCESS(MACEQUAL, ceiling, crp->cr_lid)
		    &&  (MAC_ACCESS(MACDOM, ceiling, crp->cr_lid)
			 || MAC_ACCESS(MACDOM, crp->cr_lid, floor))
		    &&  pm_denied(crp, P_FSYSRANGE))
			error = EACCES;
		else {
			vattr->va_mask = AT_TYPE | AT_MODE;
			vattr->va_type = VDIR;
			vattr->va_mode &= MODEMASK;
			lcred = crdup(crp);
			lcred->cr_wkgpriv |= pm_privbit(P_MACWRITE);
			error = VOP_MKDIR(vp, eff_dirname, vattr, &tvp,
					  lcred);
			crfree(lcred);
			if (error == 0)
				dnlc_enter(vp, eff_dirname, NULL, tvp, NOCRED);
			else if (error == EEXIST)
				/*
				 * Some other context, doing an mld_deflect to
				 * the same effective dir at the same time,
				 * beat us to creating it.  Try looking it up
				 * again.
				 */
				goto retry;
		}
	}
	if (error == 0) {
		ASSERT(tvp->v_macflag & VMAC_SUPPORT);
		if (tvp->v_type != VDIR)
			error = ENOTDIR;
		else if (tvp->v_lid != crp->cr_lid)
			error = EINVAL;
		if (error)
			VN_RELE(tvp);
	}
	*tvpp = tvp;

	kmem_free(vattr, sizeof(vattr_t));

	return error;
}



struct mkmlda {
	char *dname;
	int dmode;
};
/*
 * int mkmld(struct mkmlda *uap, rval_t *rvp)
 * 	Make a Multi-Level Directory (MLD).
 * 
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, mkmld returns zero.  On failure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 */

/* ARGSUSED */
int
mkmld(struct mkmlda *uap, rval_t *rvp)
{
	vnode_t *vp;
	struct vattr vattr;
	int error;

	ASSERT(getpl() == PLBASE);

	/*
	 * We must have P_MULTIDIR privilege to execute this system call
	 */
	if (pm_denied(u.u_lwpp->l_cred, P_MULTIDIR))
		error = EPERM;
	else {
		/*
		 * same code as for mkdir(2), except specify
		 * CRMKMLD in call to vn_create.
		 */
		vattr.va_type = VDIR;
		vattr.va_mode = (uap->dmode & PERMMASK) & ~u.u_procp->p_cmask;
		vattr.va_mask = AT_TYPE|AT_MODE;
		if ((error = vn_create(uap->dname, UIO_USERSPACE, &vattr,
		     EXCL, 0, &vp, CRMKMLD)) == 0)
			VN_RELE(vp);
	}
	return error;
}


/*
 *
 * int mac_vaccess(struct vnode *vp, int mode, cred_t *credp)
 *	performs MAC checks on the specified ``lid'', based on the
 *	mode of operation and the specified credentials ``credp''.
 *	The MAC override privileges are performed if MAC checks fail.
 *
 * Calling/Exit State:
 *	This function return 0 if subject has access to vnode. Otherwise,
 *	EACCES is returned.   
 *
 * Remarks:
 * 	Note that MAC_VACCESS() has already checked for equality.
 * 	So it appears that mac_vaccess() is doing a redundant equality
 * 	check at the beginning.  However, we'd like to be able at times
 * 	to call this routine without calling MAC_VACCESS() first.
 */

int
mac_vaccess(struct vnode *vp, int mode, cred_t *credp)
{
	/* regardless of mode, if lids equal, succeed */
	if (MAC_ACCESS(MACEQUAL, vp->v_lid, credp->cr_lid) == 0)
		return 0;

	/* if write request, check P_MACWRITE privilege */
	if ((mode & VWRITE) && pm_denied(credp, P_MACWRITE))
		return EACCES;

	/* if read or exec request, check domination and P_MACREAD */
	if ((mode & (VREAD|VEXEC)) &&  mac_liddom(credp->cr_lid, vp->v_lid)
	    &&  pm_denied(credp, P_MACREAD))
		return EACCES;

	return 0;
}


/*
 * int mac_checks(vnode_t *dvp, cred_t *cr)
 *	Checks that MAC allow creation of a new directory entry in
 *	dvp.  This includes checking that the lwp's level is equal to
 *	the directories level, and that the level of the new directory
 *	entry (which will be the lwp's level) is within the file
 *	system range.  Override privileges are considered when access
 *	fails.
 *
 * Calling/Exit State:
 *	This function return 0 if subject can write the directory
 *	entry.  Otherwise, the errno is returned.
 *
 */
int
mac_checks(struct vnode *dvp, cred_t *cr)
{
	int error;
	lid_t ceiling;

	/* Make sure user has MAC write access to directory. */
	if ((error = MAC_VACCESS(dvp, VWRITE, cr)) != 0)
		return error;
	/*
	 * Level of object is that of the calling process.
	 * Make sure that this level is within the fs range.
	 * The MAC equality checks are added for performance,
	 * i.e., if the level is that of the floor or ceiling,
	 * there is no need to do domination checks.
	 */
	ceiling = dvp->v_vfsp->vfs_macceiling;
	if (MAC_ACCESS(MACEQUAL, dvp->v_vfsp->vfs_macfloor, cr->cr_lid)
	    && MAC_ACCESS(MACEQUAL, ceiling, cr->cr_lid)
	    && (MAC_ACCESS(MACDOM, ceiling, cr->cr_lid)
		|| MAC_ACCESS(MACDOM, cr->cr_lid, dvp->v_vfsp->vfs_macfloor))
	    &&  pm_denied(cr, P_FSYSRANGE)) {
		return EACCES;
	}
	return 0;
}


struct lvlvfsa {
	char	*fname;
	int 	cmd;
	lid_t *hilevelp;
};
/*
 *
 * int lvlvfs(struct lvlvfsa *uap, rval_t *rvp)
 * 	Get or Set a mounted file system MAC ceiling.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, secsys returns zero.  On failure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 */
/* ARGSUSED */
int
lvlvfs(struct lvlvfsa *uap, rval_t *rvp)
{
	int err;
	vnode_t *vp;
	lid_t lid;

	ASSERT(getpl() == PLBASE);

	if (pm_denied(u.u_lwpp->l_cred, P_MOUNT)) 
		return EPERM;

	if (err = lookupname(uap->fname, UIO_USERSPACE, FOLLOW, NULLVPP, &vp))
		return err;

	switch(uap->cmd) {
	case MAC_GET:
		if (copyout((caddr_t)&vp->v_vfsp->vfs_macceiling,
		    (caddr_t)uap->hilevelp, sizeof(lid_t)))
			err = EFAULT;
		break;

	case MAC_SET:
		if (copyin((caddr_t)uap->hilevelp, (caddr_t)&lid, 
		    sizeof(lid_t))) {
			err = EFAULT;
		} else if (mac_valid(lid)) {
			err = EINVAL;
		} else if (MAC_ACCESS(MACDOM, lid, vp->v_vfsp->vfs_macfloor)) {
			/* 
		 	 * check that the new ceiling dominate the current
		 	 * floor of the file system
		 	 */
			err = ERANGE;
		} else 
			err = VFS_SETCEILING(vp->v_vfsp, lid);
		break;

	default:
		err =  EINVAL;
		break;
	}
	VN_RELE(vp);
	return err;
}


struct devstata {
	char		*pathp;
	int  		cmd;
	struct devstat 	*bufp;
};
/*
 *                                                 
 * int devstat(struct devstata	*uap, rval_t *rvp)
 *	This system call Get or Set the security attributes of a 
 *	block or character special file.
 *                                                 
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, devstat returns zero.  On failure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 */
/* ARGSUSED */
int
devstat(struct devstata	*uap, rval_t *rvp)
{

	register int error;
	struct vnode *vp;
	struct cred *crp;

	ASSERT(getpl() == PLBASE);

	crp = u.u_lwpp->l_cred;
	if (error = lookupname(uap->pathp, UIO_USERSPACE, FOLLOW, 
	    (struct vnode **)0, &vp))
		return error;
	error = cdevstat(vp, uap->cmd, uap->bufp, crp);
	VN_RELE(vp);
	return error;
}


struct fdevstata {
	int	fildes;
	int 	cmd;
	struct devstat	*bufp;
};
/*
 *                                                 
 * int fdevstat(struct devstata	*uap, rval_t *rvp)
 *	This system call Get or Set the security attributes of a 
 *	block or character special file.
 *                                                 
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, devstat returns zero.  On failure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 */
/* ARGSUSED */
int
fdevstat(struct fdevstata *uap, rval_t 	*rvp)
{

	register int error;
	struct cred *crp;
	file_t *fp;
	vnode_t *vp;

	crp = u.u_lwpp->l_cred;
	if (error = getf(uap->fildes, &fp))
		return error;
	vp = fp->f_vnode;
	error = cdevstat(fp->f_vnode, uap->cmd, uap->bufp, crp);
	ADT_GETF(vp);
	FTE_RELE(fp);
	return error;
}


/*
 *
 * STATIC int cdevstat(vnode_t *vp, int cmd, struct devstat *ubp, cred_t *crp)	
 *	Set or Get security attributes of a block or character special file.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, cdevstat returns zero.  On failure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 */
STATIC int
cdevstat(struct vnode *vp, int cmd, struct devstat *ubp, struct cred *crp)	
{
	register int error;
	struct devstat devbuf;

	ASSERT(getpl() == PLBASE);

	if ((vp->v_type != VBLK) && (vp->v_type != VCHR))
		return ENODEV;
	if (error = pm_denied(crp, P_DEV))
		return EPERM;

	switch (cmd) {
	case DEV_GET:
		error = VOP_GETDVSTAT(vp, &devbuf, crp);
		if (error == 0
		    && copyout((caddr_t)&devbuf, (caddr_t)ubp, sizeof(devbuf))) 
			error = EFAULT;
		break;

	case DEV_SET:
		if (copyin((caddr_t)ubp, (caddr_t)&devbuf, sizeof(devbuf)))
			return EFAULT;
		error = VOP_SETDVSTAT(vp, &devbuf, crp);
		break;

	default:
		error = EINVAL;
		break;
	}
	return error;
}
