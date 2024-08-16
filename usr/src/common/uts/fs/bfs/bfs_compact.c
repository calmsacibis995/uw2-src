/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/bfs/bfs_compact.c	1.3"
#ident	"$Header: $"

#include <fs/bfs/bfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/types.h>

#ifdef DEBUG
STATIC int bfs_debugcomp;
#endif /* DEBUG */

STATIC int bfs_getnxtcblock(struct bfs_vfs *, daddr_t, inode_t **, cred_t *);

/*
 * void
 * bfs_compact(struct bfs_vfs *bp, cred_t *cr)
 *	Compact file system by closing any gaps between files.
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the following locks:
 *	   - bfs_writelock 
 *	   - bfs_fs_rwlock in exclusive mode
 *
 *	No other BFS locks must be held by the callers.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
void
bfs_compact(struct bfs_vfs *bp, cred_t *cr)
{
	daddr_t		nxtblk;
	daddr_t		cureblk;
	daddr_t		gapsize = 0;
	boolean_t	any_shifted = B_FALSE;
	inode_t		inode, *ip;
	dinode_t	*dip;
	char		*bufferp;

	ASSERT(KS_HOLD0LOCKS());

	cmn_err(CE_CONT, "Compacting BFS filesystem\n");

	bufferp = kmem_alloc(BFSBUFSIZE, KM_SLEEP);
	nxtblk = (bp->bfs_startfs / BFS_BSIZE);
	cureblk = 0;

	/*
	 * Set ip to point to the local inode buffer
	 */
	ip = &inode;

	/*
	 * Each time bfs_getnxtcblock() is called, ip will be pointing to the
	 * local inode buffer. On return ip could be pointing to the inode
	 * in-core if it was found already in memory. If the inode was not
	 * found in memory a copy of the inode was read in from disk to the
	 * local inode buffer and ip will be pointing to it.
	 */
	while (bfs_getnxtcblock(bp, cureblk, &ip, cr) != 0) {

		dip = &ip->i_diskino;

		ASSERT(BFS_FILESIZE(dip) != 0);

		/*
		 * If this file is not the next block after the previous file,
		 * there is a gap.  The global gapsize must be increased and the
		 * file must be shifted.
		 */
		if (dip->d_sblock != nxtblk + 1 && dip->d_sblock != nxtblk) {

			gapsize = (dip->d_sblock - nxtblk) - 1;
#ifdef DEBUG
			if (bfs_debugcomp)
			    cmn_err(CE_CONT, "Found a gap. New gapsize is %d\n",
			      gapsize);
#endif
			bfs_shiftfile(bp, ip, gapsize, bufferp, cr);
			any_shifted = B_TRUE;
		}
		nxtblk = dip->d_eblock;
		cureblk = nxtblk;
		/*
		 * If this inode is in-core, must unlock the inode and
		 * release it.
		 * Also, ip must be reset to point to the local inode buffer.
		 */
		if (ip->i_vnode.v_count > 0) {
			BFS_IRWLOCK_UNLOCK(ip);
			VN_RELE(ITOV(ip));
			ip = &inode;
		}
	}

	kmem_free(bufferp, BFSBUFSIZE);

	if (any_shifted == B_TRUE)
		cmn_err(CE_CONT, "Compaction of BFS filesystem completed\n");
	else
		cmn_err(CE_CONT, "BFS filesystem was already compacted\n");

}


