/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/namefs/namevfs.c	1.20"
#ident	"$Header: $"

/*
 * This file supports the vfs operations for the NAMEFS file system.
 */

#include <util/types.h>
#include <util/engine.h>
#include <acc/dac/acl.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/statvfs.h>
#include <fs/buf.h>
#include <fs/fs_hier.h>
#include <fs/vfs.h>
#include <fs/dnlc.h>
#include <fs/vnode.h>
#include <fs/namefs/namehier.h>
#include <fs/namefs/namenode.h>
#include <io/strsubr.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/plocal.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#endif

/*
 * Define variables.
 */

dev_t		namedev;
struct vfs	*namevfsp;

/*
 *+ Lock protecting namenode data.
 */
LKINFO_DECL(nmlock_lkinfo, "NM::nm_lock", 0);

/*
 * Define the routines in this file.
 */
STATIC int	nm_mount(vfs_t *, vnode_t *, struct mounta *, cred_t *);
STATIC int	nm_unmount(vfs_t *, cred_t *);
STATIC int	nm_root(vfs_t *, vnode_t **);
STATIC int	nm_statvfs(vfs_t *, struct statvfs *);
STATIC int	nm_sync(vfs_t *, int, cred_t *);

/*
 * Define external variables and routines.
 */
lock_t		namelist_mutex;	/* protects namealloc list */

extern	struct namenode *namefind(vnode_t *, vnode_t *);
extern	void	nameinsert(struct namenode *);
extern	int	nameremove(struct namenode *, int);
extern	ino_t	nmgetid(void);
extern	void	nmclearid(ino_t);
extern	int	nm_unmountall(vnode_t *, cred_t *);
extern int nm_aclstore(struct namenode *, struct acl *, long);
extern struct vnodeops nm_vnodeops;

/*
 * Define the vfs operations vector.
 */
struct vfsops nmvfsops = {
	nm_mount,
        nm_unmount,
	nm_root,
	nm_statvfs,
	nm_sync,
	(int (*)())fs_nosys,    /* vget */
	(int (*)())fs_nosys,    /* mountroot */
	(int (*)())fs_nosys,    /* swapvp */
	(int (*)())fs_nosys,    /* filler */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
};

struct vfsops dummyvfsops = {
	(int (*)())fs_nosys,    /* mount */
        (int (*)())fs_nosys,    /* unmount */
	(int (*)())fs_nosys,    /* root */
	nm_statvfs,
	nm_sync,
	(int (*)())fs_nosys,    /* vget */
	(int (*)())fs_nosys,    /* mountroot */
	(int (*)())fs_nosys,    /* swapvp */
	(int (*)())fs_nosys,    /* filler */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
};


/*
 * STATIC int nm_mount(vfs_t *vfsp, vnode_t *mvp, 
 *			struct mounta *uap, cred_t *crp)
 * 	Mount a file descriptor onto the node in the file system.
 *
 * Calling/Exit State:
 *    The mount point vp->v_lock is locked exclusive on entry and
 *    remains locked at exit.
 *
 * Description:
 * 	Create a new vnode, update the attributes with info from the 
 * 	file descriptor and the mount point.  The mask, mode, uid, gid,
 * 	atime, mtime and ctime are taken from the mountpt.  Link count is
 * 	set to one, the file system id is namedev and nodeid is unique 
 * 	for each mounted object.  Other attributes are taken from mount point.
 * 	Make sure the process is owner with write permissions on mount point
 * 	or is privileged.
 * 	Hash the new vnode and return 0.
 * 	Upon entry to this routine, the file descriptor is in the 
 * 	fd field of a struct namefd.  Copy that structure from user
 * 	space and retrieve the file descriptor.
 */
