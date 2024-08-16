/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/fdfs/fdops.c	1.12"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/dirent.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/fs_hier.h>
#include <fs/pathname.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/fdfs/data.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <proc/resource.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <acc/dac/acl.h>


STATIC vnode_t 		fdfs_vroot;
STATIC int		fdfs_rdev;
STATIC int		fdfs_dev;
extern int 		fdfs_fstype;

STATIC int	fdfs_open(vnode_t **, int, cred_t *);
STATIC int      fdfs_close(vnode_t *, int, boolean_t , off_t, cred_t *);
STATIC int	fdfs_read(vnode_t *, uio_t *, int, cred_t *);
STATIC int	fdfs_access(vnode_t *, int, int, cred_t *);
STATIC int	fdfs_getattr(vnode_t *, vattr_t *, int, cred_t *);
STATIC int      fdfs_seek(vnode_t *, off_t, off_t *);
STATIC int	fdfs_lookup(vnode_t *, char *, vnode_t **, pathname_t *, int,
			    vnode_t *, cred_t *);
STATIC int	fdfs_create(vnode_t *, char *, vattr_t *, enum vcexcl,
                        int, vnode_t **, cred_t *);
STATIC int	fdfs_readdir(vnode_t *, uio_t *, cred_t *, int *);
STATIC void	fdfs_inactive(vnode_t *, cred_t *);
STATIC int 	fdfs_get(char *comp, vnode_t **);

STATIC vnodeops_t fdfs_vnodeops = {
	fdfs_open,
	fdfs_close,		/* close */
	fdfs_read,		/* read */
	(int (*)())fs_nosys,	/* write */
	(int (*)())fs_nosys,	/* ioctl */
	(int (*)())fs_nosys,	/* setfl */
	fdfs_getattr,
	(int (*)())fs_nosys,	/* setattr */
	fdfs_access,
	fdfs_lookup,
	fdfs_create,
	(int (*)())fs_nosys,	/* remove */
	(int (*)())fs_nosys,	/* link */
	(int (*)())fs_nosys,	/* rename */
	(int (*)())fs_nosys,	/* mkdir */
	(int (*)())fs_nosys,	/* rmdir */
	fdfs_readdir,
	(int (*)())fs_nosys,	/* symlink */
	(int (*)())fs_nosys,	/* readlink */
	(int (*)())fs_nosys,	/* fsync */
	fdfs_inactive,
	(void (*)())fs_nosys,	/* release */
	(int (*)())fs_nosys,	/* fid */
	fs_rwlock,		/* rwlock */
	fs_rwunlock,		/* rwunlock */
	fdfs_seek,		/* seek */
	fs_cmp,
	(int (*)())fs_nosys,	/* frlock */
	(int (*)())fs_nosys,	/* realvp */
	(int (*)())fs_nosys,	/* getpage */
	(int (*)())fs_nosys,	/* putpage */
	(int (*)())fs_nosys,	/* map */
	(int (*)())fs_nosys,	/* addmap */
	(int (*)())fs_nosys,	/* delmap */
	(int (*)())fs_nosys,	/* poll */
	fs_pathconf,
	(int (*)())fs_nosys,	/* getacl */
	(int (*)())fs_nosys,	/* setacl */
	(int (*)())fs_nosys,	/* setlevel */
	(int (*)())fs_nosys,	/* getdevstat */
	(int (*)())fs_nosys,	/* setdevstat */
	(int (*)())fs_nosys,	/* makemlk */
	(int (*)())fs_nosys,	/* testmld */
	(int (*)())fs_nosys,	/* stablestore */
	(int (*)())fs_nosys,	/* relstore */
	(int (*)())fs_nosys,	/* getpagelist */
	(int (*)())fs_nosys,	/* putpagelist */
	(int (*)())fs_nosys,	/* msgio */
	(int (*)())fs_nosys,	/* filler[4] ... */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};


/*
 * int
 * fdfs_open(vnode_t **vpp, int flag, cred_t *cr)
 *      Open a file.
 *
 * Calling/Exit State:
 *      No lock is held on entry and exit.
 *
 * Description:
 *	The fd-entry is created via copen().
 */
