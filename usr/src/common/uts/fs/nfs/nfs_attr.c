/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_attr.c	1.14"
#ident	"$Header: $"

/*
 *	nfs_attr.c, routines for nfs attribute caching.
 *
 *	Attributes are cached in the rnode in struct vattr form. There is a
 *	time associated with the cached attributes (r_attrtime) which tells
 *	whether the attributes are valid. The time is initialized to the
 *	difference between current time and the modify time of the vnode when
 *	new attributes are cached. This allows the attributes for files that
 *	have changed recently to be timed out sooner than for files that have
 *	not changed for a long time. There are minimum and maximum timeout
 *	values that can be set per mount point.
 */

#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/clock.h>
#include <fs/vnode.h>
#include <fs/vfs.h>
#include <acc/dac/acl.h>
#include <svc/errno.h>
#include <fs/buf.h>
#include <fs/stat.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <mem/kmem.h>
#include <mem/pvn.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/tiuser.h>
#include <util/sysmacros.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/nfs_clnt.h>
#include <fs/nfs/nfslk.h>
#include <fs/nfs/rnode.h>

extern	void	dnlc_purge_vp(vnode_t *);
extern	int	vfs_fixedmajor(struct vfs *);
extern	int	nfs_doputpage(vnode_t *, page_t *, int, cred_t *);

/*
 * nfs_cache_attr(vp, va)
 *	Set attributes in the cache for given vnode.
 *
 * Calling/Exit State:
 *	Assumes r_statelock not held on entry.
 *
 *	Returns a void.
 *	
 * Description:
 *	Set attributes in the cache for given vnode.
 *
 * Parameters:
 *
 *	vp			# vnode to set attr for
 *	va			# file attr
 *
 */
void
nfs_cache_attr(struct vnode *vp, struct vattr *va)
{
	struct	rnode	*rp = vtor(vp);
	pl_t		opl;

	if (rp->r_flags & RNOCACHE)
		return;

	NFSLOG(0x8, "nfs_cache_attr: vnode %d\n", vp, 0);

	/*
	 * this is special kludge for the automounter, ao that
	 * it can change the file type. no lock is needed as this
	 * is an enum. one or the other value will be seen.
	 */
	vp->v_type = va->va_type;

	/*
	 * now get the statelock, and set the attributes
	 */
	opl = LOCK(&rp->r_statelock, PLMIN);
	rp->r_attr = *va;

	/*
	 * initialize the cache time
	 */
	set_attrcache_time(vp);

	UNLOCK(&rp->r_statelock, opl);
}

/*
 * nfsgetattr(vp, vap, cred)
 *	Return either cached or remote attributes.
 *
 * Calling/Exit State:
 *	Assumes r_statelock not held on entry. The caller must
 *	guarantee that the creds passed in are held appropriately.
 *
 *	Returns 0 for success, error otherwise.
 *	
 * Description:
 *	Return either cached or remote attributes.
 *
 * Parameters:
 *
 *	vp			# vnode to get attr for
 *	vap			# return attr in this
 *	cred			# caller's cred
 *
 */
int
nfsgetattr(struct vnode *vp, struct vattr *vap, cred_t *cred)
{
	struct	rnode	*rp = vtor(vp);
	pl_t		opl;
	int		error;

	NFSLOG(0x8, "nfsgetattr: rnode %d\n", vtor(vp), 0);

	/*
	 * get the statelock to check if the cached attributes
	 * have timed out. lock for hrestime not needed as we
	 * only look at the seconds.
	 */
	opl = LOCK(&rp->r_statelock, PLMIN);
	if (hrestime.tv_sec < rp->r_attrtime.tv_sec) {
		/*
		 * cached attributes are valid.
		 */

		*vap = rp->r_attr;

		/*
		 * return the rnode r_size, as we only use va_size for
		 * checking if server modified the file.
		 */
		vap->va_size = rp->r_size;

		UNLOCK(&rp->r_statelock, opl);

		NFSLOG(0x8, "nfsgetattr: rnode %d attr valid\n", rp, 0);

		return (0);
	}

	/*
	 * cached attr are invalid. first release the statelock
	 */
	UNLOCK(&rp->r_statelock, opl);

	/*
	 * now get fresh attributes from the server.
	 */
	if (vtomi(vp)->mi_protocol == NFS_V2)
		error = nfs_getattr_otw(vp, vap, cred);

#ifdef NFSESV
	else
		error = nfs_getesvattr_otw(vp, vap, cred);
#endif
	/*
	 * return the rnode r_size, as we only use va_size for
	 * checking if server modified the file.
	 */
	vap->va_size = rp->r_size;

	ASSERT(vap->va_size >= 0);

	return (error);
}