STATIC int
nm_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *crp)
{
	struct namefd namefdp;
	vnode_t *filevp;		/* file descriptor vnode */
	file_t *fp;
	vnode_t *newvp;			/* vnode representing this mount */
	struct namenode *nodep;		/* namenode for this mount */
	vattr_t filevattr;		/* attributes of file dec.  */
	vattr_t *vattrp;		/* attributes of this mount */
        struct acl *tmpaclp;            /* temporary ACL buffer */
        long entries;			/* number of ACL entries for node */
        int tmp_entries;                /* temp for return from VOP_GETACL */
        long dentries;                  /* number of default ACL entries */
	int aentries;			/* number of non-default entries */
	ino_t nodeid;
	int error = 0;
	pl_t	pl;

	/*
	 * Get the file descriptor from user space.
	 * Make sure the file descriptor is valid and has an
	 * associated file pointer.
	 * If so, extract the vnode from the file pointer.
	 */
	if (uap->datalen != sizeof(struct namefd))
		return (EINVAL);
	if (copyin(uap->dataptr, (caddr_t) &namefdp, uap->datalen))
		return (EFAULT);
	if (error = getf(namefdp.fd, &fp))
		return (error);
	/*
	 * If the mount point already has something mounted
	 * on it, disallow this mount.  (This restriction may 
	 * be removed in a later release).
	 */
	if (mvp->v_flag & VROOT) {
		FTE_RELE(fp);
		return (EBUSY);
	}

	filevp = fp->f_vnode;
	if (filevp->v_type == VDIR) {
		FTE_RELE(fp);
		return (EINVAL);
	}

	/*
         * Make sure the file descriptor is not the root of some
         * file system.
         */
        if (filevp->v_flag & VROOT) {
		FTE_RELE(fp);
                return (EBUSY);
	}

	/* Must have MAC write access to mount point */
	if (MAC_VACCESS(mvp, VWRITE, crp)) {
		FTE_RELE(fp);
		return EACCES;
	}
#ifdef CC_PARTIAL
        MAC_ASSERT (mvp, MAC_SAME);
#endif
 
        /*
         * The mount point's level must be the same as the level of
         * the file descriptor.  This is a strict equality and no
         * privilege can be used as an override.
         */
        if (MAC_ACCESS(MACEQUAL, mvp->v_lid, filevp->v_lid)) {
		FTE_RELE(fp);
                return EACCES;
	}
#ifdef CC_PARTIAL
        MAC_ASSUME (filevp, MAC_SAME);
#endif
	
        /*
         * Create a reference and allocate a namenode to represent
         * this mount request.
         */
	nodep = (struct namenode *) kmem_zalloc(sizeof(struct namenode), 
		KM_SLEEP);

	vattrp = &nodep->nm_vattr;

	vattrp->va_mask = AT_ACLCNT | AT_STAT;

	if ((error = VOP_GETATTR(mvp, vattrp, 0, crp)) != 0)
		goto out;

	filevattr.va_mask = AT_STAT;

	if ((error = VOP_GETATTR(filevp, &filevattr, 0, crp)) != 0)
		goto out;
	/*
	 * Make sure the process is the owner of the mount point (or
	 * is privileged) and has write permission.
	 */
	if (vattrp->va_uid != crp->cr_uid && pm_denied(crp, P_OWNER)) {
		error = EPERM;
		goto out;
	}
	if (error = VOP_ACCESS(mvp, VWRITE, 0, crp))
		goto out;

	/*
	 * If the file descriptor has file/record locking, don't
	 * allow the mount to succeed.
	 */
	if (filevp->v_filocks) {
		error = EACCES;
		goto out;
	}
	/*
	 * Establish a unique node id to represent the mount.
	 * If can't, return error.
	 */
	if ((nodeid = nmgetid()) == 0) {
		error = ENOMEM;
		goto out;
	}

	/*
	 * Populate namenode with ACL from mount point.
	 * Only do work if there are stored ACL entries on the moint point.
	 * Allocate storage to hold mount point ACL entries and get the ACL.
	 * Store non-default ACL entries on namenode.
	 * Free temporary storage.
	 *
	 * Note that at this point, vattrp->va_aclcnt contains the acl count
	 * returned from the VOP_GETATTR on the mount point.  This *includes*
	 * the NACLBASE entries.  After we store the ACL on the namenode,
	 * vattrp->va_aclcnt will contain only the number of extra ACL
	 * entries, *excluding* the NACLBASE entries.  (If nm_aclstore()
	 * is called (there are extra ACL entries), it will set
	 * vattrp->va_aclcnt accordingly.  Otherwise (there are no extra ACL
	 * entries), we set vattrp->va_aclcnt to 0.)
	 */
	entries = vattrp->va_aclcnt;
retry:
        if (entries > NACLBASE) {
                tmpaclp = (struct acl *)kmem_alloc(entries * sizeof(struct acl),                                                KM_SLEEP);
                error = VOP_GETACL(mvp, entries, &dentries, tmpaclp, crp,
                                	&tmp_entries);
                if (error == 0) {
			if ((aentries = tmp_entries - dentries) > 0)
				error = nm_aclstore(nodep, tmpaclp, aentries);
			else
				vattrp->va_aclcnt = 0;
		} else if (error == ENOSPC) {
			/*
			 * Between the VOP_GETATTR and the VOP_GETACL,
			 * the ACL grew.  Get the new size and try again.
			 *
			 * Use a temporary vattr structure, instead of vattrp,
			 * in case the VOP_GETATTR call tinkers with fields
			 * other than the acl count.
			 */
			vattr_t tmpvattr;

			tmpvattr.va_mask = AT_ACLCNT;
			if ((error = VOP_GETATTR(mvp, &tmpvattr, 0, crp)) == 0) {
				kmem_free((caddr_t)tmpaclp,
					  entries * sizeof(struct acl));
				entries = tmpvattr.va_aclcnt;
				goto retry;
			}
		}
			
                kmem_free((caddr_t)tmpaclp, entries * sizeof(struct acl));
        } else
		vattrp->va_aclcnt = 0;
        if (error != 0)
                goto out;

	/*
	 * Initialize the namenode.
	 */
	RWSLEEP_INIT(&nodep->nm_lock, NM_HIER,
		&nmlock_lkinfo, KM_NOSLEEP);
	nodep->nm_mountpt = mvp;
	nodep->nm_filep = fp;
	nodep->nm_filevp = filevp;

	/*
	 * save f_flag in nm_flag for checks in fifo_access()
	 */
	if (fp->f_flag & FREAD)
		nodep->nm_flag |= NMREAD;
	if (fp->f_flag & FWRITE)
		nodep->nm_flag |= NMWRITE;

	/*
	 * Hold filevp and fp to be sure they are ours.
	 * filevp is held until nm_inactive or nm_unmount
	 * fp is held until nm_unmount
	 */
	VN_HOLD(filevp);
	FTE_HOLD(fp);
	if (filevp->v_stream) {
		pl = STREAM_LOCK(filevp->v_stream);
		filevp->v_stream->sd_flag |= STRMOUNT;
		STREAM_UNLOCK(filevp->v_stream, pl);
	}

	/*
	 * The attributes for the mounted file descriptor were initialized 
	 * above by applying VOP_GETATTR to the mount point.  Some of
	 * the fields of the attributes structure will be overwritten 
	 * by the attributes from the file descriptor.
	 */
	vattrp->va_type    = filevattr.va_type;
	vattrp->va_fsid    = namedev;
	vattrp->va_nodeid  = nodeid;
	vattrp->va_nlink   = 1;
	vattrp->va_size    = filevattr.va_size;
	vattrp->va_rdev    = filevattr.va_rdev;
	vattrp->va_blksize = filevattr.va_blksize;
	vattrp->va_nblocks = filevattr.va_nblocks;
	vattrp->va_vcode   = filevattr.va_vcode;

	/*
	 * Initialize the new vnode structure for the mounted file
	 * descriptor.
	 */
	newvp = NMTOV(nodep);
	VN_INIT(newvp, vfsp, filevp->v_type, filevp->v_rdev, 0, KM_SLEEP);
	newvp->v_flag = filevp->v_flag | VROOT | VNOMAP | VNOSWAP;

	/*
         * The level of the namenode is assigned the level of the
         * file descriptor node.
         */
        newvp->v_lid = filevp->v_lid;
	newvp->v_macflag |= VMAC_SUPPORT;
	if(filevp->v_macflag & VMAC_DOPEN)
		newvp->v_macflag |= VMAC_DOPEN;

	newvp->v_op = &nm_vnodeops;
	newvp->v_stream = filevp->v_stream;
	newvp->v_data = (caddr_t) nodep;

	/*
	 * Initialize the vfs structure.
	 */
	vfsp->vfs_fstype = namevfsp->vfs_fstype;
	vfsp->vfs_bsize = NMBSIZE;
	vfsp->vfs_fsid.val[0] = namedev;
	vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
	vfsp->vfs_data = (caddr_t) nodep;
	vfsp->vfs_dev = namedev;
	vfsp->vfs_bcount = 0;

	pl = LOCK(&namelist_mutex, PLNM);
	nameinsert(nodep);
	UNLOCK(&namelist_mutex, pl);
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	vfs_add(mvp, vfsp, uap->flags);
	SLEEP_UNLOCK(&vfslist_lock);
	FTE_RELE(fp);
	return (0);
out:
	kmem_free((caddr_t)nodep, sizeof(struct namenode));

	FTE_RELE(fp);
	return (error);
}