/* ARGSUSED */
STATIC int
fdfs_open(vnode_t **vpp, int mode, cred_t *cr)
{
	ASSERT(KS_HOLD0LOCKS());

	/*
	* No lock is needed here when setting the vnode flag because 
	* each open(2) on this file system has it's own unique vnode,
	* but in the future, if the fdfs is changed to support cache vnodes, 
	* the vnode spin lock should be added around the vnode flag setting.
	*/
	if ((*vpp)->v_type != VDIR){
		(*vpp)->v_flag |= VDUP;
	}
	return 0;
}

/*
 * int
 * fdfs_close(vnode_t *vp, int flag, boolean_t lastclose,
 *	 off_t offset, cred_t *cr
 *      Close a file.
 *
 * Calling/Exit State:
 *      No lock is held on entry and exit.
 *
 */
/* ARGSUSED */
STATIC int
fdfs_close(vnode_t *vp, int flag, boolean_t lastclose, off_t offset,
	 cred_t *cr)
{
	return(0);
}

/*
 * int
 * fdfs_read(vnode_t *vp, uio_t *uio, int ioflag, cred_t *cr)
 *      Transfer data from <vp> to the calling process's address
 *      space.
 *
 * Calling/Exit State:
 *	No lock is held on entry or exit.
 *
 * Description:
 *      A return value of 0 indicates success; otherwise a valid errno
 *      is returned. Errors returned directly by this routine are:
 *		ENOTDIR if it's not a directory
 *
 */
/* ARGSUSED */
STATIC int
fdfs_read(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	static struct fdfsdirect dotbuf[] = {
		{ FDFSROOTINO, "."  },
		{ FDFSROOTINO, ".." }
	};
	struct fdfsdirect dirbuf;
	register int i, n;
	int minfd, maxfd, modoff, error=0;
	int nentries;

	nentries = min(u.u_procp->p_fdtab.fdt_sizeused,
		      u.u_rlimits->rl_limits[RLIMIT_NOFILE].rlim_cur);
	if (vp->v_type != VDIR) {
		return ENOTDIR;
	}
	/*
	 * Fake up ".", "..", and the /dev/fd directory entries.
	 */
	if (uiop->uio_offset < 0 || uiop->uio_resid <= 0) {
		return EINVAL;
	}
	if ( uiop->uio_offset >= (nentries + 2) * FDFSSDSIZE) {
		return 0;
	}
	if (uiop->uio_offset < TFDFSDIRS) {
		error = uiomove((caddr_t)dotbuf + uiop->uio_offset,
		  min(uiop->uio_resid, TFDFSDIRS - uiop->uio_offset),
		  UIO_READ, uiop);
		if (uiop->uio_resid <= 0 || error) {
			return error;
		}
	}
	minfd = (uiop->uio_offset - TFDFSDIRS)/FDFSSDSIZE;
	maxfd = (uiop->uio_offset + uiop->uio_resid - 1)/FDFSSDSIZE;
	modoff = uiop->uio_offset % FDFSSDSIZE;
	for (i = 0; i < FDFSDIRSIZE; i++) {
		dirbuf.d_name[i] = '\0';
	}
	for (i = minfd; i < min(maxfd, nentries); i++) {
		n = i;
		dirbuf.d_ino = fdfstoi(n);
		numtos((long)n, dirbuf.d_name);
		error = uiomove((caddr_t)&dirbuf + modoff,
		  min(uiop->uio_resid, FDFSSDSIZE - modoff),
		    UIO_READ, uiop);
		if (uiop->uio_resid <= 0 || error) {
			return error;
		}
		modoff = 0;
	}

	return error;
}

/*
 * int
 * fdfs_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
 *      Return attributes for a vnode.
 *
 * Calling/Exit State:
 *      No locks are held on entry or exit.
 *
 * Description:
 *	This routine provides all attributes of a vnode.
 *	Note : The file system dev is kept in the vfs struct.
 */
