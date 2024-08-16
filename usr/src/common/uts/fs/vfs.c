/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/vfs.c	1.59"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/dnlc.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/fs_hier.h>
#include <fs/fstyp.h>
#include <fs/mount.h>
#include <fs/pathname.h>
#include <fs/statfs.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <mem/seg_kvn.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/mod/modfs.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif

/*
 * VFS global data.
 */
vnode_t *rootdir;		/* pointer to root vnode. */

STATIC struct vfs root;
struct vfs *rootvfs = &root;	/* pointer to root vfs; head of VFS list. */

sleep_t vfslist_lock;
LKINFO_DECL(vfslist_lkinfo, "FS:vfslist_lock:vfs list (global)", 0);

/* lkinfo for per vfs spin lock */
LKINFO_DECL(vfs_mutex_lkinfo, "FS:vfs_mutex: vfs spin lock (per vfs)", 0);

STATIC int	cstatvfs(vfs_t *, struct statvfs *);
STATIC int	cstatfs(vfs_t *, struct statfs *, int);

/*
 * System calls.
 */

/*
 * int
 * mount(struct mounta *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    This is the entry point of the mount system call.
 *    "struct mounta" defined in sys/vfs.h.
 */

/* ARGSUSED */
int
mount(struct mounta *uap, rval_t *rvp)
{
	vnode_t *vp = NULL;
	vnode_t *rootvp;	/* file system root vnode (MAC use) */
	vfs_t *vfsp;
	struct vfssw *vswp;
	register int error;
	int remount = 0, ovflags;
	pl_t pl;
	u_int fstype;
	struct module *modp;
	struct modctl *mcp;
	int (*mount)();

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

again:
	/*
	 * Resolve path name of mount point.
	 */
	if (error = lookupname(uap->dir, UIO_USERSPACE, FOLLOW, NULLVPP, &vp))
		return error;
	/*
	 * If a mount is already in progress, fail this mount.
	 * Otherwise, turn on the VMOUNTING bit so that any
	 * attempt to mount on the same mount point or to
	 * remove the mount point would fail.
	 */
	VN_LOCK(vp);
	if (vp->v_flag & VMOUNTING) {
		VN_UNLOCK(vp);
		VN_RELE(vp);
		return (EBUSY);
	} else
		vp->v_flag |= VMOUNTING;
	VN_UNLOCK(vp);

	/*
	 * Lock the vnode so that lookuppn() will not venture into the
	 * covered vnode's subtree.
	 */
	RWSLEEP_WRLOCK(&vp->v_lock, PRIVFS);

	/*
	 * Check the VGONE bit to see if we have lost the
	 * race with dirremove/dirrename. If we did, go
	 * back and try to look up again instead of failing
	 * the mount because what's gone might have been one
	 * of our alias.
	 */
	VN_LOCK(vp);
	vp->v_flag &= ~VMOUNTING;
	if (vp->v_flag & VGONE) {
		vp->v_flag &= ~VGONE;
		VN_UNLOCK(vp);
		RWSLEEP_UNLOCK(&vp->v_lock);
		VN_RELE(vp);
		goto again;
	}

	if (vp->v_flag & VNOMOUNT) {
		VN_UNLOCK(vp);
		error =  EINVAL;
		goto out;
	}
	VN_UNLOCK(vp);

	/*
	 * If the multiple mount restriction is removed in the future,
	 * lookuppn() and traverse() in lookup.c must be modified
	 * as well.
	 */
	if ((vp->v_flag & VROOT) && (!(uap->flags & MS_REMOUNT))) {
		error = EBUSY;
		goto out;
	}

	/*
	 * Backward compatibility: require the user program to
	 * supply a flag indicating a new-style mount, otherwise
	 * assume the fstype of the root file system and zero
	 * values for dataptr and datalen.  MS_FSS indicates an
	 * SVR3 4-argument mount; MS_DATA is the preferred way
	 * and indicates a 6-argument mount.
	 */
	if (uap->flags & (MS_DATA|MS_FSS)) {
		u_int n;
		char fsname[FSTYPSZ];

		/*
		 * we want a user-supplied fstype name here,
		 * but for backward compatibility we have to accept a
		 * number if one is provided.  The heuristic used is to
		 * assume that a "pointer" with a numeric value of less
		 * than 256 is really an int.
		 */
		if ((fstype = (u_int)uap->fstype) < 256) {
			if (fstype == 0 || fstype >= nfstype) {
				error = EINVAL;
				goto out;
			}
			/* Range check done above */
                        vswp = &vfssw[fstype];
		} else if (error = copyinstr(uap->fstype, fsname,
					     FSTYPSZ, &n)) {
			if (error == ENAMETOOLONG)
				error = EINVAL;
			goto out;
		} else {
			vswp = vfs_getvfssw(fsname);
			if (vswp == NULL) {
				error = EINVAL;
				goto out;
			} else
				fstype = vswp - vfssw;
		}
	} else
		fstype = rootvfs->vfs_fstype;

	if ((uap->flags & MS_DATA) == 0) {
		uap->dataptr = NULL;
		uap->datalen = 0;
	}
		
	/*
	 * If this is a remount we don't want to create a new VFS.
	 * Instead we pass the existing one with a remount flag.
	 */
	if (uap->flags & MS_REMOUNT) {
		remount = 1;
		/*
		 * Confirm that the vfsp associated with the mount point
		 * has already been mounted on.
		 */
		if ((vp->v_flag & VROOT) == 0) {
			error = ENOENT;
			goto out;
		}
		/*
		 * Disallow making file systems read-only.  Ignore other flags.
		 */
		if (uap->flags & MS_RDONLY) {
			error = EINVAL;
			goto out;
		}
		vfsp = vp->v_vfsp;
		pl = LOCK(&vfsp->vfs_mutex, FS_VFSPPL);
		ovflags = vfsp->vfs_flag;
		vfsp->vfs_flag |= VFS_REMOUNT;
		vfsp->vfs_flag &= ~VFS_RDONLY;
		UNLOCK(&vfsp->vfs_mutex, pl);
	} else {
		vfsp = (vfs_t *) kmem_zalloc(sizeof(vfs_t), KM_SLEEP);
		VFS_INIT(vfsp, vfssw[fstype].vsw_vfsops, (caddr_t) NULL);
	}
		
	dnlc_purge_vp(vp);
	vfsp->vfs_fstype = fstype;

	/*
	 * If the FS is loadable and is loaded, and this is not a remount,
	 * increase the hold count of the FS type.
	 */
	(void)RW_RDLOCK(&mod_vfssw_lock, PLDLM);
	mount = vfsp->vfs_op->vfs_mount;
	if ((modp = vfssw[fstype].vsw_modp) != NULL && !remount) {
		if (MOD_IS_UNLOADING(modp)) {
			RW_UNLOCK(&mod_vfssw_lock, PLBASE);
			mount = &mod_fs_mount;
		} else {
			RW_UNLOCK(&mod_vfssw_lock, PLDLM);
			MOD_HOLD_L(modp, PLBASE);
		}
	} else
		RW_UNLOCK(&mod_vfssw_lock, PLBASE);

mod_mount_again:
	if (error = (*mount)(vfsp, vp, uap, CRED())) {
		/*
		 * ENOLOAD indicates that the FS is loadable, but
		 * not loaded.
		 */
		if (!remount && error == ENOLOAD) {
			if (modld(mod_fsname(fstype), sys_cred, &mcp, 0) == 0) {
				/*
				 * The vfsops value initialized earlier
				 * become invalid after the FS is loaded.
				 * Set the vfsops to the real one now.
				 */
				vfsp->vfs_op = vswp->vsw_vfsops;
				modp = mcp->mc_modp;
				MOD_HOLD_L(modp, PLBASE);
				mount = vfsp->vfs_op->vfs_mount;
				goto mod_mount_again;
			}
		}
		if (modp != NULL && !remount)
			MOD_RELE(modp);

		if (remount)
			vfsp->vfs_flag = ovflags;
		else {
			LOCK_DEINIT(&vfsp->vfs_mutex);
			kmem_free((caddr_t) vfsp, sizeof(struct vfs));
		}
		RWSLEEP_UNLOCK(&vp->v_lock);
		VN_RELE(vp);
	} else {
		pl=LOCK(&vp->v_vfsp->vfs_mutex,FS_VFSPPL);
		if (remount) {
			vfsp->vfs_flag &= ~VFS_REMOUNT;
			UNLOCK(&vp->v_vfsp->vfs_mutex,pl);
			RWSLEEP_UNLOCK(&vp->v_lock);
			VN_RELE(vp);
		} else {
			UNLOCK(&vp->v_vfsp->vfs_mutex,pl);
			/*
			 * Set the file system root vnode's level to
			 * that of the mount point.
			 * Set also the ceiling level and the floor level
			 * of mounted file system to that of mount point
			 */
			if (VFS_ROOT(vfsp, &rootvp) == 0) {
				rootvp->v_lid = vp->v_lid;
				vfsp->vfs_macceiling = vp->v_lid;
				vfsp->vfs_macfloor = vp->v_lid;
				VN_RELE(rootvp);
			}
			RWSLEEP_UNLOCK(&vp->v_lock);
		}
	}
	return error;

out:
	RWSLEEP_UNLOCK(&vp->v_lock);
	VN_RELE(vp);
	return error;
}

