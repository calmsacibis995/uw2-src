/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/cdfs/cdfs_vnops.c	1.14"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <acc/dac/acl.h>
#include <fs/buf.h>
#include <fs/cdfs/cdfs_hier.h>
#include <fs/cdfs/cdfs.h>
#include <fs/cdfs/cdfs_data.h>
#include <fs/cdfs/cdfs_fs.h>
#include <fs/cdfs/cdfs_inode.h>
#include <fs/cdfs/cdfs_susp.h>
#include <fs/cdfs/cdrom.h>
#include <fs/cdfs/iso9660.h>
#include <fs/dirent.h>
#include <fs/fbuf.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/fs_subr.h>
#include <fs/mount.h>
#include <fs/pathname.h>
#include <fs/specfs/snode.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/seg_vn.h>
#include <mem/swap.h>
#include <mem/vmmeter.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/seg.h>
#include <proc/signal.h>
#include <proc/resource.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/inline.h>
#if ((defined CDFS_DEBUG)  && (!defined DEBUG))
#define		DEBUG	YES
#include	<util/debug.h>
#undef		DEBUG
#else
#include	<util/debug.h>
#endif
#include <util/param.h>
#include <util/ipl.h>
#include <util/sysmacros.h>
#include <util/types.h>

STATIC	int cdfs_open(vnode_t **, int, cred_t *);
STATIC	int cdfs_close(vnode_t *, int, boolean_t , off_t, cred_t *);
STATIC	int cdfs_read(vnode_t *, struct uio *, int, cred_t *);
STATIC	int cdfs_readi(cdfs_inode_t *, uio_t *, enum uio_rw , int);
STATIC	int cdfs_getattr(vnode_t *, vattr_t *, int, cred_t *);
STATIC	int cdfs_access(vnode_t *, int , int, cred_t *);
STATIC	int cdfs_lookup(vnode_t *, char *, vnode_t **, pathname_t *, int,
		        vnode_t *, cred_t *);
STATIC	int cdfs_readdir(vnode_t *, struct uio *, cred_t *, int *);
STATIC	int cdfs_readlink(vnode_t *, struct uio *, cred_t *);
STATIC	int cdfs_fsync(vnode_t *, cred_t *);
STATIC	void cdfs_inactive(vnode_t *, cred_t *);
STATIC	int cdfs_fid(vnode_t *, struct fid **);
STATIC	int cdfs_rwlock(vnode_t *, off_t, int, int, int);
STATIC	void cdfs_rwunlock(vnode_t *, off_t, int);
STATIC	int cdfs_seek(vnode_t *, off_t, off_t *);
STATIC	int cdfs_frlock(vnode_t *, int, struct flock *, int, off_t, cred_t *);

STATIC	int cdfs_getpage(vnode_t *, uint_t, uint_t, uint_t *, page_t **,
		uint_t, struct seg *, vaddr_t, enum seg_rw, cred_t *);
STATIC int cdfs_getpageio (struct vnode *, off_t, uint_t, struct page *,
		daddr_t *, int);
STATIC	int cdfs_map(vnode_t *, off_t, struct as *, vaddr_t *, uint_t,
		uint_t, uint_t, uint_t, cred_t *);
STATIC	int cdfs_addmap(vnode_t *, uint_t, struct as *, vaddr_t , uint_t,
		uint_t, uint_t, uint_t, cred_t *);
STATIC	int cdfs_delmap(vnode_t *, uint_t, struct as *, vaddr_t , uint_t,
		uint_t, uint_t, uint_t, cred_t *);
STATIC	int cdfs_rdonly();

extern int	cdfs_ioctl(vnode_t *, int, int, int, cred_t *, int *);
extern void     map_addr(vaddr_t *, uint_t, off_t, int);

vnodeops_t cdfs_vnodeops = {
	cdfs_open,
	cdfs_close,
	cdfs_read,
	cdfs_rdonly,		/* write */
	cdfs_ioctl,
	fs_setfl,
	cdfs_getattr,
	cdfs_rdonly,		/* setattr */
	cdfs_access,
	cdfs_lookup,
	cdfs_rdonly,		/* create */
	cdfs_rdonly,		/* remove */
	cdfs_rdonly,		/* link */
	cdfs_rdonly,		/* rename */
	cdfs_rdonly,		/* mkdir */
	cdfs_rdonly,		/* rmdir */
	cdfs_readdir,
	cdfs_rdonly,		/* symlink */
	cdfs_readlink,
	cdfs_fsync,
	cdfs_inactive,
	(void (*)())fs_nosys,	/* release */
	cdfs_fid,
	cdfs_rwlock,
	cdfs_rwunlock,
	cdfs_seek,
	fs_cmp,
	cdfs_frlock,
	(int (*)())fs_nosys,	/* realvp */
	cdfs_getpage,
	cdfs_rdonly,		/* putpage */
	cdfs_map,
	cdfs_addmap,
	cdfs_delmap,
	fs_poll,
	fs_pathconf,
	(int (*)())fs_nosys,	/* getacl */	
	(int (*)())fs_nosys,	/* setacl */ 	
	(int (*)())fs_nosys,	/* setlevel */ 	
	(int (*)())fs_nosys,    /* getdvstat */
	(int (*)())fs_nosys,    /* setdvstat */
        (int (*)())fs_nosys,    /* makemld */
        (int (*)())fs_nosys,    /* testmld */
	cdfs_rdonly,		/* allocstore */
	cdfs_rdonly,		/* relstore */
	cdfs_rdonly,		/* getpagelist */
	cdfs_rdonly,		/* putpagelist */
	(int (*)())fs_nosys,	/* msgio */
	(int (*)())fs_nosys,	/* filler[4]... */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};



/*
 * STATIC int
 * cdfs_rdonly()
 * 	Return an error to indicate that CDFS currently
 * 	Read-only file systems.
 *
 * Calling/Exit State:
 * 	No locks are held on entry or exit.
 */
/* ARGSUSED */
STATIC int
cdfs_rdonly()
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	return(EROFS);
}



/*
 * STATIC int
 * cdfs_open(vnode_t **vpp, int flag, cred_t *cr)
 * 	Open an ordinary file.
 *
 * Calling/Exit State:
 * 	No locks are held on entry or exit.
 *
 * Note: Device nodes are handled by the lookup() routine.
 */
/* ARGSUSED */
STATIC int
cdfs_open(vnode_t **vpp, int flag, cred_t *cr)
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Nothing special needs to be done other than to verify
	 * that this is a file-type we know how to deal with.
	 */
	if (((*vpp)->v_type == VREG) ||
		((*vpp)->v_type == VDIR)) {
		return(0);
	}

	return(EACCES);
}




