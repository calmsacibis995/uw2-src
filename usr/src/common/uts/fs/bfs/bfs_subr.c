/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/bfs/bfs_subr.c	1.6"
#ident	"$Header: $"

#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/bfs/bfs.h>
#include <fs/fs_hier.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/clock.h>
#include <util/bitmasks.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

STATIC int bfs_dotsearch(inode_t *, char *, cred_t *, ushort *, off_t *);

/*
 * int
 * bfs_iget(vfs_t *vfsp, bfs_ino_t ino, inode_t **ipp, boolean_t excl,
 *  	    cred_t *cr)
 *	Lookup an inode by inode number and return it locked.
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the following locks:
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of the root directory in shared mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
int
bfs_iget(vfs_t *vfsp, bfs_ino_t ino, inode_t **ipp, boolean_t excl, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)vfsp->vfs_data;
	inode_t		*ip;
	inode_t		*pip;
	inode_t		*tip;
	int		error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	ASSERT(ino >= BFSROOTINO);

	(void)LOCK(&bp->bfs_inolist_mutex, FS_BFSLISTPL);

	bfs_searchlist(bp, ino, &ip, &pip);

	/*
	 * If the inode is not found on the list:
	 * drop the list lock and allocate space with KM_SLEEP set.
	 * Initialize the inode and vnode and read the inode from disk.
	 * At this point, must reacquire the inode list lock and search the
	 * list again, since someone else may have allocated the inode in
	 * the meanwhile.
	 */
	if (ip == NULL) {

		UNLOCK(&bp->bfs_inolist_mutex, PLBASE);

		tip = kmem_zalloc(sizeof(inode_t), KM_SLEEP);

		BFS_INIT_INODE(tip, ino, vfsp);

		error = BFS_GETINODE(bp->bfs_devnode, tip->i_inodoff,
					&tip->i_diskino, cr);
		if (error) {
			BFS_DEINIT_INODE(tip);
			kmem_free(tip, sizeof(inode_t));
			return error;
		}
		ASSERT(ino == tip->i_diskino.d_ino);

		(void)LOCK(&bp->bfs_inolist_mutex, FS_BFSLISTPL);

		bfs_searchlist(bp, ino, &ip, &pip);
		if (ip != NULL) {
			/*
			 * We lost the race, and someone else created the inode
			 * first. Throw away the inode we allocated and use the
			 * one on the inode list.
			 */
			BFS_DEINIT_INODE(tip);
			kmem_free(tip, sizeof(inode_t));
		} else {

			/*
			 * Inode not found in the list; add it to end of
			 * the inode list
			 */
			ip = tip;
			if (pip != NULL){
				ASSERT(pip->i_next == NULL);
				pip->i_next = ip;
			}
			else
				bp->bfs_inolist = ip;
		}
	}

	/*
	 * We now have a pointer to the inodeon the inode list,
	 * unlock the inode list and lock the inode atomically.
	 * Increment the vnode count, and return the inode locked in ipp.
	 */
	if (excl)
		BFS_IWRLOCK_RELLOCK(ip, bp);
	else
		BFS_IRDLOCK_RELLOCK(ip, bp);

	VN_HOLD(ITOV(ip));

	*ipp = ip;
	return 0;
}


