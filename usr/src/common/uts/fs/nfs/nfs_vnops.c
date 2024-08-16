/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_vnops.c	1.62"
#ident	"$Header: $"

/*
 *	nfs_vnops.c, vnode operations for nfs
 */

#include <acc/dac/acl.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dirent.h>
#include <fs/dnlc.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/fs_hier.h>
#include <fs/fs_subr.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/nfs_clnt.h>
#include <fs/nfs/nfslk.h>
#include <fs/nfs/rnode.h>
#include <fs/pathname.h>
#include <fs/specfs/snode.h>
#include <fs/stat.h>
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
#include <net/lockmgr/lockmgr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/tiuser.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

extern	rwsleep_t	nfs_rtable_lock;
extern	char		*newname(void);
extern	u_int		setdirmode(struct vnode *, u_int);
extern	int		nfs_strategy(struct buf *);
extern	int		do_bio(struct buf *);
extern	void		map_addr(vaddr_t *, uint_t, off_t, int);
extern	int		klm_lockctl(lockhandle_t *, struct flock *, int cmd,
				struct cred *, pid_t);
extern	int		convoff(off_t, flock_t *, int, off_t);

extern	int		nfs_nra;
extern	int		nfs_cto;
extern	int		nfs_dnlc;


int			rwvp(struct vnode *, struct uio *, enum uio_rw, int,
					struct cred *);
int			nfs_lockrelease(struct vnode *, int, off_t,
					struct cred *);
int			nfs_readrp(rnode_t *, struct uio *, cred_t *);
int			nfs_writerp(rnode_t *, struct uio *, int);
int			nfs_getpageio(rnode_t *, off_t, uint_t, page_t *, int);
int			nfs_putpageio(rnode_t *, page_t *, off_t, size_t,
					int, cred_t *);
int			nfs_doputpage(vnode_t *, page_t *, int, cred_t *);


/*
 * These are the vnode ops routines which implement the vnode interface to
 * the networked file system. These routines just take their parameters,
 * make them look networkish by putting the right info into interface structs,
 * and then calling the appropriate remote routine(s) to do the work.
 */

STATIC	int	nfs_open(vnode_t **, int, cred_t *);
STATIC	int	nfs_close(vnode_t *, int, boolean_t , off_t, cred_t *);
STATIC	int	nfs_read(vnode_t *, struct uio *, int, cred_t *);
STATIC	int	nfs_write(vnode_t *, struct uio *, int, cred_t *);
STATIC	int	nfs_ioctl(vnode_t *, int, int, int, cred_t *, int *);
STATIC	int	nfs_getattr(vnode_t *, vattr_t *, int, cred_t *);
STATIC	int	nfs_setattr(vnode_t *, vattr_t *, int, int, cred_t *);
STATIC	int	nfs_access(vnode_t *, int, int, cred_t *);
STATIC	int	nfs_lookup(vnode_t *, char *, vnode_t **, pathname_t *, int,
			   vnode_t *, cred_t *);
STATIC	int	nfs_create(vnode_t *, char *, vattr_t *, enum vcexcl,
			int , vnode_t **, cred_t *);
STATIC	int	nfs_remove(vnode_t *, char *, cred_t *);
STATIC	int	nfs_link(vnode_t *, vnode_t *, char *, cred_t *);
STATIC	int	nfs_rename(vnode_t *, char *, vnode_t *, char *, cred_t *);
STATIC	int	nfs_mkdir(vnode_t *, char *, vattr_t *, vnode_t **, cred_t *);
STATIC	int	nfs_rmdir(vnode_t *, char *, vnode_t *, cred_t *);
STATIC	int	nfs_readdir(vnode_t *, struct uio *, cred_t *, int *);
STATIC	int	nfs_symlink(vnode_t *, char *, vattr_t *, char *, cred_t *);
STATIC	int	nfs_readlink(vnode_t *, struct uio *, cred_t *);
STATIC	int	nfs_fsync(vnode_t *, cred_t *);
STATIC	void	nfs_inactive(vnode_t *, cred_t *);
STATIC	int	nfs_fid(vnode_t *, struct fid **);
STATIC	int	nfs_rwlock(vnode_t *, off_t, int, int, int);
STATIC	void	nfs_rwunlock(vnode_t *, off_t, int);
STATIC	int	nfs_seek(vnode_t *, off_t, off_t *);
STATIC	int	nfs_frlock(vnode_t *, int, struct flock *, int, off_t,
			cred_t *);
STATIC	int	nfs_realvp(vnode_t *, vnode_t **);
STATIC	int	nfs_getpage(vnode_t *, u_int, u_int, u_int *, page_t **,
			u_int, struct seg *, vaddr_t, enum seg_rw, cred_t *);
STATIC	int	nfs_putpage(vnode_t *, off_t, u_int, int, cred_t *);
STATIC	int	nfs_map(vnode_t *, off_t, struct as *, vaddr_t *, uint_t,
			uint_t, uint_t, uint_t, cred_t *);
STATIC	int	nfs_addmap(vnode_t *, uint_t, struct as *, vaddr_t, uint_t,
			uint_t, uint_t, uint_t, cred_t *);
STATIC	int	nfs_delmap(vnode_t *, uint_t, struct as *, vaddr_t, uint_t,
			uint_t, uint_t, uint_t, cred_t *);
STATIC	int	nfs_stablestore(vnode_t **, off_t *, size_t *, void **,
			cred_t *);
STATIC	int	nfs_relstore(vnode_t *, off_t, size_t, void *, cred_t *);
STATIC	int	nfs_getpagelist(vnode_t *, off_t, uint_t, page_t *,
			void *, int, cred_t *);
STATIC	int	nfs_putpagelist(vnode_t *, off_t, page_t *, void *, int,
			cred_t *);
STATIC	int	nfs_setlevel(vnode_t *, lid_t, cred_t *);
STATIC	int	nfs_makemld(vnode_t *, char *, vattr_t *, vnode_t **,
			cred_t *);
STATIC	void	nfs_release(vnode_t *);

vnodeops_t nfs_vnodeops = {
	nfs_open,
	nfs_close,
	nfs_read,
	nfs_write,
	nfs_ioctl,
	fs_setfl,
	nfs_getattr,
	nfs_setattr,
	nfs_access,
	nfs_lookup,
	nfs_create,
	nfs_remove,
	nfs_link,
	nfs_rename,
	nfs_mkdir,
	nfs_rmdir,
	nfs_readdir,
	nfs_symlink,
	nfs_readlink,
	nfs_fsync,
	nfs_inactive,
	nfs_release,
	nfs_fid,
	nfs_rwlock,
	nfs_rwunlock,
	nfs_seek,
	fs_cmp,
	nfs_frlock,
	nfs_realvp,
	nfs_getpage,
	nfs_putpage,
	nfs_map,
	nfs_addmap,
	nfs_delmap,
	fs_poll,
	fs_pathconf,
	(int (*)())fs_nosys,	/* getacl */
	(int (*)())fs_nosys,	/* setacl */
	nfs_setlevel,
	(int (*)())fs_nosys,	/* getdvstat */
	(int (*)())fs_nosys,	/* setdvstat */
	nfs_makemld,
	(int (*)())fs_nosys,	/* testmld */
	nfs_stablestore,
	nfs_relstore,
	nfs_getpagelist,
	nfs_putpagelist,
	(int (*)())fs_nosys,	/* msgio */
	(int (*)())fs_nosys,	/* filler[4]... */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};

/*
 * nfs_open(vnode_t **vpp, int flag, cred_t *cred)
 *	Open an nfs file.
 * 
 * Calling/Exit State:
 *	Calling LWP holds vnode's r/w sleep lock (v_lock) in *shared* mode
 *	for duration of call.
 *
 * Description:
 *	Open an nfs file. basically gets fresh attributes over the wire,
 *	if needed.
 *
 * Parameters:
 *
 *	vpp			# already looked up vnode.
 *	flag			# open flags
 *	cred			# creds to open with
 */
/*ARGSUSED*/
STATIC int
nfs_open(vnode_t **vpp, int flag, cred_t *cred)
{
	struct	vattr	va;
	int		error = 0;
#ifdef NFSESV
	int		mode = 0;
#endif

	NFSLOG(0x8000, "nfs_open %s %x ", vtomi(*vpp)->mi_hostname, *vpp);
	NFSLOG(0x8000, "flag %d\n", flag, 0);

	/*
	 * if close-to-open consistancy checking is turned off
	 * we can avoid the over the wire getattr.
	 */
#ifdef NFSESV
	if (nfs_cto || !vtomi(*vpp)->mi_nocto || mac_installed) {
#else
	if (nfs_cto || !vtomi(*vpp)->mi_nocto) {
#endif
		/*
		 * force a call to the server to get fresh attributes
		 * so we can check file caches. this is required for
		 * close-to-open consistency.
		 */
		if (vtomi(*vpp)->mi_protocol == NFS_V2) {
			error = nfs_getattr_otw(*vpp, &va, cred);
		}

#ifdef NFSESV
		else {
			nfs_purge_caches(*vpp);
			if (flag & FWRITE)
				mode |= VWRITE;
			if (flag & FREAD)
				mode |= VREAD;

			error = nfs_access(*vpp, mode, 0, cred);
		}
#endif

	}

	NFSLOG(0x20000, "nfs_open: returns %d\n", error, 0);

	return (error);
}

/* 
 * nfs_close(vnode_t *vp, int flag, boolean_t lastclose,
 *		off_t offset, cred_t *cred)
 *	Close an nfs file.
 *
 * Calling/Exit State:
 *	The caller doesn't hold any locks on the vnode in question.
 *
 * Description:
 *	Close an nfs file. Also, flush all dirty pages back to the server.
 *
 * Parameter
 *
 *	vp			# vnode to close
 *	flag			# flags from file pointer
 *	lastclose		# is this the last close of the file ?
 *	offset			# current offset in file
 *	cred			# creds of calling proc
 *
 */
/* ARGSUSED */
STATIC int
nfs_close(vnode_t *vp, int flag, boolean_t lastclose, off_t offset,
	  cred_t *cred)
{
	struct	rnode	*rp = vtor(vp);
	pl_t		opl;
	int		error;

	NFSLOG(0x8000, "nfs_close %s %x ", vtomi(vp)->mi_hostname, vp);
	NFSLOG(0x8000, "flag %d\n", flag, 0);

	/*
	 * first release all file-record locks.
	 */
	(void) nfs_lockrelease(vp, flag, offset, cred);

	if (!lastclose)
		return (0);

	/*
	 * for directories, turn off the eof flag
	 */
	opl = LOCK(&rp->r_statelock, PLMIN);
	if ( (rp->r_flags & REOF) && (vp->v_type == VDIR) )
		rp->r_flags &= ~REOF;
	UNLOCK(&rp->r_statelock, opl);

	/*
	 * if the file is an unlinked file, then flush the lookup
	 * cache so that nfs_inactive will be called if this is
	 * the last reference. Otherwise, if close-to-open
	 * consistancy is turned on and the file was open
	 * for writing or we had an asynchronous write error, we
	 * force the "sync on close" semantic by calling nfs_putpage.
	 */
	if (rp->r_unldvp != NULL || rp->r_error) {
		(void) nfs_putpage(vp, 0, 0, B_INVAL, cred);
		dnlc_purge_vp(vp);
	} else if ((nfs_cto || !vtomi(vp)->mi_nocto)
		&& ((flag & FWRITE) || rp->r_error)) {
		if (rp->r_error == 0) {
			rp->r_error = nfs_putpage(vp, 0, 0, 0, cred);
		} else	{
			(void) nfs_putpage(vp, 0, 0, 0, cred);
		}
	}

	error = (flag & FWRITE ? rp->r_error : 0);

	NFSLOG(0x20000, "nfs_close: returns %d\n", error, 0);

	return(error);
}

/*
 * nfs_read(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *cred)
 *	Read from an nfs file.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the rnode's rwlock in at least
 *	*shared* mode. rwlock remains held on exit. This lock is
 *	usually obtained by a call to VOP_RWRDLOCK() specifying the
 *	same length, offset that's in <uiop>.
 *
 * Description:
 *	Transfer data from <vp> to the calling process's address
 *	space.
 *
 *	The VM segmap driver is used to establish a mapping for the
 *	vnode and offset to a kernel address space managed by the segmap
 *	driver. It then calls uiomove() to move data from the kernel
 *	address space to the calling process's buffer. Accessing the
 *	kernel address space causes the a fault which is handled
 *	by the segmap driver. The segmap driver calls VOP_GETPAGE in
 *	response to the fault where the pages are sought first in the
 *	page cache, and if necessary, read in from the file's backing
 *	store, ie from the server.
 *
 * Parameters:
 *
 *	vp			# vnode to read from
 *	uiop			# uio with the offset and len
 *	ioflag			# type of read (append, async etc)
 *	cred			# cred to use for input
 */
/*ARGSUSED*/
STATIC int
nfs_read(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *cred)
{
	struct	rnode	*rp = vtor(vp);
	struct	vattr	va;
	pl_t		opl;
	addr_t		base;
	u_int		flags;
	int		n, on;
	int		error = 0;
	int		eof = 0;
	int		diff;

	NFSLOG(0x10000, "nfs_read(): %s %x ", vtomi(vp)->mi_hostname, vp);
	NFSLOG(0x10000, "len %d cred 0x%x\n", uiop->uio_iov->iov_len, cred);

	if (vp->v_type != VREG) {
		NFSLOG(0x40000, "nfs_read: EISDIR\n", 0, 0);

		return (EISDIR);
	}

	if (uiop->uio_resid == 0) {
		/*
		 * no more to read.
		 */
		return (0);
	}

	if (uiop->uio_offset < 0 || (uiop->uio_offset + uiop->uio_resid) < 0) {
		/*
		 * bad offset.
		 */
		return (EINVAL);
	}

        /*
	 * do an otw getattr if file is locked.
	 */
        if (rp->r_flags & RNOCACHE) {

                error = nfs_getattr_otw(vp, &va, cred);
                if (error)
                        return (error);
        }

	/*
	 * cred handling: we are passed the open-time file cred for
	 * read/write, but we need them from the current lwp cred.
	 */
	cred = crdup(cred);
	mac_rele(cred->cr_lid);
	cred->cr_lid = u.u_lwpp->l_cred->cr_lid;
	mac_hold(cred->cr_lid);
	cred->cr_wkgpriv = u.u_lwpp->l_cred->cr_wkgpriv;
	cred->cr_maxpriv = u.u_lwpp->l_cred->cr_maxpriv;

	/*
	 * install the creds in the rnode. note that we must add
	 * an extra reference to the creds here because we use
	 * them in things like nfs_getattr() and another read()
	 * may cause the creds in the rnode to be freed.
	 *
	 * the extra reference will be taken off at the end of
	 * the nfs_read().
	 */
	opl = LOCK(&rp->r_statelock, PLMIN);
	if (rp->r_cred)
		crfree(rp->r_cred);
	rp->r_cred = cred;
	crhold(cred);
	UNLOCK(&rp->r_statelock, opl);

	do {
		/*
		 * "on" is the starting offset in block to read from and
		 * "n" is the number of bytes left to read in the block
		 */
		on = uiop->uio_offset & MAXBOFFSET;
		n = MIN(MAXBSIZE - on, uiop->uio_resid);

		/*
		 * make sure cache is not timed out.
		 */
		if ((rp->r_flags & RNOCACHE) == 0) {
			(void) nfsgetattr(vp, &va, cred);
		}

		/*
		 * check with curent eof. we do not need the statelock
		 * here as we are simply peeking at the r_size.
		 */
		diff = rp->r_size - uiop->uio_offset;
		if (diff <= 0) {
			/*
			 * read beyond eof
			 */
			break;
		}

		/*
		 * check with the bytes left in the file (diff).
		 */
		if (diff < n) {
			/*
			 * limit read to what is left in the file
			 */
			n = diff;
			eof = 1;
		}

		/*
		 * set up kernel segmap mapping for data to be read
		 */
		base = segmap_getmap(segkmap, vp, uiop->uio_offset, n,
				S_READ, B_FALSE, NULL);

		/*
		 * move the data from the segmap buffer (pages) to the
		 * user's buffer. this may cause a page fault which will
		 * end up in nfs_getpage().
		 */
		error = uiomove(base + on, n, UIO_READ, uiop);

		if (error == 0) {
			flags = 0;

			if (rp->r_flags & RNOCACHE) {
				/*
				 * will not cache pages, so invalidate them.
				 */
				flags = SM_INVAL;
			} else {
				/*
				 * If read a whole block or read to eof,
				 * won't need this buffer again soon.
				 */
				if (n + on == MAXBSIZE ||
						uiop->uio_offset == rp->r_size)
					flags = SM_DONTNEED;
			}

			/*
			 * now release the segmap mapping.
			 */
			error = segmap_release(segkmap, base, flags);
		} else {
			NFSLOG(0x40000, "nfs_read(): uiomove error %d\n",
						error, 0);

			/*
			 * simply release the mapping on error
			 */
			(void) segmap_release(segkmap, base, 0);
		}
	} while (error == 0 && uiop->uio_resid > 0 && !eof);

	/*
	 * free up our extra reference on the creds
	 */
	crfree(cred);

	NFSLOG(0x40000, "nfs_read(): returns %d\n", error, 0);

	return (error);
}

/*
 * nfs_write(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *cred)
 *	Write to an nfs file.
 *
 * Calling/Exit State:
 *	The calling LWP holds the rnode's rwlock in *exclusive* mode on
 *	entry; it remains held on exit. The rwlock was acquired by calling
 *	VOP_RWWRLOCK specifying the same length, offset pair that's
 *	in <uiop>.
 *
 * Description:
 *	Transfer data from the calling process's address space to <vp>.
 *
 * Parameters:
 *
 *	vp			# vnode to write to
 *	uiop			# uio with the offset and len
 *	ioflag			# type of write (append, async etc)
 *	cred			# cred to use for write
 */
/*ARGSUSED*/
STATIC int
nfs_write(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *cred)
{
	struct	rnode	*rp = vtor(vp);
	struct	vattr	va;
	int		error = 0;
	pl_t		opl;

	NFSLOG(0x10000, "nfs_rdwr: %s %x ", vtomi(vp)->mi_hostname, vp);
	NFSLOG(0x10000, "len %d cred 0x%x\n", uiop->uio_iov->iov_len, cred);

	if (vp->v_type != VREG) {
		NFSLOG(0x40000, "nfs_write: EISDIR\n", 0, 0);

		return (EISDIR);
	} else if (uiop->uio_offset >= (ulong_t)uiop->uio_limit) {
		NFSLOG(0x40000, "nfs_write: EFBIG\n", 0, 0);

		return(EFBIG);
	}

	if (uiop->uio_offset < 0 || (uiop->uio_offset + uiop->uio_resid) < 0) {
		/*
		 * bad offset
		 */
		NFSLOG(0x40000, "nfs_write: EINVAL bad offset\n", 0, 0);

		return (EINVAL);
	}

	if (uiop->uio_resid == 0) {
		/*
		 * no more data to write.
		 */
		return (0);
	}

        if (rp->r_flags & RNOCACHE) {
		/*
		 * do an otw getattr if file is locked.
		 */
                error = nfs_getattr_otw(vp, &va, cred);
		if (error)
			return (error);
        }

	if (ioflag & IO_APPEND) {
		/*
		 * for the append case, make sure our caches are updated.
		 */
		error = nfs_getattr(vp, &va, 0, cred);
		if (error == 0) 
			uiop->uio_offset = va.va_size;
		else
			return (error);
	}

	/*
	 * cred handling: we are passed the open-time file cred for
	 * read/write, but we need them from the current lwp.
	 */
	cred = crdup(cred);
	mac_rele(cred->cr_lid);
	cred->cr_lid = u.u_lwpp->l_cred->cr_lid;
	mac_hold(cred->cr_lid);
	cred->cr_wkgpriv = u.u_lwpp->l_cred->cr_wkgpriv;
	cred->cr_maxpriv = u.u_lwpp->l_cred->cr_maxpriv;

	/* 
 	 * always update the credential for write request. this will keep
	 * the permission in the rnode good enough for putpage.
	 *
	 * we do not need an extra reference on the creds here like we
	 * did in nfs_read() because only read and write replace creds in
	 * the rnode, and only one write can proceed at a time.
 	 */
	opl = LOCK(&rp->r_statelock, PLMIN);
	if (rp->r_cred)
		crfree(rp->r_cred);
	rp->r_cred = cred;
	UNLOCK(&rp->r_statelock, opl);

	error = nfs_writerp(rp, uiop, ioflag);

	NFSLOG(0x40000, "nfs_write: returns %d\n", error, 0);

	return (error);
}

/*
 * nfs_writerp(struct rnode *rp, struct uio *uio, int ioflag)
 *	Write to an rnode.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the rnode's rwlock in at least *exclusive*
 *	mode. This lock must be acquired from above the VOP interface
 *	via VOP_RWRDLOCK(). This routine must only be called from below the
 *	VOP interface.
 *
 *	Returns 0 on success, error on failure.
 *
 * Description:
 *	The VM segmap driver is used to establish a mapping for the
 *	vnode and offset to a kernel address space managed by the segmap
 *	driver. It then calls uiomove() to move data from the user's
 *	buffer to the kernel address space.
 *
 * Parameters:
 *
 *	rp			# rnode to write to
 *	uio			# uio struct with io info
 *	ioflag			# type of write
 *
 */
int
nfs_writerp(rnode_t *rp, struct uio *uio, int ioflag)
{
	struct	vnode	*vp = rtov(rp);
	long		oresid = uio->uio_resid;
	u_int		off;
	addr_t		base;
	u_int		flags;
	int		n, on;
	int		error = 0;
	int		newn1, newn2;
	pl_t		opl;

	if (vp->v_type == VREG && uio->uio_offset+uio->uio_resid >
			u.u_rlimits->rl_limits[RLIMIT_FSIZE].rlim_cur) {
		NFSLOG(0x40000, "nfs_writerp: EFBIG\n", 0, 0);

		/*
		 * write too large, signal lwp.
		 */
		sigtolwp(u.u_lwpp, SIGXFSZ, (sigqueue_t *)NULL);

		return (EFBIG);
	}

	do {
		/*
		 * "off" is the current write offset, "on" is the offset in
		 * block to write to and "n" is the bytes left to write in
		 * the block.
		 */
		off = uio->uio_offset;
		on = uio->uio_offset & MAXBOFFSET;
		n = MIN(MAXBSIZE - on, uio->uio_resid);

		/*
		 * keep returning errors on rnode until rnode goes away.
		 */
		if (rp->r_error) {
			error = rp->r_error;

			NFSLOG(0x40000, "nfs_writerp(): error %d in rnode\n",
						error, 0);

			break;
		}

		/*
		 * setup segmap mapping
		 */
		base = segmap_getmap(segkmap, vp, off, n, S_WRITE,
						B_FALSE, NULL);

		if ((on + n >= PAGESIZE) && on < PAGESIZE) {
			/*
			 * the write is across page boundaries. we need
			 * to divide it up into two uiomoves. this is
			 * because we update the file size after the
			 * uiomove(). a fault on the second page might
			 * cause the first page to be written to the server
			 * with the wrong file size, causing loss of data.
			 *
			 * "newn1" is the number of bytes to write in first
			 * page and "newn2" is the number of bytes to write
			 * in second page.
			 *
			 * XXX: this breaks when MAXBSIZE is > 2 pages.
			 */
			newn1 = PAGESIZE - on;
			newn2 = on + n - PAGESIZE;
			error = uiomove(base + on, newn1, UIO_WRITE, uio);
			if (!error) {
				/*
				 * now up the file size for the first page
				 */
				opl = LOCK(&rp->r_statelock, PLMIN);
				if (rp->r_size < uio->uio_offset) {
					rp->r_size = uio->uio_offset;
				}

				/*
				 * also mark the file as dirty
				 */
				rp->r_flags |= RDIRTY;
				UNLOCK(&rp->r_statelock, opl);

				/*
				 * now move the data to the second page
				 */
				error = uiomove(base + PAGESIZE, newn2,
						UIO_WRITE, uio);
			}
		} else {
			/*
			 * write to one page. do it in one shot.
			 */
			error = uiomove(base + on, n, UIO_WRITE, uio);
		}

		if (error == 0) {
			flags = 0;

			/*
			 * r_size is the maximum number of bytes known to
			 * be in the file. make sure it is at least as high
			 * as the last byte we just wrote into the buffer.
			 */

			NFSLOG(0x40000, "writerp size = %d, uio_off = %d\n",
				rp->r_size, uio->uio_offset);

			opl = LOCK(&rp->r_statelock, PLMIN);
			if (rp->r_size < uio->uio_offset) {
				rp->r_size = uio->uio_offset;
			}

			/*
			 * also mark the file as dirty
			 */
			rp->r_flags |= RDIRTY;
			UNLOCK(&rp->r_statelock, opl);

			if (rp->r_flags & RNOCACHE) {
				/*
				 * will not cache pages, so invalidate them
				 */
				flags = SM_WRITE | SM_INVAL;
			} else {
				if ((n + on == MAXBSIZE) ||
						(rp->r_swapcnt > 0)) {
					/*
					 * have written a whole block.
					 * Start an asynchronous write
					 * and mark the buffer to
					 * indicate that it won't be
					 * needed again soon.
					 */
					flags = SM_WRITE | SM_ASYNC |
							SM_DONTNEED;
				}
			}

			if (ioflag & IO_SYNC) {
				flags &= ~SM_ASYNC;
				flags |= SM_WRITE;
			}

			/*
			 * now release the segmap mapping
			 */
			error = segmap_release(segkmap, base, flags);
		} else {
			off_t	noff;

			ASSERT(uio->uio_offset < off + n);

			NFSLOG(0x40000, "nfs_writerp: err in uiomove\n", 0, 0);

			/*
			 * If we had some sort of error during uiomove,
			 * call segmap_abort_create to have the pages
			 * aborted if we created them.
			 */
			noff = segmap_abort_create(segkmap,
					base, uio->uio_offset,
					(off + n - uio->uio_offset));

			if (noff != -1 && noff < uio->uio_offset) {
				/*
				 * some pages aborted, need to fix resid.
				 */
				uio->uio_resid += uio->uio_offset - noff;
			}

			(void) segmap_release(segkmap, base, SM_INVAL);
		}

	} while (error == 0 && uio->uio_resid > 0);

	/*
	 * If we've already done a partial-write, terminate
	 * the write but return no error.
	 */
	if (oresid != uio->uio_resid)
		error = 0;

	NFSLOG(0x40000, "nfs_writerp: returns %d\n", error, 0);

	return (error);
}

/*
 * nfs_ioctl(vnode_t *vp, int cmd, int arg, int flag, cred_t *cr, int *rvalp)
 *	Perform an ioctl over NFS, not suported.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 * 	Ioctl() is not supported over NFS.
 *
 */
/*ARGSUSED*/
STATIC int
nfs_ioctl(vnode_t *vp, int com, int arg, int flag, cred_t *cred, int *rvalp)
{
	NFSLOG(0x20000, "nfs_ioctl: called\n", 0, 0);

	return (ENOTTY);
}

/*
 * nfs_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cred)
 *	Return attributes for a remote file.
 *
 * Calling/Exit State:
 *	The caller doesn't hold any rnode locks on entry or exit.
 *
 *	Returns 0 on success, error on failure.
 *
 * Description:
 *	Gets attributes of remote file. Only goes remote if
 *	cached attributes are invalid.
 *
 * Parameters:
 *
 *	vp			# vnode to get attr
 *	vap			# to return attr in
 *	flags			# unused
 *	cred			# creds to use in getattr
 *
 */
/*ARGSUSED*/
STATIC int
nfs_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cred)
{
	struct	rnode	*rp = vtor(vp);
	int		error;
	int		setacl = 0;

	NFSLOG(0x8000, "nfs_getattr: %s %x\n", vtomi(vp)->mi_hostname, vp);

	/*
	 * nfs does not support acls yet.
	 */
	if (vap->va_mask & AT_ACLCNT) {
		setacl = 1;
	}

	/*
	 * do not need the r_statelock here as we are just interested
	 * in a snapshot
	 */
	if (rp->r_flags & RDIRTY) {
		/*
		 * Since we know we have pages which are dirty because
		 * we went thru rwvp for writing, we sync pages so the
		 * mod time is right. Note that if a page which is mapped
		 * in user land is modified, the page will not be flushed
		 * until the next sync or appropriate fsync or msync operation.
		 */
		(void) nfs_putpage(vp, 0, 0, 0, cred);
	}

	error = nfsgetattr(vp, vap, cred);

	NFSLOG(0x20000, "nfs_getattr: returns %d\n", error, 0);

	if (setacl)
		vap->va_aclcnt = NACLBASE;

	return (error);
}