/* ARGSUSED */
STATIC int
fdfs_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
{
	fd_table_t *fdtp;
	ASSERT(KS_HOLD0LOCKS());

	if (vp->v_type == VDIR) {
		vap->va_nlink = 2;
		fdtp = GET_FDT(u.u_procp);
		vap->va_size = (fdtp->fdt_sizeused + 2) * FDFSSDSIZE;
		vap->va_mode = 0555;
		vap->va_nodeid = FDFSROOTINO;
	} else {
		vap->va_nlink = 1;
		vap->va_size = 0;
		vap->va_mode = 0666;
		vap->va_nodeid = fdfstoi(vp->v_rdev);
	}
	vap->va_type = vp->v_type;
	vap->va_rdev = fdfs_dev;
	vap->va_blksize = 1024L;
	vap->va_nblocks = 0;
	/*
	* No lock is held when setting times because all time fields
	* are gettting the same value at the same time.
	*/
	vap->va_atime = vap->va_mtime = vap->va_ctime = hrestime;
	vap->va_uid = 0;
	vap->va_gid = 0;
	vap->va_fsid = fdfs_dev; 
	vap->va_vcode = 0;
	if (vap->va_mask & AT_ACLCNT) {
		vap->va_aclcnt = NACLBASE;
	}
	return 0;
}

/*
 * int
 * fdfs_access(vnode_t *vp, int mode, int flags, cred_t *cr)
 *      Access a file.
 *
 * Calling/Exit State:
 *      No lock is held on entry and exit.
 *
 * Description:
 *      No special actions required for fdfs files.
 */
/* ARGSUSED */
STATIC int
fdfs_access(vnode_t *vp, int mode, int flags, cred_t *cr)
{
	ASSERT(KS_HOLD0LOCKS());
	return 0;
}

/*
 * int
 * fdfs_lookup(vnode_t *dp, char *comp, vnode_t **vpp, pathname_t *pnp,
 *	       int lookup_flags, vnode_t *rootvp, cred_t *cr)
 *      Check whether a given directory contains a file named <comp>.
 *
 * Calling/Exit State:
 *      No locks on entry or exit.
 *
 * Description:
 *      We treat null components as a synonym for the directory being
 *      searched. In this case and "." and "..", merely increment the
 *	directory's reference count and return.
 *	For all other cases, allocate a file via fdfs_get().
 */
/* ARGSUSED */
STATIC int
fdfs_lookup(vnode_t *dp, char *comp, vnode_t **vpp, pathname_t *pnp,
	    int lookup_flags, vnode_t *rootvp, cred_t *cr)
{
	ASSERT(KS_HOLD0LOCKS());

	if (comp[0] == 0 || strcmp(comp, ".") == 0 || strcmp(comp, "..") == 0) {
		VN_HOLD(dp);
		*vpp = dp;
		return 0;
	}
	return fdfs_get(comp, vpp);
}

/*
 * fdfs_create(vnode_t *dvp, char *name, vattr_t *vap, enum vcexcl excl,
 *           int mode, vnode_t **vpp, cred_t *cr)
 *      Create a file <name> in a given directory <dvp>.
 *
 * Calling/Exit State:
 *      No locks are held on entry and on exit.
 *
 * Description:
 *	File is created via fdfs_get().
 */
/* ARGSUSED */
STATIC int
fdfs_create(vnode_t *dvp, char *comp, vattr_t *vap, enum vcexcl excl,
	int mode, vnode_t **vpp, cred_t *cr)
{
	ASSERT(KS_HOLD0LOCKS());

	return fdfs_get(comp, vpp);
}

/*
 * int
 * fdfs_readdir(vnode_t *vp, uio_t *uiop, cred_t *fcr, int *eofp)
 *      Read from a directory.
 *
 * Calling/Exit State:
 *      A return value of not -1 indicates success; otherwise a valid
 *      errno is returned. Errnos returned directly by this routine
 *      are:
 *              EINVAL if offset is negative or not on directory entry
 *		 boundary. 
 *              EINVAL if no entries has been returned.
 *		EFAULT if user address is invalid.
 *
 *      On success, an <*eofp> value of 1 indicates that end-of-file
 *      has been reached, i.e., there are no more directory entries
 *      that may be read.
 */