struct umounta {
	char	*pathp;
};

/*
 * int
 * umount(struct umounta *uap, rval_t *rvp)
 *    Unmount a file system.
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    This is the entry point of the unmount system call.
 */

/* ARGSUSED */
int
umount(struct umounta *uap, rval_t *rvp)
{
	vnode_t *fsrootvp, *rootvp;
	vfs_t *vfsp;
	int error;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	/*
	 * Lookup user-supplied name.
	 */
	if (error = lookupname(uap->pathp, UIO_USERSPACE, FOLLOW,
	  NULLVPP, &fsrootvp))
		return error;
	/*
	 * Find the vfs to be unmounted.  The caller may have specified
	 * either the directory mount point (preferred) or else (for a
	 * disk-based file system) the block device which was mounted.
	 * Check to see which it is; if it's the device, search the VFS
	 * list to find the associated vfs entry.
     	 * With the vfsp, find a root vnode and assert
     	 * a reference on the root vnode.
     	 */
	if (fsrootvp->v_flag & VROOT)
		vfsp = fsrootvp->v_vfsp;
	else if (fsrootvp->v_type == VBLK) {
		SLEEP_LOCK(&vfslist_lock, PRIVFS);
		vfsp = vfs_devsearch(fsrootvp->v_rdev);
		if (vfsp != NULL && VFS_ROOT(vfsp, &rootvp) == 0) {
			VN_RELE(fsrootvp);
			fsrootvp = rootvp;
		}
		SLEEP_UNLOCK(&vfslist_lock);
	} else
		vfsp = NULL;

	if (vfsp == NULL) {
		VN_RELE(fsrootvp);
		return EINVAL;
	}
	/*
	 * Perform the unmount.
	 */
	error = dounmount(vfsp, CRED());
	if (error != 0) {
		VN_RELE(fsrootvp);
	}
	return error;
}