/*
 * nfs_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cred)
 *	Modify/Set a remote file's attributes.
 *
 * Calling/Exit State:
 *	The caller doesn't hold any inode locks on entry or exit.
 *
 *	Returns 0 on success, error on failure.
 *
 * Description:
 *	The r_rwlock is grabbed in writer mode in this routine to prevent
 *	concurrent nfs_setattr(), nfs_read() and nfs_write operations.
 *
 * 	Except AT_SIZE, which will have all the pages associated with
 * 	the rnode invalidated before a remote request is made to the
 * 	server, the update of attributes to the rnode will be done 
 * 	upon receiving response from the server.
 *
 * Parameters:
 *
 *	vp			# vnode to set attr
 *	vap			# new attibutes
 *	flags			# which attr to set
 *	ioflags			# open file flags
 *	cred			# creds to use in setattr
 */
STATIC int
nfs_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cred)
{
	struct	rnode		*rp = vtor(vp);
	struct	nfssaargs	args;
	struct	nfsattrstat	*ns;
#ifdef NFSESV
	struct	nfsesvsaargs	cargs;
	struct	nfsesvattrstat	*cns;
#endif
	long			mask = vap->va_mask;
	int			error;
	pl_t			opl;

	NFSLOG(0x8000, "nfs_setattr %s %x\n", vtomi(vp)->mi_hostname, vp);

	if (mask & AT_NOSET) {
		NFSLOG(0x20000, "nfs_setattr: EINVAL at_noset\n", 0, 0);

		return(EINVAL);
	}

	if ((mask & AT_MODE) && ((vap->va_mode & (VSGID | (VEXEC >> 3)))
							== VSGID)) {
		/*
		 * trying to activate mandatory locking on
		 * the file. since this is not supported over
		 * nfs, we will fail it here.
		 */
		NFSLOG(0x20000, "nfs_setattr: EINVAL mand lock\n", 0, 0);

		return(EINVAL);
	}
			

	ns = (struct nfsattrstat *)kmem_zalloc(sizeof (*ns), KM_SLEEP);

#ifdef NFSESV
	cns = (struct nfsesvattrstat *)kmem_alloc(sizeof (*cns), KM_SLEEP);
#endif

	/*
	 * first get the r_rwlock in writer mode.
	 * TODO: do we really need it ??
	 */
	RWSLEEP_WRLOCK(&rp->r_rwlock, PRINOD);

	if (mask & AT_SIZE) {
		/*
		 * get the statelock to set file size.
		 */
		opl = LOCK(&rp->r_statelock, PLMIN);
		if (vap->va_size < rp->r_size) {
			/*
			 * decrease the size and drop the statelock
			 */
			rp->r_size = vap->va_size;
			UNLOCK(&rp->r_statelock, opl);

			/*
			 * zero out the part of the last page not in the file.
			 */
			pvn_trunczero(vp, (u_int)vap->va_size,
			  (u_int)(PAGESIZE - (vap->va_size & PAGEOFFSET)));

			/*
			 * now abort the pages beyond new eof.
			 */
			pvn_abort_range(vp, (off_t)vap->va_size, (uint_t)0);
		} else {
			rp->r_size = vap->va_size;
			UNLOCK(&rp->r_statelock, opl);
		}
	}
			
	/*
	 * write out all dirty pages before setting attributes
	 */
	rp->r_error = nfs_putpage(vp, 0, 0, 0, cred);

	/*
	 * allow SysV-compatible option to set access and
	 * modified times if root, owner, or write access.
	 *
	 * XXX:	For now, va_mtime.tv_nsec == -1 flags this.
	 *
	 * XXX:	Until an NFS Protocol Revision, this may be
	 *	simulated by setting the client time in the
	 *	tv_sec field of the access and modified times
	 *	and setting the tv_nsec field of the modified
	 *	time to an invalid value (1,000,000). This
	 *	may be detected by servers modified to do the
	 *	right thing, but will not be disastrous on
	 *	unmodified servers.
	 *
	 * XXX:	1,000,000 is actually a valid nsec value, but
	 *	the protocol says otherwise.
	 */
	if ((mask & AT_MTIME) && !(flags & ATTR_UTIME)) {
		vap->va_atime.tv_sec = hrestime.tv_sec;
		vap->va_atime.tv_nsec = hrestime.tv_nsec;
		vap->va_mtime.tv_sec = hrestime.tv_sec;
		vap->va_mtime.tv_nsec = 1000000000;
	} else {
		vap->va_mtime.tv_nsec = 0;
	}

	/*
	 * make sure we only set the right fields
	 */
	if (!(mask & AT_MODE)) {
		vap->va_mode = (mode_t)-1;
	}
	if (!(mask & AT_UID))
		vap->va_uid = -1;
	if (!(mask & AT_GID))
		vap->va_gid = -1;
	if (!(mask & AT_SIZE))
		vap->va_size = (u_long)-1;

	/*
	 * set if: AT_ATIME not set and
	 * AT_MTIME set and ATTR_UTIME not set
	 */
	if (!(mask & AT_ATIME) && (!(mask & AT_MTIME) || (flags & ATTR_UTIME)))
		vap->va_atime.tv_sec = vap->va_atime.tv_nsec = -1;
	if (!(mask & AT_MTIME))
		vap->va_mtime.tv_sec = vap->va_mtime.tv_nsec = -1;

	if (vtomi(vp)->mi_protocol == NFS_V2) {
		/*
		 * convert vnode attr to settable network attr
		 * and make the call to the server.
		 */
		vattr_to_sattr(vap, &args.saa_sa);
		args.saa_fh = *vtofh(vp);
		error = rfscall(vtomi(vp), RFS_SETATTR, 0, xdr_saargs,
			(caddr_t)&args, xdr_attrstat, (caddr_t)ns, cred);

		/*
		 * hack for systems that won't grow files but won't tell us
		 */
		if ((mask & AT_SIZE) == AT_SIZE && vap->va_size != 0 &&
				ns->ns_attr.na_size == 0)
			error = EINVAL;

		if (error == 0) {
			error = geterrno(ns->ns_status);
			if (error == 0) {
				timestruc_t	mtime;
				struct	vattr	va;
				pl_t		opl;
				u_long		curr_fsize;

				/*
				 * first convert attr to vnode format.
				 */
				nattr_to_vattr(vp, &ns->ns_attr, &va);

				/*
				 * then set the file size to what the
				 * server returned, but save the old size.
				 */
				opl = LOCK(&rp->r_statelock, PLMIN);
				curr_fsize = rp->r_size;
				rp->r_size = va.va_size;
				UNLOCK(&rp->r_statelock, opl);

				ASSERT(rp->r_size >= 0);


				/*
				 * check the cache if its still valid.
				 * it should never be valid as we just
				 * modified the attributes on the server.
				 */
				mtime.tv_sec = ns->ns_attr.na_mtime.tv_sec;
				mtime.tv_nsec =
					ns->ns_attr.na_mtime.tv_usec*1000;
				nfs_cache_check(vp, mtime,
					ns->ns_attr.na_size, curr_fsize);

				/*
				 * now cache the new attributes.
				 */
				nfs_cache_attr(vp, &va);
			} else {
				NFSLOG(0x20000, "nfs_setattr: got error %d\n",
						error, 0);

				/*
				 * on error, mark the cache timed out and
				 * get rid of the file handle.
				 */
				PURGE_ATTRCACHE(vp);
				PURGE_STALE_FH(error, vp);
			}
		} else {
			NFSLOG(0x20000,
			"nfs_setattr: error %d after rfscall\n", error, 0);
		}
	}

#ifdef NFSESV
	else {
		/*
		 * no LID or ACL to be set
		 */
		vattr_to_esvsattr(vap, &cargs.saa_sa,
			&vtomi(vp)->mi_addr, NULL, NULL, 0);
		cargs.saa_fh = *vtofh(vp);
		error = rfscall(vtomi(vp), RFS_SETATTR, 0, xdr_esvsaargs,
			(caddr_t)&cargs, xdr_esvattrstat, (caddr_t)cns, cred);

		/*
		 * hack for systems that won't grow files but won't tell us
		 */
		if ((mask&AT_SIZE) == AT_SIZE && vap->va_size != 0 &&
			cns->ns_attr.na_size == 0)
			error = EINVAL;

		if (error == 0) {
			error = geterrno(cns->ns_status);
			if (error == 0) {
				timestruc_t	mtime;

				/*
				 * now put it all in the cache
				 */
				mtime.tv_sec = cns->ns_attr.na_mtime.tv_sec;
				mtime.tv_nsec =
					cns->ns_attr.na_mtime.tv_usec*1000;
				nfs_esvcache_check(vp, mtime,
						cns->ns_attr.na_size);
				nfs_esvattrcache(vp, &cns->ns_attr);
			} else {
				PURGE_ATTRCACHE(vp);
				PURGE_STALE_FH(error, vp);
			}
		}
	}
#endif

	/*
	 * now release the r_rwlock.
	 */
	RWSLEEP_UNLOCK(&rp->r_rwlock);

	kmem_free((caddr_t)ns, sizeof (*ns));

#ifdef NFSESV
	kmem_free((caddr_t)cns, sizeof (*cns));
#endif

	NFSLOG(0x20000, "nfs_setattr: returning %d\n", error, 0);

	return (error);
}

/*
 * nfs_access(vnode_t *vp, int mode, int flags, cred_t *cred)
 *	Determine the accessibility of a file to the calling lwp.
 *
 * Calling/Exit State:
 *	No locks held on entry; no locks held on exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned.
 *
 * Description:
 *	Determine the accessibility of a file to the calling lwp.
 *
 * Parameters:
 *	vp			# vnode ptr
 *	mode			# mode to access check
 *	flags			# not used for nfs
 *	cred			# lwp creds for access
 *
 */
/* ARGSUSED */
STATIC int
nfs_access(vnode_t *vp, int mode, int flags, cred_t *cred)
{
#ifdef NFSESV
	struct	nfsaccessargs	acca;
	struct	nfsaccessres	accres;
#endif
	struct	vattr		va;
	int			denied_mode, lshift;
	int			error, i;

	NFSLOG(0x8000, "nfs_access %s %x ", vtomi(vp)->mi_hostname, vp);
	NFSLOG(0x8000, "mode %d uid %d\n", mode, cred->cr_uid);

	if ((mode & VWRITE) && (vp->v_vfsp->vfs_flag & VFS_RDONLY))
		return (EROFS);

#ifdef NFSESV
	if (vtomi(vp)->mi_protocol == NFS_ESV &&
				!nfs_esvgetattr_cache(vp, &va)) {
		acca.acc_fhandle = *vtofh(vp);
		acca.acc_flag = 0;

		if (mode & VWRITE)
			acca.acc_flag |= ACCESS_WRITE;
		if (mode & VREAD)
			acca.acc_flag |= ACCESS_READ;
		if (mode & VEXEC)
			acca.acc_flag |= ACCESS_EXEC;

		error = rfscall(vtomi(vp), RFS_ACCESS, 0, xdr_accessargs,
			(caddr_t)&acca, xdr_accessres, (caddr_t)&accres, cred);
		if (!error)
			error = geterrno(accres.acc_status);
		if (!error) {
			timestruc_t mtime;

			mtime.tv_sec = accres.acc_attr.na_mtime.tv_sec;
			mtime.tv_nsec = accres.acc_attr.na_mtime.tv_usec * 1000;
			nfs_esvcache_check(vp, mtime, accres.acc_attr.na_size);
			nfs_esvattrcache(vp, &accres.acc_attr);
		}

		NFSLOG(0x20000, "nfs_access: returning %d\n", error, 0);

		return (error);
	} else {
#endif
		/*
		 * get cached or new attributes
		 */
		error = nfsgetattr(vp, &va, cred);
		if (error) {
			NFSLOG(0x20000, "nfs_access: returning %d\n", error, 0);

			return (error);
		}
#ifdef NFSESV
	}
#endif

	/*
	 * access check is based on only one of owner, group, public.
	 * If not owner, then check group. If not a member of the group,
	 * then check public access.
	 */
	if (cred->cr_uid == va.va_uid) {
		lshift = 0;
	} else if (groupmember(va.va_gid, cred)) {
		mode >>= TST_GROUP;
		lshift = TST_GROUP;
	} else {
		mode >>= TST_OTHER;
		lshift = TST_OTHER;
	}

	if ((i = (va.va_mode & mode)) == mode) {
		return (0);
	}

	denied_mode = (mode & (~i));
	denied_mode <<= lshift;

	if ((denied_mode & (VREAD | VEXEC)) && pm_denied(cred, P_DACREAD)) {

		NFSLOG(0x20000,
			"nfs_access: read access denied %d\n", EACCES, 0);

		return (EACCES);
	}

	if ((denied_mode & VWRITE) && pm_denied(cred, P_DACWRITE)) {

		NFSLOG(0x20000,
			"nfs_access: write access denied %d\n", EACCES, 0);

		return (EACCES);
	}

	return (0);
}