/* ARGSUSED */
STATIC int
fdfs_readdir(vnode_t *vp, uio_t *uiop, cred_t *cr, int *eofp)
{
	/* bp holds one dirent structure */
	ulong bp[round(sizeof(dirent_t)-1+FDFSNSIZE+1)/4];
	dirent_t *dirent = (dirent_t *)bp;
	int reclen, nentries;
	register int i, n;
	int oresid, dsize, error;
	off_t off;

	if (uiop->uio_offset < 0 || uiop->uio_resid <= 0
	  || (uiop->uio_offset % FDFSSDSIZE) != 0) {
		return EINVAL;
	}

	dsize = (char *)dirent->d_name - (char *)dirent;
	oresid = uiop->uio_resid;
	nentries = min(u.u_procp->p_fdtab.fdt_sizeused,
		      u.u_rlimits->rl_limits[RLIMIT_NOFILE].rlim_cur);

	off = 0;
	for (; uiop->uio_resid > 0; uiop->uio_offset = off + FDFSSDSIZE) {
		if ((off = uiop->uio_offset) == 0) {	/* "." */
			dirent->d_ino = FDFSROOTINO;
			dirent->d_name[0] = '.';
			dirent->d_name[1] = '\0';
			reclen = dsize+1+1;
		} else if (off == FDFSSDSIZE) {		/* ".." */
			dirent->d_ino = FDFSROOTINO;
			dirent->d_name[0] = '.';
			dirent->d_name[1] = '.';
			dirent->d_name[2] = '\0';
			reclen = dsize+2+1;
		} else {
			/*
			 * Return entries corresponding to the allowable
			 * number of file descriptors for this process.
			 */
			if ((n = (off-2*FDFSSDSIZE)/FDFSSDSIZE) >= nentries)
				break;
			dirent->d_ino = fdfstoi(n);
			numtos((long)n, dirent->d_name);
			reclen = dsize + strlen(dirent->d_name) + 1;
		}
		dirent->d_off = uiop->uio_offset + FDFSSDSIZE;
		/*
		 * Pad to nearest word boundary (if necessary).
		 */
		for (i = reclen; i < round(reclen); i++){
			dirent->d_name[i-dsize] = '\0';
		}
		dirent->d_reclen = reclen = round(reclen);
		if (reclen > uiop->uio_resid) {
			/*
			 * Error if no entries have been returned yet.
			 */
			if (uiop->uio_resid == oresid) {
				return EINVAL;
			}
			break;
		}
		/*
		 * uiomove() updates both resid and offset by the same
		 * amount.  But we want offset to change in increments
		 * of FDFSSDSIZE, which is different from the number of bytes
		 * being returned to the user.  So we set uio_offset
		 * separately, ignoring what uiomove() does.
		 */
		error = uiomove((caddr_t) dirent, reclen, UIO_READ, uiop);
		if (error) {
			return EFAULT;
		}
	}
	if (eofp) {
	    *eofp = ((uiop->uio_offset-TFDFSDIRS)/FDFSSDSIZE >= nentries);
	}
	return 0;
}

/*
 * void fdfs_inactive(vnode_t *vp, cred_t *cr)
 *	Free memory of an inactive file.
 *
 * Calling/Exit State:
 *  	No lock is held on entry or exit.
 *
 * Description:
 *	If the inactive vnode is a directory no performance is required.
 *	It ensures that it is not racing with another lwp that is doing
 *      a lookup (in which case it returns after releasing appropriate
 *      locks). If there is no such race, it deallocates the file node.
 */
/* ARGSUSED */
STATIC void
fdfs_inactive(vnode_t *vp, cred_t *cr)
{

	ASSERT(KS_HOLD0LOCKS());

	ASSERT(vp->v_count >= 0);

	if (vp->v_type == VDIR) {
		return;
	}
	/* 
	* There is no race condition between iget()/inactive() 
	* in this file system, therefore no need to check the vnode count.
	* In the future, if the fdfs is changed to support I/O operations 
	* this portion of code needs to change to protect the vnode
	* identity when deactivating inodes.
	*/
	VN_DEINIT(vp);
	kmem_free((caddr_t)vp, sizeof(vnode_t));
}