/*
 * nfs_getattr_otw(vp, vap, cred)
 *	Get vnode attr from the server (over the wire).
 *
 * Calling/Exit State:
 *	Assumes r_statelock not held on entry.
 *
 *	Returns 0 for success, error otherwise.
 *	
 * Description:
 *	Get vnode attr from the server (over the wire).
 *
 * Parameters:
 *
 *	vp			# vnode to get attr for
 *	vap			# return attr in this
 *	cred			# caller's cred
 *
 */
int
nfs_getattr_otw(struct vnode *vp, struct vattr *vap, cred_t *cred)
{
	struct	nfsattrstat	*ns;
	int			error;

	NFSLOG(0x8, "nfs_getattr_otw: rnode %d\n", vtor(vp), 0);

	ns = (struct nfsattrstat *)kmem_zalloc(sizeof (*ns), KM_SLEEP);

	/*
	 * make a call to the server
	 */
	error = rfscall(vtomi(vp), RFS_GETATTR, 0, xdr_fhandle,
		(caddr_t)vtofh(vp), xdr_attrstat, (caddr_t)ns, cred);

	if (error == 0) {
		error = geterrno(ns->ns_status);
		if (error == 0) {
			struct	rnode	*rp = vtor(vp);
			pl_t		opl;
			u_long		ofsize;

			/*
			 * no error in rpc and no error on server.
			 * first convert the attributes to vnode format.
			 * note that nattr_to_vattr() does not set the
			 * file size (r_size) anymore.
			 */
			nattr_to_vattr(vp, &ns->ns_attr, vap);

			/*
			 * now set the file size (r_size), but first
			 * save the original size. it is needed to write
			 * out all the dirty pages if the server modifed
			 * the file.
			 */
			opl = LOCK(&rp->r_statelock, PLMIN);
			ofsize = rp->r_size;
			if ((rp->r_size < vap->va_size) ||
					((rp->r_flags & RDIRTY) == 0)) {
				rp->r_size = vap->va_size;
			}
			UNLOCK(&rp->r_statelock, opl);

			/*
			 * now check if the server modified the file
			 * and purge our caches if it did.
			 */
			nfs_cache_check(vp, vap->va_mtime, vap->va_size,
							ofsize);

			/*
			 * now cache the new attributes
			 */
			nfs_cache_attr(vp, vap);
		} else {
			/*
			 * error on the server, get rid of the file handle
			 * so that next time lookup is done again.
			 */
			PURGE_STALE_FH(error, vp);
		}
	}

	kmem_free((caddr_t)ns, sizeof (*ns));

	return (error);
}

/*
 * nfs_cache_check(vp, mtime, nfsize, ofsize)
 *	Check if the file caches are valid.
 *
 * Calling/Exit State:
 *	Assumes r_statelock not held on entry.
 *
 *	Returns a void.
 *	
 * Description:
 *	Check if the file caches are valid i.e if the server did
 *	not modify the file in our caching interval.
 *
 * Parameters:
 *
 *	vp			# vnode to flush
 *	mtime			# mod time of vnode from server
 *	nfsize			# file size of vnode from server
 *	ofsize			# original file size (r_size)
 *
 */