/*
 * nfs_readlink(vnode_t *vp, struct uio *uiop, cred_t *cred)
 *	Read a symbolic link file over nfs.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 *	On success, 0 is returned; otherwise, a valid error is returned. 
 *
 * Description:
 *	Read a symbolic link file over nfs.
 *
 * Parameters:
 *
 *	vp			# vnode to read
 *	uiop			# uio with read info
 *	cred			# cred to use for read
 *
 */
STATIC int
nfs_readlink(vnode_t *vp, struct uio *uiop, cred_t *cred)
{
	struct	mntinfo		*mi = vtomi(vp);
	struct	nfsrdlnres	rl;
#ifdef NFSESV
	struct	nfsesvrdlnres	crl;
#endif
	int			error;

	NFSLOG(0x8000, "nfs_readlink %s %x\n", mi->mi_hostname, vp);

	if (vp->v_type != VLNK) {
		NFSLOG(0x8000, "nfs_readlink: ENXIO\n", 0, 0);

		return (ENXIO);
	}

	rl.rl_data = (char *)kmem_alloc(NFS_MAXPATHLEN, KM_SLEEP);

#ifdef NFSESV
	crl.rl_data = (char *)kmem_alloc(NFS_MAXPATHLEN, KM_SLEEP);
#endif

	if (mi->mi_protocol == NFS_V2) {
		error = rfscall(mi, RFS_READLINK, 0, xdr_fhandle,
			(caddr_t)vtofh(vp), xdr_rdlnres, (caddr_t)&rl, cred);
		if (!error) {
			error = geterrno(rl.rl_status);
			if (!error) {
				/*
				 * move the data to user land.
				 */
				error = uiomove(rl.rl_data, (int)rl.rl_count,
					UIO_READ, uiop);
			} else {
				NFSLOG(0x20000, "nfs_readlink: geterr\n", 0, 0);

				/*
				 * on error, get rid of the file handle.
				 */
				PURGE_STALE_FH(error, vp);
			}
		}
	}

#ifdef NFSESV
	else {
		error = rfscall(mi, RFS_READLINK, 0, xdr_fhandle,
			(caddr_t)vtofh(vp), xdr_esvrdlnres, (caddr_t)&crl,
						cred);
		if (!error)
			error = geterrno(crl.rl_status);
		if (!error) {
			timestruc_t mtime;
			mtime.tv_sec = crl.rl_attr.na_mtime.tv_sec;
			mtime.tv_nsec = crl.rl_attr.na_mtime.tv_usec * 1000;
			nfs_esvcache_check(vp, mtime, crl.rl_attr.na_size);
			nfs_esvattrcache(vp, &crl.rl_attr);
			error = uiomove(crl.rl_data, (int)crl.rl_count,
				UIO_READ, uiop);
		} else {
			PURGE_STALE_FH(error, vp);
		}
	}
#endif

	kmem_free((caddr_t)rl.rl_data, NFS_MAXPATHLEN);

#ifdef NFSESV
	kmem_free((caddr_t)crl.rl_data, NFS_MAXPATHLEN);
#endif

	NFSLOG(0x20000, "nfs_readlink: returning %d\n", error, 0);

	return (error);
}

/*
 * nfs_fsync(vnode_t *vp, cred_t *cred)
 *	Synchronously flush a file's modified pages to the server.
 *
 * Calling/Exit State:
 *	No locks held on entry; no locks held on exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	error is returned. 
 *
 * Description:
 *	Synchronously flush a file's modified pages to the server.
 *
 * Parameters:
 *
 *	vp			# vnode to flush
 *	cred			# creds to use for flush
 *
 */
/*ARGSUSED*/
STATIC int
nfs_fsync(vnode_t *vp, cred_t *cred)
{
	struct	rnode	*rp = vtor(vp);

	NFSLOG(0x8000, "nfs_fsync %s %x\n", vtomi(vp)->mi_hostname, vp);

	if (rp->r_swapcnt == 0)
		rp->r_error = nfs_putpage(vp, 0, 0, 0, cred);

	NFSLOG(0x20000, "nfs_fsync returning %d\n", rp->r_error, 0);

	return (rp->r_error);
}

/*
 * nfs_inactive(vnode_t *vp, cred_t *cred)
 *	Inactivate an nfs file.
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Perform cleanup on the last reference of a file. Note that
 *	another reference may be created on the file while this is
 *	called. This is taken care of in this routine.
 *
 * 	NOTE: if the file was removed while it was open it got renamed
 * 	(by nfs_remove) instead. Here we remove the renamed file.
 *
 * Parameters:
 *
 *	vp			# vnode to inactivate
 *	cred			# creds to use
 *
 */
/*ARGSUSED*/
STATIC void
nfs_inactive(vnode_t *vp, cred_t *cred)
{
	struct	rnode	*rp = vtor(vp);
	int		error = 0;
	pl_t		opl;

	NFSLOG(0x8000, "nfs_inactive %s, %x\n", vtomi(vp)->mi_hostname, vp);

	/*
	 * v_count should at least be one here.
	 */
	ASSERT(vp->v_count >= 1);

	/*
	 * get the nfs_rtable_lock and the vnode lock first.
	 */
	RWSLEEP_WRLOCK(&nfs_rtable_lock, PRINOD);
	VN_LOCK(vp);

	if (vp->v_count > 1) {
		/*
		 * someone generated a new reference before we
		 * acquired the vnode lock. the vnode has now
		 * become someone else's responsibility. give up
		 * our reference and return.
		 */
		vp->v_count--;
		VN_UNLOCK(vp);
		RWSLEEP_UNLOCK(&nfs_rtable_lock);

		return;
	}

	/*
	 * no-one else grabbed the vnode, drop the vnode lock.
	 * the nfs_rtable_lock is still held, so no-one else
	 * can generate a reference.
	 */
redo:
	VN_UNLOCK(vp);

	ASSERT(vp->v_count == 1);
	ASSERT(rp->r_swapcnt == 0);
	ASSERT(rp->r_mapcnt == 0);

	if (rp->r_unldvp == NULL) {
		/*
		 * do last reference processing on the rnode.
		 */
		rp_lastref(rp);
	} else {
		struct	nfsdiropargs	da;
		enum	nfsstat		status;
		vnode_t			*unldvp;
		char			*unlname;
		cred_t			*unlcred;

		/*
		 * drop the nfs_rtable_lock as we do not want to
		 * hold it across the remote call.
		 */
		RWSLEEP_UNLOCK(&nfs_rtable_lock);

		/*
		 * we will remove the unlinked file now. the nfs_rtable_lock
		 * has been dropped, so new references may be acquired
		 * on the vnode, but this is ok as the file was supposed
		 * to be hidden. the lwps which got new references will
		 * get ESTALE sometime soon.
		 *
		 * make local copies of the pertinent rnode fields and do
		 * it atomically so that the unl* fields do not become
		 * inconsistent with respect to each other due to a race
		 * condition between this code and nfs_remove().
		 */
		opl = LOCK(&rp->r_statelock, PLMIN);
		unldvp = rp->r_unldvp;
		rp->r_unldvp = NULL;
		unlname = rp->r_unlname;
		rp->r_unlname = NULL;
		unlcred = rp->r_unlcred;
		rp->r_unlcred = NULL;
		rp->r_flags &= ~RDIRTY;
		UNLOCK(&rp->r_statelock, opl);
		
		if (vp->v_type != VCHR) {
			/*
			 * abort all pages.
			 */
			pvn_abort_range(vp, 0, 0);
		}

		/*
		 * do the remove operation on the renamed file.
		 */
		setdiropargs(&da, unlname, unldvp);
		error = rfscall(vtomi(unldvp), RFS_REMOVE, 0,
			xdr_diropargs, (caddr_t)&da,
			xdr_enum, (caddr_t)&status, unlcred);

		if (error == 0)
			error = geterrno(status);

		/*
		 * give up our reference on the directory.
		 */
		VN_RELE(unldvp);
		kmem_free((caddr_t)unlname, NFS_MAXNAMLEN);
		crfree(unlcred);

		/*
		 * get the locks again to check the vnode count.
		 */
		RWSLEEP_WRLOCK(&nfs_rtable_lock, PRINOD);
		VN_LOCK(vp);
		if (vp->v_count > 1) {
			/*
			 * someone generated a new reference before we
			 * acquired the vnode lock. the vnode has now
			 * become someone else's responsibility. give up
			 * our reference and return.
			 */
			vp->v_count--;
			VN_UNLOCK(vp);
			RWSLEEP_UNLOCK(&nfs_rtable_lock);

			return;
		}

		if (rp->r_unldvp != NULL) {
			/*
			 * someone renamed the file again. this is very
			 * unlikely, but may happen. redo everything.
			 */
			goto redo;
                }

		/*
		 * give up the vnode lock, and do the last reference
		 * processing on the rnode. we still hold nfs_rtable_lock.
		 */
		VN_UNLOCK(vp);
		rp_lastref(rp);
	}

	/*
	 * all last reference processing on the rnode/vnode has been done.
	 * give up our reference and return. note that we can decrement
	 * v_vount without holding the vnode lock here as no-one else could
	 * possibly be doing anything with the vnode.
	 */
	vp->v_count--;
	RWSLEEP_UNLOCK(&nfs_rtable_lock);

	NFSLOG(0x8000, "nfs_inactive done\n", 0, 0);
}

/*
 * nfs_release(vnode_t *vp)
 *	Release the storage for a totally unreferenced vnode.
 *
 * Calling/Exit State:
 *	The user may hold locks.
 *
 *	Return a void.
 *
 * Description:
 *	Release the storage for a totally unreferenced vnode. This
 *	does not do anything in nfs, as vnodes are never freed.
 *
 * Parameters:
 *
 *	vp			# vnode to free.
 *
 */
/* ARGSUSED */
void
nfs_release(vnode_t *vp)
{
}

/*
 * nfs_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, pathname_t *pnp,
 *	      int lookup_flags, vnode_t *rootvp, cred_t *cred)
 *	Check whether a given directory contains a file named <nm>.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid errno
 *	is returned. 
 *
 * Description:
 *	Check whether a given directory contains a file named <nm>.
 *
 * Parameters:
 *
 *	dvp			# directory vnode pointer
 *	nm			# name fo file to search
 *	vpp			# to return vnode of nm (if found)
 *	cred			# lwp creds
 *
 */
/* ARGSUSED */
int
nfs_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, pathname_t *pnp,
	int lookup_flags, vnode_t *rootvp, cred_t *cred)
{
	struct	mntinfo		*mi = vtomi(dvp);
	struct	vattr		va;
	struct	nfsdiropargs	da;
	struct	nfsdiropres	*dr;
#ifdef NFSESV
	struct	nfsesvdiropres	*cdr;
#endif
	int			error;
	void			*cookie;
	boolean_t		softhold;

	NFSLOG(0x8000, "nfs_lookup %s %x ", mi->mi_hostname, dvp);
	NFSLOG(0x8000, "'%s'\n", nm, 0);

	/*
	 * before checking dnlc, validate caches.
	 */
	error = nfsgetattr(dvp, &va, cred);
	if (error) {
		NFSLOG(0x20000, "nfs_lookup: validate error %d\n", error, 0);

		return (error);
	}

	*vpp = (struct vnode *)dnlc_lookup(dvp, nm, &cookie, &softhold, cred);

#ifdef NFSESV
	if (*vpp && (vtor(*vpp)->r_flags & (RMLD|REFFMLD))) {
		ASSERT(softhold == B_FALSE);
		/*
		 * we shouldn't have gotten an MLD. Try again
		 */
		VN_RELE(*vpp);
		dnlc_remove(dvp, nm);
		*vpp = 0;
	}
#endif

	if (*vpp) {
		ASSERT(softhold == B_FALSE);

		/*
		 * make sure we can search this directory (after the
		 * fact). It's done here because over the wire lookups
		 * verify permissions on the server. VOP_ACCESS will
		 * one day go over the wire, so let's use it sparingly.
		 */
		error = VOP_ACCESS(dvp, VEXEC, 0, cred);
		if (error) {
			VN_RELE(*vpp);

			NFSLOG(0x20000, "nfs_lookup: error %d\n", error, 0);

			return (error);
		}
	} else {
		dr = (struct nfsdiropres *)kmem_alloc(sizeof (*dr), KM_SLEEP);

#ifdef NFSESV
		cdr = (struct nfsesvdiropres *)kmem_alloc(sizeof(*cdr),
						KM_SLEEP);
#endif
		setdiropargs(&da, nm, dvp);

		if (mi->mi_protocol == NFS_V2) {
			error = rfscall(mi, RFS_LOOKUP, 0, xdr_diropargs,
				(caddr_t)&da, xdr_diropres, (caddr_t)dr, cred);
			if (error == 0)
				error = geterrno(dr->dr_status);
			if (error == 0) {
				/*
				 * the directory has changed, so purge it
				 * and then make the rnode for the file.
				 */
				PURGE_STALE_FH(error, dvp);
				*vpp = makenfsnode(&dr->dr_fhandle,
						&dr->dr_attr, dvp->v_vfsp);
			}
		}

#ifdef NFSESV
		else {
			error = rfscall(mi, RFS_LOOKUP, 0, xdr_diropargs,
				(caddr_t)&da, xdr_esvdiropres,
					(caddr_t)cdr, cred);
			if (error == 0)
				error = geterrno(cdr->dr_status);
			if (error == 0) {
				PURGE_STALE_FH(error, dvp);
				*vpp = makeesvnfsnode(&cdr->dr_fhandle,
					&cdr->dr_attr, dvp->v_vfsp);
			}
		}

		/*
		 * Several cases to consider if we're in MLD virtual mode
		 * (and are using the ESV protocol: we'll never get an MLD
		 * with the v.2 protocol):
		 *
		 * 1) We just got a parent MLD
		 *	-> lookup the child for our level and use it.
		 *	-> if the child doesn't exist create it.
		 * 2) We just got a parent MLD and are looking for ".."
		 *	-> lookup ".." in the parent and return the grandparent
		 *	a) The parent MLD is the current global root directory
		 *	   -> go back to the child
		 *	b) The parent MLD is the filesystem root directory
		 *	   -> replace with the vnode covered until not a
		 *	   root dir, then do VOP_LOOKUP in that new dir.
		 *
		 * NOTE: the VOP_LOOKUP above is ok because rfscall/makenfsnode
		 *	   do not automatically deflect through MLD links.
		 */
		if (error == 0 &&
			!(cred->cr_flags & CR_MLDREAL) &&
			(mi->mi_protocol == NFS_ESV) &&
			(vtor(*vpp)->r_flags & RMLD)) {
			if (nm[0] == '.' && nm[1] == '.' && nm[2] == '\0') {
				struct vnode *gpvp;

				NFSLOG(0x8000,
		"nfs_lookup: looking for \"..\" in a MLD\n", 0, 0);

				/*
				 * Adapted from lookuppn()
				 */
checkforroot:
				if (VN_CMP(*vpp, u.u_lwpp->l_rdir)
						|| VN_CMP(*vpp, rootdir)) {
					VN_RELE(*vpp);
					*vpp = dvp;
					VN_HOLD(*vpp);
					goto skip;
				}

				if ((*vpp)->v_flag & VROOT) {
					gpvp = *vpp;
					*vpp =
					  (*vpp)->v_vfsp->vfs_vnodecovered;
					VN_HOLD(*vpp);
					VN_RELE(gpvp);
					goto checkforroot;
				}

				if ((error = MAC_VACCESS(*vpp,
						VEXEC, cred)) == 0)
					error = VOP_LOOKUP(*vpp, nm, &gpvp,
							(struct pathname *)NULL,
							 0, (struct vnode *) 0,
							cred);
				if (!error) {
					VN_RELE(*vpp);
					*vpp = gpvp;
				}
			} else {
				char effname[2*sizeof(lid_t)+1];
				struct vnode *effvp;

				NFSLOG(0x8000,
			"nfs_lookup: in parent MLD, deflecting at LID %d\n",
					cred->cr_lid, 0);
				fs_itoh(cred->cr_lid, effname);
				error = VOP_LOOKUP(*vpp, effname, &effvp,
							(struct pathname *)NULL,
							0, (struct vnode *) 0,
							cred);
				if (!error) {
					VN_RELE(*vpp);
					*vpp = effvp;
					vtor(*vpp)->r_flags |= REFFMLD;
					/* make sure it's a directory */
					if ((*vpp)->v_type != VDIR) {
						error = ENOTDIR;
						VN_RELE(*vpp);
					}
				}

				if (error == ENOENT) {
					struct cred *tmpcred;
					struct vattr effva;

					NFSLOG(0x20000,
		"nfs_lookup: effective MLD subdir not found, creating it\n",
						0, 0);

					tmpcred = crdup(cred);
					tmpcred->cr_wkgpriv |=
						pm_privbit(P_DACWRITE);
					effva = vtor(*vpp)->r_attr;
					effva.va_mask = AT_TYPE|AT_MODE;
					error = VOP_MKDIR(*vpp, effname,
						&effva, &effvp, tmpcred);
					crfree(tmpcred);
					if (!error) {
						VN_RELE(*vpp);
						*vpp = effvp;
						vtor(*vpp)->r_flags |= REFFMLD;
					}
				}
			}
		}

		/*
		 * if a symlink set the level to the parent directory's level
		 */
		if (error == 0 && (*vpp)->v_type == VLNK) {
			(*vpp)->v_lid = dvp->v_lid;
		}

#endif /* NFSESV */

		if (error == 0) {
			if (nfs_dnlc && (dvp->v_vfsp->vfs_fsid.val[1] ==
			  (*vpp)->v_vfsp->vfs_fsid.val[1]) &&
			  !(vtor(dvp) ->r_flags & (RMLD|REFFMLD)) &&
			  !(vtor(*vpp)->r_flags & (RMLD|REFFMLD))) {
				dnlc_enter(dvp, nm, *vpp, NULL, cred);
			}
		} else {
			NFSLOG(0x20000, "nfs_lookup: after make error %d\n",
								error, 0);

			*vpp = (struct vnode *)0;
		}

#ifdef NFSESV
skip:
#endif
		kmem_free((caddr_t)dr, sizeof (*dr));

#ifdef NFSESV
		kmem_free((caddr_t)cdr, sizeof (*cdr));
#endif
	}

#ifdef NFSDL
	/*
	 * if vnode is a device create special vnode
	 */
	if (!error && ISVDEV((*vpp)->v_type)) {
		struct	vnode	*newvp;

		newvp = specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type, cred);
		VN_RELE(*vpp);
		*vpp = newvp;
	}
#endif

	NFSLOG(0x20000, "nfs_lookup returning %d vp = %x\n", error, *vpp);

	return (error);
}

/*
 * nfs_create(vnode_t *dvp, char *nm, vattr_t *va, enum vcexcl exclusive, 
 *		int mode, vnode_t **vpp, cred_t *cred)
 *	Create a file in a given directory.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *
 * Description:
 *	Create an nfs file in given dir. The vnode of created file
 *	is returned in vpp. It is NULL if could not create.
 *
 * Parameters:
 *
 *	dvp			# directory vnode ptr
 *	nm			# name of file to create
 *	va			# attributes of created file
 *	exclusive		# exclusive create or not
 *	mode			# mode of created file
 *	vpp			# vnode of file returned in this
 *	cred			# creds to use for create
 */