/*
 * void
 * bfs_tryiget(struct bfs_vfs *bp, bfs_ino_t ino, inode_t **ipp, boolean_t excl)
 *	Lookup an ino by ino # and return it locked if found in the inode list.
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the following locks:
 *	   - bfs_writelock 
 *	   - bfs_fs_rwlock in shared or exclusive mode
 *
 *	Callers of this routine may be holding the inode rwlock of the root
 *	directory in exclusive mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
void
bfs_tryiget(struct bfs_vfs *bp, bfs_ino_t ino, inode_t **ipp, boolean_t excl)
{
	inode_t		*ip;
	inode_t		*pip;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	ASSERT(ino >= BFSROOTINO);

	(void)LOCK(&bp->bfs_inolist_mutex, FS_BFSLISTPL);

	bfs_searchlist(bp, ino, &ip, &pip);

	/*
	 * If the inode is found on the inode list:
	 * unlock the inode list and lock the inode atomicaly.
	 * Increment the vnode count,
	 * and return the inode locked in ipp.
	 * If inode is not found in the list just unlock the inolist.
	 */
	if (ip != NULL) {
		if (excl)
			BFS_IWRLOCK_RELLOCK(ip, bp);
		else
			BFS_IRDLOCK_RELLOCK(ip, bp);
		VN_HOLD(ITOV(ip));
	} else
		UNLOCK(&bp->bfs_inolist_mutex, PLBASE);

	*ipp = ip;
	return;
}


/*
 * void
 * bfs_searchlist(struct bfs_vfs *bp, bfs_ino_t ino, inode_t **ipp,
 *		  inode_t **pipp)
 *	Search the inode list for the given inode number.
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the following locks:
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of the root directory in shared mode.
 *
 *	All locks held on entry will be held on exit.
 *
 */
void
bfs_searchlist(struct bfs_vfs *bp, bfs_ino_t ino, inode_t **ipp, inode_t **pipp)
{
	inode_t	*ip;
	inode_t *pip;

	/*
	 * After the search for the inode is finished:
	 * ip will point to the searched ino or to NULL if the ino is not found
	 * and  pip will point to the previous ino in the list or to NULL
	 * if there are no inodes in the list.
	 */
	for (pip = ip = bp->bfs_inolist; ip != NULL; pip = ip, ip = ip->i_next){
		if (ip->i_diskino.d_ino == ino)
			break;
	}
	*ipp = ip;
	*pipp = pip;
	return;
}


/*
 * off_t
 * bfs_searchdir(inode_t *ip, char *nm, cred_t *cr)
 *	Search the root directory for a file called nm.
 *
 * Calling/Exit State:
 *	Callers of this routine may be holding the bfs_writelock 
 *
 *	Callers of this routine must hold the following locks:
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of the root directory in shared/exclusive mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	Search the root for a file called nm.  Return the offset of the file
 *	inode so that the caller can OPTIONALLY create a vnode.
 *	If not found return zero.
 */
off_t
bfs_searchdir(inode_t *ip, char *nm, cred_t *cr)
{
	bfs_ino_t ino;
	off_t ino_offset = 0;
	int error;

	ASSERT(KS_HOLD0LOCKS());

	error = bfs_dotsearch(ip, nm, cr, &ino, (off_t *)0);
	if (error)
		return 0;
	if (ino != 0)
		ino_offset = BFS_INO2OFF(ino);
	return ino_offset;
}


/*
 * int
 * bfs_rmdirent(inode_t *ip, char *nm, cred_t *cr)
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the following locks:
 *	   - bfs_writelock 
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of the root directory in exclusive mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	Search the root directory for a file called nm and remove the entry
 *	from the directory by zeroing out the inode field.
 */
int
bfs_rmdirent(inode_t *ip, char *nm, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)ITOV(ip)->v_vfsp->vfs_data;
	dinode_t	*dip;
	bfs_ino_t	ino;
	off_t		offset = 0;
	int		error;

	ASSERT(KS_HOLD0LOCKS());

	ASSERT(ITOV(ip) == bp->bfs_rootvnode);
	error = bfs_dotsearch(ip, nm, cr, &ino, &offset);
	if (error)
		return error;

	/*
	 * If the entry is found, zero the inode field in the ROOT directory
	 */
	if (ino != 0 && offset > 0) {
		ino = 0;
		error = vn_rdwr(UIO_WRITE, bp->bfs_devnode, &ino,
				sizeof(ino), offset, UIO_SYSSPACE, IO_SYNC,
				RLIM_INFINITY, cr, (int *)0);
	} else
		error = ENOENT;

	if (!error) {
		dip = &ip->i_diskino;
		FSPIN_LOCK(&ip->i_mutex);
		dip->d_fattr.va_atime = hrestime.tv_sec;
		dip->d_fattr.va_mtime = hrestime.tv_sec;
		FSPIN_UNLOCK(&ip->i_mutex);
		/*
		 * Update the ROOT inode
		 */
		error = BFS_PUTINODE(bp->bfs_devnode, ip->i_inodoff, dip, cr);
	}
	return error;
}