/*
 * STATIC int
 * cdfs_close(vnode_t *vp, int flag, int count, off_t offset, cred_t *cr)
 *
 *
 * Calling/Exit State:
 * 	No locks are held on entry or exit.
 */
/* ARGSUSED */
STATIC int
cdfs_close(vnode_t *vp, int flag, boolean_t lastclose, off_t offset, cred_t *cr)
{
	cdfs_inode_t	*ip;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Close the device:
	 * - Lock the inode.
	 * - Clean up the system-level locks.
	 * - Unlock the inode.
	 */
	ip = VTOI(vp);
	if (vp->v_filocks){
		CDFS_SLEEP_LOCK(ip);	
	  	cleanlocks(vp, u.u_procp->p_epid, u.u_procp->p_sysid);
		CDFS_SLEEP_UNLOCK(ip);	
	}
	return(0);
}




/*
 * STATIC int
 * cdfs_read(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 *
 * Calling/Exit State:
 * 	No locks are held on entry or exit.
 *
 * 	A return value of 0 indicates success; othwerise a valid errno
 *      is returned.
 */
/* ARGSUSED */
STATIC int
cdfs_read(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	struct cdfs_inode	*ip;
	int			retval;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	ip = VTOI(vp);

	retval = cdfs_readi(ip, uiop, UIO_READ, ioflag);
	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		return(retval);
	}
		
	return(0);
}



/*
 * STATIC int
 * cdfs_readi(cdfs_inode_t *ip, uio_t *uio, enum uio_rw rw, int ioflag)
 * 	Read from an Inode.
 *
 * Calling/Exit State:
 * 	No locks are held on entry or exit.
 *
 * Description:
 *      The VM segmap driver is used to establish a mapping for the
 *      vnode and offset to a kernel address space managed by the segmap
 *      driver. It then calls uiomove() to move data from the kernel
 *      address space to the calling process's buffer. Accessing the
 *      kernel address space causes the a fault which is handled
 *      by the segmap driver. The segmap driver calls VOP_GETPAGE in
 *      response to the fault where the pages are sought first in the
 *      page cache, and if necessary, reads them in from the file's
 *      backing store.
 */
/* ARGSUSED */
STATIC int
cdfs_readi(cdfs_inode_t *ip, uio_t *uiop, enum uio_rw rw, int ioflag)
{
	ulong_t		bytes_left;
	ulong_t		pageoff;
	ulong_t		blkoff;
	ulong_t		count;
	off_t		off;

	addr_t		base;
	vnode_t		*vp;
	int		retval;
	u_int		flags;
	long    	oresid = uiop->uio_resid;

	vp = ITOV(ip);
	off = uiop->uio_offset;
	ASSERT (((ip->i_Mode & IFMT) == IFREG) ||
		((ip->i_Mode & IFMT) == IFDIR) ||
		((ip->i_Mode & IFMT) == IFLNK));

	/*
	 * Validate UIO parameters.
	 */
	if ((off < 0) ||
		(off + uiop->uio_resid < 0)) {
		return(EINVAL);
	}

	if (uiop->uio_resid == 0) {
		return(0);
	}
 
	if (ioflag & IO_SYNC) {
		ip->i_Flags |= ISYNC;
	}

	while (uiop->uio_resid > 0) {
		/*
		 * Make sure we'are not beyond the end of the file.
		 */
		if (off >= ip->i_Size) {
			retval = RET_OK;
			break;
		}
		bytes_left = ip->i_Size - off;

		/*
		 * Compute the file offset corrsponding to the largest
		 * possible block.  Also, compute the offset relative
		 * to the start of the I/O block.
		 */
		pageoff = off & MAXBOFFSET;

		/*
		 * Compute the transfer count for this iteration.
		 * - The transfer count is limited by:
		 *	 1) The # of bytes remaining in the request.
		 *	 2) The # of bytes left in the block.
		 *	 3) The # of bytes left in the file.
		 */
		blkoff = pageoff;
		count = MIN(uiop->uio_resid, MAXBSIZE - blkoff);
		count = MIN(count, bytes_left);
		ASSERT(count != 0);

		/*
		 * Get a kernel mapping for file offset and move data.
		 */
		base = segmap_getmap(segkmap, vp, off, count, S_READ, B_FALSE, NULL);
		retval = uiomove(base + pageoff, (long)count, UIO_READ, uiop);

		if (retval != 0) {
			(void)segmap_release(segkmap, base, 0);
			break;
		}
			
		/*
		 * The transfer was successful, so if it ended
		 * on a block boundry or at the end of the file,
		 * we probably won't need the data again any time soon.
		 */
		off = uiop->uio_offset;
		flags = 0;
		if ((blkoff + count == MAXBSIZE) ||
			(uiop->uio_offset >= ip->i_Size)) {
				flags |= SM_DONTNEED;
		} else
			flags = 0;
		retval = segmap_release(segkmap, base, flags);
		if (retval != 0) {
			break;
		}
	}

	/* check if it's a partial read, terminate without error */

	if (oresid != uiop->uio_resid)
		retval = 0;
	return(retval);
}




/*
 * STATIC int
 * cdfs_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
 *	Return attributes for a vnode
 *
 * Calling/Exit State:
 * 	No locks are held on entry or exit.
 */
/* ARGSUSED */
STATIC int
cdfs_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
{
	vfs_t		*vfsp;
	cdfs_inode_t	*ip;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	ip = VTOI(vp);
	vfsp = vp->v_vfsp;

	/*
	 * Copy from inode table.
	 */
	vap->va_type = vp->v_type;

	vap->va_mode = cdfs_GetPerms(vfsp, ip) & MODEMASK;
	vap->va_uid = cdfs_GetUid(vfsp, ip);
	vap->va_gid = cdfs_GetGid(vfsp, ip);
		
	vap->va_fsid = CDFS_DEV(vfsp);

	vap->va_nodeid = CDFS_INUM(vfsp, ip->i_Fid.fid_SectNum,
		ip->i_Fid.fid_Offset); 

	vap->va_nlink = ip->i_LinkCnt;
	vap->va_size = ip->i_Size;

	vap->va_vcode = ip->i_VerCode;

	if ((vp->v_type == VCHR) || (vp->v_type == VBLK)) {
		vap->va_rdev = cdfs_GetDevNum(vfsp, ip);
	} else {
		vap->va_rdev = 0;	/* not b/c device */
	}

	vap->va_atime = ip->i_AccessDate;
	vap->va_mtime = ip->i_ModDate;
	vap->va_ctime = ip->i_CreateDate;

	switch (ip->i_Mode & IFMT) {
		case IFBLK:
		case IFCHR: {
			vap->va_blksize = MAXBSIZE;
			break;
		}
		default: {
			vap->va_blksize = CDFS_BLKSZ(vfsp);
			break;
		}
	}

	vap->va_nblocks =
		((ip->i_Size + CDFS_BLKSZ(vfsp)-1) & ~(CDFS_BLKMASK(vfsp))) >>
			DEV_BSHIFT;
	if(vap->va_mask & AT_ACLCNT) {
		vap->va_aclcnt = NACLBASE;
	}

	return(0);
}