/*ARGSUSED*/
STATIC int
nfs_create(vnode_t *dvp, char *nm, vattr_t *va, enum vcexcl exclusive, 
	int mode, vnode_t **vpp, cred_t *cred)
{
	struct	mntinfo		*mi = vtomi(dvp);
	struct	nfscreatargs	args;
	struct	nfsdiropres	*dr;
#ifdef NFSESV
	struct	nfsesvcreatargs	cargs;
	struct	nfsesvdiropres	*cdr;
#endif
	int			error;
	pl_t			opl;

	NFSLOG(0x8000, "nfs_create %s %x ", mi->mi_hostname, dvp);
	NFSLOG(0x8000, "'%s' excl=%d, ", nm, exclusive);
	NFSLOG(0x8000, "mode=%o\n", mode, 0);


	/*
	 * the generic file system code will let a create
	 * come this far even for a read-only file system,
	 * if the file exists (to allow writes to special
	 * files on read-only file systems). nfs does not
	 * do this.
	 *
	 * XXX: System V nfs does not support diskless booting.
	 * if you want to implement this, you will need to
	 * modify this check.
	 */
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		NFSLOG(0x20000, "nfs_create: error EROFS\n", 0, 0);

		return(EROFS);
	}

	if (exclusive == EXCL) {
		/*
		 * XXX: there is a race between the lookup and the
		 * create. we should send the exclusive flag over the wire.
		 */
		error = nfs_lookup(dvp, nm, vpp, NULL, 0, NULL, cred);
		if (!error) {
			VN_RELE(*vpp);

			NFSLOG(0x20000, "nfs_create: EEXIST on race\n", 0, 0);

			return (EEXIST);
		}
	}

	*vpp = (struct vnode *)0;
	if (mi->mi_protocol == NFS_V2) {
		dr = (struct nfsdiropres *)kmem_alloc(sizeof (*dr), KM_SLEEP);
		setdiropargs(&args.ca_da, nm, dvp);
	}

#ifdef NFSESV
	else {
		cdr = (struct nfsesvdiropres *)
				kmem_alloc(sizeof(*cdr), KM_SLEEP);
		setdiropargs(&cargs.ca_da, nm, dvp);
	}
#endif

	/*
	 * Decide what the group-id of the created file should be.
	 * Set it in attribute list as advisory...then do a setattr
	 * if the server didn't get it right the first time.
	 */
	va->va_gid = setdirgid(dvp, cred);

	/*
	 * also set the uid, as vn_open() doesn't set it
	 */
	va->va_uid = cred->cr_uid;

	/*
	 * This is a completely gross hack to make mknod
	 * work over the wire until we can wack the protocol
	 */
#define	IFCHR		0020000		/* character special */
#define	IFBLK		0060000		/* block special */
	if (va->va_type == VCHR) {
		va->va_mode |= IFCHR;
		va->va_size = (u_long)va->va_rdev;
		va->va_mask |= AT_SIZE;
	} else if (va->va_type == VBLK) {
		va->va_mode |= IFBLK;
		va->va_size = (u_long)va->va_rdev;
		va->va_mask |= AT_SIZE;
	} else if (va->va_type == VFIFO) {
		/*
		 * xtra kludge for namedpipe
		 */
		va->va_mode |= IFCHR;

		/*
		 * XXX: 
		 */
		va->va_size = (u_long)NFS_FIFO_DEV;
		va->va_mask |= AT_SIZE;
	}

	va->va_atime.tv_sec = va->va_atime.tv_nsec = -1;
	va->va_mtime.tv_sec = va->va_mtime.tv_nsec = -1;

	if (mi->mi_protocol == NFS_V2) {
		vattr_to_sattr(va, &args.ca_sa);

		/*
		 * mark size not set if AT_SIZE not in va_mask
		 */
		if (!(va->va_mask & AT_SIZE))
			args.ca_sa.sa_size = (u_long)-1;
	}

#ifdef NFSESV
	else {
		/*
		 * XXX: when ACL's are supported we need to deal with
		 * default ACL's here.
		 */
		vattr_to_esvsattr(va, &cargs.ca_sa, &mi->mi_addr,
				&cred->cr_lid, NULL, 0);
		if (!(va->va_mask & AT_SIZE))
			cargs.ca_sa.sa_size = (u_long)-1;
	}
#endif

	if (nm && *nm)
		dnlc_remove(dvp, nm);
	if (mi->mi_protocol == NFS_V2)
		error = rfscall(mi, RFS_CREATE, 0, xdr_creatargs,
			(caddr_t)&args, xdr_diropres, (caddr_t)dr, cred);
#ifdef NFSESV
	else
		error = rfscall(mi, RFS_CREATE, 0, xdr_esvcreatargs,
			(caddr_t)&cargs, xdr_esvdiropres, (caddr_t)cdr, cred);
#endif

	/*
	 * mod time changed
	 */
	PURGE_ATTRCACHE(dvp);

	if (!error)
#ifdef NFSESV
		error = geterrno((mi->mi_protocol == NFS_V2) ?
				 dr->dr_status : cdr->dr_status);
#else
		error = geterrno(dr->dr_status);
#endif

	if (!error) {
		gid_t	gid;

		if (mi->mi_protocol == NFS_V2)
			*vpp = makenfsnode(&dr->dr_fhandle, &dr->dr_attr,
				dvp->v_vfsp);
#ifdef NFSESV
		else
			*vpp = makeesvnfsnode(&cdr->dr_fhandle, &cdr->dr_attr,
				dvp->v_vfsp);
#endif

		if (va->va_size == 0) {
			struct	rnode	*rp = vtor(*vpp);

			opl = LOCK(&rp->r_statelock, PLMIN);
			rp->r_size = 0;
			if (((*vpp)->v_pages != NULL) &&
					((*vpp)->v_type != VCHR)) {
				/*
				 * the rnode returned from makenfsnode
				 * has pages hanging on it. take care of
				 * these pages here.
				 */
				rp->r_flags &= ~RDIRTY;
				rp->r_error = 0;
				UNLOCK(&rp->r_statelock, opl);

				/*
				 * toss all pages
				 */
				pvn_abort_range(*vpp, 0, 0); 
			} else {
				UNLOCK(&rp->r_statelock, opl);
			}
		}

		if (nfs_dnlc && nm && *nm &&
				!(vtor(dvp)->r_flags & (RMLD|REFFMLD))) {
			dnlc_enter(dvp, nm, *vpp, NULL, cred);
		}

		/*
		 * make sure the gid was set correctly.
		 * if not, try to set it (but don't lose
		 * any sleep over it).
		 */
		gid = va->va_gid;
		if (mi->mi_protocol == NFS_V2) {
			/*
			 * this does not set file size anymore, but
			 * we took care of that above.
			 */
			nattr_to_vattr(*vpp, &dr->dr_attr, va);
		}
#ifdef NFSESV
		else {
			vtor(*vpp)->r_aclcnt = acl_getmax();
			nattresv_to_vattr(*vpp, &cdr->dr_attr, va,
				&(*vpp)->v_lid, vtor(*vpp)->r_acl,
					&vtor(*vpp)->r_aclcnt);
		}
#endif

		if (gid != va->va_gid) {
			struct	vattr	vattr;

			vattr.va_mask = AT_GID;
			vattr.va_gid = gid;
			(void) nfs_setattr(*vpp, &vattr, 0, 0, cred);
			va->va_gid = vattr.va_gid;
		}

#ifdef NFSDL
		/*
		 * if vnode is a device create special vnode
		 */
		if (ISVDEV((*vpp)->v_type)) {
			struct	vnode	*newvp;

			newvp = specvp(*vpp, (*vpp)->v_rdev,
					(*vpp)->v_type, cred);
			VN_RELE(*vpp);
			if (newvp != NULL)
				*vpp = newvp;
			else 
				error = ENOSYS;
		}
#endif
	} else {
		NFSLOG(0x20000, "nfs_create: error %d\n", error, 0);

		PURGE_STALE_FH(error, dvp);
	}

	if (mi->mi_protocol == NFS_V2)
		kmem_free((caddr_t)dr, sizeof (*dr));
#ifdef NFSESV
	else
		kmem_free((caddr_t)cdr, sizeof (*cdr));
#endif

	NFSLOG(0x20000, "nfs_create returning %d\n", error, 0);

	return (error);
}

/*
 * nfs_remove(vnode_t *dvp, char *nm, cred_t *cred)
 *	Remove a file from a directory.
 *
 * Calling/Exit State:
 *	The caller holds no rnode locks on entry or exit.
 *
 * Description:
 *	Remove a file from a directory.
 *
 * 	NOTE: if the vnode to be removed has another reference,
 * 	we rename it instead of removing it. nfs_inactive() will
 *	remove the renamed vnode when it is called when the
 *	last reference goes away.
 *
 * Parameters:
 *
 *	dvp			# directory vnode
 *	nm			# name to remove
 *	cred			# cred of lwp
 *
 */
STATIC int
nfs_remove(vnode_t *dvp, char *nm, cred_t *cred)
{
	struct	rnode		*rp;
	struct	nfsdiropargs	da;
	enum	nfsstat		status;
	struct	vnode		*vp;
#ifdef NFSDL
	struct	vnode		*oldvp;
	struct	vnode		*realvp;
#endif
	char			*tmpname;
	int			error;
	pl_t			opl;

	NFSLOG(0x8000, "nfs_remove %s %s\n", vtomi(dvp)->mi_hostname, nm);

	status = NFS_OK;
	error = nfs_lookup(dvp, nm, &vp, NULL, 0, NULL, cred);

#ifdef NFSDL
	/*
	 * lookup may have returned a non-nfs vnode!
	 * get the real vnode.
	 */
	if (error == 0 && VOP_REALVP(vp, &realvp) == 0) {
		oldvp = vp;
		vp = realvp;
	} else {
		oldvp = NULL;
	}
#endif

	if (error == 0 && vp != NULL) {
		rp = vtor(vp);

		/*
		 * we need to flush the name cache so we can
		 * check the real reference count on the vnode
		 */
		dnlc_purge_vp(vp);

		/*
		 * grab the rtable lock in writer mode to prevent
		 * anyone from getting to the rnode.
		 */
		RWSLEEP_WRLOCK(&nfs_rtable_lock, PRINOD);

		/*
		 * grab the vnode lock to check count
		 */
		VN_LOCK(vp);

		if (vp->v_count > 1) {
			/*
			 * we will rename the file if we can. we will now
			 * drop both the locks. it is possible that the
			 * reference count drops to 1. however, this is ok
			 * since the renamed file will get removed when
			 * we do a VN_RELE() below.
			 */
			VN_UNLOCK(vp);
			RWSLEEP_UNLOCK(&nfs_rtable_lock);

			/*
			 *
			 * check if someone else is renaming already
			 */
			opl = LOCK(&rp->r_statelock, PLMIN);
			if ((rp->r_unldvp == NULL) &&
					((rp->r_flags & RRENAME) == 0)) {
				/*
				 * set the renaming flag in the rnode to
				 * prevent racing with another remove
				 * this flag will never be reset
				 */
				rp->r_flags |= RRENAME;
				UNLOCK(&rp->r_statelock, opl);

				/*
				 * get a name for the new file, and do the
				 * rename operation.
				 */
				tmpname = newname();
				error = nfs_rename(dvp, nm, dvp, tmpname, cred);
				if (error) {
					/*
					 * this may be caused by a remove on
					 * the server or by another client.
					 * do nothing (and the rnode will
					 * eventually go away)
					 */
					NFSLOG(0x20000,
				 "nfs_remove: rename error %d\n", error, 0);

					kmem_free((caddr_t)tmpname,
							NFS_MAXNAMLEN);
				} else {
					ASSERT(rp->r_unldvp == NULL);

					/*
					 * first get a reference on the
					 * directory we're in. this reference
					 * will be released when we remove
					 * the renamed file.
					 */
					VN_HOLD(dvp);

					/*
					 * now put the stuff in the rnode.
					 */
					opl = LOCK(&rp->r_statelock, PLMIN);
					rp->r_unldvp = dvp;
					rp->r_unlname = tmpname;
					if (rp->r_unlcred != NULL) {
						crfree(rp->r_unlcred);
					}
					crhold(cred);
					rp->r_unlcred = cred;
					UNLOCK(&rp->r_statelock, opl);
				}
			} else {
				/*
				 * lost the rename race, just unlock the rnode.
				 */
				UNLOCK(&rp->r_statelock, opl);

				NFSLOG(0x20000,
				"nfs_remove: lost rename race\n", 0, 0);
			}

			/*
			 * release our reference on the vnode.
			 */
#ifdef NFSDL
			if (oldvp) {
				VN_RELE(oldvp);
			} else {
				VN_RELE(vp);
			}
#else
			VN_RELE(vp);
#endif

		} else {
			/*
			 * only we have a reference on the vnode.
			 * and since we hold the nfs_rtable_lock,
			 * no-one else can get to the vnode
			 * we can drop the vnode lock here.
			 */
			VN_UNLOCK(vp);

			ASSERT(vp->v_count == 1);

			/*
			 * first do the remove on the server.
			 */
			setdiropargs(&da, nm, dvp);
			error = rfscall(vtomi(dvp), RFS_REMOVE, 0,
				xdr_diropargs, (caddr_t)&da, xdr_enum,
				(caddr_t)&status, cred);

			if (!error)
				error = geterrno(status);

			if (!error) {
				/*
				 * reset the dirty flag
				 */
				rp->r_flags &= ~RDIRTY;

				/*
				 * abort all pages.
				 */
				pvn_abort_range(vp, 0, 0);

				ASSERT(vp->v_pages == NULL);

				/*
				 * mod time changed
				 */
				PURGE_ATTRCACHE(dvp);	
				PURGE_ATTRCACHE(vp);

				/*
				 * do last reference processing on the rnode.
				 * since it has no pages, it will be put on
				 * the free list and removed from the hash
				 * table.
				 */
				rp_lastref(rp);

				/*
				 * release our reference on the vnode.
				 */
				vp->v_count--;

				ASSERT(vp->v_count == 0);

				RWSLEEP_UNLOCK(&nfs_rtable_lock);
			} else {
				/*
				 * purge the directory handle if we
				 * got ESTALE back.
				 */
				PURGE_STALE_FH(error, dvp);

				RWSLEEP_UNLOCK(&nfs_rtable_lock);
				/*
				 * release our reference on the vnode.
				 */
#ifdef NFSDL
				if (oldvp) {
					VN_RELE(oldvp);
				} else {
					VN_RELE(vp);
				}
#else
				VN_RELE(vp);
#endif
			}
		}
	}

	NFSLOG(0x20000, "nfs_remove: returning %d\n", error, 0);

	return (error);
}

/*
 * nfs_link(vnode_t *tdvp, vnode_t *svp, char *tnm, cred_t *cred)
 *	Make a link to a file or a directory.
 *
 * Calling/Exit State:
 *	No vnode/rnode locks are held on entry or exit.
 *
 * Description:
 *	Makes a link to a file or dir.
 *
 * Parameters:
 *
 *	tdvp			# the dir containing the 'to' file
 *	svp			# file to link to
 *	tnm			# name of link to create
 *	cred			# lwp creds
 *
 */
STATIC int
nfs_link(vnode_t *tdvp, vnode_t *svp, char *tnm, cred_t *cred)
{
	struct	mntinfo		*smi = vtomi(svp);
	struct	nfslinkargs	args;
	enum	nfsstat		status;
	struct	vnode		*realvp;
	int			error;

	if (VOP_REALVP(svp, &realvp) == 0) {
		svp = realvp;
	}

	NFSLOG(0x8000, "nfs_link from %s %x ", smi->mi_hostname, svp);
	NFSLOG(0x8000, "to %s %x ", smi->mi_hostname, tdvp);
	NFSLOG(0x8000, "'%s'\n", tnm, 0);

	args.la_from = *vtofh(svp);
	setdiropargs(&args.la_to, tnm, tdvp);
	error = rfscall(smi, RFS_LINK, 0, xdr_linkargs, (caddr_t)&args,
		xdr_enum, (caddr_t)&status, cred);
	/*
	 * mod time changed
	 */
	PURGE_ATTRCACHE(tdvp);
	
	/*
	 * link count changed
	 */
	PURGE_ATTRCACHE(svp);
	if (!error) {
		error = geterrno(status);
		PURGE_STALE_FH(error, svp);
		PURGE_STALE_FH(error, tdvp);
	}

	NFSLOG(0x20000, "nfs_link returning %d\n", error, 0);

	return (error);
}

/*
 * nfs_rename(vnode_t *odvp, char *onm, vnode_t *ndvp, char *nnm, cred_t *cred)
 *	Rename a file or directory.
 *
 * Calling/Exit State:
 *	The caller holds no vnode/rnode locks on entry or exit.
 *
 * Description:
 *	Rename a file or directory. Just goes over the wire
 *	to do this.
 *
 * Parameters:
 *
 *	odvp			# vnode of dir containing file to rename
 *	onm			# name of file to rename
 *	ndvp			# new dir vnode ptr
 *	nnm			# new name of file
 *	cred			# lwp creds
 *
 */
STATIC int
nfs_rename(vnode_t *odvp, char *onm, vnode_t *ndvp, char *nnm, cred_t *cred)
{
	enum	nfsstat		status;
	struct	nfsrnmargs	args;
	struct	vnode		*realvp;
	int			error;

	NFSLOG(0x8000,
		"nfs_rename from %s %x ", vtomi(odvp)->mi_hostname, odvp);
	NFSLOG(0x8000, "'%s' to %s ", onm, vtomi(ndvp)->mi_hostname);
	NFSLOG(0x8000, "%x '%s'\n", ndvp, nnm);

	if (VOP_REALVP(ndvp, &realvp) == 0)
		ndvp = realvp;

	/*
	 * cannot rename . and ..
	 */
	if (!strcmp(onm, ".") || !strcmp(onm, "..") || !strcmp(nnm, ".") ||
						!strcmp (nnm, "..")) {
		NFSLOG(0x20000, "nfs_rename: EINVAL\n", 0, 0);

		error = EINVAL;
	} else {
		dnlc_remove(odvp, onm);
		dnlc_remove(ndvp, nnm);
		setdiropargs(&args.rna_from, onm, odvp);
		setdiropargs(&args.rna_to, nnm, ndvp);
		error = rfscall(vtomi(odvp), RFS_RENAME, 0, xdr_rnmargs,
			(caddr_t)&args, xdr_enum, (caddr_t)&status, cred);
		PURGE_ATTRCACHE(odvp);
		PURGE_ATTRCACHE(ndvp);
		if (!error) {
			error = geterrno(status);
			PURGE_STALE_FH(error, odvp);
			PURGE_STALE_FH(error, ndvp);
		}
	}

	NFSLOG(0x20000, "nfs_rename returning %d\n", error, 0);

	return (error);
}

/*
 * nfs_mkdir(vnode_t *dvp, char *nm, vattr_t *va, vnode_t *vpp, cred_t *cred)
 *	Create a directory file.
 *
 * Calling/Exit State:
 *	Caller holds no vnode/rnode locks on entry or exit.
 *
 * Description:
 *	Create a directory over nfs.
 *
 * Parameters:
 *
 *	dvp			# directory to create in
 *	nm			# name of dir to create
 *	vpp			# vnode of new dir returned in this
 *	cred			# creds to use
 *
 */