void
nfs_cache_check(struct vnode *vp, timestruc_t mtime,
		u_long nfsize, u_long ofsize)
{
	struct	rnode	*rp;
	pl_t		opl;

	rp = vtor(vp);

	NFSLOG(0x8, "nfs_cache_check: rnode %d\n", rp, 0);

	/*
	 * get the statelock and check cache validity.
	 */
	opl = LOCK(&rp->r_statelock, PLMIN);
	if (!CACHE_VALID(rp, mtime, nfsize)) {
		/*
		 * invalid, purge the caches. need the old
		 * file size here.
		 */
		UNLOCK(&rp->r_statelock, opl);
		nfs_purge_caches(vp, ofsize);
	} else {
		UNLOCK(&rp->r_statelock, opl);
	}
}

/*
 * nfs_purge_caches(vp, ofsize)
 *	Purge caches for a vnode.
 *
 * Calling/Exit State:
 *	Returns a void.
 *	
 * Description:
 *	This routine purges the caches for a vnode. It removes
 *	it from the dnlc, writes all the dirty pages to the server
 *	and releases the vnode.
 *
 * Parameters:
 *
 *	vp			# vnode to purge
 *	ofsize			# original file size (r_size)
 *				# needed to write out all dirty pages
 */
void
nfs_purge_caches(struct vnode *vp, u_long ofsize)
{
	NFSLOG(0x8, "nfs_purge_caches: vnode %x\n", vp, 0);

	/*
	 * invalidate the attributes cache.
	 */
	INVAL_ATTRCACHE(vp);

	/*
	 * remove from the lookup cache
	 */
	dnlc_purge_vp(vp);

	/*
	 * flush any pages
	 */
	if (vp->v_pages != NULL) {
		struct	rnode	*rp = vtor(vp); 
		pl_t		opl;

		NFSLOG(0x8, "nfs_purge_caches: pages present in %d\n", rp, 0);

		/*
		 * need to hold the vnode here for putpage.
		 */
		VN_HOLD(vp);

		/*
		 * make sure we have credentials to write out the pages.
		 * also clear eof flag.
		 */
		opl = LOCK(&rp->r_statelock, PLMIN);
		if (rp->r_cred == NULL)
			rp->r_cred = u.u_lwpp->l_cred;
		rp->r_flags &= ~REOF;
		UNLOCK(&rp->r_statelock, opl);

		/*
		 * write out the dirty pages and abort them (B_INVAL).
		 * we used to call VOP_PUTPAGE() here, but we need the
		 * file size for the getdirty algorithm. at this point,
		 * the file size in already modified so we cannot use
		 * it in VOP_PUTPAGE(), and we also cannot pass the ofsize
		 * argument to it either. so we do its work here.
		 */
		(void)pvn_getdirty_range(nfs_doputpage, vp, 0, 0, 0,
				0, ofsize, B_INVAL, rp->r_cred);

		VN_RELE(vp);
	}
}

/*
 * set_attrcache_time(vp)
 *	Set attr cache time in vnode attr.
 *
 * Calling/Exit State:
 *	Assumes r_statelock is held on entry. It is still held on exit.
 *
 *	Returns a void.
 *	
 * Description:
 *	Initializes cache time for attr in vnode.
 *
 * Parameters:
 *
 *	vp			# vnode to init time for
 *
 */