/*
 * int
 * dounmount(struct vfs *vfsp, register cred_t *cr)
 *    Finish the rest of the unmount work.
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    Do the bulk of the work for unmount.
 */
int
dounmount(vfs_t *vfsp, cred_t *cr)
{
	vnode_t *coveredvp;
	int error;

	/*
	 * Block traversals into the file system being removed
	 * We TRYLOCK the coveredvp here. This code can only
	 * race with "lookup". If TRYLOCK fails, that means
	 * someone is in the process of traversing the mount
	 * point and establishing new references to files in
	 * this file system. Since we are going to catch that
	 * and fail "unmount" later in xxx_iflush anyways, we
	 * might as well fail it here and save ourselves some
	 * work.
	 */
	/*
	 * Get covered vnode.
	 */
	coveredvp = vfsp->vfs_vnodecovered;

	if ((coveredvp == NULLVP) ||
	     (RWSLEEP_TRYWRLOCK(&coveredvp->v_lock) != B_TRUE)) {
		return EBUSY;
	}

	VFS_SYNC(vfsp, 0, cr);

	if (error = VFS_UNMOUNT(vfsp, cr))
		RWSLEEP_UNLOCK(&coveredvp->v_lock);
	else {
		struct module *modp;

		RWSLEEP_UNLOCK(&coveredvp->v_lock);

		/*
		 * Decrease the hold count if this FS type is loadable.
		 */
		if ((modp = vfssw[vfsp->vfs_fstype].vsw_modp) != NULL)
			MOD_RELE(modp);

		pm_clrdev(vfsp);	/* rm all entries from priv table */
		LOCK_DEINIT(&vfsp->vfs_mutex);
		kmem_free((caddr_t)vfsp, (u_int)sizeof(*vfsp));
		VN_RELE(coveredvp);
	}
	return error;
}

/*
 * void
 * sync(int attr)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    Update every mounted file system.  We call the vfs_sync operation of
 *    each file system type, passing it a NULL vfsp to indicate that all
 *    mounted file systems of that type should be updated.
 *
 * Note: this function is needed for other kernel functions.
 */