/*ARGSUSED*/
STATIC int
nfs_mkdir(vnode_t *dvp, char *nm, vattr_t *va, vnode_t **vpp, cred_t *cred)
{
	struct	mntinfo		*mi = vtomi(dvp);
	struct	nfscreatargs	args;
	struct	nfsdiropres	*dr;
#ifdef NFSESV
	struct	nfsesvcreatargs	cargs;
	struct	nfsesvdiropres	*cdr;
#endif
	int			error;

	NFSLOG(0x8000, "nfs_mkdir %s %x ", mi->mi_hostname, dvp);
	NFSLOG(0x8000, "'%s'\n", nm, 0);

	dr = (struct nfsdiropres *)kmem_alloc(sizeof (*dr), KM_SLEEP);

#ifdef NFSESV
	cdr = (struct nfsesvdiropres *)kmem_alloc(sizeof (*cdr), KM_SLEEP);
#endif

	if (mi->mi_protocol == NFS_V2)
		setdiropargs(&args.ca_da, nm, dvp);
#ifdef NFSESV
	else
		setdiropargs(&cargs.ca_da, nm, dvp);
#endif

	/*
	 * decide what the group-id and set-gid bit of the created directory
	 * should be. May have to do a setattr to get the gid right.
	 */
	va->va_gid = setdirgid(dvp, cred);
	va->va_mode = (u_short) setdirmode(dvp, va->va_mode);

	/*
	 * also set the uid
	 */
	va->va_uid = cred->cr_uid;

	if (mi->mi_protocol == NFS_V2)
		vattr_to_sattr(va, &args.ca_sa);
#ifdef NFSESV
	else
		/*
		 * XXX: when ACL's are supported we need to deal with
		 * default ACL's here.
		 */
		vattr_to_esvsattr(va, &cargs.ca_sa, &mi->mi_addr,
				&cred->cr_lid, NULL, 0);
#endif

	dnlc_remove(dvp, nm);

	if (mi->mi_protocol == NFS_V2)
		error = rfscall(mi, RFS_MKDIR, 0, xdr_creatargs,
			(caddr_t)&args, xdr_diropres, (caddr_t)dr, cred);
#ifdef NFSESV
	else
		error = rfscall(mi, RFS_MKDIR, 0, xdr_esvcreatargs,
			(caddr_t)&cargs, xdr_esvdiropres, (caddr_t)cdr, cred);
#endif

	PURGE_ATTRCACHE(dvp);

	if (!error) {
#ifdef NFSESV
		error = geterrno((mi->mi_protocol == NFS_V2) ?
			dr->dr_status : cdr->dr_status);
#else
		error = geterrno(dr->dr_status);
#endif
		PURGE_STALE_FH(error, dvp);
	}

	if (!error) {
		gid_t	gid;

		/*
		 * due to a pre-4.0 server bug the attributes that come back
		 * on mkdir are not correct. use them only to set the vnode
		 * type in makenfsnode.
		 */
		if (mi->mi_protocol == NFS_V2)
			*vpp = makenfsnode(&dr->dr_fhandle, &dr->dr_attr,
						dvp->v_vfsp);
#ifdef NFSESV
		else
			*vpp = makeesvnfsnode(&cdr->dr_fhandle, &cdr->dr_attr,
						dvp->v_vfsp);
#endif

		PURGE_ATTRCACHE(*vpp);

		if (nfs_dnlc && !(vtor(*vpp)->r_flags & (REFFMLD|RMLD))) {
			dnlc_enter(dvp, nm, *vpp, NULL, cred);
		}

		/*
		 * make sure the gid was set correctly.
		 * If not, try to set it (but don't lose
		 * any sleep over it).
		 */
		gid = va->va_gid;
		if (mi->mi_protocol == NFS_V2) {
			nattr_to_vattr(*vpp, &dr->dr_attr, va);
		}

#ifdef NFSESV
		else {
			vtor(*vpp)->r_aclcnt = acl_getmax();
			nattresv_to_vattr(*vpp, &cdr->dr_attr, va,
					&(*vpp)->v_lid, vtor(*vpp)->r_acl,
					&vtor(*vpp)->r_aclcnt);
		}
#endif

		if (gid != va->va_gid) {
			va->va_mask = AT_GID;
			va->va_gid = gid;
			(void) nfs_setattr(*vpp, va, 0, 0, cred);
		}
	} else {
		NFSLOG(0x20000, "nfs_mkdir: error %d\n", error, 0);

		*vpp = (struct vnode *)0;
	}

	kmem_free((caddr_t)dr, sizeof(*dr));

#ifdef NFSESV
	kmem_free((caddr_t)cdr, sizeof(*cdr));
#endif

	NFSLOG(0x20000, "nfs_mkdir returning %d\n", error, 0);

	return (error);
}

/*
 * nfs_rmdir(vnode_t *dvp, char *nm, vnode_t *cdir, cred_t *cred)
 *	Remove an nfs diretory.
 *
 * Calling/Exit State:
 *	The caller holds no vnode/rnode locks on entry or exit.
 *
 * Description:
 *	Remove an nfs diretory.
 *
 * Parameters:
 *
 *	dvp			# parent dir
 *	nm			# directory to remove
 *	cdir			# current dir
 *	cred			# lwp cred
 *
 */
/* ARGSUSED */
STATIC int
nfs_rmdir(vnode_t *dvp, char *nm, vnode_t *cdir, cred_t *cred)
{
	enum	nfsstat		status;
	struct	nfsdiropargs	da;
	struct	vnode		*vp;
	int			error;

	NFSLOG(0x8000, "nfs_rmdir %s %x ", vtomi(dvp)->mi_hostname, dvp);
	NFSLOG(0x8000, "'%s'\n", nm, 0);

	/*
	 * Attempt to prevent a rmdir(".") from succeeding
	 */
	if (error = VOP_LOOKUP(dvp, nm, &vp,
					(struct pathname *)NULL, 0,
                                        (struct vnode *) 0, cred)) {
		NFSLOG(0x20000, "nfs_rmdir: error %d\n", error, 0);

		return(error);
	} else {
		if (VN_CMP(vp, cdir)) {
			VN_RELE(vp);

			NFSLOG(0x20000, "nfs_rmdir: EINVAL\n", 0, 0);

			return (EINVAL);
		} else {
			VN_RELE(vp);
		}
	}

	setdiropargs(&da, nm, dvp);
	dnlc_purge_vp(dvp);
	error = rfscall(vtomi(dvp), RFS_RMDIR, 0, xdr_diropargs, (caddr_t)&da,
		xdr_enum, (caddr_t)&status, cred);
	PURGE_ATTRCACHE(dvp);

	if (!error) {
		error = geterrno(status);
		if (error == NFSERR_NOTEMPTY)
			error = EEXIST;
		PURGE_STALE_FH(error, dvp);
	}

	NFSLOG(0x20000, "nfs_rmdir returning %d\n", error, 0);

	return (error);
}

/*
 * int
 * nfs_symlink(vnode_t *dvp, char *lnm, vattr_t *tva, char *tnm, cred_t *cred)
 *	Create a symbolic link file.
 *
 * Calling/Exit State:
 *	Caller holds no vnode/rnode locks on entry or exit.
 *
 * Description:
 *	Create a symbolic link file.
 *
 * Parameters:
 *
 *	dvp			# parent dir of existing file
 *	lnm			# existing file name
 *	tva			# attributes of new link
 *	tnm			# name of link
 *	cred			# lwp cred
 *
 */
STATIC int
nfs_symlink(vnode_t *dvp, char *lnm, vattr_t *tva, char *tnm, cred_t *cred)
{
	struct	mntinfo		*mi = vtomi(dvp);
	struct	nfsslargs	args;
#ifdef NFSESV
	struct	nfsesvslargs	cargs;
#endif
	enum	nfsstat		status;
	int			error;

	NFSLOG(0x8000, "nfs_symlink %s %x ", mi->mi_hostname, dvp);
	NFSLOG(0x8000, "'%s' to '%s'\n", lnm, tnm);

	if (mi->mi_protocol == NFS_V2) {
		setdiropargs(&args.sla_from, lnm, dvp);
		vattr_to_sattr(tva, &args.sla_sa);
		args.sla_sa.sa_uid = u.u_lwpp->l_cred->cr_uid;
		args.sla_sa.sa_gid = setdirgid(dvp, cred);
		if (!(tva->va_mask & AT_SIZE))
			args.sla_sa.sa_size = (u_long)-1;
		if (!(tva->va_mask & AT_ATIME)) {
			args.sla_sa.sa_atime.tv_sec = (u_long)-1;
			args.sla_sa.sa_atime.tv_usec = (u_long)-1;
		}
		if (!(tva->va_mask & AT_MTIME)) {
			args.sla_sa.sa_mtime.tv_sec = (u_long)-1;
			args.sla_sa.sa_mtime.tv_usec = (u_long)-1;
		}
		args.sla_tnm = tnm;

		error = rfscall(mi, RFS_SYMLINK, 0, xdr_slargs,
			(caddr_t)&args, xdr_enum, (caddr_t)&status, cred);
	}

#ifdef NFSESV
	else {
		setdiropargs(&cargs.sla_from, lnm, dvp);
		vattr_to_esvsattr(tva, &cargs.sla_sa, &mi->mi_addr,
				&cred->cr_lid, NULL, 0);
		cargs.sla_sa.sa_uid = u.u_lwpp->l_cred->cr_uid;
		cargs.sla_sa.sa_gid = setdirgid(dvp, cred);
		if (!(tva->va_mask & AT_SIZE))
			cargs.sla_sa.sa_size = (u_long)-1;
		if (!(tva->va_mask & AT_ATIME)) {
			cargs.sla_sa.sa_atime.tv_sec = (u_long)-1;
			cargs.sla_sa.sa_atime.tv_usec = (u_long)-1;
		}
		if (!(tva->va_mask & AT_MTIME)) {
			cargs.sla_sa.sa_mtime.tv_sec = (u_long)-1;
			cargs.sla_sa.sa_mtime.tv_usec = (u_long)-1;
		}
		cargs.sla_tnm = tnm;

		error = rfscall(mi, RFS_SYMLINK, 0, xdr_esvslargs,
			(caddr_t)&cargs, xdr_enum, (caddr_t)&status, cred);
	}
#endif

	PURGE_ATTRCACHE(dvp);

	if (!error) {
		error = geterrno(status);
		PURGE_STALE_FH(error, dvp);
	}

	NFSLOG(0x20000, "nfs_sysmlink: returning %d\n", error, 0);

	return (error);
}

/*
 * nfs_readdir(vnode_t *vp, struct uio *uiop, cred_t *cred, int *eofp)
 *	Read from an nfs directory.
 *
 * Calling/Exit State:
 *	The calling LWP holds the rnode's rwlock in *shared* mode. The
 *	rwlock was obtained by a call to VOP_RWRDLOCK.
 *
 *	A return value of not -1 indicates success; otherwise a valid
 *	errno is returned. 
 *
 *	On success, an <*eofp> value of 1 indicates that end-of-file
 *	has been reached, i.e., there are no more directory entries
 *	that may be read.
 *
 * Description:
 * 	Read directory entries.
 *
 * 	There are some weird things to look out for here. The uio_offset
 * 	field is either 0 or it is the offset returned from a previous
 * 	readdir. It is an opaque value used by the server to find the
 * 	correct directory block to read. The byte count must be at least
 * 	vtoblksz(vp) bytes. The count field is the number of blocks to
 * 	read on the server. This is advisory only, the server may return
 * 	only one block's worth of entries. Entries may be compressed on
 * 	the server.
 *
 * Parameters:
 *
 *	vp			# directory vnode
 *	uiop			# uio to return entries in
 *	cred			# caller's creds
 *	eofp			# end-pf-file indicator
 */
STATIC int
nfs_readdir(vnode_t *vp, struct uio *uiop, cred_t *cred, int *eofp)
{
	struct	mntinfo		*mi = vtomi(vp);
	struct	iovec		*iovp;
	unsigned		alloc_count, count;
	struct	nfsrddirargs	rda;
	struct	nfsrddirres 	rd;
#ifdef NFSESV
	struct	nfsesvrddirres	crd;
#endif
	struct	rnode		*rp = vtor(vp);
	int			error = 0;
	pl_t			opl;

	/*
	 * TODO: we may be able to release the r_rwlock here
	 * and grab it again at the end of this routine, because
	 * the real locking is done on the server.
	 *
	 */

	if (eofp)
		*eofp = 0;

	/*
	 * N.B.: It appears here that we're treating the directory
	 * cookie as an offset. Not true. It's simply that getdents
	 * passes us the cookie to use in the uio_offset field of a
	 * uio structure.
	 */
	opl = LOCK(&rp->r_statelock, PLMIN);
	if ((rp->r_flags & REOF) &&
			(rp->r_size == (u_long)uiop->uio_offset) &&
			(hrestime.tv_sec < rp->r_attrtime.tv_sec)) {
		UNLOCK(&rp->r_statelock, opl);
		if (eofp)
			*eofp = 1;

		NFSLOG(0x8000, "nfs_readdir retuns eofp", 0, 0);

		return (0);
	}
	UNLOCK(&rp->r_statelock, opl);

	iovp = uiop->uio_iov;
	alloc_count = count = iovp->iov_len;

	/*
	 * XXX: SunOS 3.2 servers apparently cannot always handle an
	 * RFS_READDIR request with rda_count set to more than 0x400. So
	 * we reduce the request size here purely for compatibility.
	 */
	if (count > 0x400)
		count = 0x400;

	NFSLOG(0x8000, "nfs_readdir %s %x ", mi->mi_hostname, vp);
	NFSLOG(0x8000, "count %d offset %d\n", count, uiop->uio_offset);

	/*
	 * XXX: we should do some kind of test for count >= DEV_BSIZE
	 */
	if (uiop->uio_iovcnt != 1) {
		NFSLOG(0x20000, "nfs_readdir: EINVAL\n", 0, 0);

		return (EINVAL);
	}

	cred = crdup(cred);
	mac_rele(cred->cr_lid);
	cred->cr_lid = u.u_procp->p_cred->cr_lid;
	mac_hold(cred->cr_lid);
	cred->cr_wkgpriv = u.u_procp->p_cred->cr_wkgpriv;
	cred->cr_maxpriv = u.u_procp->p_cred->cr_maxpriv;

	rda.rda_offset = uiop->uio_offset;
	rd.rd_entries = (struct dirent *)kmem_alloc(alloc_count, KM_SLEEP);

#ifdef NFSESV
	crd.rd_entries = (struct dirent *)kmem_alloc(alloc_count, KM_SLEEP);
#endif

	rda.rda_fh = *vtofh(vp);

	do {
		count = MIN(count, mi->mi_curread);
		rda.rda_count = count;
		rd.rd_size = count;
#ifdef NFSESV
		crd.rd_size = count;
#endif

		if (mi->mi_protocol == NFS_V2)
			error = rfscall(mi, RFS_READDIR, 0, xdr_rddirargs,
				(caddr_t)&rda, xdr_getrddirres, (caddr_t)&rd,
				cred);

#ifdef NFSESV
		else
			error = rfscall(mi, RFS_READDIR, 0, xdr_rddirargs,
				(caddr_t)&rda, xdr_esvgetrddirres,
				(caddr_t)&crd, cred);
#endif

	} while (error == ENFS_TRYAGAIN);

	crfree(cred);
	if (!error) {
#ifdef NFSESV
		error = geterrno((mi->mi_protocol == NFS_V2) ?
			rd.rd_status : crd.rd_status);
#else
		error = geterrno(rd.rd_status);
#endif
		PURGE_STALE_FH(error, vp);
	}

	if (!error) {
		/*
		 * move dir entries to user land
		 */
		if (mi->mi_protocol == NFS_V2) {
			if (rd.rd_size) {
				error = uiomove((caddr_t)rd.rd_entries,
					(int)rd.rd_size, UIO_READ, uiop);
				rda.rda_offset = rd.rd_offset;
				uiop->uio_offset = rd.rd_offset;
			}

			if (rd.rd_eof) {
				opl = LOCK(&rp->r_statelock, PLMIN);
				rp->r_flags |= REOF;
				rp->r_size = uiop->uio_offset;
				UNLOCK(&rp->r_statelock, opl);
				if (!error && eofp)
					*eofp = 1;
			}
		}

#ifdef NFSESV
		else {
			timestruc_t	mtime;

			if (crd.rd_size) {
				error = uiomove((caddr_t)crd.rd_entries,
					(int)crd.rd_size, UIO_READ, uiop);
				rda.rda_offset = crd.rd_offset;
				uiop->uio_offset = crd.rd_offset;
			}

			if (crd.rd_eof) {
				opl = LOCK(&rp->r_statelock, PLMIN);
				rp->r_flags |= REOF;
				rp->r_size = uiop->uio_offset;
				UNLOCK(&rp->r_statelock, opl);

				if (!error && eofp)
					*eofp = 1;
			}

			mtime.tv_sec = crd.rd_attr.na_mtime.tv_sec;
			mtime.tv_nsec = crd.rd_attr.na_mtime.tv_usec * 1000;
			nfs_esvcache_check(vp, mtime, crd.rd_attr.na_size);
			nfs_esvattrcache(vp, &crd.rd_attr);
		}
#endif

	}

	kmem_free((caddr_t)rd.rd_entries, alloc_count);

#ifdef NFSESV
	kmem_free((caddr_t)crd.rd_entries, alloc_count);
#endif

	NFSLOG(0x8000, "nfs_readdir: returning %d resid %d, ", error,
			uiop->uio_resid);
	NFSLOG(0x8000, "offset %d\n", uiop->uio_offset, 0);

	return (error);
}

/*
 * nfs_fid(vnode_t *vp, struct fid **fidpp)
 *	Return file id, not supported.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Not supported in NFS.
 *
 * Parameters:
 *
 */
/* ARGSUSED */
STATIC int
nfs_fid(vnode_t *vp, struct fid **fidpp)
{
	NFSLOG(0x20000, "nfs_fid: called\n", 0, 0);

	return (ENOSYS);
}

/*
 * nfs_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
 *	Obtain, if possible, the rnode's rwlock according to <mode>.
 *
 * Calling/Exit State:
 *	A return value of 0 indicates success.
 *
 *	On success, the rwlock of the rnode is held according to
 *	mode. It is also guaranteed that the caller will not block
 *	on I/O operations to the range indicated by <off, len>
 *	while holding the rwlock (i.e., until a subsequent
 *	VOP_RWUNLOCK() is performed).
 *
 *	On failure, the rwlock of the inode is *not* held.
 *
 *	NOTE: Currently, <off> and <len> are ignored. In the future,
 *	for example, they might be used for a finer grained locking
 *	scheme.
 *
 * Parameters:
 *
 *	vp			# vnode to lock
 *	off			# offset to lock from
 *	len			# len to lock
 *	fmode			# file mode, not used
 *	mode			# locking mode
 *
 */
/* ARGSUSED */
STATIC int
nfs_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
{
	struct	rnode	*rp = vtor(vp);

	NFSLOG(0x8000, "nfs_rwlock: entered\n", 0,0);

	if (mode == LOCK_EXCL) {
		RWSLEEP_WRLOCK(&rp->r_rwlock, PRINOD);
	} else if (mode == LOCK_SHARED) {
		RWSLEEP_RDLOCK(&rp->r_rwlock, PRINOD);
	} else {
		/*
		 *+ Invalid lock mode requested.
		 *+ This indicates a software problem in the kernel.
		 */
		cmn_err(CE_PANIC, "nfs_rwlock: invalid lock mode requested");
	}

	return (0);
}

/*
 * nfs_rwunlock(vnode_t *vp, off_t off, int len)
 *	Release the rnode's rwlock.
 *
 * Calling/Exit State:
 *	On entry, the calling LWP must hold the rnode's rwlock
 *	in either *shared* or *exclusive* mode. On exit, the
 *	caller's hold on the lock is released.
 *
 *	NOTE: Currently, <off> and <len> are ignored. In the future,
 *	for example, they might be used for a finer grained locking
 *	scheme.
 *
 * Parameters:
 *
 *	vp			# vnode to unlock
 *	off			# offset to unlock from
 *	len			# len to unlock
 */
/* ARGSUSED */
STATIC void
nfs_rwunlock(vnode_t *vp, off_t off, int len)
{
	struct	rnode	*rp = vtor(vp);

	NFSLOG(0x8000, "nfs_rwunlock: entered\n", 0,0);

	RWSLEEP_UNLOCK(&rp->r_rwlock);
}

/*
 * nfs_seek(vnode_t *vp, off_t ooff, off_t *noffp)
 *	Validate a seek pointer.
 *
 * Calling/Exit State:
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. 
 *
 * Description:
 *	Validate a seek pointer. No locking is necessary since the
 *	result of this routine depends entirely on the value of <*noffp>.
 *
 * Parameters:
 *
 *	vp			# vnode to validate seek ptr of
 *	ooff			# old seek ptr
 *	noffp			# new seek ptr
 *
 */
/* ARGSUSED */
STATIC int
nfs_seek(vnode_t *vp, off_t ooff, off_t *noffp)
{
	NFSLOG(0x20000, "nfs_seek: entered\n", 0,0);

	return (*noffp < 0 ? EINVAL : 0);
}

/*
 * nfs_map(vnode_t *vp, off_t off, struct as *as, vaddr_t *addrp, *uint_t len,
 *		 uint_t prot, uint_t maxprot, uint_t flags, cred_t *cred)
 *	Map a part of whole of an nfs file.
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Map the specified <vp, offset> at address "addrp" of address
 *	space "as". The address space is locked exclusive before calling 
 *	as_map() to prevent multiple lwp's from extablishing or relinquish
 *	mappings concurrently.
 *
 * Parameters:
 *
 *	vp			# vnode to map from
 *	off			# offset to map from
 *	as			# addr space to map to
 *	addrp			# addr to map to in as
 *	len			# length of mapping
 *	prot			# prot for mapped pages
 *	maxprot			# maxprot for mapped pages
 *	flags			# type of maping
 *	cred			# lwp creds
 */
