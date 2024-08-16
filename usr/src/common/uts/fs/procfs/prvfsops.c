/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/procfs/prvfsops.c	1.17"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/fs_subr.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <util/var.h>
#include <fs/procfs/prdata.h>
#include <fs/mount.h>
#include <fs/fs_hier.h>

/*
 * Indicate to the configuration tools that the filesystem name for
 * this module is "proc", even though the module name is "procfs".
 */
char prextmodname[] = "proc";

struct vfs *procvfs;		/* Points to /proc vfs entry. */
dev_t procdev;

struct prnode prrootnode;
int prmounted = 0;		/* Set to 1 if /proc is mounted. */
STATIC int procfstype = 0;	/* Set in prinit */

/*
 * /proc VFS operations vector.
 */
STATIC int prmount(), prunmount(), prroot(), prstatvfs();

STATIC struct vfsops prvfsops = {
	prmount,
	prunmount,
	prroot,
	prstatvfs,
	fs_sync,
	(int (*)())fs_nosys,	/* vget */
	(int (*)())fs_nosys,	/* mountroot */
	(int (*)())fs_nosys,	/* setceiling */
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
 * void prinit(struct vfssw *vswp, int fstype)
 * 	One-time initialization.
 *
 * Calling/Exit State:
 *	None; called at system startup.
 */
void
prinit(struct vfssw *vswp, int fstype)
{
	procfstype = fstype;
	ASSERT(procfstype != 0);
	/*
	 * Associate VFS ops vector with this fstype.
	 */
	vswp->vsw_vfsops = &prvfsops;

	/*
	 * Assign a unique "device" number (used by stat(2)).
	 */
	if ((procdev = getudev()) == NODEV) {
		/*
		 *+ The system was unable to assign a unique "device
		 *+ number" to the /proc file system during system
		 *+ initialization.  Zero was used instead.
		 */
		cmn_err(CE_WARN, "prinit: can't get unique device number");
		procdev = 0;
	}
}


/*
 * int prmount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
 *
 * Calling/Exit State:
 *    The mount point mvp->v_lock is locked exclusive on entry and remains
 *    locked at exit. Holding this lock prevents new lookups into the
 *    file system the mount point is in (see the lookup code for more).
 */
/* ARGSUSED */
STATIC int
prmount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
{
	register struct vnode *vp;
	register struct prnode *pnp;

	if (pm_denied(cr, P_MOUNT))
		return EPERM;
	if (mvp->v_type != VDIR)
		return ENOTDIR;
	if (mvp->v_count > 1 || (mvp->v_flag & VROOT))
		return EBUSY;

	SLEEP_LOCK(&vfslist_lock, PRIVFS);

	/*
	 * Prevent duplicate mount.
	 */
	if (prmounted) {
		SLEEP_UNLOCK(&vfslist_lock);
		return EBUSY;
	}

	procvfs = vfsp;
	pnp = &prrootnode;
	vp = &pnp->pr_vnode;
	VN_INIT(vp, procvfs, VDIR, 0, VROOT, KM_SLEEP);
	vp->v_op = &prvnodeops;
	vp->v_lid = mvp->v_lid;
	vp->v_macflag |= VMAC_SUPPORT;
	vp->v_data = (caddr_t) pnp;

	pnp->pr_type = PR_PROCDIR;
	pnp->pr_mode = 0555;	/* read and search permissions */
	pnp->pr_ino = prino(0, 0, PR_PROCDIR);
	pnp->pr_flags = 0;
	pnp->pr_common = NULL;
	pnp->pr_parent = vp;	/* Self-referent */
	pnp->pr_index = 0;
	pnp->pr_files = NULL;
	pnp->pr_next = NULL;

	vfsp->vfs_fstype = procfstype;
	vfsp->vfs_data = NULL;
	vfsp->vfs_dev = procdev;
	vfsp->vfs_fsid.val[0] = procdev;
	vfsp->vfs_fsid.val[1] = procfstype;
	vfsp->vfs_bsize = 1024;

	prmounted = 1;
	vfs_add(mvp, vfsp, (vfsp->vfs_flag & VFS_RDONLY) ? MS_RDONLY : 0);
	SLEEP_UNLOCK(&vfslist_lock);

	return 0;
}


/*
 * int prunmount(vfs_t *vfsp, cred_t *cr)
 *
 * Calling/Exit State:
 *    The mount point vp->v_lock is locked exclusive on entry and remains
 *    locked at exit.
 */
STATIC int
prunmount(vfs_t *vfsp, cred_t *cr)
{
	register proc_t *p;
	int error;

	if (pm_denied(cr, P_MOUNT))
		return EPERM;
	/*
	 * Ensure that no /proc vnodes are in use.
	 */
	if (prrootnode.pr_vnode.v_count > 2)
		return EBUSY;

	error = 0;
	(void)RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);
	for (p = practive; p != NULL; p = p->p_next) {
		if (p->p_trace != NULL) {
			error = EBUSY;
			break;
		}
	}
	RW_UNLOCK(&proc_list_mutex, PLBASE);
	if (error)
		return error;

	VN_RELE(&prrootnode.pr_vnode);
	prmounted = 0;
	procvfs = NULL;

	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	vfs_remove(vfsp);
	SLEEP_UNLOCK(&vfslist_lock);
	return 0;
}


/*
 * int prroot(vfs_t *vfsp, vnode_t **vpp)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
prroot(vfs_t *vfsp, vnode_t **vpp)
{
	struct vnode *vp = &prrootnode.pr_vnode;

	VN_HOLD(vp);
	*vpp = vp;
	return 0;
}


/*
 * int prstatvfs(vfs_t *vfsp, struct statvfs *sp)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
prstatvfs(struct vfs *vfsp, struct statvfs *sp)
{
	int n;
	proc_t *p;
	int slot;

	ASSERT(getpl() == PLBASE);

	n = v.v_proc;
	for (slot = 0; (p = pid_next_entry(&slot)) != NULL; ++slot) {
		UNLOCK(&p->p_mutex, PLBASE);
		--n;
	}

	bzero(sp, sizeof *sp);
	sp->f_bsize	= 1024;
	sp->f_frsize	= 1024;
	sp->f_blocks	= 0;
	sp->f_bfree	= 0;
	sp->f_bavail	= 0;
	sp->f_files	= v.v_proc + 2;
	sp->f_ffree	= n;
	sp->f_favail	= n;
	sp->f_fsid	= vfsp->vfs_dev;
	strcpy(sp->f_basetype, vfssw[procfstype].vsw_name);
	sp->f_flag = vf_to_stf(vfsp->vfs_flag);
	sp->f_namemax = 42;
	strcpy(sp->f_fstr, "/proc");
	strcpy(&sp->f_fstr[6], "/proc");
	return 0;
}