/*
 * int
 * bfs_addirent(inode_t *ip, char *nm, bfs_ino_t inode, cred_t *cr)
 *	Add a directory entry.
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the following locks:
 *	   - bfs_writelock 
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of the root directory in exclusive mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	Search the root directory for a file called nm. If file nm is not found,
 *	add the entry to the directory.
 */
int
bfs_addirent(inode_t *ip, char *nm, bfs_ino_t inode, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)ITOV(ip)->v_vfsp->vfs_data;
	dinode_t	*dip;
	struct bfs_ldirs ld;
	bfs_ino_t	ino;
	off_t		offset = 0;
	int		error;

	ASSERT(KS_HOLD0LOCKS());

	error = bfs_dotsearch(ip, nm, cr, &ino, &offset);
	if (error)
		return error;
	if (ino != 0)
		return EEXIST;

	/*
	 * If no match AND no empty slot found, find out if there is
	 * space to increase the ROOT directory. If so, update the ROOT
	 * inode to reflect the new size and set "offset" for the next
	 * entry on the ROOT directory.
	 */
	dip = &ip->i_diskino;
	if (offset == 0) {
		if ((dip->d_eoffset + sizeof(struct bfs_ldirs)) < 
		     (dip->d_eblock + 1) * BFS_BSIZE) {
			offset = dip->d_eoffset + 1;
			dip->d_eoffset += sizeof(struct bfs_ldirs);
		} else 
			/*
			 * no more available slots in directory.
			 */
			return ENFILE;
	}

	if (offset > 0) {
		strncpy(ld.l_name, nm, BFS_MAXFNLEN);
		ld.l_ino = inode;

		/*
		 * Add the entry to the directory
		 */
		error = vn_rdwr(UIO_WRITE, bp->bfs_devnode, &ld,
				sizeof(struct bfs_ldirs), offset, UIO_SYSSPACE,
				IO_SYNC, RLIM_INFINITY, cr, (int *)0);
	}
	if (!error) {
		FSPIN_LOCK(&ip->i_mutex);
		dip->d_fattr.va_atime = hrestime.tv_sec;
		dip->d_fattr.va_mtime = hrestime.tv_sec;
		FSPIN_UNLOCK(&ip->i_mutex);
		/*
		 * Update the ROOT inode
		 */
		error = BFS_PUTINODE(bp->bfs_devnode, ip->i_inodoff,
					dip, cr);
	}

	return error;
}


/*
 * int
 * bfs_rendirent(inode_t *ip, char *snm, char *tnm, cred_t *cr)
 *	Remove a directory entry.
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the following locks:
 *	   - bfs_writelock 
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of the root directory in exclusive mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	Search the root directory for a file called snm, rename to tnm
 *	and update the directory entry.
 */