/*
 * STATIC int
 * cdfs_access(vp, mode, flags, cr)
 *      Determine the accessibility of a file to the calling
 *      process.
 * Calling/Exit State:
 *      No locks held on entry; no locks held on exit.
 *
 * Description:
 *      Use cdfs_iaccess() to determine accessibility. Return what it does
 *      after releasing the shared/exclusive lock.
 */
/*ARGSUSED*/
STATIC int
cdfs_access(vnode_t *vp, int mode, int flags, cred_t *cr)
{
	struct cdfs_inode	*ip;
	int			retval;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	ip = VTOI(vp);
	retval = cdfs_iaccess(vp->v_vfsp, ip, mode, cr);

	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		return(retval);
	}

	return(0);
}





/*
 * STATIC int
 * cdfs_readlink(vnode_t *vp, uio_t *uiop, cred_t *cr)
 *      Read a symbolic link file.
 *
 * Calling/Exit State:
 *      No inode/vnode locks are held on entry or exit.
 *
 *      On success, 0 is returned; otherwise, a valid errno is
 *      returned. Errnos returned directly by this routine are:
 *              EINVAL  The vnode is not a symbolic link file.
 *
 */
/* ARGSUSED */
STATIC int
cdfs_readlink(vnode_t *vp, uio_t *uiop, cred_t *cr)
{
	cdfs_inode_t	*ip;
	pathname_t		*symlink;
	int			retval;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	if (vp->v_type != VLNK) {
		return EINVAL;
	}

	ip = VTOI(vp);
	symlink = &(ip->i_Rrip->rrip_SymLink);

	retval = uiomove(symlink->pn_buf, (long)symlink->pn_pathlen,
		UIO_READ, uiop);
	if (retval != 0) {
		if (retval < 0) {
			retval = EINVAL;
		}
		return(retval);
	}
	return(0);
}




/*
 * STATIC int
 * cdfs_fsync(vnode_t *vp, cred_t *cr)
 *	It's no-op.
 * 
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
cdfs_fsync(vnode_t *vp, cred_t *cr)
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * - CDFS WRITE SUPPORT:
	 * Need to synch media with in-core data.
	 * See ufs_syncip() for details.
	 */
	return(0);
}




/*
 * STATIC void
 * cdfs_inactive(vnode_t *vp, cred_t *cr)
 *	Perform cleanup on an unreferenced inode.
 *
 * Calling/Exit State:
 *      No lock is held on entry or at exit.
 *
 * Description:
 *      Simply call cdfs_iinactive() with the flag indicating
 *      that the inode's sleep lock is *not* held for <vp>.
 *
 *      cdfs_iinactive is called without applying any locks. cdfs_iinactive
 *      will check is there is another LWP waiting the inode to become
 *      available, i.e., another LWP holds the inode's sleep lock and
 *      is spinning in VN_HOLD. In this situation, the inode is not
 *      released and is given to the waiting LWP. Another interaction
 *      with cdfs_iget is checked for: cdfsiget establishes a reference
 *      to an inode by obtaining the inode table lock, searching a hash
 *      list for an inode, and then, atomically enqueuing for the
 *      inode's sleeplock while dropping the inode table lock. cdfs_iinactive
 *      will obtain the inode table lock and check whether any LWPs are
 *      queued on the inode's sleep lock. If there are any blocked LWPs,
 *      the inode is not inactivated.
 *
 */
/*ARGSUSED*/
STATIC void
cdfs_inactive(vnode_t *vp, cred_t *cr)
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	cdfs_iinactive(VTOI(vp));
	return;
}


/*
 * STATIC int
 * cdfs_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, pathname_t *pnp,
 *	       int lookup_flags, vnode_t *rootvp, cred_t *cr)
 *	Check whether a given directory contains a file named <nm>.
 *
 * Calling/Exit State:
 *      No locks on entry or exit.
 *
 *      A return value of 0 indicates success; otherwise, a valid errno
 *      is returned. Errnos returned directly by this routine are:
 *              ENOSYS  The file found in the directory is a special file but
 *                      SPECFS was unable to create a vnode for it.
 */
/* ARGSUSED */
STATIC int
cdfs_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, pathname_t *pnp,
	    int lookup_flags, vnode_t *rootvp, cred_t *cr)
{
	struct vfs		*vfsp;
	struct cdfs_inode	*ip;
	struct vnode		*vp;
	struct vnode		*newvp;
	int			retval;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	vfsp = dvp->v_vfsp; 

	/*
	 * Check for special case pathname components:
	 * - NULL is a synonym for the current directory.
	 * - CDFS_DOT yields the current directory.
	 * - CDFS_DOTDOT yields the parent directory.
	 */
	if ((nm[0] == '\0') ||
		(strcmp((caddr_t)nm, (caddr_t)CDFS_DOT) == 0)) {
		VN_HOLD(dvp);
		*vpp = dvp;
		return(0);
	}

	if (strcmp((caddr_t)nm, (caddr_t)CDFS_DOTDOT) == 0) {
		retval = cdfs_DirLookup(vfsp, VTOI(dvp),
				 (uchar_t *)CDFS_POSIX_DOTDOT, &ip, cr);
	} else {
		retval = cdfs_DirLookup(vfsp, VTOI(dvp), (uchar_t *)nm, &ip, cr);
	}

	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		return(retval);
	}

	vp = ITOV(ip);
	/*
	 * If vnode is a device-type vnode, then return the SPECFS
	 * vnode instead of the CDFS vnode.  Otherwise, unlock
	 * the CDFS vnode and return it.
	 * Note: We need to recompute the device # to allow for
	 * XCDR device mapping.
	 */
	if ((vp->v_type == VCHR) || (vp->v_type == VBLK) ||
		(vp->v_type == VFIFO) || (vp->v_type == VXNAM)) {
		vp->v_rdev = cdfs_GetDevNum(vfsp, ip);
		newvp = specvp(vp, vp->v_rdev, vp->v_type, cr);
		VN_RELE(vp);
		if (newvp == NULL) {
			*vpp = NULL;
			return(ENOSYS);
		}
		vp = newvp;
	}

	*vpp = vp;
	return(0);
}




/*
 * STATIC int
 * cdfs_readdir(vnode_t *vp, uio_t *uiop, cred_t *cr, int *eofp)
 *	Read from a directory.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 *      A return value of not -1 indicates success; otherwise a valid
 *      errno is returned. Errnos returned directly by this routine
 *      are:
 *              ENOTDIR <vp> is not a directory.
 *              ENXIO
 *
 *      On success, an <*eofp> value of 1 indicates that end-of-file
 *      has been reached, i.e., there are no more directory entries
 *      that may be read.
 */