void
sync(int attr)
{
	register int i;
	pl_t pl;
	struct module *modp;

#ifdef CC_PARTIAL
	CC_COUNT(CC_SPEC_SYNC, CCBITS_SPEC_SYNC);
#endif
	ASSERT(KS_HOLD0LOCKS());

	pl = RW_RDLOCK(&mod_vfssw_lock, PLDLM);
	for (i = 1; i < nfstype; i++) {
		if ((modp = vfssw[i].vsw_modp) != NULL) {
			(void)LOCK(&modp->mod_lock, PLDLM);
			if (modp->mod_holdcnt == 0) {
				UNLOCK(&modp->mod_lock, PLDLM);
				continue;
			} else
				RW_UNLOCK(&mod_vfssw_lock, PLDLM);
			MOD_HOLD_L(modp, pl);
			(void)(*vfssw[i].vsw_vfsops->vfs_sync)(NULL, attr,
								CRED());
			MOD_RELE(modp);
		} else {
			RW_UNLOCK(&mod_vfssw_lock, pl);
			(void)(*vfssw[i].vsw_vfsops->vfs_sync)(NULL, attr,
								CRED());
		}
		pl = RW_RDLOCK(&mod_vfssw_lock, PLDLM);
	}
	RW_UNLOCK(&mod_vfssw_lock, pl);
}

/*
 * int
 * syssync(char *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    This is the entry point of the sync system call.
 */

/* ARGSUSED */
int
syssync(char *uap, rval_t *rvp)
{
	sync(0);
	return 0;
}

/*
 * Get file system statistics (statvfs and fstatvfs).
 */

struct statvfsa {
	char	*fname;
	struct	statvfs *sbp;
};


/*
 * int
 * statvfs(struct statvfsa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    This is the entry point of the statvfs system call.
 */

/* ARGSUSED */
int
statvfs(struct statvfsa *uap, rval_t *rvp)
{
	vnode_t *vp;
	vfs_t *vfsp;
	int error;

	error = lookupname(uap->fname, UIO_USERSPACE, FOLLOW, NULLVPP, &vp);
	if (error) {
		return error;
	}
	if ((vfsp = vp->v_vfsp) == NULL) {
		error = EINVAL;
		goto out;
	}
	error = cstatvfs(vfsp, uap->sbp);
out:
	VN_RELE(vp);
	return error;
}

struct fstatvfsa {
	int	fdes;
	struct	statvfs *sbp;
};

/*
 * int
 * fstatvfs(struct fstavfsa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    This is the entry point of the fstatvfs system call.
 */

/* ARGSUSED */
int
fstatvfs(struct fstatvfsa *uap, rval_t *rvp)
{
	struct file *fp;
	vfs_t *vfsp;
	int error;

	if (error = getf(uap->fdes, &fp))
		return error;
	if ((vfsp = fp->f_vnode->v_vfsp) == NULL) {
		error = EINVAL;
		goto out;
	}
	error = cstatvfs(vfsp, uap->sbp);
out:
	FTE_RELE(fp);
	return error;
}

/*
 * cstatvfs(vfs_t *vfsp, struct statvfs *ubp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    Common routine for statvfs and fstatvfs.
 *
 */
STATIC int
cstatvfs(vfs_t *vfsp, struct statvfs *ubp)
{
	struct statvfs ds;
	int error;

	struct_zero((caddr_t)&ds, sizeof(ds));
	if (error = VFS_STATVFS(vfsp, &ds))
		return error;

#ifdef CC_PARTIAL
	/*
	 * If the level of the calling process does not dominate the
	 * file system level ceiling, zero out blocks free and files
	 * free to prevent a covert channel.  If the process has
	 * P_FSYSRANGE or P_COMPAT, don't bother. 
	 */
	if (MAC_ACCESS(MACDOM, CRED()->cr_lid, vfsp->vfs_macceiling)
	&&  pm_denied(CRED(), P_FSYSRANGE) 
	&&  pm_denied(CRED(), P_COMPAT)) {
		ds.f_bfree = 0;
		ds.f_ffree = 0;
	}
#endif /* CC_PARTIAL */

	if (copyout((caddr_t)&ds, (caddr_t)ubp, sizeof(ds)))
		error = EFAULT;
	return error;
}

/*
 * statfs(2) and fstatfs(2) have been replaced by statvfs(2) and
 * fstatvfs(2) and will be removed from the system in a near-future
 * release.
 */
struct statfsa {
	char	*fname;
	struct	statfs *sbp;
	int	len;
	int	fstyp;
};

/*
 * statfs(struct statfsa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    This is the entry point of the statfs system call.
 */

/* ARGSUSED */
int
statfs(struct statfsa *uap, rval_t *rvp)
{
	vnode_t *vp;
	vfs_t *vfsp;
	int error;

	if (uap->fstyp != 0)
		return EINVAL;

	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  FOLLOW, NULLVPP, &vp))
		return error;

	if ((vfsp = vp->v_vfsp) == NULL) {
		error = EINVAL;
		goto out;
	}
	error = cstatfs(vfsp, uap->sbp, uap->len);
