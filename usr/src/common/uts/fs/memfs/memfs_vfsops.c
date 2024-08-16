/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/memfs/memfs_vfsops.c	1.1.1.13"

#include <acc/priv/privilege.h>
#include <fs/fs_subr.h>
#include <fs/memfs/memfs.h>
#include <fs/memfs/memfs_hier.h>
#include <fs/memfs/memfs_mnode.h>
#include <fs/memfs/memfs_mkroot.h>
#include <fs/mount.h>
#include <fs/pathname.h>
#include <fs/stat.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/anon.h>
#include <mem/kmem.h>
#include <mem/memresv.h>
#include <mem/seg_kvn.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>


/*
 * memfs vfs operations.
 */
STATIC int memfs_mount(vfs_t *, vnode_t *, struct mounta *, cred_t *);
STATIC int memfs_unmount(vfs_t *, cred_t *);
STATIC int memfs_root(vfs_t *, vnode_t **);
STATIC int memfs_statvfs(vfs_t *, statvfs_t *);
STATIC int memfs_sync();
STATIC int memfs_vget(vfs_t *, vnode_t **, fid_t *);
STATIC int memfs_mountroot(vfs_t *, enum whymountroot);

extern int memfs_dirinit(mem_vfs_t *, mnode_t *, mnode_t *, cred_t *);
extern void memfs_kmemfree(mem_vfs_t *, void *, u_int);
extern void memfs_dirtrunc(mem_vfs_t *, mnode_t *, boolean_t);
extern int memfsfstype;
extern struct mnode *mnode_alloc(mem_vfs_t *, struct vattr *, struct cred *);
extern int memfs_mflush(vfs_t *);

extern void	*memfsmeta_plist;
extern size_t	memfsmeta_size;
extern memfs_image_t	*memfsroot_mp;
extern vaddr_t		memfsroot_fsp;