void
set_attrcache_time(struct vnode *vp)
{
	struct	rnode	*rp = vtor(vp);
	int		delta;

	NFSLOG(0x8, "set_attrcache_time: rnode %d\n", rp, 0);

	/*
	 * initialize attr cache time in rnode to be current time.
	 * we do not need hrestime lock here as we just copy seconds.
	 */
	rp->r_attrtime.tv_sec = hrestime.tv_sec;
	rp->r_attrtime.tv_usec = 0;

	/*
	 * delta is the number of seconds that we will cache
	 * attributes of the file. It is based on the number of seconds
	 * since the last change (i.e. files that changed recently
	 * are likely to change soon), but there is a minimum and
	 * a maximum for regular files and for directories.
	 */
	delta = (hrestime.tv_sec - rp->r_attr.va_mtime.tv_sec) >> 4;
	if (vp->v_type == VDIR) {
		if (delta < vtomi(vp)->mi_acdirmin) {
			delta = vtomi(vp)->mi_acdirmin;
		} else if (delta > vtomi(vp)->mi_acdirmax) {
			delta = vtomi(vp)->mi_acdirmax;
		}
	} else {
		if (delta < vtomi(vp)->mi_acregmin) {
			delta = vtomi(vp)->mi_acregmin;
		} else if (delta > vtomi(vp)->mi_acregmax) {
			delta = vtomi(vp)->mi_acregmax;
		}
	}

	/*
	 * add delta to attr cache time
	 */
	rp->r_attrtime.tv_sec += delta;
}

/*
 * nattr_to_vattr(vp, na, vap)
 *	Convert network attr to vnode attr.
 *
 * Calling/Exit State:
 *	Returns a void.
 *	
 * Description:
 *	Convert network attr to vnode attr.
 *
 * Parameters:
 *
 *	vp			# vnode to convert for
 *	na			# network attr to convert
 *	vap			# return vonde attr in this
 *
 */
void
nattr_to_vattr(struct vnode *vp, struct nfsfattr *na, struct vattr *vap)
{
	NFSLOG(0x8, "nattr_to_vattr: vnode %d\n", vp, 0);

	/*
	 * type, mode, uid, gid are not modified here
	 */
	vap->va_type = (enum vtype)na->na_type;
	vap->va_mode = na->na_mode;
	vap->va_uid = na->na_uid;
	vap->va_gid = na->na_gid;

	/*
	 * fsid must be negative, so OR with 0x80000000
	 */
	vap->va_fsid = 0x80000000 |
		(long)makedevice(vfs_fixedmajor(vp->v_vfsp),
				vtomi(vp)->mi_mntno);

	/*
	 * nodeid, links, size
	 */
	vap->va_nodeid = na->na_nodeid;
	vap->va_nlink = na->na_nlink;
	vap->va_size = na->na_size;

	/*
	 * pick up various times
	 */
	vap->va_atime.tv_sec  = na->na_atime.tv_sec;
	vap->va_atime.tv_nsec = na->na_atime.tv_usec*1000;
	vap->va_mtime.tv_sec  = na->na_mtime.tv_sec;
	vap->va_mtime.tv_nsec = na->na_mtime.tv_usec*1000;
	vap->va_ctime.tv_sec  = na->na_ctime.tv_sec;
	vap->va_ctime.tv_nsec = na->na_ctime.tv_usec*1000;

	/*
	 * Shannon's law - uncompress the received dev_t
	 *
	 * if the top half is zero indicating a response
	 * from an `older style' OS. Except for when it is a
	 * `new style' OS sending the maj device of zero,
	 * in which case the algorithm still works because the
	 * fact that it is a new style server
	 * is hidden by the minor device not being greater
	 * than 255 (a requirement in this case).
	 */
	if ((na->na_rdev & 0xffff0000) == 0)
		vap->va_rdev = expdev(na->na_rdev);
	else
		vap->va_rdev = na->na_rdev;

	vap->va_nblocks = na->na_blocks;

	/*
	 * set block size depending on file type
	 */
	switch(na->na_type) {

	case NFBLK:
		vap->va_blksize = DEV_BSIZE;
		break;

	case NFCHR:
		vap->va_blksize = MAXBSIZE;
		break;

	default:
		vap->va_blksize = na->na_blocksize;
		break;
	}

	/*
	 * This bit of ugliness is a *TEMPORARY* hack to preserve the
	 * over-the-wire protocols for named-pipe vnodes. It remaps the
	 * special over-the-wire type to the VFIFO type. (see note in nfs.h)
	 *
	 * BUYER BEWARE:
	 *  If you are porting the NFS to a non-SUN server, you probably
	 *  don't want to include the following block of code. The
	 *  over-the-wire special file types will be changing with the
	 *  NFS Protocol Revision.
	 */
	if (NA_ISFIFO(na)) {
		vap->va_type = VFIFO;
		vap->va_mode = (vap->va_mode & ~S_IFMT) | S_IFIFO;
		vap->va_rdev = 0;
		vap->va_blksize = na->na_blocksize;
	}
}