/* ARGSUSED */
STATIC int
cdfs_readdir(vnode_t *vp, uio_t *uiop, cred_t *cr, int *eofp)
	/* Directory's Vnode structure	*/
	/* Caller's buffer to put data	*/
	/* Caller's credential structure*/
	/* Ret Addr for EOF flag		*/
{
	cdfs_inode_t	*ip;		/* Directory's Inode structure	*/
	cdfs_iobuf_t	drec_buf;	/* I/O buffer to scan directory */
	pathname_t	name;		/* Name of current Dir Rec	*/
	pathname_t	last;		/* Name of Dir Rec last copied 	*/
	ino_t		inum;		/* Inode # of current Dir Rec	*/

	iovec_t		*iovp;		/* I/O Vector of output buffer	*/ 
	uint_t		bytes_wanted;	/* Total # of bytes wanted	*/

	caddr_t		tmpbuf;		/* Tmp buf to build output data	*/
	dirent_t	*dp;		/* Pntr to cur. output struct 	*/
	uint_t		reclen;		/* Reclen of cur. output struct	*/

	int		i;		/* Misc counter		*/
	int		retval;		/* Ret value of called routines	*/
	vfs_t		*vfsp;
	uint_t		offset;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	ip = VTOI(vp);
	vfsp = ip->i_vfs;
	offset = uiop->uio_offset;
	if (vp->v_type != VDIR) {
                return (ENOTDIR);
        }
	/*
	 * Compute and validate the starting Vnode offset and the
	 * # of bytes to be transfered.
	 * - Validate the Vnode offset.
	 * - Compute the transfer count.
	 * - Validate the transfer count.
	 */
	if (offset >= ip->i_Size) {
		if (eofp != NULL) {
			*eofp = 1;
		}
		return(0);
	}

	bytes_wanted = 0;
	iovp = uiop->uio_iov;
	for (i=0; i < uiop->uio_iovcnt; i++) {
		bytes_wanted += iovp->iov_len;
		iovp++;
	}

	if (bytes_wanted == 0) {
		return(0);
	}
	/*
         * Make sure that bytes_wanted is at least one
         * dirent struct in size. If it is less than this then
         * the system would hang as cdfs_readdir() would get
         * into a loop of fbread() and fbrelse() calls.
         */

	if (bytes_wanted < sizeof(struct dirent)) {
		return(EINVAL);
	}

	/* Truncate request to file size */
	if (offset + bytes_wanted > ip->i_Size)
		bytes_wanted = ip->i_Size - offset;
	/*
	 * Allocate a temporary buffer to buffer-up several
	 * dirent structures.  It is hoped that coping each 
	 * dirent structure to a temp buffer and doing a
	 * single 'uiomove()' is faster than doing two
	 * 'uiomove()'s (1 for the name string and 1 for the
	 * other dirent data) for each dirent structure.
	 */
	tmpbuf = (caddr_t)kmem_alloc(cdfs_TmpBufSz,  KM_SLEEP);
	/* LINTED pointer alignment */
	dp = (struct dirent *)tmpbuf;

	CDFS_SETUP_IOBUF(&drec_buf, CDFS_FBUFIO);
	drec_buf.sb_vp = vp;
	CDFS_ILOCK(ip);		
	if (offset >= ip->i_DirOffset) {
		drec_buf.sb_offset = ip->i_DirOffset;
	} else {
		drec_buf.sb_offset = 0;
	}
	CDFS_IUNLOCK(ip);		

	*eofp = 0;
	pn_alloc(&name);
	pn_alloc(&last);
	while (bytes_wanted != 0) {
		retval = cdfs_ReadDrec(vfsp, &drec_buf);
		if (retval != RET_OK) {
			/*
			 * EOF condition signifies when to stop searching
			 * and is expected at some point.  Therefore, set 
			 * the caller's EOF flag and reset the error condition.
			 */
			if (retval == RET_EOF) {
				*eofp = 1;
				retval = RET_OK;
			}
			break;
		}

		/*
		 * If the caller wants this Dir Rec, then add it
		 * to the output buffer.
		 */
		if (drec_buf.sb_offset >= offset) {
			/*
			 * Get the name of the Dir Rec entry.
			 * If RRIP is active, then try to obtain the RRIP name.
			 * If RIP is not active or we couldn't get the RRIP name
			 * then we get the ISO name and apply the XCDR Name conversion.
			 *
			 * Note: This algorithm should complement the lookup
			 * algorithm used in cdfs_CmpDrecName().
			 */
			if (((CDFS_FLAGS(vfsp) & CDFS_RRIP_ACTIVE) == 0) ||
			  (cdfs_GetRripName(vfsp, &drec_buf, &name) != RET_OK) ||
				(name.pn_pathlen == 0)) {
				if (cdfs_GetIsoName(vfsp, &drec_buf, &name) ||
					(cdfs_XcdrName(vfsp, (uchar_t *)name.pn_buf,
							name.pn_pathlen, &name) != RET_OK) ||
					(name.pn_pathlen == 0)) {

					/*
					 * Didn't find a name - All we can do is quit.
					 */
					retval = RET_ERR; 
					break;
				}
			}

			/*
			 * We add the name of this Dir Rec to the buffer only if:
			 * - The Dir Rec is not to be "hidden" from the user,AND
			 * - The Dir Rec does not immediately follow a Dir Rec
			 *	 having the same name (including any XCDR conversion).
			 *	 This prevents multiple listings of the same filename
			 *	 for multi-extent and/or multi-version files.
			 */
			if ((cdfs_HiddenDrec(vfsp, &drec_buf) == RET_FALSE) &&
				((name.pn_pathlen != last.pn_pathlen) ||
				(strncmp(&name.pn_buf[0], &last.pn_buf[0],
					last.pn_pathlen) != 0))) {

				/*
				 * Check for special case directory entries 
				 * (DOT and DOTDOT).  If found, then the name
				 * must be changed to their POSIX counter-part
				 * ('.' and '..') and the TRUE Inode Number must
				 * obtained in order to maintain consistency.
				 */
				if ((strncmp(name.pn_buf, (caddr_t)CDFS_DOT,
						name.pn_pathlen) == 0) &&
					(CDFS_DOT[name.pn_pathlen] == '\0')) {
					/*
					 * Note: Since this is the current
				 	 * directory, we can just use it's FID
					 * to compute the Inode Num.
					 */
					pn_set(&name, (caddr_t)CDFS_POSIX_DOT);
					inum = CDFS_INUM(vfsp, ip->i_Fid.fid_SectNum,
						ip->i_Fid.fid_Offset); 

				} else
				if ((strncmp(name.pn_buf, (caddr_t)CDFS_DOTDOT,
						name.pn_pathlen) == 0) &&
					(CDFS_DOTDOT[name.pn_pathlen] == '\0')) {
					/*
					 * Note: If the Parent FID is valid
					 * then we can just use it. Otherwise,
					 * there is no choice but to get the
					 * Parent Inode and use its FID.
					 */
					pn_set(&name, (caddr_t)CDFS_POSIX_DOTDOT);
					if (CDFS_CMPFID(&ip->i_ParentFid, &CDFS_NULLFID) ==
							B_FALSE) {
						inum = CDFS_INUM(vfsp, ip->i_ParentFid.fid_SectNum,
							ip->i_ParentFid.fid_Offset); 
					} else {
						cdfs_inode_t	*pip;
						
						drec_buf.sb_start = NULL;
						fbrelse(drec_buf.sb_fbp, 0);
						drec_buf.sb_fbp = NULL;

						retval = cdfs_GetParent(vfsp, ip, &pip, cr);
						if (retval != RET_OK) {
							break;
						}
						inum = CDFS_INUM(vfsp, pip->i_Fid.fid_SectNum,
							pip->i_Fid.fid_Offset); 
						VN_RELE(ITOV(pip));

						retval = cdfs_ReadDrec(vfsp, &drec_buf);
						if (retval != RET_OK) {
							/*
			 				* EOF condition signifies when to stop searching
			 				* and is expected at some point.  Therefore, set 
			 				* the caller's EOF flag and reset the error condition.
			 				*/
							if (retval == RET_EOF) {
								*eofp = 1;
								retval = RET_OK;
							}
							break;
						}
					}

				} else {
					/*
					 * No special case (DOT and/or DOTDOT)
					 * so just compute the Inode # based on
					 * the location of this Dir Rec entry.
					 */
					inum = CDFS_INUM(vfsp, drec_buf.sb_sect,
						(drec_buf.sb_ptr - drec_buf.sb_start));
				}

				/*
				 * If there is not enough room in the output
				 * buffer, then flush it to the caller's space.
				 *
				 * Note: Since 'bytes_wanted' > 0, the entire
				 * contents of the buffer should be transfered.
				 */
				reclen = CDFS_STRUCTOFF(dirent, d_name) + name.pn_pathlen + 1;
				reclen = roundup(reclen, sizeof(int));

				if (reclen > bytes_wanted) {
					break;
				}

				if (reclen > (PAGESIZE - ((caddr_t)dp - tmpbuf))) {
					uiomove(tmpbuf, ((caddr_t)dp - tmpbuf), UIO_READ, uiop);
					/* LINTED pointer alignment */
					dp = (struct dirent *)tmpbuf;
				}

				/*
				 * Copy the dirent data to the output buffer.
				 *
				 * Note: Save the name of the Dir Rec so that we
				 * can avoid duplicate entries as coded above.
				 *
				 * - According to the dirent(4) (Admin Ref)
				 * man-page, 'd_off' contains the offset (within
				 * the directory data) of the CURRENT directory
				 * entry. However, the implementations of readir
				 * and telldir (directory(3C) man-page) library
				 * routines require that 'd_off' be set to the
				 * offset of the NEXT directory entry.
				 * If 'd_off' is set per the dirent(4),
				 * 'du', as an example, will  enter an 
				 * infinite-loop reprocessing the first
				 * 'leaf' directory of the sub-tree.
				 */
				dp->d_ino = (ino_t)inum;
				dp->d_reclen = (ushort_t)reclen;
				dp->d_off = drec_buf.sb_offset + drec_buf.sb_reclen;

				strncpy(&dp->d_name[0], &name.pn_buf[0], name.pn_pathlen);
				dp->d_name[name.pn_pathlen] = '\0';

				cdfs_pn_set(&last, (uchar_t *)&name.pn_buf[0],
					name.pn_pathlen);

				dp = (struct dirent *)((int)(dp) + reclen); 
				bytes_wanted -= reclen;
			}
		}
		
		/*
		 * Increment next Dir Rec.
		 */
		drec_buf.sb_ptr += drec_buf.sb_reclen;
		drec_buf.sb_offset += drec_buf.sb_reclen;
		
		if (drec_buf.sb_offset >= ip->i_Size) {
			*eofp = 1;
			break;
		}
	}
	pn_free(&name);
	pn_free(&last);

	/*
	 * Send any residual buffer data to caller.
	 */
	if ((caddr_t)dp != tmpbuf) {
		uiomove(tmpbuf, ((caddr_t)dp - tmpbuf), UIO_READ, uiop);
	}
	uiop->uio_offset = drec_buf.sb_offset;

	/*
	 * Release the Dir Rec buffer, if still allocated.
	 */
	CDFS_RELEASE_IOBUF(&drec_buf);
	CDFS_ILOCK(ip);
	ip->i_DirOffset = drec_buf.sb_offset;
	CDFS_IUNLOCK(ip);

	/*
	 * Release the temporary buffer.
	 */
	kmem_free(tmpbuf, cdfs_TmpBufSz);

	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		return(retval);
	}

	return(0);
}



