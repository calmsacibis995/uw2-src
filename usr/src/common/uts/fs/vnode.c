/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/vnode.c	1.35"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <acc/audit/audit.h>		/* audit recording function */
#include <acc/mac/mac.h>		/* for MAC access checks */
#include <acc/priv/privilege.h>		/* for vn_remove (MAC) */
#include <fs/file.h>
#include <fs/fs_hier.h>
#include <fs/pathname.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>


/*
 * Convert stat(2) formats to vnode types and vice versa.  (Knows about
 * numerical order of S_IFMT and vnode types.)
 */
vtype_t iftovt_tab[] = {
	VNON, VFIFO, VCHR, VNON, VDIR, VXNAM, VBLK, VNON,
	VREG, VNON, VLNK, VNON, VNON, VNON, VNON, VNON
};

ushort_t vttoif_tab[] = {
	0, S_IFREG, S_IFDIR, S_IFBLK, S_IFCHR, S_IFLNK, S_IFIFO, S_IFNAM, 0
};

LKINFO_DECL(vmutex_lkinfo, "FS:v_filocks_mutex:vnode (per-vnode)", 0);
LKINFO_DECL(vlock_lkinfo, "FS:v_lock:vnode (per-vnode)", 0);

/*
 * int
 * vn_rdwr(uio_rw_t rw, vnode_t *vp, void *base, int len, off_t offset,
 *	   uio_seg_t seg, int ioflag, long ulimit, cred_t *cr, int *residp)
 * 	Read or write a vnode.  Called from kernel code.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	Hold inode's rwlock in *shared* mode before calling VOP_READ or
 *	*exclusive* mode before calling VOP_WRITE; mode is decided by
 *	rw enumeration argument.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EROFS	when trying to write to a read-only filesystem
 *		EIO	if residual count is not NULL
 */
int
vn_rdwr(uio_rw_t rw, vnode_t *vp, void *base, int len, off_t offset,
	uio_seg_t seg, int ioflag, long ulimit, cred_t *cr, int *residp)
{
	uio_t	uio;
	iovec_t	iov;
	int	error;

	if (!(vp->v_flag & VNOMAP) && rw == UIO_WRITE &&
			(vp->v_vfsp->vfs_flag & VFS_RDONLY &&
			vp->v_type != VBLK &&
			vp->v_type != VCHR &&
			vp->v_type != VFIFO)) {
		return (EROFS);
	}

	iov.iov_base = base;
	iov.iov_len = len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = offset;
	uio.uio_segflg = (short)seg;
	uio.uio_resid = len;
	uio.uio_limit = ulimit;
	if (rw == UIO_WRITE) {
		uio.uio_fmode = FWRITE;
		error = VOP_RWWRLOCK(vp, offset, len, uio.uio_fmode);
		if (error) {
			return (error);
		}
		error = VOP_WRITE(vp, &uio, ioflag, cr);
	} else {
		uio.uio_fmode = FREAD;
		error = VOP_RWRDLOCK(vp, offset, len, uio.uio_fmode);
		if (error) {
			return (error);
		}
		error = VOP_READ(vp, &uio, ioflag, cr);
	}
	VOP_RWUNLOCK(vp, offset, len);
	if (residp) {
		*residp = uio.uio_resid;
	} else if (uio.uio_resid) {
		error = EIO;
	}
	return (error);
}

/*
 * int
 * vn_create(char *pnamep, uio_seg_t seg, vattr_t *vap, vcexcl_t excl,
 *	     int mode, vnode_t **vpp, create_t why)
 *      Create a vnode (makenode).  Check permissions, check file
 *      existance, check file/record lock and mandatory lock, and called
 *      an appropriate VOP.
 *
 * Calling/Exit State:
 *      No locks should be held upon entry, none are held on exit.
 *
 * Description:
 *      This may be callable by the kernel. why is defined based on
 *      the vnode's type.
 *
 *      A returned value of 0 if success; otherwise a valid error is
 *      returned. Errors directly returned by this routine are:
 *              EACCES if no access on the parent directory.
 *              EEXIST if a named file already exists.
 *              EROFS if create a node on read-only file system.
 */