#ifdef NFSESV

/*
 * nattresv_to_vattr(vp, na, vap)
 *	Convert network attr to vnode attr.
 *
 * Calling/Exit State:
 *	Returns a void. On entry, nacl is number of acl entries that will
 *	fit in aclp. On exit, it is overwritten with the number of actually
 *	written acls.
 *	
 * Description:
 *	Convert network attr to vnode attr. Used by
 *	extended (esv) protocol. 
 *
 * Parameters:
 *
 *	vp			# vnode to convert for
 *	na			# network attr to convert
 *	vap			# return vonde attr in this
 *	lidp			# lid
 *	aclp			# ptr to area to write acl entries
 *	nacl			# number of acl entries
 *
 */
void
nattresv_to_vattr(struct vnode *vp, struct nfsesvfattr *na, struct vattr *vap,
		  lid_t *lidp, struct acl *aclp, u_int *nacl)
{
	NFSLOG(0x8, "nattresv_to_vattr: vnode %d\n", vp, 0);

	vap->va_type = (enum vtype)na->na_type;
	vap->va_mode = na->na_mode;
	vap->va_uid = na->na_uid;
	vap->va_gid = na->na_gid;

	/*
	 * fsid must be negative, so OR with 0x80000000
	 */
	vap->va_fsid = 0x80000000 |
			(long)makedevice(vfs_fixedmajor(vp->v_vfsp),
					vtomi(vp)->mi_mntno);

	vap->va_nodeid = na->na_nodeid;
	vap->va_nlink = na->na_nlink;

	vap->va_size = na->na_size;

	vap->va_atime.tv_sec  = na->na_atime.tv_sec;
	vap->va_atime.tv_nsec = na->na_atime.tv_usec*1000;
	vap->va_mtime.tv_sec  = na->na_mtime.tv_sec;
	vap->va_mtime.tv_nsec = na->na_mtime.tv_usec*1000;
	vap->va_ctime.tv_sec  = na->na_ctime.tv_sec;
	vap->va_ctime.tv_nsec = na->na_ctime.tv_usec*1000;

	if ((na->na_rdev & 0xffff0000) == 0)
		vap->va_rdev = expdev(na->na_rdev);
	else
		vap->va_rdev = na->na_rdev;

	vap->va_nblocks = na->na_blocks;

	switch(na->na_type) {

	case NFBLK:
		vap->va_blksize = DEV_BSIZE;
		break;

	case NFCHR:
		vap->va_blksize = MAXBSIZE;
		break;

	default:
		vap->va_blksize = na->na_blocksize;
		break;
	}

	/*
	 * This bit of ugliness is a *TEMPORARY* hack to preserve the
	 * over-the-wire protocols for named-pipe vnodes.  It remaps the
	 * special over-the-wire type to the VFIFO type. (see note in nfs.h)
	 *
	 * BUYER BEWARE:
	 *  If you are porting the NFS to a non-SUN server, you probably
	 *  don't want to include the following block of code.  The
	 *  over-the-wire special file types will be changing with the
	 *  NFS Protocol Revision.
	 */
	if (NA_ISFIFO(na)) {
		vap->va_type = VFIFO;
		vap->va_mode = (vap->va_mode & ~S_IFMT) | S_IFIFO;
		vap->va_rdev = 0;
		vap->va_blksize = na->na_blocksize;
	}

	if (map_local_token(na->na_sens, SENS_T, (caddr_t)lidp,
				sizeof(lid_t)) != sizeof(lid_t)) {
		vp->v_lid = (lid_t)0;
	}

	*nacl = map_local_token(na->na_acl, ACL_T, (caddr_t)aclp,
					*nacl * sizeof (struct acl));
	*nacl /= sizeof(struct acl);
}