out:
	VN_RELE(vp);
	return error;
}

struct fstatfsa {
	int	fdes;
	struct	statfs *sbp;
	int	len;
	int	fstyp;
};

/*
 * fstatfs(struct fstatfsa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    This is the entry point of the fstatfs system call.
 */

/* ARGSUSED */
int
fstatfs(struct fstatfsa *uap, rval_t *rvp)
{
	struct file *fp;
	vfs_t *vfsp;
	int error;

	if (uap->fstyp != 0)
		return EINVAL;

	if (error = getf(uap->fdes, &fp))
		return error;
	if ((vfsp = fp->f_vnode->v_vfsp) == NULL) {
		error = EINVAL;
		goto out;
	}
	error = cstatfs(vfsp, uap->sbp, uap->len);
out:
	FTE_RELE(fp);
	return error;
}

/*
 * cstatfs(vfs_t *vfsp, struct statfs *sbp, int len)
 *
 * Calling/Exit State:
 *    No locking on entry or exit. 
 *
 * Description:
 *    Common routine for fstatfs and statfs.
 */
STATIC int
cstatfs(vfs_t *vfsp, struct statfs *sbp, int len)
{
	struct statfs sfs;
	struct statvfs svfs;
	register int error, i;
	char *cp, *cp2;
	register struct vfssw *vswp;

	if (len < 0 || len > sizeof(struct statfs))
		return EINVAL;
	if (error = VFS_STATVFS(vfsp, &svfs))
		return error;

	/*
	 * Map statvfs fields into the old statfs structure.
	 */
	struct_zero((caddr_t)&sfs, sizeof(sfs));
	sfs.f_bsize = svfs.f_bsize;
	sfs.f_frsize = (svfs.f_frsize == svfs.f_bsize) ? 0 : svfs.f_frsize;
	/* 
	 * Replaced divide by 512 with a shift by 9 
	 */
	sfs.f_blocks = svfs.f_blocks * (svfs.f_frsize >> 9);
	sfs.f_bfree = svfs.f_bfree * (svfs.f_frsize >> 9);
	sfs.f_files = svfs.f_files;
	sfs.f_ffree = svfs.f_ffree;

	cp = svfs.f_fstr;
	cp2 = sfs.f_fname;
	i = 0;
	while (i++ < sizeof(sfs.f_fname))
		if (*cp != '\0')
			*cp2++ = *cp++;
		else
			*cp2++ = '\0';
	while (*cp != '\0'
	  && i++ < (sizeof(svfs.f_fstr) - sizeof(sfs.f_fpack)))
		cp++;
	cp++;
	cp2 = sfs.f_fpack;
	i = 0;
	while (i++ < sizeof(sfs.f_fpack))
		if (*cp != '\0')
			*cp2++ = *cp++;
		else
			*cp2++ = '\0';
	if ((vswp = vfs_getvfssw(svfs.f_basetype)) == NULL)
		sfs.f_fstyp = 0;
	else
		sfs.f_fstyp = vswp - vfssw;

#ifdef CC_PARTIAL
	/*
	 * If the level of the calling process does not dominate the
	 * file system level ceiling, zero out blocks free and files
	 * free to prevent a covert channel.  If the process has
	 * P_FSYSRANGE or P_COMPAT, don't bother.  
	 */
	if (MAC_ACCESS(MACDOM, CRED()->cr_lid, vfsp->vfs_macceiling)
	&&  pm_denied(CRED(), P_FSYSRANGE)
	&&  pm_denied(CRED(), P_COMPAT)) {
		sfs.f_bfree = 0;
		sfs.f_ffree = 0;
	}
#endif /* CC_PARTIAL */

	if (copyout((caddr_t)&sfs, (caddr_t)sbp, len))
		return EFAULT;

	return 0;
}

/*
 * System call to map fstype numbers to names, and vice versa.
 */
struct fsa {
	int	opcode;
};

struct	fsinda {
	int	opcode;
	char	*fsname;
};

struct fstypa {
	int opcode;
	int index;
	char *cbuf;
};

/*
 * sysfsind(struct fsinda *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    Translate fs identifier to an index into the vfssw structure.
 */