int
bfs_rendirent(inode_t *ip, char *snm, char *tnm, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)ITOV(ip)->v_vfsp->vfs_data;
	struct bfs_ldirs ld;
	dinode_t	*dip;
	bfs_ino_t	ino;
	off_t		offset = 0;
	int		error;

	ASSERT(KS_HOLD0LOCKS());

	error = bfs_dotsearch(ip, snm, cr, &ino, &offset);
	if (error)
		return error;

	/*
	 * If the entry is found, change the name field to tnm.
	 */
	if (ino != 0 && offset > 0) {
		ld.l_ino = ino;
		strncpy(ld.l_name, tnm, BFS_MAXFNLEN);
		error = vn_rdwr(UIO_WRITE, bp->bfs_devnode, &ld,
				sizeof(struct bfs_ldirs), offset, UIO_SYSSPACE,
				IO_SYNC, RLIM_INFINITY, cr, (int *)0);
	} else
		error = ENOENT;

	dip = &ip->i_diskino;
	if (!error) {
		FSPIN_LOCK(&ip->i_mutex);
		dip->d_fattr.va_atime = hrestime.tv_sec;
		dip->d_fattr.va_mtime = hrestime.tv_sec;
		FSPIN_UNLOCK(&ip->i_mutex);
		/*
		 * Update the ROOT inode
		 */
		error = BFS_PUTINODE(bp->bfs_devnode, ip->i_inodoff,
					dip, cr);
	}
	return error;
}


/*
 * int
 * bfs_dotsearch(inode_t *ip, char *nm, cred_t *cr, bfs_ino_t *inop,
 *		 off_t *offp)
 *	Search the root directory for a file called nm.
 *
 * Calling/Exit State:
 *	Callers of this routine may be holding the bfs_writelock 
 *
 *	Callers of this routine must hold the following locks:
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of the root directory in shared/exclusive mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	Search a directory for a file called nm. 
 *	If found, set offset to the entry in dir and the ino to the inode #.
 *	If not found, the offset is set to the first empty slot in the
 *	directory and ino is set to zero.
 *	If not found, and no empty slots in dir, offset and ino are set to 0.
 */
STATIC int
bfs_dotsearch(inode_t *ip, char *nm, cred_t *cr, bfs_ino_t *inop, off_t *offp)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)ip->i_vnode.v_vfsp->vfs_data;
	dinode_t		*dip;
	struct bfs_ldirs	*ld;
	void			*buf;
	off_t			offset;
	off_t			eslot = -1;
	off_t			i;
	int			error;
	boolean_t		found = B_FALSE;
	int			buflen;
	int			len, chunksize;

	ASSERT(KS_HOLD0LOCKS());

	dip = &ip->i_diskino;
	ASSERT(dip->d_ino == BFSROOTINO);
	ASSERT(BFS_FILESIZE(dip) != 0);

	len = BFS_NZFILESIZE(dip);
	chunksize = MIN(len, DIRBUFSIZE);
	buf = kmem_alloc(chunksize, KM_SLEEP);

	*inop = 0;
	for (offset = (dip->d_sblock * BFS_BSIZE); len > 0 && !found;
	     len -= buflen, offset += buflen) {	

		buflen = MIN(chunksize, len);
		/*
		 * Get the list of entries stored in the ROOT directory
		 */
		error = BFS_GETDIRLIST(bp->bfs_devnode, offset, buf, buflen,cr);
		if (error) {
			kmem_free(buf, chunksize);
			return error;
		}

		ld = (struct bfs_ldirs *)buf;
		for (i = 0; i < buflen; i += sizeof(struct bfs_ldirs), ld++) {
			if (ld->l_ino != 0 && 
			    strncmp(ld->l_name, nm, BFS_MAXFNLEN) == 0) {
				found = B_TRUE;
				*inop = ld->l_ino;
				if (offp)
					*offp = offset + i;
				break;
			}
			if (ld->l_ino == 0 && eslot == -1) {
				eslot = offset + i;
				if (offp)
					*offp = offset + i;
			}
		}
	}

	/*
	 * Update the access time of the ROOT inode
	 */
	if (WRITEALLOWED(bp->bfs_rootvnode, cr)) {
		FSPIN_LOCK(&ip->i_mutex);
		dip->d_fattr.va_atime = hrestime.tv_sec;
		FSPIN_UNLOCK(&ip->i_mutex);
		BFS_PUTINODE(bp->bfs_devnode, ip->i_inodoff, dip, cr);
	}

	kmem_free(buf, chunksize);
	return error;
}