/*
 * STATIC int
 * cdfs_fid(vnode_t *vp, fid_t **fidpp)
 *
 * Calling/Exit State:
 * 	No locks are held on entry or exit.
 *
 * Description:
 *      The file identifier for the vnode is generated from the invariant
 *      fields of the inode. Thus, the fields are simply copied without
 *      any locking.
 */
/* ARGSUSED */
STATIC int
cdfs_fid(vnode_t *vp, fid_t **fidpp)
{
	fid_t		*fid;
	cdfs_fid_t	*cdfs_fid;
	uint_t		size;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Allocate memory for a generic 'fid' structure.
	 */
	size = CDFS_STRUCTOFF(fid, fid_data[0]) + sizeof(*cdfs_fid);
	fid = (struct fid *)kmem_zalloc(size, KM_SLEEP);

	/*
	 * Fill in the 'fid' structure with the length info
	 * and the CDFS FID info.
	 */
	fid->fid_len = sizeof(*cdfs_fid);
	/* LINTED pointer alignment */
	cdfs_fid = (struct cdfs_fid *) &(fid->fid_data[0]);
	*cdfs_fid = VTOI(vp)->i_Fid;

	*fidpp = fid;
	return(0);
}

/*
 * STATIC int
 * cdfs_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
 *	No-op
 *
 * Calling/Exit State:
 * 	No locks are held on entry or exit.
 */
/* ARGSUSED */
STATIC int
cdfs_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});
	return (0);

}





/* 
 * STATIC void
 * cdfs_rwunlock(vnode_t *vp, off_t off, int len)
 * 	No-op
 *
 * Calling/Exit State:
 * 	No locks are held on entry or exit.
 */
