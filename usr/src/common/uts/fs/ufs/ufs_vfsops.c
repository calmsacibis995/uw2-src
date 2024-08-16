/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/ufs/ufs_vfsops.c	1.19"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <fs/fs_subr.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_inode.h>
#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/debug.h>
#include <util/var.h>
#include <util/cmn_err.h>
#include <util/mod/moddefs.h>

STATIC int ufs_load(void);
STATIC int ufs_unload(void);

extern void sfs_qtinit();
extern void sfs_deinitqt();

int	ufs_fsflags = 0;		/* to initialize vswp->vsw_flag */

MOD_FS_WRAPPER(ufs, ufs_load, ufs_unload, "Loadable UFS FS Type");

/*
 * ufs vfs operations.
 */
STATIC int ufs_mount(vfs_t *, vnode_t *, struct mounta *, cred_t *);
STATIC int ufs_mountroot(vfs_t *, enum whymountroot);
extern int sfs_unmount(vfs_t *, cred_t *);
extern int sfs_root(vfs_t *, vnode_t **);
extern int sfs_statvfs(vfs_t *, struct statvfs *);
extern int sfs_sync(vfs_t *, int, cred_t *);
extern int sfs_vget(vfs_t *, vnode_t **vpp, struct fid *);
extern int sfs_mount_cmn(vfs_t *, vnode_t *, struct mounta *, cred_t *, long);
extern int sfs_mountroot_cmn(vfs_t *, enum whymountroot, long);

vfsops_t ufs_vfsops = {
	ufs_mount,
	sfs_unmount,
	sfs_root,
	sfs_statvfs,
	sfs_sync,
	sfs_vget,
	ufs_mountroot,
	(int (*)())fs_nosys,	/* filler */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};


/*
 * int
 * ufsinit(struct vfssw *vswp)
 *	Initialize UFS file system type at system boot
 *
 * Calling/Exit State:
 *	The ufs file system type will be initialized by this call. Since
 *	the UFS implementation is shared with SFS, this routine does
 *	virtually nothing.
 */
int
ufsinit(struct vfssw *vswp)
{
	vswp->vsw_vfsops = &ufs_vfsops;
	/* 
	 *	initialize quotas.  These should only be utilized
	 * 	if UFS is installed.
	 */
	sfs_qtinit();
	return (0);
}


/*
 * STATIC int
 * ufs_load(void)
 *	Initialize UFS file system type when loading
 *
 * Calling/Exit State:
 *	The ufs file system type will be initialized by this call. Since
 *	the UFS implementation is shared with SFS, this routine does
 *	virtually nothing.
 */
STATIC int
ufs_load(void)
{
	struct	vfssw	*vswp;

	vswp = vfs_getvfssw("ufs");
	if (vswp == NULL) {
		/*
                 *+ SFS file system is not registered before
                 *+ attempting to load it.
                 */
                cmn_err(CE_NOTE, "!MOD: SFS is not registered.");
                return (EINVAL);
	}
	/* 
	 *	initialize quotas.  These should only be utilized
	 * 	if UFS is installed.
	 */
	sfs_qtinit();
	return(0);
}

/*
 * STATIC int
 * ufs_unload(void)
 *	Deinitialize UFS file system type when unloading
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 */
STATIC int
ufs_unload()
{
	sfs_deinitqt();
	return (0);
}

/*
 * ufs_mount(struct vfs *vfsp, struct vnode *mvp, struct mounta *uap, \
 *    struct cred *cr)
 *    Do fs specific portion of mount.
 *
 * Calling/Exit State:
 *    The mount point vp->v_lock is locked exclusive on entry and remains
 *    locked at exit. Holding this lock prevents new lookups into the
 *    file system the mount point is in (see the lookup code for more).
 *
 * Description:
 *    Wrapper for sfs_mount_cmn() which does the work.
 *
 */
STATIC int
ufs_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
{
	return(sfs_mount_cmn(vfsp, mvp, uap, cr, UFS_MAGIC));
}

/*
 * STATIC int
 * ufs_mountroot(struct vfs *vfsp, enum whymountroot why)
 *    Mount an ufs file system as root.
 *
 * Calling/Exit State:
 *    The global vfslist_lock is locked on entry and remain
 *    locked at exit.
 *
 * Description:
 *	Wrapper for sfs_mountroot_cmn().
 *
 */
STATIC int
ufs_mountroot(vfs_t *vfsp, enum whymountroot why)
{
        /*
         * Call the common SFS/UFS mountroot function.
         */
        return(sfs_mountroot_cmn(vfsp, why, UFS_MAGIC));
}