/*
 * int fdfs_get(char *comp, vnode_t **vpp)
 *	Construct a vnode for the file.
 *	Common code for fdfs_create() and fdfs_lookup(). Check name, allocate
 *      a vnode, initialize the vnode, and make the device.
 *
 * Calling/Exit State:
 *	No locks are held on entry and on exit.
 *
 * Description:	
 *	Errors returned directly by this routines are:
 *		ENOENT if name is not in the range of 0-9.
 */
STATIC int
fdfs_get(char *comp, vnode_t **vpp)
{
	register int n = 0;
	register vnode_t *vp;

	while (*comp) {
		if (*comp < '0' || *comp > '9') {
			return ENOENT;
		}
		n = 10 * n + *comp++ - '0';
	}
	/* 
	 * Multiple allocations of a vnode is allowed here since
	 * it will be released after the function returned to the caller
	 */ 
	vp = kmem_zalloc(sizeof(vnode_t), KM_SLEEP);
	ASSERT(vp != NULL);

	/*
         * Initialize the vnode.
         */
        VN_INIT(vp, fdfs_vroot.v_vfsp, VCHR, n,
			VNOMAP|VNOMOUNT|VNOSWAP, KM_SLEEP);

	vp->v_op = &fdfs_vnodeops;
	vp->v_data = NULL;
	*vpp = vp;
	return 0;
}

STATIC int	fdfs_mount(), fdfs_unmount(), fdfs_root(), fdfs_statvfs();

STATIC struct vfsops fdfs_vfsops = {
	fdfs_mount,
	fdfs_unmount,
	fdfs_root,
	fdfs_statvfs,
	fs_sync,
	(int (*)())fs_nosys,	/* vget */
	(int (*)())fs_nosys,	/* mountroot */
	(int (*)())fs_nosys,	/* not used */
	(int (*)())fs_nosys,	/* setceiling */
	(int (*)())fs_nosys,	/* filller */
	(int (*)())fs_nosys,	
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
};

/*
 * int fdfs_init(vfssw_t *vswp, int fstype)
 * 	FDFS file system one-time initialization. Called at system startup time.
 *
 * Calling/Exit State:
 *	No locks is needed at system start up.
 *
 * Description:
 *	Initialize the vfs operation, construct the root vnode 
 *	and make devices.
 */
int
fdfs_init(vfssw_t *vswp, int fstype)
{

	/*
	 * Associate VFS ops vector with this fstype.
	 */
	vswp->vsw_vfsops = &fdfs_vfsops;

	/*
	 * Assign unique "device" number for dev (reported by stat(2)).
	 */
	fdfs_dev = getudev();
	if (fdfs_dev == NODEV) {
		/*
		 *+ All numbers are in used.
		 *+ The system cannot get a unique device number.
		 */
		cmn_err(CE_WARN, "fdfsinit: can't get unique device number");
		if (fdfs_dev == -1)
			fdfs_dev = 0;
	}
	fdfs_fstype = fstype;
	/* 
	 * Construct the root vnode
	 */
	VN_INIT(&fdfs_vroot, (vfs_t *)0, VDIR, fdfs_dev, 0, KM_SLEEP); 
	/* 
	* The root vnode count is reset back to 0 here because
	* it's used to protect multiple mount operations. 
	*/
	fdfs_vroot.v_count = 0;
	return 0;
}