/*ARGSUSED*/
STATIC int
nfs_map(vnode_t *vp, off_t off, struct as *as, vaddr_t *addrp, uint_t len, 
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *cred)
{
	struct	segvn_crargs	vn_a;
	struct	rnode		*rp = vtor(vp);
	int			error;
	pl_t			opl;

	if ((int)off < 0 || (int)(off + len) < 0) {
		/*
		 * bad offset
		 */
		NFSLOG(0x20000, "nfs_map: EINVAL off\n", 0,0);

		return (EINVAL);
	}

	if (vp->v_flag & VNOMAP) {
		/*
		 * no mapping allowed of this vnode.
		 */
		NFSLOG(0x20000, "nfs_map: ENOSYS\n", 0,0);

		return (ENOSYS);
	}

	/*
	 * can map only reg files
	 */
	if (vp->v_type != VREG) {
		NFSLOG(0x20000, "nfs_map: ENODEV\n", 0,0);

		return (ENODEV);
	}

	/*
	 * if file is being locked, disallow mapping.
	 */
	if (vp->v_filocks != NULL)
		return (EAGAIN);

	/*
	 * check to see if the vnode is currently marked as not cachable.
	 * if so, we have to refuse the map request as this violates the
	 * don't cache attribute.
	 */
	if (rp->r_flags & RNOCACHE) {
		NFSLOG(0x20000, "nfs_map: EIO\n", 0,0);

		return (EIO);
	}

	/*
	 * both rnode and as have to be write locked.
	 */
	RWSLEEP_WRLOCK(&vtor(vp)->r_rwlock, PRINOD);
	as_wrlock(as);

	if ((flags & MAP_FIXED) == 0) {
		map_addr(addrp, len, (off_t)off, 0);
		if (*addrp == NULL) {
			as_unlock(as);

			NFSLOG(0x20000, "nfs_map: ENOMEM, map_addr\n", 0,0);

			return (ENOMEM);
		}
	} else {
		/*
		 * user specified address - blow away any previous mappings
		 */
		(void) as_unmap(as, *addrp, len);
	}

	/*
	 * cred handling: see nfs_read()
	 */
	cred = crdup(cred);
	mac_rele(cred->cr_lid);
	cred->cr_lid = u.u_lwpp->l_cred->cr_lid;
	mac_hold(cred->cr_lid);
	cred->cr_wkgpriv = u.u_lwpp->l_cred->cr_wkgpriv;
	cred->cr_maxpriv = u.u_lwpp->l_cred->cr_maxpriv;

	/*
	 * set up args for as_map
	 */
	vn_a.vp = vp;
	vn_a.offset = off;
	vn_a.type = flags & MAP_TYPE;
	vn_a.prot = (u_char)prot;
	vn_a.maxprot = (u_char)maxprot;
	vn_a.cred = cred;

	error = as_map(as, *addrp, len, segvn_create, &vn_a);

	as_unlock(as);
	RWSLEEP_UNLOCK(&vtor(vp)->r_rwlock);

	/*
	 * install the creds in the rnode only if rp->r_cred is null.
	 */
	opl = LOCK(&rp->r_statelock, PLMIN);
	if (rp->r_cred)
		crfree(cred);
	else
		rp->r_cred = cred;
	UNLOCK(&rp->r_statelock, opl);

	NFSLOG(0x20000, "nfs_map: returns %d\n", error, 0);

	return (error);
}

/*
 * nfs_addmap(vnode_t *vp, uint_t off, as_t *as, vaddr_t addr, uint_t len,
 *		uchar_t prot, uchar_t maxprot, uint_t flags, cred_t *cred)
 *	Add a mapping of an nfs file.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	NFS now keeps a map count.
 *
 * Parameters:
 *
 *	vp			# vnode being mapped
 *	off			# offset of mapping
 *	as			# addr space being mapped in
 *	addr			# addr being mapped in as
 *	len			# length of mapping
 *	prot			# prot for mapped pages
 *	maxprot			# maxprot for mapped pages
 *	flags			# type of maping
 *	cred			# lwp creds
 *
 */
/*ARGSUSED*/
STATIC int
nfs_addmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len, 
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *cred)
{
	struct	rnode	*rp = vtor(vp);
	int		error;
	pl_t		opl;

	NFSLOG(0x8000, "nfs_addmap: entered\n", 0,0);

	if (vp->v_flag & VNOMAP) {
		NFSLOG(0x20000, "nfs_addmap: ENOSYS\n", 0,0);

		error = ENOSYS;
	} else {
		/*
		 * increment the map count by the number of pages being mapped.
		 */
		opl = LOCK(&rp->r_statelock, PLMIN);
		rp->r_mapcnt += btopr(len);
		UNLOCK(&rp->r_statelock, opl);
		error = 0;
	}

	return (error);
}

/*
 * nfs_delmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, 
 *	uint_t len, uint_t prot, uint_t maxprot, uint_t flags, cred_t *cr)
 *	Decrement mapping count.
 *
 * Calling/Exit State:
 *
 * Description:
 *	Decrement the count of active mappings of vp by the number
 *	of pages of mapping being relinquished.
 *
 * Parameters:
 *
 *	vp			# vnode mapped
 *	off			# offset of mapping
 *	as			# addr space mapped
 *	addr			# addr mapped in as
 *	len			# length of mapping
 *	prot			# prot for mapped pages
 *	maxprot			# maxprot for mapped pages
 *	flags			# type of maping
 *	cred			# lwp creds
 *
 */
/*ARGSUSED*/
STATIC int
nfs_delmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, 
	   uint_t len, uint_t prot, uint_t maxprot, uint_t flags, cred_t *cr)
{
	struct	rnode	*rp = vtor(vp);
	int		error;
	pl_t		opl;

	NFSLOG(0x8000, "nfs_delmap: entered\n", 0,0);

	if (vp->v_flag & VNOMAP) {
		NFSLOG(0x20000, "nfs_delmap: ENOSYS\n", 0,0);

		error = ENOSYS;
	} else {
		/*
		 * decrease the map count by the number of pages being
		 * unmapped.
		 */
		opl = LOCK(&rp->r_statelock, PLMIN);
		rp->r_mapcnt -= btopr(len);
		ASSERT(rp->r_mapcnt >= 0);
		UNLOCK(&rp->r_statelock, opl);
		error = 0;
	}

	return (error);
}

/*
 * nfs_frlock(vnode_t *vp, int cmd, struct flock *bfp, int flag,
 *		off_t offset, cred_t *cr)
 *	Establish or interrogate the state of an advisory
 *	lock on the vnode.
 *
 * Calling/Exit State:
 *	No vnode/rnode locks are held on entry or exit.
 *
 * Description:
 *	Establish or interrogate the state of an advisory
 *	lock on the vnode. We do not need to acquire the
 *	rnode rwlock here as we do not support mandatory
 *	file/record locking over nfs and we do not use
 *	the file/record locking routines on the client.
 *
 *	We now use v_filocks in the vnode to keep an account
 *	of the file/record locks on this vnode.	However, it
 *	is only used to determine if the process holds any
 *	such locks on the file at close time, so that we
 *	make the call to the lock manager only when needed.
 *
 * Parameters:
 *
 *	vp			# vnode to get/check lock of.
 *	cmd			# what to do.
 *	bfp			# describes the lock to get/check.
 *	flag			# file pointer flags.
 *	offset			# offset if file.
 *	cred			# lwp credentials.
 */
/* ARGSUSED */
STATIC int
nfs_frlock(vnode_t *vp, int cmd, struct flock *bfp, int flag,
	   off_t offset, cred_t *cred)
{
	struct	rnode	*rp = vtor(vp);
	proc_t		*pp = u.u_procp;
	lockhandle_t	lh;
	int		error = 0;
	short		whence;
	pl_t		opl;
	off_t		tmp_l_end;

	NFSLOG(0x8000, "nfs_frlock(): cmd %d, flag %d, ", cmd, flag);
	NFSLOG(0x8000, "offset %d, start %d, ", offset, bfp->l_start);
	NFSLOG(0x8000, "len %d, whence %d\n", bfp->l_len, bfp->l_whence);

	/*
	 * allow SVR3 apps to work.
	 */
	if (cmd == F_O_GETLK)
		cmd = F_GETLK;

	if (cmd != F_GETLK && cmd != F_SETLK && cmd != F_SETLKW) {

		NFSLOG(0x20000, "nfs_frlock: Invalid command %d\n", cmd, 0);

		return (EINVAL);
	}

	switch (bfp->l_type) {

		case F_RDLCK:
			if (!((cmd == F_GETLK) || (cmd == F_RGETLK)) &&
				!(flag & FREAD)) {

				return (EBADF);
			}

			break;
 
		case F_WRLCK:
			if (!((cmd == F_GETLK) || (cmd == F_RGETLK)) &&
				!(flag & FWRITE)) {

				NFSLOG(0x20000, "nfs_frlock: EBADF\n", 0, 0);

				return (EBADF);
			}

			break;
 
		case F_UNLCK:

			break;

		default:

			NFSLOG(0x20000, "nfs_frlock: EINVAL, default\n", 0, 0);

			return (EINVAL);
	} 

	/*
	 * if we are setting a lock mark the rnode RNOCACHE so the page
	 * cache does not give inconsistent results on locked files shared
	 * between clients. the RNOCACHE flag is never turned off as long
	 * as the rnode is active because it is hard to figure out when the
	 * last lock is gone.
	 *
	 * XXX: what if some already has the rnode mapped in?
	 */
	if (((rp->r_flags & RNOCACHE) == 0) &&
			(bfp->l_type != F_UNLCK) && (cmd != F_GETLK)) {
		rp->r_flags |= RNOCACHE;
		error = nfs_putpage(vp, 0, 0, B_INVAL, cred);
		PURGE_ATTRCACHE(vp);
		rp->r_error = error;
	}

	whence = bfp->l_whence;
	if (error = convoff(rp->r_size, bfp, 0, offset)) {

		NFSLOG(0x20000, "nfs_frlock: convoff: error %d\n", error, 0);

		return(error);
	}

	/*
	 * check l_len
	 */
	if (bfp->l_len < 0) {
		/*
		 * adjust the start point and make len its abolute value.
		 */
		bfp->l_start += bfp->l_len;
		bfp->l_len = -(bfp->l_len);
	}

	tmp_l_end = bfp->l_start + bfp->l_len - 1;

	/*
	 * check for validity and arith overflow.
	 */
	if (bfp->l_len && (bfp->l_start > tmp_l_end)) {

		NFSLOG(0x20000, "nfs_frlock: arithmetic overflow\n", 0, 0);

		return (EINVAL);
	}

	/*
	 * set pid and sysid into flock struct
	 */
	bfp->l_pid = pp->p_epid;
	bfp->l_sysid = pp->p_sysid;

	/*
	 * setup the lock handle
	 */
	lh.lh_vp = vp;
	lh.lh_servername = vtomi(vp)->mi_hostname;
	bcopy((caddr_t)vtofh(vp), (caddr_t)&lh.lh_id, sizeof (fhandle_t));

	/*
	 * make the call to the local lock manager.
	 */
	error = klm_lockctl(&lh, bfp, cmd, cred, pp->p_epid);

	/*
	 * restore l_len.
	 */
	if (bfp->l_end == MAXEND)
		bfp->l_len = 0;

	/*
	 * POSIX compatibility requires that we disable this perfectly
	 * nice behavior. The returned lock description will be normalized
	 * relative to SEEK_SET (0) instead of being converted back to the
	 * format that was passed in.
	 *
	 * (void) convoff(rp->r_size, lckdat, whence, offset);
	 */

	/*
	 * we want to keep track of locking activies on the client side,
	 * so as to do appropriate unlock calls during a close().
	 */
	if ((!error) && (cmd == F_SETLK || cmd == F_SETLKW)) {
		struct	filock	*new;
		struct	filock	*insrtp;
		struct	filock	*ptr;
		int		used = 0;

		/*
		 * we will allocate the filock here as v_filocks_mutex is a
		 * spin lock. we will deallocate it later, if is not used.
		 */
		new = (filock_t *) kmem_zalloc(sizeof (filock_t), KM_SLEEP);

		opl = LOCK(&vp->v_filocks_mutex, FS_VPFRLOCKPL);

		/*
		 * v_filocks list is ordered in ascending order by pid.
		 */
		insrtp = NULL;
		ptr = vp->v_filocks;

		/*
		 * try to find the filock for this pid in the list, and set
		 * the insrtp where a new filock may be inserted if bfp is
		 * a new lock.
		 */
		while (ptr != NULL) {
			/*
			 * look for the insertion point
			 */
			if (pp->p_epid > ptr->set.l_pid)
				insrtp = ptr;
			if (pp->p_epid == ptr->set.l_pid)
				break;
			else
				ptr = ptr->next;
		}

		if (bfp->l_type != F_UNLCK) {
			/*
			 * we are getting a lock
			 */
			if (ptr == NULL) {
				/*
				 * a filock doesn't exist for this pid.
				 * create one and add it to the list at the
				 * location after insrtp.
				 */
				new->set.l_pid = pp->p_epid;
				new->set.l_sysid = bfp->l_sysid;
				new->set.l_start = bfp->l_start;
				/*
				 * store end point.
				 */
				new->set.l_end = (bfp->l_len == 0) ?
							0 : tmp_l_end;
				if (insrtp == NULL) {
					new->next = vp->v_filocks;
					if (new->next != NULL)
						new->next->prev = new;
					vp->v_filocks = new;
				} else {
					new->next = insrtp->next;
					if (insrtp->next != NULL)
						insrtp->next->prev = new;
					insrtp->next = new;
				}
				new->prev = insrtp;
				used = 1;
			} else {
				/*
				 * if there is a filock, update the
				 * l_start & l_end so that it covers
				 * the region of new lock bfp.
				 */
				if (bfp->l_start < ptr->set.l_start)
					ptr->set.l_start = bfp->l_start;
				if (bfp->l_len == 0)
					ptr->set.l_end = 0;
				else if (tmp_l_end > ptr->set.l_end)
					ptr->set.l_end = tmp_l_end;
			}
		} else {
			/*
			 * we are unlocking.
			 */
			if (ptr != NULL) {
				/*
				 * we only want to remove the filock
				 * for this pid iff the unlock covers
				 * the whole region of the filock in
				 * the list.
				 *
				 * this only means that if the process
				 * does unlocks of sections of locks, it
				 * may end up making a call to the lock
				 * manager at close time, even when it
				 * does not hold any locks. however, rarely
				 * do processes unlock in sections.
				 */
				if ((bfp->l_start <= ptr->set.l_start) &&
				  ((bfp->l_len == 0) ||
				  (tmp_l_end >= ptr->set.l_end))) {
					if (ptr->prev != NULL)
						ptr->prev->next = ptr->next;
					else
						vp->v_filocks = ptr->next;
					if (ptr->next != NULL)
						ptr->next->prev = ptr->prev;

					kmem_free((caddr_t)ptr,
							sizeof (filock_t));
				}
			}
		}

		UNLOCK(&vp->v_filocks_mutex, opl);

		/*
		 * release the allocated filock if it was not consumed.
		 */
		if (used == 0)
			kmem_free((caddr_t)new, sizeof (filock_t));
	}

	NFSLOG(0x20000, "nfs_frlock: error = %d\n", error, 0);

	return (error);
}

/*
 * nfs_realvp(vnode_t *vp, vnode_t **vpp)
 *	Return real vnode behind vp.
 *
 * Calling/Exit State:
 *	No vnode/rnode locks are held on entry or exit.
 *
 * Description:
 *	Not supported in nfs.
 *
 * Parameters:
 *
 *	vp			# vnode to get real vnode of
 *	vpp			# to return real vnode in
 *
 */
/*ARGSUSED*/
STATIC int
nfs_realvp(vnode_t *vp, vnode_t **vpp)
{
	NFSLOG(0x20000, "nfs_realvp: called\n", 0, 0);

	return (EINVAL);
}

/*
 * nfs_makemld(vnode_t *dvp, char *nm, vattr_t *vap, vnode_t **vpp,
 * 		cred_t *cred)
 *	Make a multilevel dir.
 *
 * Calling/Exit State:
 *	No lock is held on entry or exit.
 *
 * Description:
 * 	Make a multi-level directory. This operation is allowed only if 
 *	the client is using the ESV protocol. We try to make a MLD, 
 *	using SA_SETMLD, and send the request remote. If the response 
 *	is success but NA_TSTMLD finds no MLD, remove the (ordinary) 
 *	directory we just created and return ENOSYS.
 *
 * Parameters:
 *
 *	dvp			# directory vnode
 *	nm			# name of mld
 *	vap			# attr of new mld
 *	vpp			# to return vnode if mld created
 *	cred			# lwp creds
 *	
 */
/*ARGSUSED*/
STATIC int
nfs_makemld(vnode_t *dvp, char *nm, vattr_t *vap, vnode_t **vpp, cred_t *cred)
{
	struct	mntinfo		*mi = vtomi(dvp);
#ifdef NFSESV
	struct	nfsesvcreatargs	cargs;
	struct	nfsesvdiropres	cdr;
#endif
	int			error = 0;

	NFSLOG(0x8000, "nfs_makemld %s, %x ", mi->mi_hostname, dvp);
	NFSLOG(0x8000, "'%s'\n", nm, 0);

	if (mi->mi_protocol != NFS_ESV) {

		NFSLOG(0x20000, "nfs_makemld: not ESV\n", 0, 0);

		return (ENOSYS);
	}

#ifdef NFSESV

	setdiropargs(&cargs.ca_da, nm , dvp);
	vap->va_gid = setdirgid(dvp, cred);
	vap->va_mode = (u_short)setdirmode(dvp, vap->va_mode);
	vap->va_uid = cred->cr_uid;

	/*
	 * XXX: need to deal with default ACL entries here
	 */
	vattr_to_esvsattr(vap, &cargs.ca_sa, &mi->mi_addr, &cred->cr_lid,
			NULL, 0);
	SA_SETMLD(&cargs.ca_sa);

	dnlc_remove(dvp, nm);
	error = rfscall(mi, RFS_MKDIR, 0, xdr_esvcreatargs, (caddr_t)&cargs,
			xdr_esvdiropres, (caddr_t)&cdr, cred);

	PURGE_ATTRCACHE(dvp);
	if (!error) {
		error = geterrno(cdr.dr_status);
		PURGE_STALE_FH(error, dvp);

		/*
		 * Many errors may simply mean
		 * the remote machine doesn't
		 * suuport MLD's
		 */
		if (error == EINVAL)
			error = ENOSYS;
	}

	if (!error) {
		gid_t gid;

		*vpp = makeesvnfsnode(&cdr.dr_fhandle, &cdr.dr_attr,
				dvp->v_vfsp);

		PURGE_ATTRCACHE(dvp);
		if (!NA_TSTMLD(&cdr.dr_attr)) {
			struct nfsdiropargs rmda;
			enum nfsstat rmstatus;

			/*
			 * created an ordinary directory. Remove it
			 * and return ENOSYS
			 */
			setdiropargs(&rmda, nm, dvp);
			(void)rfscall(mi, RFS_RMDIR, 0, xdr_diropargs,
					(caddr_t)&rmda, xdr_enum,
					(caddr_t)&rmstatus, cred);
			VN_RELE(*vpp);
			*vpp = NULL;
			return (ENOSYS);
		}

		gid = vap->va_gid;
		vtor(*vpp)->r_aclcnt = acl_getmax();
		nattresv_to_vattr(*vpp, &cdr.dr_attr, vap, &(*vpp)->v_lid,
				vtor(*vpp)->r_acl, &vtor(*vpp)->r_aclcnt);
		if (gid != vap->va_gid) {
			vap->va_mask = AT_GID;
			vap->va_gid = gid;
			(void) nfs_setattr(*vpp, vap, 0, 0, cred);
		}
	} else {
		NFSLOG(0x20000, "nfs_makemld: error %d\n", error, 0);

		*vpp = (struct vnode *)0;
	}

	NFSLOG(0x20000, "nfs_makemld: returning %d\n", error, 0);

#endif

	return (error);
}

/*
 * nfs_setlevel(vnode_t *vp, lid_t level, cred_t *cred)
 *	Set a lid on file.
 *
 * Calling/Exit State:
 *	No lock is held on entry or exit.
 *
 * Description:
 *	Reset the file level (ESV protocol only). First make some
 *	sanity checks, then send an NFSPROC_SETATTR request with only the file
 *	level set. Set the local level based on the response. We don't do the
 *	full permissions checks here - the server must perform them. We also
 *	don't check for file tranquility, as that information is not available.
 *
 * Parameters:
 *
 *	vp			# vnode to set lid of
 *	level			# level to set
 *	cred			# lwp creds
 *
 */