/*
 * nfs_esvattrcache(vp, na)
 *	Set attributes cache for given vnode using nfsesvfattr.
 *
 * Calling/Exit State:
 *	Returns a void.
 *	
 * Description:
 *	Set attributes cache for given vnode using nfsesvfattr.
 *	Used by esv protocol.
 *
 * Parameters:
 *
 *	vp			# vnode to set attr for
 *	na			# file attr from the server
 *
 */
void
nfs_esvattrcache(struct vnode *vp, struct nfsesvfattr *na)
{
	struct	rnode	*rp = vtor(vp);
	lid_t		tmplid;

	if (rp->r_flag & RNOCACHE)
		return;

	NFSLOG(0x8, "nfs_esvattrcache: vnode %d\n", vp, 0);

	rp->r_aclcnt = acl_getmax();

	/*
	 * convert network to vnode attr
	 */
	nattresv_to_vattr(vp, na, &rp->r_attr, &tmplid,
					rp->r_acl, &rp->r_aclcnt);

	if (tmplid != vp->v_lid) {

		NFSLOG(0x8, "nfs_esvattrcache: new LID %d != old LID %d\n",
				tmplid, vp->v_lid);

		vp->v_lid = tmplid;
	}

	/*
	 * initialize the cache time in the vnode, need to lock rnode
	 * statelock
	 */
	set_attrcache_time(vp);
}

/*
 * nfs_getesvattr_otw(vp, vap, cred)
 *	Get vnode attr from the server (over the wire).
 *
 * Calling/Exit State:
 *	Returns 0 for success, error otherwise.
 *	
 * Description:
 *	Get vnode attr from the server (over the wire).
 *	Extended attr for esv protocol.
 *
 * Parameters:
 *
 *	vp			# vnode to get attr for
 *	vap			# return attr in this
 *	cred			# caller's cred
 *
 */
int
nfs_getesvattr_otw(struct vnode *vp, struct vattr *vap, cred_t *cred)
{
	struct	nfsesvattrstat	*ns;
	lid_t			tmplid;
	int			error;

	NFSLOG(0x8, "nfs_getesvattr_otw: rnode %d\n", vtor(vp), 0);

	ns = (struct nfsesvattrstat *)kmem_alloc(sizeof (*ns), KM_SLEEP);

	/*
	 * make the call to the server
	 */
	error = rfscall(vtomi(vp), RFS_GETATTR, 0, xdr_fhandle,
		(caddr_t)vtofh(vp), xdr_esvattrstat, (caddr_t)ns, cred);

	if (error == 0) {
		error = geterrno(ns->ns_status);
		if (error == 0) {
			/*
			 * no error in rpc and no error on server
			 * pick up attr
			 */
			vtor(vp)->r_aclcnt = acl_getmax();
			nattresv_to_vattr(vp, &ns->ns_attr, vap, &tmplid,
					vtor(vp)->r_acl, &vtor(vp)->r_aclcnt);
			if (tmplid != vp->v_lid) {
				NFSLOG(1,
			"nfs_getesvattr_otw: new LID %d != old LID %d\n",
					tmplid, vp->v_lid);

				vp->v_lid = tmplid;
			}
			nfs_esvcache_check(vp, vap->va_mtime, vap->va_size);
			nfs_esvattrcache_va(vp, vap);
		} else {
			PURGE_STALE_FH(error, vp);
		}
	}
	kmem_free((caddr_t)ns, sizeof (*ns));

	return (error);
}

#endif