int
vn_create(char *pnamep, uio_seg_t seg, vattr_t *vap, vcexcl_t excl,
	  int mode, vnode_t **vpp, create_t why)
{
	vnode_t		*dvp;	/* ptr to parent dir vnode */
	pathname_t	pn;
	int		error;
	vnode_t 	*tvp;	/* temporary created vnode ptr (MAC) */
	vnode_t 	*avp;
	cred_t		*crp = CRED();

	ASSERT((vap->va_mask & (AT_TYPE|AT_MODE)) == (AT_TYPE|AT_MODE));

	/*
	 * Lookup directory.
	 * If new object is a file, call lower level to create it.
	 * Note that it is up to the lower level to enforce exclusive
	 * creation, if the file is already there.
	 * This allows the lower level to do whatever
	 * locking or protocol that is needed to prevent races.
	 * If the new object is directory call lower level to make
	 * the new directory, with "." and "..".
	 */
	*vpp = NULL;
	error = pn_get(pnamep, seg, &pn);
	if (error) {
		return (error);
	}
	dvp = NULL;
	/*
	 * lookup will find the parent directory for the vnode.
	 * When it is done the pn holds the name of the entry
	 * in the directory.
	 * If this is a non-exclusive create we also find the node itself.
	 */
	if (excl == EXCL) {
		error = lookuppn(&pn, NO_FOLLOW, &dvp, NULLVPP); 
	} else {
		error = lookuppn(&pn, FOLLOW, &dvp, vpp);
	}
	if (error) {
		pn_free(&pn);
		if (why == CRMKDIR && error == EINVAL) {
			error = EEXIST;		/* SVID */
		}
		return (error);
	}

	/* for audit subsystem */
	avp = *vpp;
	if (avp)
		VN_HOLD(avp);

	if (why != CRMKNOD) {
		vap->va_mode &= ~VSVTX;
	}

	/*
	 * MAC checks are performed early on to prevent possible
	 * covert channels.  Note, however, that the time 
	 * between the MAC checks and VOP_CREATE is rather long,
	 * and may include the process going to sleep.
	 * File system types which support levels are responsible
	 * for performing additional MAC checks on creation.
	 */
	if (*vpp != NULL) {
		/*
		 * File exists; make sure user has appropriate MAC
		 * access to file.
		 */
		if ((*vpp)->v_type != VCHR && (*vpp)->v_type != VBLK) {
			error = MAC_VACCESS(*vpp, mode, crp);
			if (error) {
				VN_RELE(*vpp);
				goto out;
			}
		}
	} else {
		/*
		 * File does not exist; make sure user has
		 * MAC access to directory.
		 */
		error = MAC_CHECKS(dvp, crp);
		if (error) {
			goto out;
		}
	}
	/*
	 * Make sure filesystem is writeable.
	 * If the file already exists do not return EROFS.
	 */
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY && (*vpp == NULL)) {
		error = EROFS;
	} else if (excl == NONEXCL && *vpp != NULL) {
		vnode_t *vp = *vpp;

		/*
		 * If the file is the root of a VFS, we've crossed a
		 * mount point and the "containing" directory that we
		 * acquired above (dvp) is irrelevant because it's in
		 * a different file system.  We apply VOP_CREATE to the
		 * target itself instead of to the containing directory
		 * and supply a null path name to indicate (conventionally)
		 * the node itself as the "component" of interest.
		 *
		 * The intercession of the file system is necessary to
		 * ensure that the appropriate permission checks are
		 * done.
		 */
		if (vp->v_flag & VROOT) {
			ASSERT(why != CRMKDIR);
			error =
			  VOP_CREATE(vp, "", vap, excl, mode, vpp, CRED());
			/*
			 * If the create succeeded, it will have created
			 * a new reference to the vnode.  Give up the
			 * original reference.
			 */
			VN_RELE(vp);
			goto out;
		}

		/*
		 * We throw the vnode away to let VOP_CREATE
		 * truncate the file in a non-racy manner.
		 */
		VN_RELE(vp);
	}

	if (error == 0) {
		/*
		 * Call fs dependent mkdir() to create dir, fs dep makemld()
		 * to create Multi-Level Directory. Otherwise, fs
		 * dep create.
		 */
		if (why == CRMKDIR)
			error = VOP_MKDIR(dvp, pn.pn_path, vap, vpp, CRED());
		else if (why == CRMKMLD)
                        error =VOP_MAKEMLD(dvp, pn.pn_path, vap, vpp, CRED());
		else
			error = VOP_CREATE(dvp, pn.pn_path, vap,
			  excl, mode, vpp, CRED());
		/*
                 * Assign the created vnode's level at this time,
                 * if not yet assigned.
                 * If the file system type does not support levels,
		 * get the level from the file system's root vnode.
		 * The root vnode's level is assigned at mount time to
		 * the mount point's level.
		 */
		tvp = *vpp;
		if (error == 0) {
			if (!(tvp->v_macflag & VMAC_SUPPORT)) {
				/*
				 * tvp's level is  assigned the file system 
				 * type level floor. If the file system level 
				 * range has not been assigned, then tvp's 
				 * level remains 0 which is an invalid LID that
				 * can only be overridden with MAC privileges.
				 */
				tvp->v_lid = tvp->v_vfsp->vfs_macfloor;
			}
		} 
	}