/* ARGSUSED */
STATIC void
cdfs_rwunlock(vnode_t *vp, off_t off, int len)
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	return;

}
			

/*
 * STATIC int
 * cdfs_seek(vnode_t *vp, off_t ooff, off_t *noffp)
 *	Validate a seek pointer.
 *
 * Calling/Exit State:
 *      A return value of 0 indicates success; otherwise, a valid
 *      errno is returned. Errnos returned directly by this routine
 *      are:
 *              EINVAL  The new seek pointer is negative.
 *
 * Description:
 *      No locking is necessary since the result of this routine
 *      depends entirely on the value of <*noffp>.
 */
/* ARGSUSED */
STATIC int
cdfs_seek(vnode_t *vp, off_t ooff, off_t *noffp)
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	if (*noffp < 0) {
		return(EINVAL);
	}
	
	return(0);
}



/*
 * STATIC int
 * cdfs_frlock(vnode_t *vp, int cmd, flock_t *bfp, int flag, off_t offset,
 *	No-op since cdfs is a read-only file system.
 *
 * Calling/Exit State:
 * 	No locks are held on entry or exit.
 */
/* ARGSUSED */
STATIC int
cdfs_frlock(vnode_t *vp, int cmd, flock_t *bfp, int flag, off_t offset,
	cred_t *cr)
{
	cdfs_inode_t	*ip;

	ip = VTOI(vp);
	if (ip->i_mapcnt > 0)	
		return(EAGAIN);

	return(fs_frlock(vp, cmd, bfp, flag, offset, cr, ip->i_Size));

}


/*
 * int
 * cdfs_getpageio(vnode_t *vp, off_t off, uint_t len, page_t *pp,
 *	daddr_t	*io_list, int flag);
 *
 * Calling/Exit State:
 * 	The page has a write-lock held and will be downgrade
 *	once it's filled with data.	
 *
 * Description:
 *	Set up for pages io and call the driver strategy routine to
 *	fill the pages. Pages are linked by p->next. If flag is B_ASYNC,
 *	don't wait for io to complete.
 */
STATIC int
cdfs_getpageio(vnode_t *vp, off_t off, uint_t len, page_t *pp,
	daddr_t *io_list, int flag)
{
	vfs_t	*vfsp;
	buf_t	*bplist[PAGESIZE/NBPSCTR];
	buf_t	**bpp;
	int	bsize;
	int	nio, blkon;
	int	dblks = 0;
	off_t	curoff;
	int	i, blkpp, err= 0, bio_err;
	dev_t	dev;
	cdfs_inode_t	*ip;

	ip = VTOI(vp);
	vfsp = vp->v_vfsp; 
	bsize = CDFS_BLKSZ(vfsp);
	
	ASSERT(len != 0);
	ASSERT(len <= MAX(PAGESIZE, bsize));
	ASSERT(pp != NULL);
	
	dev = CDFS_DEV(vfsp);
	if (bsize >= PAGESIZE)
		blkpp = 1;
	else
		blkpp = PAGESIZE/bsize;

	blkon = btodb(off & CDFS_BLKMASK(vfsp));
	
	ASSERT(blkon == 0 || blkpp == 1);
	
	nio = pp->p_nio;
	ASSERT(nio >= 1);
	
	curoff = off & PAGEOFFSET;
	for (bpp = bplist, i = 0; i < blkpp && curoff < ip->i_Size; 
				i++, curoff += bsize) {
		if (io_list[i] == DB_HOLE)
			continue;
		*bpp = pageio_setup(pp, curoff, MIN(len, bsize), flag | B_READ);

		dblks += btodb((*bpp)->b_bcount);

		(*bpp)->b_edev = dev;
		(*bpp)->b_blkno = 
		    (io_list[i] << (CDFS_BLKSHFT(vfsp) - DEV_BSHIFT)) + blkon;  

		(*bdevsw[getmajor(dev)].d_strategy)(*bpp);
		bpp++;
	}
	ASSERT((bpp - bplist) == nio);
	
	/* Update the number dev sized blocks read by this LWP */
	ldladd(&u.u_ior, dblks);
#ifdef PERF
	mets_fsinfo[MET_OTHER].pgin++;
	/*
	 * btopr(len) isn't the right number of pages if there are big holes.
	 * But it's probably a good estimate in most cases.
	 */
	mets_fsinfo[MET_OTHER].pgpgin += btopr(len);
	mets_fsinfo[MET_OTHER].sectin += dblks;
#endif /* PERF */

	if (flag & B_ASYNC) {
		
#ifdef PERF	
		/*
		 * btopr(len) isn't the right number of pages if there are
		 * big holes.  But it's a good estimate in most cases.
		 */
		mets_fsinfo[MET_OTHER].rapgpgin += btopr(len);
		mets_fsinfo[MET_OTHER].rasectin += dblks;
	
#endif	/* PERF */
		return 0;
	}
	for (bpp = bplist; nio-- > 0; bpp++) {
		bio_err = biowait(*bpp);
		if (bio_err)
			err = bio_err;
		pageio_done(*bpp);
	}

	return(err);
}
	
/*
 * STATIC int
 * cdfs_getapage(vnode_t *vp, uint_t off, unit_t len, uint_t *protp,
 *	page_t *pl[], uint_t plsz, struct seg *seg, vaddr_t addr,
 *	enum seg_rw rw, cred_t *cr)
 * 	Called from pvn_getpages() or cdfs_getpage() to get a particular page.
 *
 * Calling/Exit State:
 *	No locks are held on entry but at exit if a page is locked if
 *	it was found/created.
 *
 * Description:
 *	Called from pvn_getpages or cdfs_getpage to get a particular page.
 *	if pl == NULL, it's a read-head. 
 *	if rw == S_OVERWRITE, it's a write operation to a file.
 * 	If ppp == NULL, async I/O is requested.
 */
