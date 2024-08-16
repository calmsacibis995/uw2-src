/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/profs/profs_vfsops.c	1.11"
#ident	"$Header: $"

/*            processorfs_vfsops.c          */

#include <util/types.h>
#include <util/debug.h>
#include <fs/vfs.h>
#include <svc/errno.h>
#include <fs/vnode.h>
#include <fs/profs/profs_data.h>
#include <fs/statvfs.h>
#include <acc/priv/privilege.h>
#include <util/engine.h>
#include <util/cmn_err.h>
#include <util/sysmacros.h>
#include <mem/kmem.h>
#include <fs/mount.h>
#include <util/ghier.h>
#include <fs/fs_hier.h>
#include <util/mod/moddefs.h>

STATIC int pro_load(void);
STATIC int pro_unload(void);
int pro_fsflags = 0;
MOD_FS_WRAPPER(pro, pro_load, pro_unload, "Loadable Processor FS");

extern int fs_nosys();
extern dev_t getudev(void);

#define PROREFCNT_HIER FS_HIER_BASE
#define PROFS_PTR_HIER FS_HIER_BASE + 1

struct vfs *provfs;	/* points to /processor vfs entry */
struct pronode prorootnode;  /* root node is statically allocated */
struct prorefcnt prorefcnt;
 
struct pronode *processorp;  /* pointer to root vnode */
struct pronode *ctlp;	/* pointer to ctl file vnode */
struct pronode *processoridp[32]; /* pointers to processor-id file vnodes */
lid_t	prorootlid;
dev_t processordev;
lock_t processorfs_ptr_lck;
sleep_t processorfs_mount_lock;

/*
 *+ This is the global reference count lock.
 */
LKINFO_DECL(prorefcnt_lock_buf, "FS:Processorfs:ref cnt lock", 0);
 
/*
 *+ This is the global pointer (spin) lock.
 */
LKINFO_DECL(processorfs_ptr_lck_buf, "FS:Processorfs:global pointer lock", 0);
 
/*
 *+ This is the root node R/W sleep lock.
 */
LKINFO_DECL(prorootnode_lock_buf, "FS:Processorfs:root r/w lock", 0);
 
/*
 *+ This is the mount/umount sleep lock.
 */
LKINFO_DECL(processorfs_mount_lock_buf, "FS:Processorfs:mount/umount sleep lock", 0);
 
/*
 * processorfs VFS operations vector.
 */

void	proinit();
STATIC int	promount(struct vfs *, struct vnode *, struct mounta *,
				struct cred *);
STATIC int	prounmount(struct vfs *, struct cred *);
STATIC int	proroot(struct vfs *, struct vnode **);
STATIC int	prostatvfs(struct vfs *, struct statvfs *);


struct vfsops pro_vfsops = {
	promount,
	prounmount,
	proroot,
	prostatvfs,
	(int (*)())fs_nosys,	/* sync */
	(int (*)())fs_nosys,	/* vget */
	(int (*)())fs_nosys,	/* mountroot */
	(int (*)())fs_nosys,	/* swapvp */
	(int (*)())fs_nosys,	/* filler */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
};


/*
 * STATIC int
 * pro_load(void)
 *	Initialize the inode hash table, the inode table lock
 *	and the synchronized variable.	
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 */
STATIC int
pro_load(void)
{
	struct	vfssw	*vswp;

	vswp = vfs_getvfssw("processorfs");
	if (vswp == NULL) {
		/*
                 *+ processorfs file system is not registered before
                 *+ attempting to load it.
                 */
                cmn_err(CE_NOTE, "!MOD: processorfs is not registered.");
                return (EINVAL);
	}
	proinit(vswp);
	return (0);
}

/*
 * STATIC int
 * pro_unload(void)
 *	Deinitialize the inode table lock
 *	and the synchronized variable.	
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 */
STATIC int
pro_unload(void)
{
	VN_DEINIT(&prorootnode.pro_vnode);
	RWSLEEP_DEINIT(&prorootnode.pro_lock);
	SLEEP_DEINIT(&processorfs_mount_lock);
	LOCK_DEINIT(&processorfs_ptr_lck);
	LOCK_DEINIT(&prorefcnt.lock);
	return(0);
}