/*ARGSUSED*/
STATIC int
nfs_setlevel(vnode_t *vp, lid_t level, cred_t *cred)
{
	struct	mntinfo		*mi = vtomi(vp);
#ifdef NFSESV
	struct	nfsesvsaargs	cargs;
	struct	nfsesvattrstat	cns;
	struct	vattr		va;
#endif
	int			error = 0;

	NFSLOG(0x8000, "nfs_setlevel %d %s\n", level, mi->mi_hostname);

	if (mi->mi_protocol != NFS_ESV)
		return (ENOSYS);

#ifdef NFSESV

	if (vp->v_flag & VROOT)
		return (EBUSY);

	if (vtor(vp)->r_attr.va_uid != cred->cr_uid &&
			pm_denied(cred, P_OWNER))
		return (EPERM);

	va.va_uid = (uid_t)-1;
	va.va_gid = (gid_t)-1;
	va.va_size = (size_t)-1;
	va.va_mode = (mode_t)-1;
	va.va_atime.tv_sec = (u_long)-1;
	va.va_atime.tv_nsec = (u_long)-1;
	va.va_mtime.tv_sec = (u_long)-1;
	va.va_mtime.tv_nsec = (u_long)-1;
	vattr_to_esvsattr(&va, &cargs.saa_sa, &mi->mi_addr, &level, NULL, 0);

	cargs.saa_fh = *vtofh(vp);
	error = rfscall(mi, RFS_SETATTR, 0, xdr_esvsaargs, (caddr_t)&cargs,
			xdr_esvattrstat, (caddr_t)&cns, cred);
	if (!error)
		error = geterrno(cns.ns_status);
	if (!error) {
		timestruc_t mtime;

		mtime.tv_sec = cns.ns_attr.na_mtime.tv_sec;
		mtime.tv_nsec = cns.ns_attr.na_mtime.tv_usec * 1000;
		nfs_esvcache_check(vp, mtime, cns.ns_attr.na_size);
		nfs_esvattrcache(vp, &cns.ns_attr);
	} else {
		PURGE_ATTRCACHE(vp);
		PURGE_STALE_FH(error, vp);
	}

	NFSLOG(0x20000, "nfs_setlevel: returning %d\n", error, 0);

#endif

	return (error);
}

/*
 * int 
 * nfs_lockrelease(vnode_t *vp, int flag, off_t offset, cred_t *cred) 
 *	Release all locks on an nfs file.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Release all file and record locks on an nfs file.
 *
 * Parameters:
 *
 *	vp			# vnode to release locks of.
 *	flag			# flag passed on to nfs_frlock
 *	offset			# offset to release all locks from
 *	cred			# lwp creds
 *
 */
int
nfs_lockrelease(vnode_t *vp, int flag, off_t offset, cred_t *cred)
{
	pl_t	opl;

	NFSLOG(0x8000, "nfs_lockrelease: flag %d offset %d\n", flag, offset);

	/*
	 * only do extra work if there are any locks on the file. note that
	 * we do not need the v_filocks_mutex here as we are just peeking.
	 */
	if (vp->v_filocks) {
		proc_t		*pp = u.u_procp;
		struct	flock	ld;
		struct	filock	*ptr;

		opl = LOCK(&vp->v_filocks_mutex, FS_VPFRLOCKPL);
		ptr = vp->v_filocks;
		while (ptr != NULL) {
			if (pp->p_epid == ptr->set.l_pid)
				break;
			else
				ptr = ptr->next;
		}

		if (ptr != NULL) {
			/*
			 * this process has at least one lock. remove its
			 * filock and unlock all its locks.
			 */
			if (ptr->prev != NULL)
				ptr->prev->next = ptr->next;
			else
				vp->v_filocks = ptr->next;
			if (ptr->next != NULL)
				ptr->next->prev = ptr->prev;

			/*
			 * we can release the v_filocks now.
			 */
			UNLOCK(&vp->v_filocks_mutex, opl);

			kmem_free((caddr_t)ptr, sizeof (filock_t));

			/*
			 * setup to unlock entire file.
			 */
			ld.l_type = F_UNLCK;
			ld.l_whence = 0;
			ld.l_start = 0;
			ld.l_len = 0;

			return(nfs_frlock(vp, F_SETLK, &ld, flag,
							offset, cred));
		}

		UNLOCK(&vp->v_filocks_mutex, opl);
	}

	return(0);
}

/*
 * nfs_stablestore(vnode_t **vpp, off_t off, size_t len, void **bmapp,
 *		   cred_t *cr)
 *	Reserve storage associated with a vnode.
 *
 * Calling/Exit State:
 *	No locks are held on entry and exit.
 *
 * Description:
 *	Reserve storage associated with the vnode pointed to by *vpp
 *	starting at offset "off" for the length of "len" bytes. Since
 *	nfs needs no blocks allocated, succeeds silently.
 *
 *	XXX: Swapping over nfs is really needed for diskless
 *	clients, which SYS V does not support. However, basic swap
 *	support is included. Note that under extremely low memory
 *	situations, swapping over nfs is deadlock prone as memory
 *	is allocated all over the networking protocol stack, and we
 *	do not make any provisions for reserving memory.
 *
 * Parameters:
 *
 *	vnode			# reserve storage for this vnode
 *	off			# start at this offset
 *	len			# for this length
 *	bmapp			# backing store mapping data struct
 *				# not used in nfs.
 *
 */
/*ARGSUSED*/
STATIC int
nfs_stablestore(vnode_t **vpp, off_t *off, size_t *len, void **bmapp,
		cred_t *cred)
{
	struct	rnode	*rp = vtor(*vpp);
	pl_t		opl;

	NFSLOG(0x8000, "nfs_stablestore: called\n", 0, 0);

	ASSERT((*vpp)->v_type == VREG);
	ASSERT(len != 0);
	ASSERT((*off & PAGEOFFSET) == 0);
	ASSERT((*len & PAGEOFFSET) == 0);
	ASSERT(rp->r_swapcnt >= 0);

	/*
	 * incr the swap count.
	 */
	opl = LOCK(&rp->r_statelock, PLMIN);
	rp->r_swapcnt++;
	UNLOCK(&rp->r_statelock, opl);

	/*
	 * set the bmapp info to NULL, as we do not use it.
	 */
	*bmapp = NULL;

	return (0);
}

/*
 * nfs_relstore(vnode_t *vp, off_t off, size_t len, void **bmapp, cred_t *cr)
 *	Release storage associated with a vnode.
 *
 * Calling/Exit State:
 *	No locks are held on entry and exit.
 *
 * Description:
 *	Release storage associated with a vnode.
 *
 *	XXX: Swapping over nfs is really needed for diskless
 *	clients, which SYS V does not support. However, basic swap
 *	support is included. Note that under extremely low memory
 *	situations, swapping over nfs is deadlock prone as memory
 *	is allocated all over the networking protocol stack, and we
 *	do not make any provisions for reserving memory.
 *
 */
/*ARGSUSED*/
STATIC int
nfs_relstore(vnode_t *vp, off_t off, size_t len, void *bmapp, cred_t *cr)
{
	struct	rnode	*rp = vtor(vp);
	pl_t		opl;

	NFSLOG(0x8000, "nfs_relstore: called\n", 0, 0);

	ASSERT(bmapp == NULL);
	ASSERT(vp->v_type == VREG);
	ASSERT(len != 0);
	ASSERT((off & PAGEOFFSET) == 0);
	ASSERT((len & PAGEOFFSET) == 0);
	ASSERT(rp->r_swapcnt >= 0);

	/*
	 * decr the swap count.
	*/
	opl = LOCK(&rp->r_statelock, PLMIN);
	rp->r_swapcnt--;
	UNLOCK(&rp->r_statelock, opl);

	return (0);
}

/*
 * nfs_getpagelist(vnode_t *vp, off_t off, page_t *pp, uint_t plsz,
 *			void *bmapp, int flags, cred_t *cr)
 *	Fills in data for each of the pages in the pp list from
 *	consecutive logical offsets starting from off.
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Fills in data for each of the pages in the pp list from
 *	consecutive logical offsets starting from off.
 *
 *	XXX: Swapping over nfs is really needed for diskless
 *	clients, which SYS V does not support. However, basic swap
 *	support is included. Note that under extremely low memory
 *	situations, swapping over nfs is deadlock prone as memory
 *	is allocated all over the networking protocol stack, and we
 *	do not make any provisions for reserving memory.
 *
 * Parameters:
 *
 *	vp			# vnode to get page list of
 *	off			# file offset to get page from
 *	len			# length in file to get
 *	pp			# ptr to return pages in
 *	bmapp			# not used for nfs
 *	flags			# type of io
 *	cred			# lwp creds
 *
 */
/*ARGSUSED*/
STATIC int
nfs_getpagelist(vnode_t *vp, off_t off, uint_t len, page_t *pp,
		void *bmapp, int flags, cred_t *cr)
{
	rnode_t	*rp = vtor(vp);
	page_t	*pp1;
	int	error;

	NFSLOG(0x10000, "nfs_getpagelist: called\n", 0, 0);

	ASSERT(bmapp == NULL);
	ASSERT(vp->v_type == VREG);
	ASSERT((off & PAGEOFFSET) == 0);
	ASSERT((len >= PAGESIZE) && ((len & PAGEOFFSET) == 0));
	ASSERT(off + len <= rp->r_size);
	ASSERT(rp->r_cred != NULL);

	do {
		pp1 = pp;
		pp = pp->p_next;
		error = nfs_getpageio(rp, off, PAGESIZE, pp1, flags);

		if (error != 0)
			break;

		off += PAGESIZE;

	} while (pp != pp1);

	NFSLOG(0x40000, "nfs_getpagelist: returns %d\n", error, 0);

	return (error);
}

/*
 * nfs_putpagelist(vnode_t *vp, off_t off, page_t *pp, void *bmapp,
 *			int flags, cred_t *cr)
 *	Fills in data for each of the pages in the pp list from
 *	consecutive logical offsets starting from off.
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Fills in data for each of the pages in the pp list from
 *	consecutive logical offsets starting from off.
 *
 *	XXX: Swapping over nfs is really needed for diskless
 *	clients, which SYS V does not support. However, basic swap
 *	support is included. Note that under extremely low memory
 *	situations, swapping over nfs is deadlock prone as memory
 *	is allocated all over the networking protocol stack, and we
 *	do not make any provisions for reserving memory.
 *
 * Parameters:
 *
 *	vp			# vnode to put page list of
 *	off			# file offset to put page from
 *	pp			# ptr to pages to put
 *	bmapp			# not used in nfs
 *	flags			# type of io
 *	cred			# lwp creds
 *
 */
/*ARGSUSED*/
STATIC int
nfs_putpagelist(vnode_t *vp, off_t off, page_t *pp, void *bmapp,
		int flags, cred_t *cred)
{
	rnode_t		*rp = vtor(vp);
	page_t		*pp1;
	page_t		*io_list;
	u_int		io_off;
	u_int		io_len = 0;
	u_int		bsize;
	u_int		lbn_off;
	daddr_t		lbn;
	int		error = 0;
	pl_t		opl;

	ASSERT((off & PAGEOFFSET) == 0);

	if (rp->r_cred == NULL) {
		/*
		 * we need creds in the rnode to write the pages to the
		 * servers disk. note that privilged creds are OK here
		 * as the server will have to share this file with root
		 * mapped to root as this is the swap file.
		 *
		 * TODO: this crdup is a potential deadlock. need to get
		 * rid of it.
		 */
		cred = crdup(cred);
		mac_rele(cred->cr_lid);
		cred->cr_lid = u.u_lwpp->l_cred->cr_lid;
		mac_hold(cred->cr_lid);
		cred->cr_wkgpriv = u.u_lwpp->l_cred->cr_wkgpriv;
		cred->cr_maxpriv = u.u_lwpp->l_cred->cr_maxpriv;

		opl = LOCK(&rp->r_statelock, PLMIN);
		if (rp->r_cred)
			crfree(rp->r_cred);
		rp->r_cred = cred;
		UNLOCK(&rp->r_statelock, opl);
	}

	/*
	 * get the block size and set the initial offset to off.
	 */
	bsize = MAX(vp->v_vfsp->vfs_bsize, PAGESIZE);
	io_off = off;

	/*
	 * now go through the list.
	 */
	while ((pp1 = pp) != NULL) {

		/*
		 * io_off is the starting offset for this loop of I/O,
		 * and lbn is the logical block number.
		 */
		io_off += io_len;
		lbn = io_off / bsize;

		/*
		 * take out one page from the list and set the I/O
		 * parameters accordingly.
		 */
		page_sub(&pp, pp1);
		io_list = pp1;
		io_len = PAGESIZE;
		lbn_off = lbn * bsize;

		while (pp != NULL && pp->p_offset < lbn_off + bsize ) {
			/*
			 * we can get more pages.
			 */
			pp1 = pp;
			page_sub(&pp, pp1);
			page_sortadd(&io_list, pp1);
			io_len += PAGESIZE;
		}

		/*
		 * we should now have one blocks worth of pages to write.
		 */
		error = nfs_putpageio(rp, io_list, io_off, io_len, flags, cred);
		if (error)
			break;
	}

	if (error != 0) 
		if (pp!= NULL)
			pvn_fail(pp, B_WRITE | flags);

	return (error);
}

/*
 * nfs_getpageio(rnode_t *rp, off_t off, uint_t len, page_t *pp, int flag)
 *	Do page io over nfs.
 *
 * Calling/Exit State:
 *	The rnode rwlock may or may not be held locked on entry.
 *
 * Description:
 *	Set up for page io and call the strategy routine to
 *	fill the pages. Pages are linked by p_next. If flag is
 *	B_ASYNC, don't wait for io to complete.
 * 
 * Parameters:
 *
 *	rp			# rnode to do io on
 *	off			# page aligned offset for I/O
 *	len			# length of io, may not be page aligned.
 *	pp			# return pages in this list
 *	flag			# type of io
 *
 */
/*ARGSUSED*/
int
nfs_getpageio(rnode_t *rp, off_t off, uint_t len, page_t *pp, int flag)
{
	vnode_t *vp = rtov(rp);
	buf_t	*bp;
	int 	error = 0;

	/*
	 * now round the request size up to page boundaries.
	 * this insures that the entire page will be initialized
	 * to zeroes if EOF is encountered.
	 */
	len = ptob(btopr(len));

	bp = pageio_setup(pp, 0, len, flag | B_READ);

	/*
	 * we now use the b_priv field for passing the vnode
	 * pointer thru to do_bio().
	 */
	bp->b_priv.un_ptr = (void *)vp;

	bp->b_blkno = btodb(off);
	bp->b_edev = 0;
	bp_mapin(bp);

	error = nfs_strategy(bp);

	NFSLOG(0x40000, "nfs_getpageio: returns %d\n", error, 0);

	return (error);
}

/* 
 * nfs_getapage(vnode_t *vp, u_int off, u_int len, u_int *protp, page_t *pl[],
 *	u_int plsz, struct seg *seg, vaddr_t addr, enum seg_rw rw,
 *	cred_t *cred)
 *
 * Calling/Exit State:
 *	The rnode rwlock may or may not be held locked.
 *
 * Description:
 *	Called from pvn_getpages or nfs_getpage to get a particular page.
 *
 *	If pl == NULL, async I/O is requested and I/O is done only if
 *	if it's convenient.
 *
 * Parameters:
 *
 *	vp			# vnode to get the page of
 *	off			# starting offset
 *	len			# requested length of get
 *	protp			# 
 *	pl			# array to return page(s) in
 *	plsz			# size of pl
 *	seg			# segment page belongs to
 *	addr			# virtual address of page
 *	rw			# type of read, segment flags
 *	cred			# caller's creds
 */