struct vfsops memfs_vfsops = {
	memfs_mount,
	memfs_unmount,
	memfs_root,
	memfs_statvfs,
	memfs_sync,
	memfs_vget,
	memfs_mountroot,
        (int (*)())fs_nosys,    /* filler[9] */
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
 * global statistics
 */
int memfs_swapmem = 0;	/* pages of swap space reserved for all memfs */
int memfs_kmemspace = 0;	/* bytes of kernel heap used by all memfs */
int memfs_swapalloc = 0;	/* approximate # of swap pages allocated to memfs */

#define	MEMFSFSMAXMOUNT	256
#define	MEMFSMAP		MEMFSFSMAXMOUNT/NBBY

STATIC LKINFO_DECL(memfs_mem_lkinfo, "MP:memfs:memfs_mem_contentslock", 0);

/*
 * int
 * memfs_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
 *
 * Calling/Exit State:
 *	The vnode rwlock of "mvp" is held exclusively on entry and
 *	remains locked at exit.
 *
 * Description:
 *	Mount a memfs on mount point "mvp".
 */
/*ARGSUSED*/
STATIC int
memfs_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
{
	mem_vfs_t *mem_vfsp = NULL;
	mnode_t *mp;
	struct pathname dpn;
	dev_t dev;
	int error = 0;
	char *data = uap->dataptr;
	int datalen = uap->datalen;
	struct memfs_args targs;
	struct vattr rattr;


	if (pm_denied(cr, P_MOUNT))  {
                return (EPERM);
        }

	if (mvp->v_type != VDIR) {
		return (ENOTDIR);
	}

	/*
	 * Get arguments
	 */
	if (datalen != 0) {
		if (datalen != sizeof (targs)) {
			return (EINVAL);
		} else {
			if (copyin(data, (caddr_t)&targs, sizeof (targs))) {
				return (EFAULT);
			}
		}
	} else {
		targs.swapmax = 0;
		targs.rootmode = 0775;
	}

	if (error = pn_get(uap->dir, UIO_USERSPACE, &dpn)) {
		return (error);
	}

	if ((dev = getudev()) == NODEV) {
                /*
                 * The system was unable to assign a unique "device
                 * number" to the memfs file system when it's mounted.
		 * Fail the mount.
                 */
		pn_free(&dpn);
		return (error);
        }

	mem_vfsp = (mem_vfs_t *)kmem_zalloc(sizeof (mem_vfs_t), KM_SLEEP);

	/*
	 * Set but don't bother entering the mutex
	 * (mem_vfs not on mount list yet)
	 */
	LOCK_INIT(&mem_vfsp->mem_contents, FS_MEMFS_HIER, FS_MEMFS_PL,
					&memfs_mem_lkinfo, KM_SLEEP);

	mem_vfsp->mem_vfsp = vfsp;
	mem_vfsp->mem_dev = dev;

	mem_vfsp->mem_mnomap = (struct mnode_map *)memfs_kmemalloc(mem_vfsp,
	    sizeof (struct mnode_map), KM_SLEEP);
	if (mem_vfsp->mem_mnomap == NULL) {
		error = ENOMEM;
		goto err;
	}

	/*
	 * nodes 0 and 1 on a file system are unused
	 */
	SETBIT(mem_vfsp->mem_mnomap->mmap_bits, 0);
	SETBIT(mem_vfsp->mem_mnomap->mmap_bits, 1);

	/*
	 * Initialize the pseudo generation number counter
	 */
	mem_vfsp->mem_gen = 0;

	vfsp->vfs_data = (caddr_t)mem_vfsp;
	vfsp->vfs_fstype = memfsfstype;
	vfsp->vfs_dev = mem_vfsp->mem_dev;
	vfsp->vfs_bsize = PAGESIZE;
	vfsp->vfs_fsid.val[0] = mem_vfsp->mem_dev;
	vfsp->vfs_fsid.val[1] = memfsfstype;
	mem_vfsp->mem_mntpath = (char *)memfs_kmemalloc(mem_vfsp,
					dpn.pn_pathlen + 1, KM_SLEEP);
	if (mem_vfsp->mem_mntpath == NULL) {
		error = ENOMEM;
		goto err;
	}
	strcpy(mem_vfsp->mem_mntpath, dpn.pn_path);

	/*
	 * allocate and initialize root mnode structure
	 */
	bzero(&rattr, sizeof (rattr));
	rattr.va_mode = (mode_t)(S_IFDIR | targs.rootmode);
	rattr.va_type = VDIR;
	rattr.va_rdev = 0;
	mp = mnode_alloc(mem_vfsp, &rattr, cr);
	if (mp == NULL) {
		error = ENOSPC;
		goto err;
	}
	mp->mno_uid = 2;
	mp->mno_gid = 2;
	MNODE_TO_VP(mp)->v_flag |= VROOT;
	if ((error = memfs_dirinit(mem_vfsp, mp, mp, cr))) {
		mnode_free(mem_vfsp, mp);
		goto err;
	}

	mem_vfsp->mem_rootnode = mp;

	/*
	 * mem_swapmax is set according to the mount arguments
	 * if any.  Otherwise, it is set to a maximum value.
	 */
	if (targs.swapmax)
		mem_vfsp->mem_swapmax = btopr(targs.swapmax);
	else
		mem_vfsp->mem_swapmax = INT_MAX;


	RWSLEEP_UNLOCK(&mp->mno_rwlock);
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	vfs_add(mvp, vfsp, uap->flags);
	SLEEP_UNLOCK(&vfslist_lock);
	pn_free(&dpn);
	return (0);
err:
	/*
	 * We had an error during the mount,
	 * so everything we've allocated must be freed.
	 */
	if (mem_vfsp && mem_vfsp->mem_mnomap) {
		memfs_kmemfree(mem_vfsp, (char *)mem_vfsp->mem_mnomap, sizeof (struct mnode_map));
	}
	if (mem_vfsp && mem_vfsp->mem_mntpath != NULL) {
		memfs_kmemfree(mem_vfsp, mem_vfsp->mem_mntpath, strlen(mem_vfsp->mem_mntpath));
	}
	if (mem_vfsp) {
		kmem_free((char *)mem_vfsp, sizeof (mem_vfs_t));
	}
	pn_free(&dpn);
	return (error);
}

/*
 * int
 * memfs_unmount(vfs_t *vfsp, cred_t *cr)
 *
 * Calling/Exit State:
 *	The vnode rwlock of "mvp" is held exclusively on entry and
 *	remains locked at exit.
 *
 * Description:
 *	Mount a memfs on mount point "mvp".
 */
STATIC int
memfs_unmount(vfs_t *vfsp, cred_t *cr)
{
	mem_vfs_t *mem_vfsp = (mem_vfs_t *)VFSTOTM(vfsp);
	struct mnode_map *tmapp0, *tmapp1;
	vnode_t *rvp;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (pm_denied(cr, P_MOUNT))  {
                return (EPERM);
        }
	SLEEP_LOCK(&vfslist_lock, PRIVFS);

	/*
	 * If NFS is establishing references to files in this
	 * file system, fail the unmount now.
	 */
	(void) LOCK(&vfsp->vfs_mutex, FS_VFSPPL);
	if (vfsp->vfs_count != 0) {
		UNLOCK(&vfsp->vfs_mutex, PLBASE);
		SLEEP_UNLOCK(&vfslist_lock);
		return EBUSY;
	}
	UNLOCK(&vfsp->vfs_mutex, PLBASE);

	/* dnlc_purge moved here from upper level.
	 * It is done after the vfslist is locked
	 * because only then can we be sure that
	 * there will be no more cache entries
	 * established via vget by NFS.
	 */
	dnlc_purge_vfsp(vfsp, 0);

	/*
	 * Flush out all mnodes in the file system, thus deleting these
	 * files. Exception: the root directory is not flushed.
	 */
	error = memfs_mflush(vfsp);
	if (error) {
		SLEEP_UNLOCK(&vfslist_lock);
		return error;
	}

	/*
	 * At this point we should only have one mnode allocated and
	 * that is the root directory.  It is treated specially since
	 * it won't be released in the normal manner.
	 *
	 * Note: Unlocked reads are acceptible since the file system is now
	 *       privately held.
	 */
	rvp = MNODE_TO_VP(mem_vfsp->mem_rootnode);
	ASSERT(mem_vfsp->mem_directories == 1);
	ASSERT(mem_vfsp->mem_files == 0);
	ASSERT(rvp->v_count == 2);

	/*
	 * Release the root vnode.
	 */
	VN_RELEN(rvp, 2);

	/*
	 * Free the inode maps
	 */
	tmapp0 = mem_vfsp->mem_mnomap;
	while (tmapp0 != NULL) {
		tmapp1 = tmapp0->mmap_next;
		memfs_kmemfree(mem_vfsp, (char *)tmapp0,
			       sizeof (struct mnode_map));
		tmapp0 = tmapp1;
	}
	ASSERT(mem_vfsp->mem_mntpath);
	memfs_kmemfree(mem_vfsp, mem_vfsp->mem_mntpath,
		       strlen(mem_vfsp->mem_mntpath) + 1);
	ASSERT(mem_vfsp->mem_kmemspace == 0);

	/*
	 * Remove vfs from vfs list.
	 */
	vfs_remove(vfsp);
	SLEEP_UNLOCK(&vfslist_lock);

	LOCK_DEINIT(&mem_vfsp->mem_contents);
	kmem_free((char *)mem_vfsp, sizeof (mem_vfs_t));

	return (0);
}

/*
 * STATIC int
 * memfs_root(vfs_t *vfsp, vnode_t **vpp)
 *
 * Calling/Exit State:
 *	No lock held on entry or at exit.
 *
 * Description:
 * 	Return root vnode for a given vfsp.
 */
STATIC int
memfs_root(vfs_t *vfsp, vnode_t **vpp)
{
	mem_vfs_t *mem_vfsp = (mem_vfs_t *)VFSTOTM(vfsp);
	mnode_t *mp = mem_vfsp->mem_rootnode;

	ASSERT(mem_vfsp->mem_rootnode);
	*vpp = (struct vnode *)MNODE_TO_VP(mp);
	VN_HOLD(*vpp);
	return (0);
}

/*
 * STATIC int
 * memfs_statvfs(vfs_t *vfsp, statvfs_t *sbp)
 *
 * Calling/Exit State:
 *	No lock on entry or at exit.
 *
 * Description:
 *	Returns file system stats.
 */
STATIC int
memfs_statvfs(vfs_t *vfsp, statvfs_t *sbp)
{
	mem_vfs_t *mem_vfsp = (mem_vfs_t *)VFSTOTM(vfsp);
	ulong_t blocks;
	uint_t ani_user_max, ani_resv;

	sbp->f_bsize = PAGESIZE;
	sbp->f_frsize = PAGESIZE;

	ani_user_max = anoninfo.ani_user_max;
	ani_resv = anoninfo.ani_resv;

	/*
	 * Find the amount of available physical swap.
	 */
	if (ani_user_max > ani_resv)
		blocks = ani_user_max - ani_resv;
	else
		blocks = 0;

	/*
	 * If mem_swapmax for this mount is less than the available swap space
	 * (minus the amount memfs can't use), use that instead
	 */
	sbp->f_blocks = MIN(ani_user_max, (int)mem_vfsp->mem_swapmax);

	/*
	 * Number of free blocks is what's available minus what's been used
	 */
	sbp->f_bfree = MIN(blocks, sbp->f_blocks - mem_vfsp->mem_swapmem);
	sbp->f_bavail = sbp->f_bfree;

	/*
	 * The maximum number of files available is approximately the number
	 * of mnodes we can allocate from the remaining kernel memory
	 * available to memfs.  This is fairly inaccurate since it doesn't
	 * take into account the names stored in the directory entries.
	 */
	sbp->f_files = MAX((memfs_maxkmem / sizeof (struct memfs_dirent)), 0);
	sbp->f_ffree = sbp->f_files - mem_vfsp->mem_direntries;
	sbp->f_favail = sbp->f_ffree;

	sbp->f_fsid = vfsp->vfs_dev;
	strcpy(sbp->f_basetype, vfssw[memfsfstype].vsw_name);
	strcpy(sbp->f_fstr, mem_vfsp->mem_mntpath);
	sbp->f_flag = vf_to_stf(vfsp->vfs_flag);
	sbp->f_namemax = MAXNAMELEN;
	return (0);
}

/*
 * STATIC int
 * memfs_sync()
 *
 * Calling/Exit State:
 *	No lock on entry or at exit.
 *
 * Description:
 *	No-op for memfs.
 */
/*ARGSUSED*/
STATIC int
memfs_sync()
{
	return (0);
}

/*
 * STATIC int
 * memfs_vget(vfs_t *vfsp, vnode_t **vpp, fid_t *fidp)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Given a file identifier, return a vnode for the file.
 *
 */
/*ARGSUSED*/
STATIC int
memfs_vget(vfs_t *vfsp, vnode_t **vpp, fid_t *fidp)
{
	struct tfid *tfid;

	/* LINTED pointer alignment */
	tfid = (struct tfid *)fidp;

  	if (mnode_get(vfsp, tfid->tfid_ino, tfid->tfid_gen, vpp))
		*vpp = NULL;

	return (0);
}

/*
 * STATIC int
 * memfs_mountroot(vfs_t *vfsp, enum whymountroot why)
 *
 * Calling/Exit State:
 *	No lock.
 *
 * Description:
 *	No supported.
 */
/*ARGSUSED*/
STATIC int
memfs_mountroot(vfs_t *vfsp, whymountroot_t why)
{
	cred_t		*cr;
	int		error = 0;
	int ovflags = vfsp->vfs_flag;

	if ( (memfsroot_fsp == 0) || (memfsroot_mp == 0 ))
		return (ENOSYS);  /* not allowed if missing kernel rawdata */

	cr = u.u_lwpp->l_cred;

	switch (why) {
	case ROOT_INIT:
		error = memfs_mountfs(vfsp, why, cr);	
		if (error) {
			vfsp->vfs_flag = ovflags;
			return (error);
		}

		/* The routine is called at system boot and at this time
		 * it still in UP state and no one has the access to the fs
		 * therefore no need to lock vfslist when adding the vfs.
		 */
		vfs_add(NULLVP, vfsp, 
			(vfsp->vfs_flag & VFS_RDONLY) ? MS_RDONLY : 0);
		error = memfs_mkrootfs( vfsp );
		break;
	case ROOT_UNMOUNT:
		/* special called from shutdown */
		break;

	default:
		return EINVAL;
	}
	return error;

}

/*
 * int
 * memfs_mountfs(vfs_t *vfsp, whymountroot_t why, cred_t *cr)
 *
 * Calling/Exit State:
 *	Called during system initialization from memfs_mountroot().
 *
 * Description:
 *	Special mount of the system root filesystem.  Called
 *	for ROOT_INIT.
 */
/*ARGSUSED*/
STATIC int
memfs_mountfs(vfs_t *vfsp, whymountroot_t why, cred_t *cr)
{
	mem_vfs_t *mem_vfsp = NULL;
	mnode_t *mp;
	struct pathname dpn;
	dev_t dev;
	int error = 0;
	struct memfs_args targs;
	struct vattr rattr;

	targs.swapmax = 0;
	targs.rootmode = 0775;

	/* create root device now */
	if ((dev = getudev()) == NODEV) {
                /*
                 * The system was unable to assign a unique "device
                 * number" to the memfs file system when it's mounted.
		 * Fail the mount.
                 */
		return (error);
        }
	rootdev = dev;
	vfsp->vfs_dev = rootdev;


	mem_vfsp = (mem_vfs_t *)kmem_zalloc(sizeof (mem_vfs_t), KM_SLEEP);

	/*
	 * Set but don't bother entering the mutex
	 * (mem_vfs not on mount list yet)
	 */
	LOCK_INIT(&mem_vfsp->mem_contents, FS_MEMFS_HIER, FS_MEMFS_PL,
					&memfs_mem_lkinfo, KM_SLEEP);

	mem_vfsp->mem_vfsp = vfsp;
	mem_vfsp->mem_dev = dev;

	mem_vfsp->mem_mnomap = (struct mnode_map *)memfs_kmemalloc(mem_vfsp,
	    sizeof (struct mnode_map), KM_SLEEP);
	if (mem_vfsp->mem_mnomap == NULL) {
		error = ENOMEM;
		goto err;
	}

	/*
	 * nodes 0 and 1 on a file system are unused
	 */
	SETBIT(mem_vfsp->mem_mnomap->mmap_bits, 0);
	SETBIT(mem_vfsp->mem_mnomap->mmap_bits, 1);

	/*
	 * Initialize the pseudo generation number counter
	 */
	mem_vfsp->mem_gen = 0;

	vfsp->vfs_data = (caddr_t)mem_vfsp;
	vfsp->vfs_fstype = memfsfstype;
	vfsp->vfs_dev = mem_vfsp->mem_dev;
	vfsp->vfs_bsize = PAGESIZE;
	vfsp->vfs_fsid.val[0] = mem_vfsp->mem_dev;
	vfsp->vfs_fsid.val[1] = memfsfstype;
	mem_vfsp->mem_mntpath = (char *)memfs_kmemalloc(mem_vfsp,
					2, KM_SLEEP);
	if (mem_vfsp->mem_mntpath == NULL) {
		error = ENOMEM;
		goto err;
	}
	strcpy(mem_vfsp->mem_mntpath, "/");

	/*
	 * allocate and initialize root mnode structure
	 */
	bzero(&rattr, sizeof (rattr));
	rattr.va_mode = (mode_t)(S_IFDIR | targs.rootmode);
	rattr.va_type = VDIR;
	rattr.va_rdev = 0;
	mp = mnode_alloc(mem_vfsp, &rattr, cr);
	mp->mno_uid = 0;
	mp->mno_gid = 3;
	if (mp == NULL) {
		error = ENOSPC;
		goto err;
	}
	MNODE_TO_VP(mp)->v_flag |= VROOT;
	if ((error = memfs_dirinit(mem_vfsp, mp, mp, cr))) {
		mnode_free(mem_vfsp, mp);
		goto err;
	}

	mem_vfsp->mem_rootnode = mp;

	/*
	 * mem_swapmax is set to a maximum value.
	 */
	mem_vfsp->mem_swapmax = INT_MAX;

	RWSLEEP_UNLOCK(&mp->mno_rwlock);

	return (0);
err:
	/*
	 * We had an error during the mount,
	 * so everything we've allocated must be freed.
	 */
	if (mem_vfsp && mem_vfsp->mem_mnomap) {
		memfs_kmemfree(mem_vfsp, (char *)mem_vfsp->mem_mnomap, sizeof (struct mnode_map));
	}
	if (mem_vfsp && mem_vfsp->mem_mntpath != NULL) {
		memfs_kmemfree(mem_vfsp, mem_vfsp->mem_mntpath, strlen(mem_vfsp->mem_mntpath));
	}
	if (mem_vfsp) {
		kmem_free((char *)mem_vfsp, sizeof (mem_vfs_t));
	}
	return (error);
}

/*
 * STATIC int
 * memfs_mkrootfs(vfs_t *vfsp )
 *
 * Calling/Exit State:
 *	No lock.
 *
 * Description:
 *	No supported.
 */
/*ARGSUSED*/
STATIC int
memfs_mkrootfs(vfs_t *vfsp )
{
	vnode_t	*vp, *wvp;
	mnode_t	*mp;
	vnode_t	**selfvp, *parentvp;
	vattr_t	va;
	mem_vfs_t	*mvfsp;
	memfs_image_t	*metap;
 	void *mapcookie;
	cred_t	*cr;
	int j;
	int error = 0;

	cr = u.u_lwpp->l_cred;
	vp = memfs_create_unnamed(memfsmeta_size, MEMFS_NORESV);
	memfs_bind(vp, ptob(btopr(memfsmeta_size)), memfsmeta_plist);

	/*
	 * Now, map the meta data vnode into kernel virtual space.
	 */
	metap = (memfs_image_t *)segkvn_vp_mapin(0, memfsmeta_size, 0, vp, 0, 
			SEGKVN_NOFLAGS, &mapcookie);
	segkvn_lock(mapcookie, SEGKVN_MEM_LOCK);

	mvfsp = (mem_vfs_t *)(rootvfs->vfs_data);
	metap[0].mi_vnode = MNODE_TO_VP(mvfsp->mem_rootnode);
	j = 0;
	while ( metap[++j].mi_type != VNON ){
		selfvp = (vnode_t **)&metap[j].mi_vnode;
		parentvp = (vnode_t *)metap[metap[j].mi_pnumber].mi_vnode;
		va.va_type = metap[j].mi_type;
		va.va_mode = metap[j].mi_mode;
		va.va_mask = AT_TYPE | AT_MODE;
		va.va_size = metap[j].mi_size;
		va.va_uid = metap[j].mi_uid;
		va.va_gid = metap[j].mi_gid;
		va.va_atime = metap[j].mi_atime;
		va.va_mtime = metap[j].mi_mtime;
		va.va_ctime = metap[j].mi_ctime;
		switch( metap[j].mi_type ){
		case VREG:
			error = VOP_CREATE(parentvp, metap[j].mi_name, &va, 
				NONEXCL, metap[j].mi_mode, (vnode_t **)&metap[j].mi_vnode, cr);
				wvp=metap[j].mi_vnode;
				mp=VP_TO_MNODE(wvp);
				mp->mno_size=metap[j].mi_size;
				memfs_bind( metap[j].mi_vnode, 
						ptob(btopr(metap[j].mi_size)),
						metap[j].mi_plist);
			break;
		case VBLK:
		case VCHR:
			error = VOP_CREATE(parentvp, metap[j].mi_name, &va, 
				NONEXCL, metap[j].mi_mode, (vnode_t **)&metap[j].mi_vnode, 
				cr);
			break;
		case VDIR:
			error = VOP_MKDIR(parentvp, metap[j].mi_name, &va, 
				(vnode_t **)&metap[j].mi_vnode, cr);
			break;
		case VLNK:
			error = VOP_SYMLINK(parentvp, metap[j].mi_name, &va, 
					metap[j].mi_tname, cr);
			break;
		default:
			error = EINVAL;
		}
		
		if ( error != 0 )
			break;
	}
	segkvn_unlock(mapcookie, SEGKVN_MEM_LOCK|SEGKVN_DONTNEED|SEGKVN_DISCARD);
	segkvn_mapout( mapcookie );
	VOP_INACTIVE ( vp, cr );
	return (error);
}