/*
 * int
 * bfs_getnxtcblock(struct bfs_vfs *bp, daddr_t cureblk, inode_t **ipp,
 *			cred_t *cr)
 *	get the block number and inode of the first file after "curblk."
 *
 * Calling/Exit State:
 *	The caller of this routine must hold the following locks:
 *	   - bfs_writelock 
 *	   - bfs_fs_rwlock in exclusive mode
 *
 *	No other BFS locks must be held by the caller.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
STATIC int
bfs_getnxtcblock(struct bfs_vfs *bp, daddr_t cureblk, inode_t **ipp, cred_t *cr)
{
	inode_t		s_inode, *s_ip;
	boolean_t	s_incore = 0;
	boolean_t	saved = 0;
	inode_t		inode, *ip;
	dinode_t	*dip;
	daddr_t		lastfsblk;
	boolean_t	incore;
	int		i;

	ASSERT(KS_HOLD0LOCKS());

	lastfsblk = (bp->bfs_endfs + 1) / BFS_BSIZE;
	ip = &inode;
	s_ip = &s_inode;

	/*
	 * Loop through all allocated inodes until the start block is the lowest
	 * number and greater than cureblk.
	 */
	for (i = BFSROOTINO; i < (bp->bfs_totalinodes + BFSROOTINO); i++) {
		if (BITMASKN_TEST1(bp->bfs_inobitmap, i)) {

			/*
			 * Try to get the inode from the inode list
			 */
			bfs_tryiget(bp, i, &ip, 1);
			if (ip != NULL) {
				/*
				 * ip is now set to the inode in-core copy, so,
				 * set the incore flag to 1 to reflect that.
				 */
				incore = 1;
				dip = &ip->i_diskino;
			} else {
				/*
				 * Must reset ip to point to the local inode
				 * buffer and get the inode from disk.
				 */
				incore = 0;
				ip = &inode;
				dip = &ip->i_diskino;
				BFS_GETINODE(bp->bfs_devnode,
						BFS_INO2OFF(i), dip, cr);
			}

			if (dip->d_sblock > cureblk &&
				dip->d_sblock < lastfsblk) {
#ifdef DEBUG
		    		if (bfs_debugcomp)
			cmn_err(CE_CONT,"nxt: fnd sblk %d, gt than %d, lt %d\n",
			  dip->d_sblock, cureblk, lastfsblk);
#endif
				lastfsblk = dip->d_sblock;

				/*
				 * If there is a previous saved inode copy or
				 * inode pointer, and that inode is in core,
				 * must unlock the inode and release it.
				 */
				if (saved && s_incore) {
					BFS_IRWLOCK_UNLOCK(s_ip);
					VN_RELE(ITOV(s_ip));
				}
				/*
				 * If the inode is not in core, save a copy of
				 * disk inode information.
				 * Else, if the inode is in core, just save the
				 * pointer to the inode.
				 */
				if (!incore) {
					ip->i_inodoff = BFS_INO2OFF(i);
					s_inode = *ip;
					s_ip = &s_inode;
				} else
					s_ip = ip;

				s_incore = incore;
				saved = 1;

			} else {
				if (incore) {
					BFS_IRWLOCK_UNLOCK(ip);
					VN_RELE(ITOV(ip));
				}
			}
		}
	}
	if (saved) {
		/*
		 * If the inode is not in core, copy the inode information
		 * into the the callers inode buffer.
		 * Else, if the inode is in core, just pass the inode pointer.
		 */
		if (!s_incore) {
			/*
			 * set v_count to 0 to signal the caller that the inode
			 * is not in core.
			 */
			s_ip->i_vnode.v_count = 0;
			**ipp = *s_ip;
		}else
			*ipp = s_ip;
	}
	return saved;
}


#define BFS_CCT_READ(bp, offset, len, buf, cr) \
	vn_rdwr(UIO_READ, bp, buf, len, offset, \
				UIO_SYSSPACE, 0, 0, cr, 0)

#define BFS_CCT_WRITE(bp, offset, len, buf, cr) \
	vn_rdwr(UIO_WRITE, bp, buf, len, offset, \
		UIO_SYSSPACE, IO_SYNC, RLIM_INFINITY, cr, (int *)0)