/*
 * STATIC int nm_unmount(vfs_t *vfsp, cred_t *crp)
 *
 *	Unmount a file descriptor from a node in the file system.
 *
 * Calling/Exit State:
 *	The mount point vp->v_lock is locked exclusive on entry and 
 *	remains locked at exit
 *
 * Description:
 * 	If the process is not the owner of the mount point and 
 *	is not privileged, the request is denied.
 *	Otherwise, remove the namenode from the hash list. 
 *	If the mounted file descriptor was that of a stream and this
 *	was the last mount of the stream, turn off the STRMOUNT flag.
 *	If the rootvp is referenced other than through the mount,
 *	nm_inactive will clean up.
 */
STATIC int
nm_unmount(vfs_t *vfsp, cred_t *crp)
{
	struct namenode *nodep = (struct namenode *) vfsp->vfs_data;
	vnode_t *vp;
	vnode_t *fvp;
	vattr_t mpattr;
	pl_t   pl_1;
	pl_t   pl_2;
	int    error;

	vp = NMTOV(nodep);
	fvp = nodep->nm_filevp;

	/* Must have MAC write access to name node */
	if (MAC_VACCESS(vp, VWRITE, crp))
		return EACCES;

	mpattr.va_mask = AT_UID;
	ASSERT(vfsp->vfs_vnodecovered != NULLVP);
	if ((error = VOP_GETATTR(vfsp->vfs_vnodecovered, &mpattr, 0, crp)) != 0)
		return error;
	if (mpattr.va_uid != crp->cr_uid && pm_denied(crp, P_OWNER))
		return (EPERM);

	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	dnlc_purge_vfsp(vfsp, 0);
	vfs_remove(vfsp);
	SLEEP_UNLOCK(&vfslist_lock);

	pl_1 = LOCK(&namelist_mutex, PLNM);
	nameremove(nodep, 0);
	nodep->nm_flag |= NMREMOVED;
	if (namefind(fvp, NULLVP) == NULL && fvp->v_stream) {
		pl_2 = STREAM_LOCK(fvp->v_stream);
		fvp->v_stream->sd_flag &= ~STRMOUNT;
		STREAM_UNLOCK(fvp->v_stream, pl_2);
	}
	UNLOCK(&namelist_mutex, pl_1);

        /* delete any ACL entries */
        if (nodep->nm_vattr.va_aclcnt > 0)
                kmem_free((caddr_t)nodep->nm_aclp,
                        nodep->nm_vattr.va_aclcnt * sizeof(struct acl));

	(void)closef(nodep->nm_filep);
	/*
	 * vp->v_count is decremented twice here because
	 * the reference posted by lookupname is not released
	 * in umount().
	 */
	VN_LOCK(vp);
	vp->v_count--;
	if (vp->v_count-- == 1) {
		VN_UNLOCK(vp);
		VN_RELE(fvp);
		nmclearid(nodep->nm_vattr.va_nodeid);
		RWSLEEP_DEINIT(&nodep->nm_lock);
		VN_DEINIT(vp);
		kmem_free((caddr_t) nodep, sizeof(struct namenode));
	} else {
		vp->v_flag &= ~VROOT;
		vp->v_vfsp = namevfsp;
		VN_UNLOCK(vp);
	}
	return (0);
}