/*
 * int
 * bfs_resetglbvars(struct bfs_vfs *bp, cred_t *cr)
 *	Reset the global vars bfs_lastfile, bfs_sblklastfile & bfs_eblklastfile.
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the following locks:
 *	   - bfs_writelock 
 *	   - bfs_fs_rwlock in shared mode
 *
 *	Callers of this routine may be holding the inode rwlock of the root
 *	directory in exclusive mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
int
bfs_resetglbvars(struct bfs_vfs *bp, cred_t *cr)
{
	inode_t		inode, *ip;
	dinode_t	*dip;
	off_t		lastfile;
	daddr_t		startblock;
	daddr_t		endblock;
	bfs_ino_t	ino;
	int		error;

	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Search through the filesystem to figure out the lastfile in the FS
	 * and its start & end block.
	 */
	endblock = 0;
 
	for (ino = BFSROOTINO; ino < bp->bfs_totalinodes + BFSROOTINO; ino++) {
		if (BITMASKN_TEST1(bp->bfs_inobitmap, ino)) {

			if (ino != BFSROOTINO) {
				/*
				 * Must set ip to point to the local inode
				 * buffer and get the inode from disk.
				 */
				ip = &inode;
				dip = &ip->i_diskino;
				error = BFS_GETINODE(bp->bfs_devnode,
						BFS_INO2OFF(ino), dip, cr);
				if (error)
					return error;
			} else {
				/*
				 * The root inode is already locked in-core
				 * and is available to us.
				 */
				ip = VTOI(bp->bfs_rootvnode);
				dip = &ip->i_diskino;
			}


			if (dip->d_eblock > endblock) {
				startblock = dip->d_sblock;
				endblock = dip->d_eblock;
				lastfile = BFS_INO2OFF(ino);
			}
		}
	}
	/*
	 * Note that since there is always a non-zero length root directory,
	 * there is always a lastfile.
	 */
	ASSERT(lastfile >= BFS_INO2OFF(BFSROOTINO));
	ASSERT(startblock >= (bp->bfs_startfs / BFS_BSIZE));
	ASSERT(startblock <= (bp->bfs_endfs / BFS_BSIZE));
	ASSERT(endblock > (bp->bfs_startfs / BFS_BSIZE));
	ASSERT(endblock <= (bp->bfs_endfs / BFS_BSIZE));
	ASSERT(endblock >= startblock);
	bp->bfs_lastfilefs = lastfile;
	bp->bfs_sblklastfile = startblock;
	bp->bfs_eblklastfile = endblock;
	return 0;
}


#define	TST_GROUP	3
#define	TST_OTHER	6

/*
 * int
 * bfs_iaccess(inode_t *ip, mode_t mode, cred_t *cr)
 *	Check for mode permission.
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the inode rwlock in shared mode.
 *
 *	The inode rwlock will be held on exit.
 *
 */
int
bfs_iaccess(inode_t *ip, mode_t mode, cred_t *cr)
{
	struct bfsvattr	*attrp;
	mode_t		denied_mode, perm;
	int		lshift;

	attrp = &ip->i_diskino.d_fattr;

	if ((mode & VWRITE) && (ITOV(ip)->v_vfsp->vfs_flag & VFS_RDONLY))
		return EROFS;
	if (cr->cr_uid == attrp->va_uid) {
		lshift = 0;			/* TST_OWNER */
	}
	else if (groupmember(attrp->va_gid, cr)) {
		mode >>= TST_GROUP;
		lshift = TST_GROUP;
	} else {
		mode >>= TST_OTHER;
		lshift = TST_OTHER;
	}
	if ((perm = (attrp->va_mode & mode)) == mode)
		return 0;

	denied_mode = (mode & (~perm));
	denied_mode <<= lshift;

	if ((denied_mode & (VREAD | VEXEC)) && pm_denied(cr, P_DACREAD))
		return EACCES;
	if ((denied_mode & VWRITE) && pm_denied(cr, P_DACWRITE))
		return EACCES;

	return 0;
}