STATIC int
sysfsind(struct fsinda *uap, rval_t *rvp)
{
	register struct vfssw *vswp;
	char fsbuf[FSTYPSZ];
	int error;
	u_int len = 0;
	int totalfs;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	error = copyinstr(uap->fsname, fsbuf, FSTYPSZ, &len);
	if (len == 1)	/* Includes null byte */
		error = EINVAL;
	if (error)
		return error;
	/*
	 * Search the vfssw table for the fs identifier
	 * and return the index.
	 */
	(void)RW_RDLOCK(&mod_vfssw_lock, PLDLM);
	totalfs = nfstype;
	RW_UNLOCK(&mod_vfssw_lock, PLBASE);
	for (vswp = vfssw; vswp < &vfssw[totalfs]; vswp++) {
		if (strcmp(vswp->vsw_name, fsbuf) == 0) {
			rvp->r_val1 = vswp - vfssw;
			return 0;
		}
	}
	return EINVAL;
}

/*
 * sysfstyp(struct fstypa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    Translate fstype index into an fs identifier.
 */

/* ARGSUSED */
STATIC int
sysfstyp(struct fstypa *uap, rval_t *rvp)
{
	register char *src;
	register int index;
	register struct vfssw *vswp;
	char *osrc;
	int totalfs;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	(void)RW_RDLOCK(&mod_vfssw_lock, PLDLM);
	totalfs = nfstype;
	RW_UNLOCK(&mod_vfssw_lock, PLBASE);
	if ((index = uap->index) <= 0 || index >= totalfs)
		return EINVAL;
	vswp = &vfssw[index];
	src = vswp->vsw_name ? vswp->vsw_name : "";
	for (osrc = src; *src++; )
		;			/* do nothing */
	if (copyout(osrc, uap->cbuf, src - osrc))
		return EFAULT;
	return 0;
}

/*
 * sysfs(struct fsa *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    This is the entry point of the sysfs system call.
 */

int
sysfs(struct fsa *uap, rval_t *rvp)
{
	int error;

	switch (uap->opcode) {
	case GETFSIND:
		error = sysfsind((struct fsinda *) uap, rvp);
		break;
	case GETFSTYP:
		error = sysfstyp((struct fstypa *) uap, rvp);
		break;
	case GETNFSTYP:
		/*
		 * Return number of fstypes configured in the system.
		 */
		rvp->r_val1 = nfstype - 1;
		error = 0;
		break;
	default:
		error = EINVAL;
	}

	return error;
}


/*
 * vfs_mountroot()
 *    vfs_mountroot is called by main() to mount the root filesystem.
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    If rootfstype is not set or not found, step through
 *    all the fstypes until we find one that will accept
 *    a mountroot() request.
 */
void
vfs_mountroot()
{
	int error;
	int i;
	vfssw_t *vsw;

	/*
	 * "rootfstype" will ordinarily have been initialized to
	 * contain the name of the fstype of the root file system
	 * (this is user-configurable). Use the name to find the
	 * vfssw table entry.
	 */
	if (vsw = vfs_getvfssw(rootfstype)) {
		VFS_INIT(rootvfs, vsw->vsw_vfsops, (caddr_t)0);
		rootvfs->vfs_fstype = vsw - vfssw;
		error = VFS_MOUNTROOT(rootvfs, ROOT_INIT);
	} else {
		/*
		 * If rootfstype is not set or not found, step through
		 * all the fstypes until we find one that will accept
		 * a mountroot() request.
		 */
		for (i = 1; i < nfstype; i++) {
			if (vfssw[i].vsw_vfsops) {
				VFS_INIT(rootvfs, vfssw[i].vsw_vfsops,
				  (caddr_t)0);
				if ((error =
				  VFS_MOUNTROOT(rootvfs, ROOT_INIT)) == 0) {
					rootvfs->vfs_fstype = i;
					break;
				}
			}
		}
	}
	if (error)
		/*
		 *+ Error occurred during VFS_MOUNTROOT causing
		 *+ root not to be mounted.
		 */
		cmn_err(CE_PANIC, "vfs_mountroot: cannot mount root");
	/*
	 * Get vnode for '/'.  Set up rootdir, u.u_lwpp->l_rdir and u.u_procp->p_cdir
	 * to point to it.  These are used by lookuppn() so that it
	 * knows where to start from ('/' or '.').
	 */
	if (VFS_ROOT(rootvfs, &rootdir))
		/*
		 *+ Could not get vnode for root.
		 */
		cmn_err(CE_PANIC, "vfs_mountroot: no root vnode");
	u.u_procp->p_cdir = rootdir;
	VN_HOLD(u.u_procp->p_cdir);
	u.u_lwpp->l_rdir = NULL;
	u.u_procp->p_rdir = NULL;
}