/*
 * void
 * proinit(struct vfssw *vswp, int fstype)
 *
 *	File system initialization routine.
 * Calling/Exit State:
 *
 *	None.
 *
 * Description:
 *
 *	This routine performs all the file
 *	system initializations such as
 *	initializations of various locks,
 *	and gets a unique device number to
 *	be associated with this file system.
 */
void
proinit(struct vfssw *vswp)
{
	int i;

	vswp->vsw_vfsops = &pro_vfsops;

	/*
	 * Initialize the global reference count lock.
	 */
	LOCK_INIT(&prorefcnt.lock, PROREFCNT_HIER, PLHI,
		&prorefcnt_lock_buf, KM_SLEEP);
	prorefcnt.count = 0;	/* initialize count to 0 */

	/*
	 * Initialize the global pointer lock.
	 */
	LOCK_INIT(&processorfs_ptr_lck, PROFS_PTR_HIER, PLHI,
		&processorfs_ptr_lck_buf, KM_SLEEP);
	/*
	 * Initialize the mount/umount sleep lock.
	 */
	SLEEP_INIT(&processorfs_mount_lock, (uchar_t) 0,
		&processorfs_mount_lock_buf, KM_SLEEP);
	/*
	 * Initialize root pronode R/W sleep lock.
	 */
	RWSLEEP_INIT(&prorootnode.pro_lock, (uchar_t) 0, 
		&prorootnode_lock_buf, KM_SLEEP);

	/*
	 * Initialize the other locks in generic vnode of root node.
	 */
	VN_INIT(&prorootnode.pro_vnode, (vfs_t *) 0, VDIR, (dev_t) 0, 0,
								KM_SLEEP);

	/*
	 * Initialize all global pointers.
	 */
	ctlp = NULL;
	for (i = 0; i < 32; i++)
		processoridp[i] = NULL;

	/*
	 * Assign a unique "device" number (used by stat(2)).
	 */
	if ((processordev = getudev()) == NODEV) {
		/*
		 *+ Could not get a unique device number for processor fs.
		 */
		cmn_err(CE_WARN, "proinit: cannot get unique device number");
		processordev = 0;
	}

	return;
}


/*
 * promount(struct vfs *vfsp, struct vnode *mvp, struct mounta *uap, 
 *    struct cred *cr)
 *
 *	Do fs specific portion of mount.
 *
 * Calling/Exit State:
 *
 *	The mount point vp->v_lock is locked exclusive on entry and remains
 *	locked at exit. Holding this lock prevents new lookups into the
 *	file system the mount point is in (see the lookup code for more).
 *
 * Description:
 *
 *	We insure the moint point is 'mountable', i.e., is a directory
 *	that is neither currently mounted on or referenced, and that
 *	the file system to mount is OK (block special file).
 */

/* ARGSUSED */
STATIC int
promount(struct vfs *vfsp, struct vnode *mvp, struct mounta *uap,
	cred_t *cr)
{
	struct vnode *vp;
	struct pronode *pnp;

	SLEEP_LOCK(&processorfs_mount_lock, PRINOD);

	if (pm_denied(cr, P_MOUNT)) {
		SLEEP_UNLOCK(&processorfs_mount_lock);
		return EPERM;
	}
	if (mvp->v_type != VDIR){
		SLEEP_UNLOCK(&processorfs_mount_lock);
		return ENOTDIR;
	}
	if (mvp->v_count > 1 || (mvp->v_flag & VROOT)) {
		SLEEP_UNLOCK(&processorfs_mount_lock);
		return EBUSY;
	}


	/*
	 * Prevent duplicate mount.
	 */
	if (processorp != NULL) {
		SLEEP_UNLOCK(&processorfs_mount_lock);
		return EBUSY;
	}

	pnp = &prorootnode;
	vp = &pnp->pro_vnode;
	vp->v_vfsp = vfsp;
	vp->v_vfsmountedhere = NULL;
	vp->v_op = &provnodeops;
	vp->v_lid = mvp->v_lid;
	prorootlid = mvp->v_lid;
	vp->v_macflag |= VMAC_SUPPORT;
	vp->v_type = VDIR;
	vp->v_data = (caddr_t) pnp;
	vp->v_flag |= VROOT;
	pnp->pro_mode = 0555;	/* read and search permissions */
	pnp->pro_filetype = PROCESSOR;
	vfsp->vfs_data = NULL;
	vfsp->vfs_dev = processordev;
	vfsp->vfs_fsid.val[0] = processordev;
	vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
	vfsp->vfs_bsize = MAXBSIZE;
	provfs = vfsp;
 
	/* set root vnode pointer */
	processorp = &prorootnode;

	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	vfs_add(mvp, vfsp, (vfsp->vfs_flag & VFS_RDONLY) ? MS_RDONLY : 0);
	SLEEP_UNLOCK(&vfslist_lock);

	SLEEP_UNLOCK(&processorfs_mount_lock);

	return 0;
}