/*ARGSUSED*/
STATIC int
nfs_getapage(vnode_t *vp, u_int off, u_int len, u_int *protp, page_t *pl[], 
	u_int plsz, struct seg *seg, vaddr_t addr, enum seg_rw rw, cred_t *cred)
{
	struct	rnode	*rp = vtor(vp);
	enum	seg_rw	orw;
	page_t		*pp;
	page_t		*io_pl[MAXBSIZE/PAGESIZE];
	page_t		**io_pl_ptr;
	u_int		bsize;
	daddr_t		lbn;
	u_int		io_len;
	u_int		vp_len;
	u_int		blkoff;
	off_t		roff;
	off_t		io_off;
	int		error = 0;
	int		i, j;
	int		sz;
	int		readahead;
	int		numpp, numpp_ok;
	pl_t		opl;

	ASSERT(vp->v_type != VDIR);

	NFSLOG(0x10000, "nfs_getapage: vp %x size %d ", vp, rp->r_size);
	NFSLOG(0x10000, "off %d pl %x ", off, pl);
	NFSLOG(0x10000, "addr %x\n", addr, 0);

	/*
	 * "lbn" is block number in file, "blkoff" is offset in file where
	 * relevant block begins and "roff" is rounded offset (to page size).
	 * also the original rw value is saved as we need it later.
	 */
	bsize = vp->v_vfsp->vfs_bsize;
	lbn = off / bsize;
	blkoff = lbn * bsize;
	roff = off & PAGEMASK;
	orw = rw;

	/*
	 * pick up the correct read ahead value.
	 */
	if (rp->r_nextr == off && !(rp->r_flags & RNOCACHE))
		readahead = nfs_nra;
	else
		readahead = 0;

	NFSLOG(0x10000, "nfs_getapage: nextr %d off %d ", rp->r_nextr, off);
	NFSLOG(0x10000, "size %d ra %d ", rp->r_size, readahead);

	if (pl != NULL) {
		/*
		 * normal sync case, try to find the page or create a new one.
		 */
		pp = page_lookup_or_create3(vp, roff, P_NODMA);

		ASSERT(pp != NULL);
		/* ASSERT(!PAGE_USELOCK_OWNED(pp)); */
	
		if (PAGE_IS_RDLOCKED(pp)) {
			/*
			 * the page was in the page cache. check the
			 * file size as it may have changed. if this
			 * is beyond new eof, return eror.
			 */
			if (pp->p_offset > rp->r_size) {
				page_unlock(pp);
				return (EFAULT);
			}

			/*
			 * return the page read-locked to indicate
			 * valid data.
			 */
			*pl++ = pp;
			*pl = NULL;

			NFSLOG(0x10000,
		"nfs_getapage:page in cache pp=%x, procp=%x\n", pp, u.u_procp);

			return (0);
		}
	} else {
		/*
	 	 * this is a read-ahead, call page_lazy_create()
	 	 * which will create the page only if memory is readily
	 	 * available.
	 	 */
		pp = page_lazy_create(vp, roff);

		if (pp == NULL) {
			/*
			 * page could not be created readily.
			 */
			NFSLOG(0x10000,
		"nfs_getapage: lazy create null, procp=%x\n", u.u_procp, 0);

			return (0);
		}
	}

	ASSERT(PAGE_IS_WRLOCKED(pp));
	/* ASSERT(!PAGE_USELOCK_OWNED(pp)); */
	ASSERT(pp->p_offset == roff);

	/*
	 * we need to check the file size again, and abort the page
	 * if it is beyond eof.
	 *
	 * we make an exception for the write() case, as the file
	 * size will not be updated by now. note that we do not
	 * need the statelock as we are only peeking at r_size.
	 */
	if ((rw != S_OVERWRITE) && (pp->p_offset > rp->r_size)) {
		NFSLOG(0x10000,
		"nfs_getapage: aborting page from A pp=%x, procp=%x\n",
				pp, u.u_procp);

		page_abort(pp);

		if (pl != NULL)
			return(EFAULT);
		else
			return (0);
	}

	/*
	 * If file is currently mapped we need to turn off the
	 * S_OVERWRITE optimization.
	 */
	if (rw == S_OVERWRITE && (rp->r_mapcnt > 0))
		rw = S_WRITE;

	/*
	 * if we are overwriting the whole page, we don't need to read
	 * it in. but we need to mark the page as uninitialized and return
	 * it to the caller writer-locked. if doing a write beyond what
	 * we believe is EOF, just zero the page.
 	 */
	if (rw == S_OVERWRITE) {
		ASSERT(pl != NULL);

		if (roff == off && len == PAGESIZE) {
			*pl++ = pp;
			*pl = NULL;

			NFSLOG(0x10000,
		"nfs_getapage: overwriting whole page pp=%x, procp=%x\n",
				pp, u.u_procp);

			return (0);
		 } else if (roff > rp->r_size) {
			/*
			 * past eof, zero out the page and return it
			 * as valid data.
			 */
			pagezero(pp, 0, PAGESIZE);
			page_downgrade_lock(pp);
			*pl++ = pp;
			*pl = NULL;

			NFSLOG(0x10000,
		"nfs_getapage: overwriting whole page Z pp=%x, procp=%x\n",
				pp, u.u_procp);

			return (0);
		}
	}

	/*
	 * need to go to server to get a block
	 */
	opl = LOCK(&rp->r_statelock, PLMIN);
	if (blkoff < rp->r_size && blkoff + bsize > rp->r_size) {
		/*
		 * if less than a block left in file, read less than a block.
		 */
		if (rp->r_size <= off) {
			/*
			 * trying to access beyond EOF,
			 * set up to get at least one page.
			 */
			vp_len = off + PAGESIZE - blkoff;
		} else {
			vp_len = rp->r_size - blkoff;
		}
	} else {
		vp_len = bsize;
	}
	UNLOCK(&rp->r_statelock, opl);

	/*
	 * only do clustering if not write() and the blocksize is greater
	 * than pagesize, and the read offset is within the block.
	 */
	if (orw != S_OVERWRITE
			&& bsize > PAGESIZE && off < blkoff + vp_len) {
		page_t		*pp_curr, *pp_next, *pp_list;
		page_t		*pp_last = NULL;
		int		abort_rest = 0;
		u_long		filesize;

		/*
		 * now do the clustering. note that vp_len may not be a
		 * multiple of PAGESIZE here. so io_len returned may not
		 * be a multiple of PAGESIZE. pp and pp_curr will point to
		 * an ordered list of clustered pages.
		 */
		pp = pp_curr = pvn_kluster(vp, roff, seg, addr,
				&io_off, &io_len, blkoff, vp_len, pp);

		/*
		 * "numpp" if the number of clustered pages returned,
		 * "io_pl_ptr" is a pointer in the io_pl array, "numpp_ok"
		 * will be the number of pages put in io_pl after the loop.
		 */
		numpp = io_len / PAGESIZE;
		if ((io_len & PAGEOFFSET) != 0)
			numpp++;
		io_pl_ptr = io_pl;
		numpp_ok = 0;

		ASSERT((numpp > 0) && (numpp <= bsize / PAGESIZE));

		/*
		 * peek at the file size and save it. we will live with
		 * the size we get here. if the size is shrunk by the time
		 * we return the pages, it will be the responsibility of
		 * the lwp which did that to abort the pages we have
		 * created here.
		 */
		filesize = rp->r_size;

		for (i = 0; i < numpp; i++) {
			ASSERT(PAGE_IS_WRLOCKED(pp_curr));
			/* ASSERT(!PAGE_USELOCK_OWNED(pp_curr)); */

			/*
			 * we need to check again if each page we created is
			 * within the file size.
			 */
			if (abort_rest || (pp_curr->p_offset > filesize)) {
				NFSLOG(0x10000,
		"nfs_getapage: aborting page from B pp=%x, procp=%x\n",
				pp_curr, u.u_procp);

				/*
				 * we need to abort this page. all clustered
				 * pages beyond this one must also be aborted.
				 */
				pp_next = pp_curr->p_next;
				pp_list = pp_curr;
				page_sub(&pp_list, pp_curr);
				page_abort(pp_curr);
				pp_curr = pp_next;
				abort_rest = 1;
			} else {
				NFSLOG(0x10000,
		"nfs_getapage: not aborting page from B pp=%x, procp=%x\n",
				pp_curr, u.u_procp);

				numpp_ok++;
				*io_pl_ptr++ = pp_curr;
				pp_last = pp_curr;
				pp_curr = pp_curr->p_next;
			}
		}

		ASSERT(numpp_ok <= numpp);

		if (numpp_ok == 0) {
			/*
			 * we did not load any pages in io_pl as the file
			 * size shrank. simply return EFAULT.
			 */
			NFSLOG(0x10000, "nfs_getapage: EFAULT C\n", 0, 0);

			return(EFAULT);
		} else if (numpp_ok != numpp) {
			/*
			 * we did not load all of the pages returned to us
			 * by pvn_kluster().
			 */
			if (off > pp_last->p_offset + PAGESIZE) {
				/*
				 * we did not get the page we wanted in
				 * the first place. simply abort all pages
				 * and return EFAULT.
				 */
				for (i = 0; i < numpp_ok; i++) {
					ASSERT(PAGE_IS_WRLOCKED(io_pl[i]));
					page_abort(io_pl[i]);
				}
				NFSLOG(0x10000,
			"nfs_getapage: EFAULT D, numpp_ok=%d\n", numpp_ok, 0);

				return(EFAULT);
			} else {
				/*
				 * adjust io_len to the end of the last page.
				 */
				io_len = io_off + (numpp_ok * PAGESIZE);

				NFSLOG(0x10000,
			"nfs_getapage: case E, numpp_ok=%d\n", numpp_ok, 0);
			}
		}
	} else {
		/*
		 * no clustering, only one page.
		 */
		io_off = roff;
		io_len = PAGESIZE;
		io_pl[0] = pp;
		numpp = numpp_ok = 1;

		NFSLOG(0x10000,
			"nfs_getapage: no cluster, numpp_ok=%d\n", numpp_ok, 0);
	}

	/*
	 * get the pages from the server.
	 */
	error = nfs_getpageio(rp, io_off, io_len, pp,
					pl == NULL ? B_ASYNC : 0);
	if (error == NFS_EOF) {
		/*
		 * this will happen when file size changed after we last
		 * checked it. we did not read a single byte.
		 */
		if (orw == S_OVERWRITE) {
			/*
			 * for the write() case, simply return the zero
			 * filled pages.
			 */
			error = 0;

			NFSLOG(0x10000,
			"nfs_getapage: case F, numpp_ok=%d\n", numpp_ok, 0);
		} else {
			/*
			 * for all other cases, return EFAULT.
			 */
			error = EFAULT;

			/*
			 * since we did not allow pvn_done() to abort the
			 * pages by not marking the buffer with B_ERROR in
			 * do_bio(), we must also abort pages.
			 */
			for (i = 0; i < numpp_ok; i++) {
				ASSERT(PAGE_IS_WRLOCKED(io_pl[i]));
				page_abort(io_pl[i]);
			}

			NFSLOG(0x10000,
			"nfs_getapage: case G, numpp_ok=%d\n", numpp_ok, 0);
		}
	}

	/*
	 * if we encountered any I/O error, the pages should have been
	 * aborted by pvn_done() or above and we need not downgrade the
	 * page lock. also, if pl was null, meaning this was an asynchronous
	 * read ahead, pvn_done() would have unlocked the pages.
	 */
	if (pl == NULL || error) {
		NFSLOG(0x10000, "nfs_getapage: error I/O %d\n", error, 0);

		return (error);
	}

	/*
	 * otherwise, load the pages in the page list to return to caller.
	 */
	if (plsz >= io_len) {
		/*
		 * everything fits in the array we have to return the pages
		 * in (pl). set up to load up all the pages.
		 */
		i = 0;
		sz = io_len;
	} else {
		/*
		 * not everything fits in pl. set up to load plsz worth
		 * starting at the needed page.
		 */
		for (i = 0; io_pl[i]->p_offset != off; i++) {
			ASSERT(i < btopr(io_len) - 1);
		}

		sz = plsz;
	}

	/*
	 * load the pages in pl.
	 */
	j = i;
	io_pl_ptr = pl;
	do {
		*io_pl_ptr = io_pl[j++];

		ASSERT(PAGE_IS_WRLOCKED(*io_pl_ptr));
		/* ASSERT(!PAGE_USELOCK_OWNED(*io_pl_ptr)); */

		NFSLOG(0x10000,
		"nfs_getapage: page_downgrade_lock pp=%x, procp=%x\n",
				*io_pl_ptr, u.u_procp);

		page_downgrade_lock(*io_pl_ptr);
		io_pl_ptr++;
		if (j >= btopr(io_len))
			j = 0;
	} while ((sz -= PAGESIZE) > 0);

	/*
	 * terminate the list
	 */
	*io_pl_ptr = NULL;

	/*
	 * unlock pages we're not returning to our caller because
	 * there was not enough space in pl.
	 */
	while (j != i) {

		ASSERT(PAGE_IS_WRLOCKED(io_pl[j]));
		/* ASSERT(!PAGE_USELOCK_OWNED(io_pl[j])); */

		NFSLOG(0x10000,
		"nfs_getapage: page_unlock pp=%x, procp=%x\n",
				io_pl[j], u.u_procp);

		page_unlock(io_pl[j++]);
		if (j >= btopr(io_len))
			j = 0;
	}

	return (0);
}

/*
 * nfs_getpage(vnode_t *vp, u_int off, u_int len, u_int *protp, page_t *pl[],
 *	u_int plsz, struct seg *seg, vaddr_t addr, enum seg_rw rw, cred_t *cred)
 *	Get pages of sn nfs file.
 *
 * Calling/Exit State:
 *	The rnode rwlock may be held shared (if called from read)
 *	or exclusive (if called from write) on entry.
 *
 * Description:
 *	Reads one or more pages from a contiguous range of file space starting
 *	at the specified <vnode, offset>. VOP_GETPAGE() is responsible for
 *	finding/creating the necessary pages with this <vnode, offset>
 *	identity, and returning these pages, reader or writer locked, to
 *	the caller. For the S_OVERWRITE case, some pages may be returned
 *	writer locked if they are not entirely initialized.
 *
 *	The pages are returned in a NULL-terminated array whose space is
 *	provided by the caller.
 *
 * Parametsrs:
 *
 *	vp			# vnode to get pages of
 *	off			# starting offset 
 *	len			# requested length
 *	protp			# 
 *	pl			# array to return pages in
 *	plsz			# size of pl
 *	seg			# segment pages belongs to
 *	addr			#
 *	rw			# type of read, segment flags
 *	cred			# caller's creds
 */
/*ARGSUSED*/
STATIC int
nfs_getpage(vnode_t *vp, u_int off, u_int len, u_int *protp, page_t *pl[],
	u_int plsz, struct seg *seg, vaddr_t addr, enum seg_rw rw, cred_t *cred)
{
	struct	rnode	*rp = vtor(vp);
	struct	vattr	va;
	ulong_t		nextoff;
	int		error = 0;
	pl_t		opl;

	NFSLOG(0x10000, "nfs_getpage: entered\n", 0, 0);

	if (vp->v_flag & VNOMAP) {
		NFSLOG(0x40000, "nfs_getpage: ENOSYS\n", 0, 0);

		return (ENOSYS);
	}

	if (protp != NULL)
		*protp = PROT_ALL;

	/*
	 * make sure we have creds in the rnode to do I/O.
	 */
	opl = LOCK(&rp->r_statelock, PLMIN);
	if (rp->r_cred == NULL) {
		/*
		 * this will only happen when the file has been memory mapped.
		 *
		 * XXX: we need to worry about system credentials here.
		 */
		crhold(cred);
		rp->r_cred = cred;
	}
	UNLOCK(&rp->r_statelock, opl);

	/*
	 * now validate that the caches are up to date.
	 */
	(void) nfsgetattr(vp, &va, cred);

retry:
	if (rw != S_OVERWRITE) {
		opl = LOCK(&rp->r_statelock, PLMIN);
		if (off + len > rp->r_size + PAGEOFFSET) {
			/*
			 * beyond eof
			 */
			UNLOCK(&rp->r_statelock, opl);

			NFSLOG(0x40000, "nfs_getpage: EFAULT eof\n", 0, 0);

			return (EFAULT);
		} else if (off + len > rp->r_size) {
			/*
			 * adjust len to agree with file size
			 */
			len = rp->r_size - off;
		}
		UNLOCK(&rp->r_statelock, opl);
	}

	if (btop(off) == btop(off + len - 1)) 
		error = nfs_getapage(vp, off, len,
				protp, pl, plsz, seg, addr, rw, cred);
	else
		error = pvn_getpages(nfs_getapage, vp, off, len, protp, pl,
				plsz, seg, addr, rw, cred);

	if (error) 
		goto error_out;

	/*
	 * start the read ahead if possible.
	 */
	nextoff = ptob(btopr(off + len));
	if (nfs_nra && rp->r_nextr == off &&
				nextoff < rp->r_size && rw != S_OVERWRITE) {
		/*
		 * for read-ahead, pass a NULL pl so that if it's
		 * not convenient, io will not be done. and also,
		 * if io is done, we don't wait for it.
		 */
		error = nfs_getapage(vp, nextoff, PAGESIZE, NULL, NULL, 0,
				seg, addr, rw, cred);

		/*
		 * ignore all read ahead errors except those
		 * that might invalidate the primary read.
		 */
		if (error != NFS_EOF && error != NFS_CACHEINVALERR)
			error = 0;
	}
	rp->r_nextr = nextoff;

error_out:

	switch (error) {

		case NFS_CACHEINVALERR:
			nfs_purge_caches(vp, rp->r_size);
			goto retry;
		case ESTALE:
			PURGE_STALE_FH(error, vp);
	}

	NFSLOG(0x40000, "nfs_getpage: returning error %d\n", error, 0);

	return (error);
}

/*
 * nfs_putpageio(rnode_t *rp, page_t *pp, off_t off, size_t len, int flags, 
 *		cred_t *cred)
 *	Setup and do output for nfs pages.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 * Description:
 *	Set up for page io and call the strategy routine to
 *	put the pages to backing store. Pages are linked by p_next.
 *	If flag is B_ASYNC, don't wait for io to complete.
 *	Flags are composed of {B_ASYNC, B_INVAL, B_DONTNEED}
 *
 * Parameters:
 *
 *	rp			# rnode to put pages of
 *	pp			# pages to put
 *	off			# starting offset
 *	len			# lenth of put
 *	flags			# buffer flags
 *	cred			# caller's creds
 *
 */
/* ARGSUSED */
int
nfs_putpageio(rnode_t *rp, page_t *pp, off_t off, size_t len, int flags, 
		cred_t *cred)
{
	struct	vnode	*vp = rtov(rp);
	struct	buf	*bp;
	int		error;

	NFSLOG(0x10000, "nfs_putpageio: called\n", 0, 0);

	bp = pageio_setup(pp, 0, len, B_WRITE | flags);

	/*
	 * we now use the b_priv field for passing the vnode
	 * pointer thru to do_bio().
	 */
	bp->b_priv.un_ptr = (void *)vp;
	bp->b_edev = 0;
	bp->b_blkno = btodb(off);

	/*
	 * map the buffer in kernel virtual space
	 */
	bp_mapin(bp);

	error = nfs_strategy(bp);

	NFSLOG(0x10000, "nfs_writelbn %s blkno %d ",
			vtomi(vp)->mi_hostname, btodb(off));
	NFSLOG(0x10000, "pp %x len %d ", pp, len);
	NFSLOG(0x10000, "flags %x error %d\n", flags, error);

	return (error);

}

/*
 * nfs_putpage(vnode_t *vp, off_t off, uint_t len, int flags, cred_t *cred)
 *	Write out pages of an nfs file.
 *
 * Calling/Exit State:
 *	The rnode rwlock may or may not be held locked on entry.
 *
 * Description:
 *	Write out pages of an nfs file.	Flags are composed of
 *	{B_ASYNC, B_INVAL, B_DONTNEED, B_FORCE}. If len == 0,
 *	do from off to EOF.
 *
 *	The normal cases should be len == 0 & off == 0 (entire vp list),
 *	len == MAXBSIZE (from segmap_release actions), and len == PAGESIZE
 *	(from pageout).
 *
 * Parameters:
 *
 *	vp			# vnode to do putpage for
 *	off			# starting offset
 *	len			# length of put
 *	flags			# buffer flags
 *	cred			# caller's creds
 *
 */
/*ARGSUSED*/
STATIC int
nfs_putpage(vnode_t *vp, off_t off, uint_t len, int flags, cred_t *cred)
{
	struct	rnode	*rp = vtor(vp);
	u_int		bsize;
	u_int		bmask;
	uint_t		kl_len;
	off_t		kl_off;
	int		error;
	
	NFSLOG(0x10000, "nfs_putpage: vp %x, off %d, ", vp, off);
	NFSLOG(0x10000, "len %d, flags %x, ", len, flags);
	NFSLOG(0x10000, "cred %x\n", cred, 0);

	if (vp->v_flag & VNOMAP) {

		NFSLOG(0x40000, "nfs_putpage: VNOMAP\n", 0, 0);

		return (ENOSYS);
	}

	if (len == 0 && (flags & B_INVAL) == 0 &&
		(vp->v_vfsp->vfs_flag & VFS_RDONLY)) {

		NFSLOG(0x40000, "nfs_putpage: len == 0 && read-only\n", 0, 0);

		return (0);
	}

	/*
	 * the following check is just for performance and therefore
	 * doesn't need to be foolproof. the subsequent code will
	 * gracefully do nothing in any case. also, we just take a
	 * snapshot of r_size, so no locks needed.
	 */
	if (vp->v_pages == NULL || off >= rp->r_size) {

		NFSLOG(0x40000, "nfs_putpage: no pages or past EOF\n", 0, 0);

		return (0);
	}

	bsize = MAX(vp->v_vfsp->vfs_bsize, PAGESIZE);
	bmask = ~(bsize - 1);

	if (len != 0) {
		/*
		 * kluster at bsize boundaries
		 */
		kl_off = off & bmask;
		kl_len = ((off + len + bsize - 1) & bmask) - kl_off;
	} else {
		kl_off = off;
		kl_len = 0;
	}

	NFSLOG(0x10000, "nfs_putpage size = %d, len = %d\n", rp->r_size, len);
	NFSLOG(0x10000,
		"nfs_putpage kl_off = %d, kl_len = %d\n", kl_off, kl_len);

	error = pvn_getdirty_range(nfs_doputpage, vp, off, len, kl_off,
				kl_len, rp->r_size, flags, cred);

	NFSLOG(0x40000, "nfs_putpage(): returning error %d\n", error, 0);

	return (error);
}

/*
 * nfs_doputpage(vnode_t *vp, page_t *dirty, int flags, cred_t *cr)
 *	 Workhorse for nfs_putpage().
 *
 * Calling/Exit State:
 *	 A list of dirty pages, prepared for I/O (in pageout state),
 *	 is passed in dirty. Other parameters are passed through from
 *	 nfs_putpage.
 *
 * Description:
 *	This does most of the work for nfs_putpage(). It is called
 *	from pvn_getdirty_range(), after being passed in as an arg
 *	to it in nfs_putpage().
 *
 * Parameters:
 *
 *	vp			# vnode to put pages of
 *	dirty			# dirty page list
 *	flags			# buffer flags
 *	cr			# caller's creds
 *
 */
int
nfs_doputpage(vnode_t *vp, page_t *dirty, int flags, cred_t *cr)
{
	struct	rnode	*rp = vtor(vp);
	struct	page	*pp;
	struct	page	*io_list;
	u_int		io_off, io_len;
	daddr_t		lbn;
	u_int		lbn_off;
	u_int		bsize;
	int		err = 0;

	NFSLOG(0x10000, "nfs_doputpage: called\n", 0, 0);

	bsize = MAX(vp->v_vfsp->vfs_bsize, PAGESIZE);

	/*
	 * destroy read ahead value (since we are really going to write)
	 */
	if (dirty != NULL)
		rp->r_nextr = 0;

	/*
	 * now pp will have the list of kept dirty pages marked for
	 * write back. It will also handle invalidation and freeing
	 * of pages that are not dirty. All the pages on the list
	 * returned need to still be dealt with here.
	 */

	/*
	 * Handle all the dirty pages not yet dealt with.
	 */
	while ((pp = dirty) != NULL) {
		/*
		 * Pull off a contiguous chunk that fits in one lbn
		 */
		io_off = pp->p_offset;
		lbn = io_off / bsize;

		page_sub(&dirty, pp);
		io_list = pp;
		io_len = PAGESIZE;
		lbn_off = lbn * bsize;

		/* ASSERT(!PAGE_USELOCK_OWNED(pp)); */

		while (dirty != NULL && dirty->p_offset < lbn_off + bsize &&
			dirty->p_offset == io_off + io_len) {
			pp = dirty;
			page_sub(&dirty, pp);
			page_sortadd(&io_list, pp);
			io_len += PAGESIZE;
		}

		/*
		 * Check for page length rounding problems
		 */
		if (io_off + io_len > lbn_off + bsize) {
			ASSERT((io_off+io_len) - (lbn_off+bsize) < PAGESIZE);
			io_len = lbn_off + bsize - io_off;
		}

		NFSLOG(0x10000, "nfs_doputpage size = %d, off = %d\n",
					rp->r_size, io_off);
		NFSLOG(0x10000, "nfs_doputpage len = %d, flags = %d\n",
					io_len, flags);

		err = nfs_putpageio(rp, io_list, io_off, io_len, flags, cr);
		if (err) {
			NFSLOG(0x40000, "nfs_doputpage: err %d\n", err, 0);

			break;
		}
	}

	if (err != 0) 
		if (dirty != NULL)
			pvn_fail(dirty, B_WRITE | flags);

	NFSLOG(0x40000, "nfs_putpage: returning %d\n", err, 0);

	return (err);
}