/*
 * vfs_add(struct vfs *vfsp)
 *    Add vfs to vfs list.
 *
 * Calling/Exit State:
 *    Called with vfs list locked on entry and at exit.
 *
 * Description:
 *    vfs_add is called by a specific filesystem's mount routine to add
 *    the new vfs into the vfs list and to cover the mounted-on vnode.
 *
 *    coveredvp is zero if this is the root.
 */
void
vfs_add(vnode_t *coveredvp, vfs_t *vfsp, int mflag)
{
	pl_t	s;

	if (coveredvp != NULL) {
		vfsp->vfs_next = rootvfs->vfs_next;
		rootvfs->vfs_next = vfsp;
		coveredvp->v_vfsmountedhere = vfsp;
	} else {
		/*
		 * This is the root of the whole world.
		 */
		rootvfs = vfsp;
		vfsp->vfs_next = NULL;
	}
	vfsp->vfs_vnodecovered = coveredvp;

	s = LOCK(&vfsp->vfs_mutex, FS_VFSPPL);
	if (mflag & MS_RDONLY)
		vfsp->vfs_flag |= VFS_RDONLY;
	else
		vfsp->vfs_flag &= ~VFS_RDONLY;

	if (mflag & MS_NOSUID)
		vfsp->vfs_flag |= VFS_NOSUID;
	else
		vfsp->vfs_flag &= ~VFS_NOSUID;
	UNLOCK(&vfsp->vfs_mutex, s);
}

/*
 * vfs_remove(struct vfs *vfsp)
 *    Remove vfs from vfs list.
 *
 * Calling/Exit State:
 *    The global vfslist_lock is locked on entry and remains
 *    locked at exit.
 *
 * Description:
 *    Remove a vfs from the vfs list, and destroy pointers to it.
 *    Should be called by filesystem "unmount" code after it determines
 *    that an unmount is legal but before it destroys the vfs.
 */
void
vfs_remove(vfs_t *vfsp)
{
	vfs_t *tvfsp;
	vnode_t *vp;
	
	/*
	 * Can't unmount root.  Should never happen because fs will
	 * be busy.
	 */
	ASSERT(vfsp != rootvfs);

	for (tvfsp = rootvfs; tvfsp != NULL; tvfsp = tvfsp->vfs_next) {
		if (tvfsp->vfs_next == vfsp) {
			/*
			 * Remove vfs from list, unmount covered vp.
			 */
			tvfsp->vfs_next = vfsp->vfs_next;
			vp = vfsp->vfs_vnodecovered;
			vp->v_vfsmountedhere = NULL;
			return;
		}
	}
	/*
	 *+ Can't find vfs to remove.
	 */
	cmn_err(CE_PANIC, "vfs_remove: vfs not found");

}

/*
 * getvfs(fsid_t *fsid)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    Translate fsid to vfsp.
 */

struct vfs *
getvfs(fsid_t *fsid)
{
	vfs_t *vfsp;

	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	for (vfsp = rootvfs; vfsp != NULL; vfsp = vfsp->vfs_next) {
		if (vfsp->vfs_fsid.val[0] == fsid->val[0]
		  && vfsp->vfs_fsid.val[1] == fsid->val[1]) {
			break;
		}
	}
	SLEEP_UNLOCK(&vfslist_lock);
	return vfsp;
}

/*
 * vfs_devsearch(dev_t dev)
 *    Search the vfs list for a specified dev.
 *
 * Calling/Exit State:
 *    vfslist locked on entry and remain locked at exit.
 *
 * Description:
 *    Search the vfs list for a specified device.  Returns a pointer to it
 *    or NULL if no suitable entry is found.
 */
vfs_t *
vfs_devsearch(dev_t dev)
{
	vfs_t *vfsp;

	for (vfsp = rootvfs; vfsp != NULL; vfsp = vfsp->vfs_next)
		if (vfsp->vfs_dev == dev) {
			return (vfsp);
		}
	return (NULL);
}

/*
 * vfs_getvfssw(char *type)
 *
 * Calling/Exit State:
 *    No locks required upon calling and no locks held upon exit.
 *
 * Description:
 *    Find a vfssw entry given a file system type name.
 */
vfssw_t *
vfs_getvfssw(char *type)
{
	int i, totalfs;
	pl_t pl;

	pl = RW_RDLOCK(&mod_vfssw_lock, PLDLM);
	totalfs = nfstype;
	RW_UNLOCK(&mod_vfssw_lock, pl);

	if (type == NULL || *type == '\0')
		return NULL;
	for (i = 1; i < totalfs; i++)
		if (strcmp(type, vfssw[i].vsw_name) == 0)
			return &vfssw[i];
	return NULL;
}