out:
	if (error) 
		ADT_CRCHECK(u.u_lwpp->l_auditp, avp? avp : dvp, error);
	else
		ADT_CRCHECK(u.u_lwpp->l_auditp, *vpp, error);

	pn_free(&pn);
	ADT_FREEPATHS(u.u_lwpp->l_auditp);
	if (avp)
		VN_RELE(avp);
	VN_RELE(dvp);
	return (error);
}

/*
 * int
 * vn_open(char *pnamep, uio_seg_t seg, int filemode,
 *	   int createmode, vnode_t **vpp, create_t crwhy)
 * 	Open/create a vnode.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 * Description:
 * 	This may be callable by the kernel, the only known use of
 * 	user context being that the current user credentials are
 * 	used for permissions. crwhy is defined iff filemode & FCREAT.
 *
 *	A returned value of 0 indicates success;  otherwise a valid error is
 *      returned. Errnos directly returned by this routine are:
 *              EISDIR if (vp->v_type == VDIR)
 *              EROFS if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
 */
int
vn_open(char *pnamep, uio_seg_t seg, int filemode,
 	int createmode, vnode_t **vpp, create_t crwhy)
{
	vnode_t	*vp;
	int	mode;
	int	error;
	vattr_t	vattr;

	mode = 0;
	if (filemode & FREAD)
		mode |= VREAD;
	if (filemode & (FWRITE|FTRUNC))
		mode |= VWRITE;
 
	if (filemode & FCREAT) {
		vcexcl_t excl;

		/*
		 * Wish to create a file.
		 */
		vattr.va_type = VREG;
		vattr.va_mode = createmode;
		vattr.va_mask = AT_TYPE|AT_MODE;
		if (filemode & FTRUNC) {
			vattr.va_size = 0;
			vattr.va_mask |= AT_SIZE;
		}
		if (filemode & FEXCL) {
			excl = EXCL;
		} else {
			excl = NONEXCL;
		}
		filemode &= ~(FTRUNC|FEXCL);
		
		/* 
		 * vn_create can take a while, so preempt.
		 */
		error = vn_create(pnamep, seg, &vattr, excl, mode, &vp, crwhy);
		if (error) {
			return (error);
		}
		if (!(vp->v_macflag & VMAC_DOPEN)
		&&  (error = MAC_VACCESS(vp, mode, CRED())))
			goto out;
	} else {
		/*
		 * Wish to open a file.  Just look it up.
		 */
		error = lookupname(pnamep, seg, FOLLOW, NULLVPP, &vp);
		if (error) {
			return (error);
		}
#ifdef EMULATORS
		/*
		 * For emulators in special read mode, we turn read access
		 * checks into execute access checks.
		 */
		if (mode == VREAD && (u.u_emul & UE_RDEXEC))
			mode = VEXEC;
#endif	/* FEMULATORS */
		/*
		 * Perform MAC check as soon as vnode is
		 * available.
		 */
		if (!(vp->v_macflag & VMAC_DOPEN) &&
		    (error = MAC_VACCESS(vp, mode, CRED())))
			goto out;
		/*
		 * Can't write directories, active text, or
		 * read-only filesystems. (unless target is device or fifo)
		 * Can't truncate files on which mandatory locks is in effect.
		 */
		if (filemode & (FWRITE|FTRUNC)) {
			if (vp->v_type == VDIR) {
				error = EISDIR;
				goto out;
			}
			if (vp->v_vfsp->vfs_flag & VFS_RDONLY &&
                             vp->v_type != VBLK &&
                             vp->v_type != VCHR &&
                             vp->v_type != VFIFO) {
                                error = EROFS;
                                goto out;
                        }
			/*
			 * Can't truncate files on which mandatory locking
			 * is in effect.
			 */
			if ((filemode & FTRUNC) && vp->v_filocks != NULL) {
				vattr.va_mask = AT_MODE;
				error = VOP_GETATTR(vp, &vattr, 0,
						    CRED());
				if (error == 0 && MANDLOCK(vp, vattr.va_mode))
					error = EAGAIN;
			}
			if (error)
				goto out;
		}
		/* Check discretionary permissions.*/
		error = VOP_ACCESS(vp, mode, 0, CRED());
		if (error) {
			goto out;
		}
	}

	/*
	 * Do opening protocol.
	 */
	error = VOP_OPEN(&vp, filemode, CRED());
	if (error == 0) {
		/*
		 * Truncate if required.
		 */
		if (filemode & FTRUNC) {
			vattr.va_size = 0;
			vattr.va_mask = AT_SIZE;
			error = VOP_SETATTR(vp, &vattr, 0, filemode, CRED());
			if (error) {
				(void) VOP_CLOSE(vp, filemode, 1, 0, CRED());
			}
		}
	}
out:
	if (error) {
		VN_RELE(vp);
	} else
		*vpp = vp;
	return (error);
}