/*
 * void
 * bfs_shiftfile(struct bfs_vfs *bp, inode_t *ip, daddr_t gapsize,
 *		 char *bufferp, cred_t *cr)
 *	Shift the file described by inode "ip", "gapsize" blocks.
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the following locks:
 *	   - bfs_writelock 
 *	   - bfs_fs_rwlock in exclusive mode if called from bfs_compact()
 *	   - bfs_fs_rwlock in shared mode if called from bfs_quickcompact()
 *
 *	The caller of this routine must hold the inode rwlock in exclusive
 *	mode if the inode is in-core.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
void
bfs_shiftfile(struct bfs_vfs *bp, inode_t *ip, daddr_t gapsize,
	      char *bufferp, cred_t *cr)
{
	dinode_t	*dip;
	daddr_t		maxshift;
	daddr_t		filesize;
	struct	bfs_sanity sw;

	ASSERT(KS_HOLD0LOCKS());

	dip = &ip->i_diskino;
	ASSERT(BFS_FILESIZE(dip) != 0);

	sw.fromblock = dip->d_sblock;
	sw.toblock = dip->d_sblock - gapsize;

#ifdef DEBUG
	if (bfs_debugcomp)
		cmn_err(CE_CONT,"Shifting a file  inode %d   from %d to %d\n",
		  dip->d_ino, sw.fromblock, sw.toblock);
#endif

	maxshift = min(BFSBUFSIZE / BFS_BSIZE, gapsize);

	/*
	 * Write sanity words for fsck to indicate compaction is in progress.
	 */
	sw.bfromblock = sw.fromblock;
	sw.btoblock = sw.toblock;
	BFS_CCT_WRITE(bp->bfs_devnode, BFS_SANITYWSTART,
		      sizeof(struct bfs_sanity), &sw, cr);

	/*
	 * Calculate the new EOF.
	 */
	dip->d_eoffset -= (gapsize * BFS_BSIZE);

	filesize = BFS_NZFILESIZE(dip);

	/*
	 * Copy as many sectors of the file at a time.
	 */
	for (;;) {
		/*
		 * Must recalculate "maxshift" each time.
		 */
		maxshift = min(maxshift, (dip->d_eblock - sw.fromblock + 1));


		BFS_CCT_READ(bp->bfs_devnode, sw.fromblock * BFS_BSIZE,
		  maxshift * BFS_BSIZE, bufferp, cr);

		BFS_CCT_WRITE(bp->bfs_devnode, sw.toblock * BFS_BSIZE,
		  maxshift * BFS_BSIZE, bufferp, cr);

		sw.fromblock += maxshift;
		sw.toblock += maxshift;

		if (sw.fromblock > dip->d_eblock)
			break;

		/*
		 * If gapsize is less than file size, must write words for fsck
		 * to indicate that compaction is in progress (specifically 
		 * which blocks are being shifted.)
		 * Otherwise, there is no need to write sanity words. If the
		 * system crashes during compaction, fsck can take it from 
		 * the top without data loss. 
		 */
		if (gapsize * BFS_BSIZE < filesize) {
			sw.bfromblock = sw.fromblock;
			sw.btoblock = sw.toblock;
			BFS_CCT_WRITE(bp->bfs_devnode, BFS_SANITYWSTART,
			  sizeof(struct bfs_sanity), &sw, cr);
		}
	}

	/*
	 * Compute the new start and end blocks and write the inode out to disk.
	 */
	dip->d_sblock -= gapsize;
	dip->d_eblock -= gapsize;
	BFS_PUTINODE(bp->bfs_devnode, ip->i_inodoff, dip, cr);

	/*
	 * If the file just shifted up is the last file in the file system,
	 * must update the start and last block of the lastfile.
	 */
	if (dip->d_ino == BFS_OFF2INO(bp->bfs_lastfilefs)) {
		bp->bfs_sblklastfile = dip->d_sblock;
		bp->bfs_eblklastfile = dip->d_eblock;
	}

	/*
	 * Must write "-1" to all 4 sanity words for fsck to indicate that
	 * compaction is no longer in progress.
	 */
	sw.fromblock = -1;
	sw.toblock = -1;
	sw.bfromblock = -1;
	sw.btoblock = -1;
	BFS_CCT_WRITE(bp->bfs_devnode, BFS_SANITYWSTART,
	  sizeof(struct bfs_sanity), &sw, cr);
#ifdef DEBUG
	if (bfs_debugcomp)
		cmn_err(CE_CONT, "File shifted\n");
#endif
}