/* ARGSUSED */
STATIC int
cdfs_getapage(vnode_t *vp, uint_t off, u_int len, u_int *protp,
	page_t *pl[], uint_t plsz, struct seg *seg,
	vaddr_t addr, enum seg_rw rw, cred_t *cr)
{
	cdfs_inode_t	*ip;
	vfs_t		*vfsp;
	uint_t		bsize;
	off_t 		roff, io_off;
        uint_t 		io_len;
        page_t 		*io_pl[MAXBSIZE/PAGESIZE];
        daddr_t 	io_list[PAGESIZE/NBPSCTR];
        off_t 		lbnoff, curoff;
	daddr_t		lbn;
        int 		blksz, i, blkoff;
        int 		sz, j, blkpp;
	int		retval;
	page_t		*pp, *pp2, **ppp;

	ip = VTOI(vp);
	vfsp = ip->i_vfs;
	bsize = CDFS_BLKSZ(vfsp);
	lbn = off / bsize;
	blkoff = lbn * bsize;
	lbnoff = off & ~CDFS_BLKMASK(vfsp);
	roff = off & PAGEMASK;
	retval = 0;

	if (bsize >= PAGESIZE)
		blkpp = 1;
	else
		blkpp = PAGESIZE/bsize;

	for (i = 0; i < blkpp; i++)
		io_list[i] = DB_HOLE;
	/*
         * If we are not doing read-ahead, we call page_lookup_or_create
         * to look up the page in the page cache or have it created. If
         * the page is in the page cache, the page is returned read-locked.
         * If the page is created, it is write-locked.
         */
	if (pl != NULL) {
		pp = page_lookup_or_create(vp, roff);
		ASSERT(pp != NULL);
		 /*
                 * If we find the page in the page cache
                 * and we are not changing the backing store,
                 * we can return the page to the caller.
                 */
                if (PAGE_IS_RDLOCKED(pp)) {
			if (blkpp > PGPRVSZ) {
				retval = cdfs_bmappage(vp, off, len, &pp,
					io_list, rw);
				if (retval)
					return (retval);
			}
                	if (pl != NULL) {
                        	*pl++ = pp;
                        	*pl = NULL;
                	} else
                       	  	page_unlock(pp);

                	return 0;
		}
	} else {
		/*
                 * This is a read-ahead, call page_lazy_create()
                 * which will create the page only if memory is readily
                 * available and page is not already in the page cache.
                 */
                pp = page_lazy_create(vp, roff);

                if (pp == NULL)
                        return 0;
        }

        ASSERT(pp != NULL);
        ASSERT(PAGE_IS_WRLOCKED(pp));

        retval = cdfs_bmappage(vp, off, len, &pp, io_list, rw);

        /*
         * If bmap fails, pages should have been unlocked by bmap.
	 */
	if (retval)
		return(retval);

	pp->p_nio = 0;
	curoff = roff;

	for (i = 0; i < blkpp && curoff < ip->i_Size; i++, curoff += bsize) {
		if (io_list[i] == DB_HOLE) {
			pagezero(pp, i * bsize, MIN(bsize, PAGESIZE));
		} else
			pp->p_nio++;
	} 

	if (pp->p_nio == 0) {
		/*
		 * All holes or past EOF
		 */
		ASSERT(PAGE_IS_WRLOCKED(pp));
		if (pl != NULL) {
			*pl++ = pp;
			*pl = NULL;
		} else
			page_unlock(pp);	/* drop page writer lock */
		return (0);
	}

	/* kluster pages if needed */
	for (i = 0; i < MAXBSIZE/PAGESIZE; i++)
                io_pl[i] = NULL;

	/*
	 * compute size that needs to get
	 */	
	if (blkoff < ip->i_Size && blkoff + bsize > ip->i_Size) {
		/*
		 * If less than a block left in file read less than a block 
		 */
		if (ip->i_Size <= off) {
			/* 
			 * Trying to access beyon EOF,
			 * setup to get at least one page.
			 */
			blksz = off + PAGESIZE - blkoff;
		} else
			blksz = ip->i_Size - blkoff;
	} else 
		blksz = bsize;

	if (bsize > PAGESIZE && off < lbnoff + blksz) {
		pp2 = pp = pvn_kluster(vp, off, seg, addr, &io_off,
					&io_len, lbnoff, blksz, pp);
		ppp = io_pl;
		do {
			*ppp++ = pp2;
		} while ((pp2 = pp2->p_next) != pp);
	} else {
		io_off = roff;
		io_len = ((bsize < PAGESIZE) ? 
			(lbnoff - roff + blksz): blksz);
		io_pl[0] = pp;
	}

	pagezero(pp->p_prev, io_len & PAGEOFFSET, 
			PAGESIZE - (io_len &PAGEOFFSET));

	retval = cdfs_getpageio(vp, io_off, io_len, pp, io_list,
				pl == NULL ? B_ASYNC : 0);
	/*
	 * If we encountered any I/O errors, the pages should have been
	 * aborted by pvn_done() and we don't need to downgrade the page
	 * lock, nor reference any of the pages in the page list.
	 */
	if (pl == NULL || retval)
		return (retval);
		
	
	/* 
	 * Otherwise, load the pages in the page list to return to the caller.
	 */
	if (plsz >= io_len) {
		/*
		 * Everthing fits, set up to load all pages.
		 */
		i = 0;
		sz = io_len;
		ppp = io_pl;
	} else {
		/* 
		 * Not everthing fits. Set up to load plsz worth
		 * starting at the needed pages.
		 */
		for (i = 0; io_pl[i]->p_offset != off; i++) {
			ASSERT(i < btopr(io_len) -1);
		}
		sz = plsz;
		if (ptob(i) + sz > io_len)
			i = btopr(io_len - sz);
	}
	ppp = pl;
	j = i;
	do {
		*ppp = io_pl[j++];
		ASSERT(PAGE_IS_WRLOCKED(*ppp));
		page_downgrade_lock(*ppp);
		ppp++;
	} while ((sz -=PAGESIZE) > 0);
	*ppp = NULL;		/* terminate the page list */ 

	/* Unlock pages we're not returning to our caller. */
	if ( j >= btopr(io_len))
		j = 0;
	while (j != i) {
		if (io_pl[j] != NULL) {
			ASSERT(PAGE_IS_WRLOCKED(io_pl[j]));
			page_unlock(io_pl[j]);	
		}
		j++;
		if ( j >= btopr(io_len))
			j = 0;
	}
	
	return 0;
}


int cdfs_ra = 1;

/*
 * STATIC int
 * cdfs_getpage(vnode_t *vp, u_int off, u_int len, u_int *protp, page_t *pl[],
 *	 uint_t plsz, seg_t *seg, vaddr_t addr, enum seg_rw rw, cred_t *cr)
 * 	Return all the pages from [off..off+len] in given file.
 *
 * Calling/Exit State:
 * 	No locks are held on entry or exit.
 * 
 * Description:
 *       Reads one or more pages from a contiguous range of file space starting
 *       at the specified <vnode, offset>.  VOP_GETPAGE() is responsible for
 *       finding/creating the necessary pages with this <vnode, offset>
 *       identity, and returning these pages, reader or writer locked, to
 *       the caller.
 */