/*
 * vn_link(char *from, char *to, uio_seg_t seg)
 *      Link a file or directory.
 *
 * Calling/Exit State:
 *      No locks should be held upon entry, none are held on exit.
 *
 * Description:
 *      This may be callable by the kernel.
 *
 *      A returned value of 0 if success; otherwise a valid error is
 *      returned. Errors directly returned by this routine are:
 *              EROFS if (tdvp->v_vfsp->vfs_flag & VFS_RDONLY)
 *              EXDEV if from and to files are on different file system.
 */
int
vn_link(char *from, char *to, uio_seg_t seg)
{
	vnode_t		*fvp;		/* from vnode ptr */
	vnode_t		*tdvp;		/* to directory vnode ptr */
	pathname_t	pn;
	int		error;
	vattr_t		vattr;
	long		fsid;

	fvp = tdvp = NULL;
	error = pn_get(to, seg, &pn);
	if (error) {
		return (error);
	}
	error = lookupname(from, seg, FOLLOW, NULLVPP, &fvp);
	if (error) {
		pn_free(&pn);
		return (error);
	}
	error = lookuppn(&pn, NO_FOLLOW, &tdvp, NULLVPP);
	if (error) {
		goto out;
	}

	/*
	 * Make sure both source vnode and target directory vnode are
	 * in the same vfs and that it is writeable.
	 */
	vattr.va_mask = AT_FSID;
	error = VOP_GETATTR(fvp, &vattr, 0, CRED());
	if (error) {
		goto out;
	}
	fsid = vattr.va_fsid;
	error = VOP_GETATTR(tdvp, &vattr, 0, CRED());
	if (error) {
		goto out;
	}
	if (fsid != vattr.va_fsid) {
		error = EXDEV;
		goto out;
	}
	if (tdvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}

	/*
	 * Must have MAC write access to both the source file and target 
	 * directory.  MAC write access is required on the source because
	 * the link count in its inode will change.
	 */
	if ((error = MAC_VACCESS(fvp, VWRITE, CRED())) == 0)
		error = MAC_VACCESS(tdvp, VWRITE, CRED());

	if (!error) {
		error = VOP_LINK(tdvp, fvp, pn.pn_path, CRED()); 
	}
out:
	pn_free(&pn);
	if (fvp)
		VN_RELE(fvp);
	if (tdvp)
		VN_RELE(tdvp);
	return (error);
}