/*
 * vf_to_stf(u_long vf)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    Map VFS flags to statvfs flags.
 */
u_long
vf_to_stf(u_long vf)
{
	u_long stf = 0;

	if (vf & VFS_RDONLY)
		stf |= ST_RDONLY;
	if (vf & VFS_NOSUID)
		stf |= ST_NOSUID;
	if (vf & VFS_NOTRUNC)
		stf |= ST_NOTRUNC;
	
	return stf;
}

/*
 * Entries for (illegal) fstype 0.
 */

STATIC int vfsstray(void);

STATIC struct vfsops vfs_strayops = {
	(int (*)())vfsstray,
	(int (*)())vfsstray,
	(int (*)())vfsstray,
	(int (*)())vfsstray,
	(int (*)())vfsstray,
	(int (*)())vfsstray,
	(int (*)())vfsstray,
	(int (*)())vfsstray,
	(int (*)())vfsstray,
	(int (*)())vfsstray,
	(int (*)())vfsstray,
	(int (*)())vfsstray,
};


/*
 * int
 * vfsstray(void)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 */
STATIC int
vfsstray(void)
{
	/*
	 *+ Illegal vfs operation.
	 */
	cmn_err(CE_PANIC, "stray vfs operation");
	/* NOTREACHED */
}

/*
 * void
 * fs_init(void)
 *	Initialize the vfs subsystem.
 *
 * Calling/Exit State:
 *	Should only be called during system startup - no locking issues.
 *
 * Description:
 *	Initializes the global-VFS data structures and each configured
 *	file system type.
 */
void
fs_init(void)
{
	int i;
	extern void finit(void);
	extern void binit(void);

	binit();		/* init the buffer cache data structures */
	finit();		/* init the file table data structures */
	dnlc_init();		/* initialize the dnlc cache/locks */
	frlck_init();		/* initialize file/record locks */

	SLEEP_INIT(&vfslist_lock, (uchar_t) 0, &vfslist_lkinfo, KM_SLEEP);

	/*
	 * fstype 0 is (arbitrarily) invalid.
	 */
	vfssw[0].vsw_vfsops = &vfs_strayops;
	vfssw[0].vsw_name = "BADVFS";

	/*
	 * Call all the init routines.
	 */
	for (i = 1; i < nfstype; i++) {
		(*vfssw[i].vsw_init)(&vfssw[i], i);
	}
}

/*
 * void
 * fs_postroot(void)
 *	Post-root initializations.
 *
 * Calling/Exit State:
 *	Called from main() after mounting root.
 */
void
fs_postroot(void)
{
	extern void fsflush(void *);
	extern void bdelay_daemon(void *);
#ifdef _PAGEIO_HIST
	extern void bufsubr_daemon(void *);
#endif

	/* Spawn the fsflush daemon. */
	(void) spawn_lwp(NP_SYSPROC, NULL, LWP_DETACHED, NULL,
			 fsflush, NULL);

	/* Spawn the bdelay daemon. */
	(void) spawn_lwp(NP_SYSPROC, NULL, LWP_DETACHED, NULL,
			 bdelay_daemon, NULL);

#ifdef _PAGEIO_HIST
	/* Spawn the bufsubr daemon. */
	(void) spawn_lwp(NP_SYSPROC, NULL, LWP_DETACHED, NULL,
			 bufsubr_daemon, NULL);
#endif

}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_vfs(const vfs_t *vfsp)
 *	Print a vfs structure.
 *
 * Calling/Exit State:
 *	No locking.
 *
 * Remarks:
 *	Intended for use from a kernel debugger.
 */
void
print_vfs(const vfs_t *vfsp)
{
	uint_t	idx;

	debug_printf("vfs = 0x%x, fstype = %2x: ",
		     vfsp, vfsp->vfs_fstype);
	if ((idx = vfsp->vfs_fstype) < nfstype)
		debug_printf("%-8s ", vfssw[idx].vsw_name);
	else
		debug_printf("         ");
	debug_printf("\top = %-8x, mounted on %x\n",
		 vfsp->vfs_op, vfsp->vfs_vnodecovered);
	debug_printf("\tflag = %8x, bsize: %8d, data: %8x, dev: %d,%d\n",
		 vfsp->vfs_flag, vfsp->vfs_bsize, vfsp->vfs_data,
		 getemajor(vfsp->vfs_dev), geteminor(vfsp->vfs_dev));
	debug_printf("\tbcount = %8x, next = %8x\n",
		 vfsp->vfs_bcount, vfsp->vfs_next);
}

#endif	/* defined(DEBUG) || defined(DEBUG_TOOLS) */