STATIC int
cdfs_getpage(vnode_t *vp, uint_t off, uint_t len, uint_t *protp, page_t *pl[],
	 uint_t plsz, struct seg *seg, vaddr_t addr, enum seg_rw rw, cred_t *cr)
{
	cdfs_inode_t	 *ip;
	int		 err;
	uint_t		rlen, nextoff;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	if ((vp->v_flag & VNOMAP) != 0) {
		return (ENOSYS);
	}
	ASSERT(rw == S_READ || rw == S_OTHER);

	ip = VTOI(vp);

	/*
	 * This check for beyond EOF allows the request to extend up to
	 * the page boundary following the EOF.	 Strictly speaking,
	 * it should be (off + len > (ip->i_Size + PAGEOFFSET) % PAGESIZE),
	 * but in practice, this is equivalent and faster.
	 *
	 * Also, since we may be called as a side effect of a bmap or
	 * dirsearch() using fbread() when the blocks might be being
	 * allocated and the size has not yet been up'ed.  In this case
	 * we disable the check and always allow the getpage to go through
	 * if the segment is seg_map, since we want to be able to return
	 * zeroed pages if bmap indicates a hole in the non-write case.
	 * For cdfs, we also might have to read some frags from the disk
	 * into a page if we are extending the number of frags for a given
	 * lbn in cdfs_bmap().
	 */

	if (off + len > ip->i_Size + PAGEOFFSET) {
		return (EFAULT);	/* beyond EOF */
	} else if (off + len > ip->i_Size)
		len = ip->i_Size - off;

	if (protp != NULL) {
		*protp = PROT_ALL;
	}

	ASSERT(pl == NULL || plsz >= PAGESIZE);
	if (btop(off) == btop(off + len - 1)) {
		err = cdfs_getapage(vp, off, len, protp, pl, plsz, seg, addr,
			 rw, cr);
	} else {
		err = pvn_getpages(cdfs_getapage, vp, off, len, protp, pl, plsz,
		    seg, addr, rw, cr);
	}
	if (err)
		return(err);
	nextoff = ptob(btopr(off + len));
	if ((cdfs_ra != 0) && (ip->i_NextByte == off) &&
		nextoff < ip->i_Size) {
		if (nextoff + PAGESIZE > ip->i_Size)
                        rlen = ip->i_Size - nextoff;
                else
                        rlen = PAGESIZE;
		/*
                 * For read-ahead, pass a NULL pl so that if it's
                 * not convenient, io will not be done. And also,
                 * if io is done, we don't wait for it.
                 */
#ifdef PERF
                mets_fsinfo[MET_OTHER].ra++;
#endif
                err = cdfs_getapage(vp, nextoff, rlen, NULL, NULL, 0,
                                seg, addr, S_OTHER, cr);
        }
	CDFS_ILOCK(ip);
        ip->i_NextByte = nextoff;

	/*
	 * If the inode is not already marked for IACC (in readi() for read)
	 * and the inode is not marked for no access time update (in readi()
	 * for write) then update the inode access time and mod time now.
	 */
	if ((ip->i_Flags & (IACC | INOACC)) == 0) {
		ip->i_Flags |= IACC;
	}
	CDFS_IUNLOCK(ip);
	return(err);
}




/*
 * STATIC int
 * cdfs_map(vnode_t *vp, uint_t off, struct as *as, addr_t *addrp, uint_t len,
 *	 uint_t prot, uint_t maxprot, uint_t flags, cred_t *fcr)
 *
 * Calling/Exit State:
 *      No lock is held on entry or at exit.
 *
 * Description:
 *      Map the specified <vp, offset> at address "addrp" of address
 *      space "as". The address space is locked exclusive before
 * 	calling as_map() to prevent multiple lwp's from extablishing
 *	 or relinquish mappings concurrently.
 *
 */
/* ARGSUSED */
STATIC int
cdfs_map(vnode_t *vp, off_t off, struct as *as, vaddr_t *addrp,
	 uint_t len, uint_t prot, uint_t maxprot, uint_t flags, cred_t *fcr)
{
	struct segvn_crargs	vn_a;
	int			retval;
	cred_t			*cr = VCURRENTCRED(fcr);

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	if (vp->v_flag & VNOMAP) {
		return (ENOSYS);
	}

	if ((vp->v_vfsp->vfs_flag & VFS_RDONLY) == 0) {
		/*
		 *+ cdfs is not a read-only file system. Recheck.
		 */
		cmn_err(CE_WARN, "cdfs is not a read-only file system\n");
	}

	if (((int)off < 0) || ((int)(off + len) < 0)) {
		return(EINVAL);
	}

	if (vp->v_type != VREG) {
		return(ENODEV);
	}

	/*
	 * If file is being locked, disallow mapping.
	 */
	if (vp->v_filocks != NULL)
		return(EAGAIN);

	as_wrlock(as);

	if ((flags & MAP_FIXED) == 0) {
		map_addr(addrp, len, (off_t)off, 0);
		if (*addrp == NULL) {
			as_unlock(as);
			return(ENOMEM);
		}
	} else {
		/*
		 * User specified address - blow away any previous mappings
		 */
		(void) as_unmap(as, *addrp, len);
	}

	vn_a.vp = vp;
	vn_a.offset = off;
	vn_a.type = flags & MAP_TYPE;
	vn_a.prot = (uchar_t)prot;
	vn_a.maxprot = (uchar_t)maxprot;
	vn_a.cred = cr;

	retval = as_map(as, *addrp, len, segvn_create, (caddr_t)&vn_a);
	as_unlock(as);
	return(retval);	
}



/*
 * STATIC int
 * cdfs_addmap(vnode_t *vp, uint_t off, struct as *as, addr_t addr, uint_t len,
 *	uchar_t prot, uchar_t maxprot, uint_t flags, cred_t *cr)
 *
 * Calling/Exit State:
 *      No lock is held on entry or at exit.
 *
 * Description:
 *      Increase the count of active mappings of "vp" by the number
 *      of pages of mapping being established.
 *
 */
/* ARGSUSED */
STATIC int
cdfs_addmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *cr)
{
	cdfs_inode_t	*ip;

	ip = VTOI(vp);
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	if (vp->v_flag & VNOMAP)
		return (ENOSYS);

	CDFS_ILOCK(ip);
	ip->i_mapcnt += btopr(len);
	CDFS_IUNLOCK(ip);
	return(0);
}


/*
 * STATIC int
 * cdfs_delmap(vnode_t *vp, uint_t off, struct as *as, addr_t addr, uint_t len,
 * 	uchar_t prot, uchar_t maxprot, uint_t flags, cred_t *cr)
 *
 * Calling/Exit State:
 *      No lock is held on entry or at exit.
 *
 * Description:
 *      Decrement the count of active mappings of "vp" by the number
 *      of pages of mapping being relinquished.
 *
 */

/*ARGSUSED*/
STATIC int
cdfs_delmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *cr)
{
	struct cdfs_inode *ip;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	if (vp->v_flag & VNOMAP) {
		return(ENOSYS);
	}

	ip = VTOI(vp);
	CDFS_ILOCK(ip);
	ip->i_mapcnt -= btopr(len); 	/* Count released mappings */
	ASSERT(ip->i_mapcnt >= 0);
	CDFS_IUNLOCK(ip);
	return(0);
}