/*
 * int fdfs_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
 *    Mount fdfs filesystem.
 *
 * Calling/Exit State:
 *	The mount point vp->v_lock is locked exclusive on entry and remains
 *      locked at exit. Holding this lock prevents new lookups into the
 *      file system the mount point is in (see the lookup code for more).
 *
 * Description:
 *    We insure the moint point is 'mountable', i.e., is a directory
 *    that is neither currently mounted on or referenced, and that
 *    the file system to mount is OK (block special file).
 *    The vnode spin lock is held to prevents multiple mounts.
 *
/* ARGSUSED */
STATIC int
fdfs_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
{
	register struct vnode *vp;

	if (pm_denied(cr, P_MOUNT)) {
		return EPERM;
	}
	if (mvp->v_type != VDIR) {
		return ENOTDIR;
	}

 	if (mvp->v_count > 1 || (mvp->v_flag & VROOT)) {
		return EBUSY;
	}
	vp = &fdfs_vroot;

	/* 
	* Lock the root vnode to prevent the multiple mounts 
	*/

	VN_LOCK(vp);
	if (vp->v_count > 0) {
		VN_UNLOCK(vp);
		return EBUSY;
	}
	vp->v_count = 1;
	vp->v_flag |= VROOT;
	VN_UNLOCK(vp);
	/* 
	* Initialize the rest of the root vnode 
	*/
	vp->v_vfsp = vfsp;
	vp->v_op = &fdfs_vnodeops;
	vp->v_data = NULL;

	/* Initialize the vfs data */

	vfsp->vfs_data = NULL;
	vfsp->vfs_fstype = fdfs_fstype;
	vfsp->vfs_dev = fdfs_dev;
	vfsp->vfs_fsid.val[0] = fdfs_dev;
	vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
	vfsp->vfs_bsize = 1024;

	SLEEP_LOCK(&vfslist_lock, PRIVFS);
        vfs_add(mvp, vfsp, uap->flags);
        SLEEP_UNLOCK(&vfslist_lock);

	return 0;
}

/*
 * int fdfs_unmount(vfs_t *vfsp, cred_t *cr)
 *    Do the fs specific portion of the unmount.
 *
 * Calling/Exit State:
 *	The mount point vp->v_lock is locked exclusive on entry and remains
 *      locked at exit. 
 *
 * Description:
 *	The vnode spin lock is used to prevent multiple umounts of the fs.
 *
 */
/* ARGSUSED */
STATIC int
fdfs_unmount(vfs_t *vfsp, cred_t *cr)
{

	ASSERT(vfsp->vfs_op == &fdfs_vfsops);

	if (pm_denied(cr, P_MOUNT)) {
		return EPERM;
	}
	VN_LOCK(&fdfs_vroot);

	if (fdfs_vroot.v_count > 2){
		VN_UNLOCK(&fdfs_vroot);
		return EBUSY;
	}

	fdfs_vroot.v_count = 0;
	VN_UNLOCK(&fdfs_vroot);

	/* Remove vfs from vfs list. */
        SLEEP_LOCK(&vfslist_lock, PRIVFS);
        vfs_remove(vfsp);
        SLEEP_UNLOCK(&vfslist_lock);

	return 0;
}

/*
 * int
 * fdfs_root(vfs_t *vfsp, vnode_t **vpp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
fdfs_root(vfs_t *vfsp, vnode_t **vpp)
{
	struct vnode *vp = &fdfs_vroot;

	VN_HOLD(vp);
	*vpp = vp;
	return 0;
}

/*
 * int
 * fdfs_statvfs(vfs_t *vfsp, struct statvfs *sp)
 *      Return file system information.
 *
 * Calling/Exit State:
 *      No relevant locking on entry or exit.
 */
STATIC int
fdfs_statvfs(vfs_t *vfsp, statvfs_t *sp)
{
	bzero((caddr_t)sp, sizeof(*sp));
	sp->f_bsize = 1024;
	sp->f_frsize = 1024;
	sp->f_blocks = 0;
	sp->f_bfree = 0;
	sp->f_bavail = 0;
	sp->f_files = min(u.u_procp->p_fdtab.fdt_sizeused,
		 u.u_rlimits->rl_limits[RLIMIT_NOFILE].rlim_cur) + 2;
	sp->f_ffree = 0;
	sp->f_favail = 0;
	sp->f_fsid = vfsp->vfs_dev;
	strcpy(sp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name);
	sp->f_flag = vf_to_stf(vfsp->vfs_flag);
	sp->f_namemax = FDFSNSIZE;
	strcpy(sp->f_fstr, "/dev/fd");
	strcpy(&sp->f_fstr[8], "/dev/fd");
	return 0;
}

/*
 * int
 * fdfs_seek(vnode_t *vp, off_t ooff, off_t *noffp);
 *	No-ops.	
 *
 * Calling/Exit State:
 *      No relevant locking on entry or exit.
 */
/* ARGSUSED */
STATIC int
fdfs_seek(vnode_t *vp, off_t ooff, off_t *noffp)
{
	return(0);
}