/*
 * vn_rename(char *from, char *to, uio_seg_t seg)
 *      Rename a file or directory.
 *
 * Calling/Exit State:
 *      No locks should be held upon entry, none are held on exit.
 *
 * Description:
 *      This may be callable by the kernel.
 *
 *      A returned value of 0 if success; otherwise a valid error is
 *      returned. Errors directly returned by this routine are:
 *              ENOENT if from file doesn't exist.
 *              EROFS if rename a file from read-only file system. 
 *              EXDEV if from and to files are on different file system.
 */
int
vn_rename(char *from, char *to, uio_seg_t seg)
{
	vnode_t		*fdvp;		/* from directory vnode ptr */
	vnode_t		*fvp;		/* from vnode ptr */
	vnode_t		*tdvp;		/* to directory vnode ptr */
	pathname_t	fpn;		/* from pathname */
	pathname_t	tpn;		/* to pathname */
	int		error;

	fdvp = tdvp = fvp = NULL;
	/*
	 * Get to and from pathnames.
	 */
	error = pn_get(from, seg, &fpn);
	if (error) {
		return (error);
	}
	error = pn_get(to, seg, &tpn);
	if (error) {
		pn_free(&fpn);
		return (error);
	}
	/*
	 * Lookup to and from directories.
	 */
	error = lookuppn(&fpn, NO_FOLLOW, &fdvp, &fvp);
	if (error) {
		goto out;
	}
	/*
	 * Make sure there is an entry.
	 */
	if (fvp == NULL) {
		error = ENOENT;
		goto out;
	}
	error = lookuppn(&tpn, NO_FOLLOW, &tdvp, NULLVPP);
	if (error) {
		goto out;
	}
	/*
	 * Make sure both the from vnode and the to directory are
	 * in the same vfs and that it is writable.
	 */
	if ((fvp->v_vfsp != tdvp->v_vfsp) ||
            (fdvp->v_vfsp != tdvp->v_vfsp)) {
		error = EXDEV;
		goto out;
	}
	if (tdvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/*
	 * Must have MAC write access to both the source's parent directory
	 * and the target's parent directory.  If the source file itself
	 * is a directory, MAC write access is required on it as well
	 * since inum for ".." will change.
	 */
	if ((error = MAC_VACCESS(fdvp, VWRITE, CRED())) == 0
	   && (error = MAC_VACCESS(tdvp, VWRITE, CRED())) == 0) {
		if (fvp->v_type == VDIR)
			error = MAC_VACCESS(fvp, VWRITE, CRED());
	}
	if (!error)
		error = VOP_RENAME(fdvp,fpn.pn_path,tdvp,tpn.pn_path,CRED());
out:
	pn_free(&fpn);
	pn_free(&tpn);
	if (fvp)
		VN_RELE(fvp);
	if (fdvp)
		VN_RELE(fdvp);
	if (tdvp)
		VN_RELE(tdvp);
	return (error);
}

/*
 * int
 * vn_remove(char *fnamep, uio_seg_t seg, rm_t dirflag)
 *      Remove a file or directory.
 *
 * Calling/Exit State:
 *      No locks should be held upon entry, none are held on exit.
 *
 * Description:
 *      This may be callable by the kernel.
 *
 *      A returned value of 0 if success; otherwise a valid error is
 *      returned. Errors directly returned by this routine are:
 *              ENOENT if file doesn't exist.
 *              EROFS if the named file is from the read-only file system. 
 *              EXDEV if from and to files are on different file system.
 *              EBUSY if (vp->v_flag & VROOT)
 *              ENOTDIR if (dirflag == RMDIRECTORY && vtype != VDIR)
 */
int
vn_remove(char *fnamep, uio_seg_t seg, rm_t dirflag)
{
	vnode_t		*vp;		/* entry vnode */
	vnode_t		*dvp;		/* ptr to parent dir vnode */
	vnode_t		*cvp;		/* ptr to current dir vnode */
	pathname_t	pn;		/* name of entry */
	vtype_t		vtype;
	lid_t		vlid;		/* for MAC check */
	int		error;
	vfs_t		*vfsp;
	cred_t		*crp;

	error = pn_get(fnamep, seg, &pn);
	if (error) {
		return (error);
	}
	vp = NULL;
	if (error = lookuppn(&pn, NO_FOLLOW, &dvp, &vp)) {
		if ((error == EINVAL) &&
		    (strncmp(".", fnamep, pn.pn_pathlen) == 0 ))
			error = EBUSY;
		pn_free(&pn);
		return (error);
	}

	/*
	 * Make sure there is an entry.
	 */
	if (vp == NULL) {
		error = ENOENT;
		goto out;
	}

	vfsp = vp->v_vfsp;

	/*
	 * If the named file is the root of a mounted filesystem, fail.
	 */
	if (vp->v_flag & VROOT) {
		error = EBUSY;
		goto out;
	}

	/*
	 * Make sure filesystem is writeable.
	 */
	if (vfsp && vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}

	/*
	 * Release vnode before removing.
	 */
	vtype = vp->v_type;
	vlid = vp->v_lid;
	VN_RELE(vp);
	vp = NULL;
	/*
	 * If caller is using rmdir(2), it can only be applied to directories.
	 * Unlink(2) can be applied to anything.
	 */
	if (dirflag == RMDIRECTORY && vtype != VDIR) {
		error = ENOTDIR;
		goto out;
	}
	/*
	 * Must have MAC write access to the vnode's parent directory
	 * and be dominated by level on file (to prevent covert channel).
	 */
	crp = CRED();
	if ((error = MAC_VACCESS(dvp, VWRITE, crp)) == 0
	   && (error = MAC_ACCESS(MACDOM, vlid, crp->cr_lid))) {
		if (!pm_denied(crp, P_COMPAT)
		||  !pm_denied(crp, P_MACWRITE))
			error = 0;
	}
	if (!error) {
		if (dirflag == RMDIRECTORY) {
			pl_t pl;
			pl = CUR_ROOT_DIR_LOCK(u.u_procp);
			cvp = u.u_procp->p_cdir;
			VN_HOLD(cvp);
			CUR_ROOT_DIR_UNLOCK(u.u_procp, pl);
			error = VOP_RMDIR(dvp, pn.pn_path, cvp, crp);
			VN_RELE(cvp);
		} else {
			error = VOP_REMOVE(dvp, pn.pn_path, crp);
		}
	}
out:
	pn_free(&pn);
	if (vp != NULL)
		VN_RELE(vp);
	VN_RELE(dvp);
	return (error);
}

/*
 * void
 * vn_init(vnode_t *vp, struct vfs *vfsp, enum vtype type,
 *		dev_t dev, ushort_t flag, int sleep)
 *
 * Description:
 *	Initialize a vnode.
 *
 * Calling/Exit State:
 *	None.
 */
void
vn_init(vnode_t *vp, struct vfs *vfsp, enum vtype type,
		dev_t dev, ushort_t flag, int sleep)
{

	vn_init_unnamed(vp, vfsp, type, dev, flag, sleep);
	vp->v_vfsmountedhere = NULL;
	vp->v_vfsp = vfsp;
	vp->v_rdev = dev;
	RWSLEEP_INIT(&vp->v_lock, (uchar_t)0, &vlock_lkinfo, sleep);
	LOCK_INIT(&vp->v_filocks_mutex, FS_VPFRLOCKHIER, FS_VPFRLOCKPL,
		   &vmutex_lkinfo, sleep);
	vp->v_filocks = NULL;
	vp->v_stream = NULL;
}

/*
 * void
 * vn_init_unnamed(vnode_t *vp, struct vfs *vfsp, enum vtype type,
 *		dev_t dev, ushort_t flag, int sleep)
 *	Initialize an unnamed vnode.
 *
 * Calling/Exit State:
 *	None.
 */
void
vn_init_unnamed(vnode_t *vp, struct vfs *vfsp, enum vtype type,
		dev_t dev, ushort_t flag, int sleep)
{

	vp->v_flag = flag;
	vp->v_count = 1;
	vp->v_softcnt = 0;
	vp->v_type = type;
	FSPIN_INIT(&vp->v_mutex);
	vp->v_pages = NULL;
	VN_LOG_INIT(vp);
}

/*
 * void
 * vn_deinit_unnamed(vnode_t *vp)
 *	De-initialize an unnamed vnode.
 *
 * Calling/Exit State
 *	None
 */
void
vn_deinit_unnamed(vnode_t *vp)
{

}

/*
 * void
 * vn_deinit(vnode_t *vp)
 *	De-initialize a vnode.
 *
 * Calling/Exit State
 *	None
 */
void
vn_deinit(vnode_t *vp)
{

	vn_deinit_unnamed(vp);
	RWSLEEP_DEINIT(&vp->v_lock);
	LOCK_DEINIT(&vp->v_filocks_mutex);
}

#ifdef DEBUG

/*
 * int
 * vn_assfail(vnode_t *vp, const char *a, const char *f, int l)
 *	Processing a failing VN_ASSERT.
 *
 * Calling/Exit State:
 *	Called with VN_LOCK held for vp.
 */
int
vn_assfail(vnode_t *vp, const char *a, const char *f, int l)
{
	VN_LOG(vp, "vn_assfail");
	VN_UNLOCK(vp);
	cmn_err(CE_CONT, "VNODE FAILURE at 0x%lx\n", (long)vp);
	assfail(a, f, l);
	/* NOTREACHED */
}

#endif /* DEBUG */

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_vnode(const vnode_t *vp)
 *	Print a vnode.
 *
 * Calling/Exit State:
 *	No locking.
 *
 * Remarks:
 *	Intended for use from a kernel debugger.
 */
void
print_vnode(const vnode_t *vp)
{
	debug_printf("vp = 0x%x, type = %2x, vdata = %8x, flag = %4x\n",
		     vp, vp->v_type, vp->v_data, vp->v_flag);
	debug_printf("\tvcount = %d, vsoftcnt = %d\n",
		     vp->v_count, vp->v_softcnt);
	debug_printf("\tvfsp = %8x, vfsmountedhere = %8x, stream = %8x\n",
		     vp->v_vfsp, vp->v_vfsmountedhere, vp->v_stream);
	debug_printf("\tpages = %8x, filocks = %8x, rdev = %d,%d\n",
		     vp->v_pages, vp->v_filocks,
		     getemajor(vp->v_rdev), geteminor(vp->v_rdev));
	debug_printf("\tlid = %8x, macflag = %8x\n", vp->v_lid, vp->v_macflag);
}

#endif	/* defined(DEBUG) || defined(DEBUG_TOOLS) */

#ifdef _VNODE_HIST

/*
 * Special vnode history logging support for debugging.
 */

/*
 * void
 * vn_log(vnode_t *vp, const char *service, int line, const char *file)
 *	Record a log entry in a vnode.
 *
 * Calling/Exit State:
 *      Called with VN_LOCK held for vp.
 */
void
vn_log(vnode_t *vp, char *service, int line, char *file)
{
	vn_hist_record_t *vnrp;

	vnrp = &vp->v_hist.vli_rec[vp->v_hist.vli_cursor];
	if (++vp->v_hist.vli_cursor == VN_HIST_SIZE)
		vp->v_hist.vli_cursor = 0;
	vnrp->vhr_service = service;
	vnrp->vhr_count = vp->v_count;
	vnrp->vhr_softcnt = vp->v_softcnt;
	vnrp->vhr_line = line;
	vnrp->vhr_file = file;
	vnrp->vhr_lwp = CURRENT_LWP();
	GET_TIME(&vnrp->vhr_stamp);

	vnlwp_log(vp, service, line, file);
}

/*
 * void
 * vnlwp_log(vnode_t *vp, char *service, int line, char *file)
 *	Record a log entry in the per-lwp vnode log of the calling lwp.
 *
 * Calling/Exit State:
 *	Called in context from an LWP, not from an interrupt service
 *	routine.
 */
void
vnlwp_log(vnode_t *vp, const char *service, int line, const char *file)
{
	lwp_t *lwpp = CURRENT_LWP();
	lwp_vn_hist_t *logp;
	lwp_vn_rec_t *vnrp;

	ASSERT(!servicing_interrupt());

	if (lwpp == NULL)
		return;

	logp = &lwpp->l_vn_log;
	vnrp = &logp->lvi_rec[logp->lvi_cursor];
	if (++logp->lvi_cursor == VN_LWP_HIST_SIZE)
		logp->lvi_cursor = 0;

	vnrp->lvr_service = service;
	if (vp != NULL) {
		vnrp->lvr_count = vp->v_count;
		vnrp->lvr_softcnt = vp->v_softcnt;
	}
	vnrp->lvr_line = line;
	vnrp->lvr_file = file;
	vnrp->lvr_vp = vp;
	GET_TIME(&vnrp->lvr_stamp);
}

/*
 * void
 * print_vnode_log(vnode_t *vp, lwp_t *lwp)
 *	Print contents of the log for vnode vp. If lwp is specified,
 *	then restrict printout to that lwp.
 *
 * Calling/Exit State:
 *	Intended for use from a kernel debugger.
 */
void
print_vnode_log(const vnode_t *vp, const lwp_t *lwp)
{
	vn_hist_record_t *vnrp;
	ulong_t last_stamp, diff;
	char digit[9];
	int i, j;
	char c, *p;
	int last;
	int cursor = vp->v_hist.vli_cursor;

	debug_printf("TIME    LWP      count   soft\n"
		     "----    ---      ----    ----\n");

	last = cursor - 1;
	if (last < 0)
		last += VN_HIST_SIZE;
	last_stamp = vp->v_hist.vli_rec[last].vhr_stamp;

	for (j = 0; j < VN_HIST_SIZE; ++j) {
		vnrp = &vp->v_hist.vli_rec[cursor];
		if (++cursor == VN_HIST_SIZE)
			cursor = 0;
		if (vnrp->vhr_service == NULL)
			continue;
		if (lwp != NULL && vnrp->vhr_lwp != lwp)
			continue;
		diff = last_stamp - vnrp->vhr_stamp;
		p = &digit[8];
		*p-- = '\0';
		diff /= 100;
		for (i = 1; i <= 6 || diff != 0; ++i) {
			if (i == 5)
				*p-- = '.';
			else {
				*p-- = (diff % 10) + '0';
				diff /= 10;
			}
		}
		debug_printf("-%s %lx %ld %ld %s from line %d of file %s\n",
			     p + 1, (ulong_t)vnrp->vhr_lwp,
			     (ulong_t)vnrp->vhr_count,
			     (ulong_t)vnrp->vhr_softcnt, vnrp->vhr_service,
			     vnrp->vhr_line, vnrp->vhr_file);
		if (debug_output_aborted())
			break;
	}
}

/*
 * void
 * print_lwp_vnlog(vnode_t *vp, lwp_t *lwp)
 *	Print contents of the vnode vp for the specified lwp. If a vp is also
 *	specified, then restrict printout to the vnode.
 *
 * Calling/Exit State:
 *	Intended for use from a kernel debugger.
 */
void
print_lwp_vnlog(const lwp_t *lwp, const vnode_t *vp)
{
	lwp_vn_hist_t	*logp = &lwp->l_vn_log;
	lwp_vn_rec_t	*vnrp;
	ulong_t last_stamp, diff;
	char digit[9];
	int i, j;
	char c, *p;
	int last;
	int cursor = logp->lvi_cursor;

	debug_printf("TIME    VP       count   soft\n"
		     "----    --       ----    ----\n");

	last = cursor - 1;
	if (last < 0)
		last += VN_LWP_HIST_SIZE;
	last_stamp = logp->lvi_rec[last].lvr_stamp;

	for (j = 0; j < VN_LWP_HIST_SIZE; ++j) {
		vnrp = &logp->lvi_rec[cursor];
		if (++cursor == VN_LWP_HIST_SIZE)
			cursor = 0;
		if (vnrp->lvr_service == NULL)
			continue;
		if (vp != NULL && vnrp->lvr_vp != vp)
			continue;
		diff = last_stamp - vnrp->lvr_stamp;
		p = &digit[8];
		*p-- = '\0';
		diff /= 100;
		for (i = 1; i <= 6 || diff != 0; ++i) {
			if (i == 5)
				*p-- = '.';
			else {
				*p-- = (diff % 10) + '0';
				diff /= 10;
			}
		}
		debug_printf("-%s %lx %ld %ld %s from line %d of file %s\n",
			     p + 1, (ulong_t)vnrp->lvr_vp,
			     (ulong_t)vnrp->lvr_count,
			     (ulong_t)vnrp->lvr_softcnt, vnrp->lvr_service,
			     vnrp->lvr_line, vnrp->lvr_file);
		if (debug_output_aborted())
			break;
	}
}

#endif /* _VNODE_HIST */