/*
 * STATIC int nm_root(vfs_t *vfsp, vnode_t **vpp)
 * 	Create a reference to the root of a mounted file descriptor.
 *
 * Calling/Exit State:
 *	No locking assumption.
 *
 * Description:
 * 	This routine is called from lookupname() in the event a path
 * 	is being searched that has a mounted file descriptor in it.
 */
STATIC int
nm_root(vfs_t *vfsp, vnode_t **vpp)
{
	struct namenode *nodep = (struct namenode *) vfsp->vfs_data;
	vnode_t *vp = NMTOV(nodep);

	VN_HOLD(vp);
	*vpp = vp;
	return (0);
}

/*
 * STATIC int nm_statvfs(vfs_t *vfsp, struct statvfs *sp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 * 	Return in sp the status of this file system.
 *
 */
STATIC int
nm_statvfs(vfs_t *vfsp, struct statvfs *sp)
{
	bzero((caddr_t)sp, sizeof(*sp));
	sp->f_bsize	= NMBSIZE;
	sp->f_frsize	= NMFSIZE;
	sp->f_blocks	= 0;
	sp->f_bfree	= 0;
	sp->f_bavail	= 0;
	sp->f_files	= 0;
	sp->f_ffree	= 0;
	sp->f_favail	= 0;
	sp->f_fsid	= vfsp->vfs_dev;	
	strcpy(sp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name);
	sp->f_flag	= vf_to_stf(vfsp->vfs_flag);	
	sp->f_namemax	= 0;
	return (0);
}

/*
 * STATIC int nm_sync(vfs_t *vfsp, int flag, cred_t *crp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	Since this file system has no disk blocks of its own, apply
 * 	the VOP_FSYNC operation on the mounted file descriptor.
 */
STATIC int
nm_sync(vfs_t *vfsp, int flag, cred_t *crp)
{
	struct namenode *nodep;
	static int	flushtime = 0;

	if (vfsp == NULL)
		return (0);

	nodep = (struct namenode *) vfsp->vfs_data;
	if (flag & SYNC_CLOSE)
		return (nm_unmountall(nodep->nm_filevp, crp));

	/* check flush time */
	if (++flushtime < nm_tflush)
		return (0);
	/* reset flushtime before flush out */
	flushtime = 0;

	return (VOP_FSYNC(nodep->nm_filevp, crp));
}