/*
 * prounmount(struct vfs *vfsp, cred_t *cr)
 *
 *	Do the fs specific portion of the unmount.
 *
 * Calling/Exit State:
 *
 *	The mount point vp->v_lock is locked exclusive on entry and remains
 *	locked at exit.
 *
 * Description:
 *
 *	Ensures that there are no active file system nodes in existence.
 *	Also, checks to see if there is any active reference to the
 *	root node by checking its v_count field.
 *	It sets the processorp pointer to NULL to indicate this file system
 *	is unmounted.
 */
/* ARGSUSED */
STATIC int
prounmount(struct vfs *vfsp, cred_t *cr)
{
	pl_t pl;

	SLEEP_LOCK(&processorfs_mount_lock, PRINOD);

	if (pm_denied(cr, P_MOUNT)) {
		SLEEP_UNLOCK(&processorfs_mount_lock);
		return EPERM;
	}
	/*
	 * Ensure that no processorfs vnodes are in use.
	 */

	pl = LOCK(&prorefcnt.lock, PLHI);
	if (prorefcnt.count > 0 ||
	    prorootnode.pro_vnode.v_count > 2) {
		UNLOCK(&prorefcnt.lock, pl);
		SLEEP_UNLOCK(&processorfs_mount_lock);
		return EBUSY;
	}
	processorp = NULL; /* indicates file system is unmounted */
	prorootnode.pro_vnode.v_count = 1;
	provfs = NULL;
	
	UNLOCK(&prorefcnt.lock, pl);
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	vfs_remove(vfsp);
	SLEEP_UNLOCK(&vfslist_lock);
	SLEEP_UNLOCK(&processorfs_mount_lock);
	return 0;
}


/*
 * int
 * proroot(struct vfs *vfsp, struct vnode **vpp)
 *
 *	Return the address of the root vnode.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Description:
 *
 *	This function simply returns the address of
 *	the generic vnode associated with the root
 *	node of the file system.
 */
/* ARGSUSED */
STATIC int
proroot(struct vfs *vfsp, struct vnode **vpp)
{
	struct vnode *vp = &prorootnode.pro_vnode;

	VN_HOLD(vp);
	*vpp = vp;
	return 0;
}


/*
 * int
 * prostatvfs(struct vfs *vfsp, struct statvfs *sp)
 *
 *      Return file system specifics for a given
 *      file system.
 *
 * Calling/Exit State:
 *
 *      No relevant locking on entry or exit.
 *
 * Description:
 *
 *	This function returns the information about
 *	the file system such as the block size,
 *	number of blocks in the file system, etc.
 *	For a pseudo file system such as this
 *	one, some of the values are simply chosen
 *	to be reasonable.
 */

STATIC int
prostatvfs(struct vfs *vfsp, struct statvfs *sp)
{

	bzero((caddr_t)sp, sizeof(*sp));
	sp->f_bsize	= MAXBSIZE;
	sp->f_frsize	= MAXBSIZE;
	sp->f_blocks	= 0;
	sp->f_bfree	= 0;
	sp->f_bavail	= 0;
 
	sp->f_files	= Nengine + 1; /* +1 for ctl */
 
	sp->f_ffree	= 0;
	sp->f_favail	= 0;
	sp->f_fsid	= vfsp->vfs_dev;
	strcpy(sp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name);
	sp->f_flag = vf_to_stf(vfsp->vfs_flag);
	sp->f_namemax = PRONSIZE; 
	strcpy(sp->f_fstr, "processor");
	return 0;
}
