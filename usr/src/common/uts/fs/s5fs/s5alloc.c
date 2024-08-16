/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/s5fs/s5alloc.c	1.11"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/mode.h>
#include <fs/buf.h>
#include <fs/stat.h>
#include <fs/s5fs/s5param.h>
#include <fs/s5fs/s5fblk.h>
#include <fs/s5fs/s5filsys.h>
#include <fs/s5fs/s5ino.h>
#include <fs/s5fs/s5inode.h>
#include <fs/s5fs/s5macros.h>
#include <fs/s5fs/s5hier.h>
#include <fs/fs_subr.h>
#include <io/conf.h>
#include <mem/pvn.h>
#include <mem/page.h>
#include <mem/swap.h>
#include <proc/disp.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/param.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <util/var.h>

typedef	struct fblk *FBLKP;

int	s5_badblock(filsys_t *, daddr_t, dev_t);
void 	s5_blkfree(vfs_t *, daddr_t);
void	s5_ifree(inode_t *);
extern	void	s5_iput(inode_t *);
extern	void	s5_iupdat(inode_t *);
extern	buf_t	*buf_search_hashlist(buf_t *, dev_t, daddr_t);

#ifdef DEBUG
void s5_brange_check(inode_t *, daddr_t);
#endif

/*
 * int
 * s5_blkalloc(vfs_t *vfsp, daddr_t *bnp)
 *	Obtain the next available free disk block from
 *	the free list of the specified device.
 *	The super-block has up to NICFREE remembered free blocks;
 * 	the last of these is read to obtain NICFREE more.
 *
 * Calling/Exit State:
 *	The inode rwlock is held on entry and exits. 
 *
 *	A return value of 0 indicates success; otherwise, a valid errno
 *	is returned. Errors returned directly by this routine is ENOSPC.
 *
 * Description:
 *	Sleep lock is held while modifying the superblock info.
 * 	no space on dev x/y -- when the free list is exhausted.
 */
int
s5_blkalloc(vfs_t *vfsp, daddr_t *bnp)
{
	dev_t		dev;
	daddr_t		bno;
	buf_t		*bp;
	int		bsize;
	filsys_t	*fp;
	s5_fs_t	*s5fsp;

	dev = vfsp->vfs_dev;
	bsize = vfsp->vfs_bsize;
	fp = getfs(vfsp);
	s5fsp = S5FS(vfsp);
	SLEEP_LOCK(&s5fsp->fs_sblock, PRIVFS);
	do {
		if (fp->s_nfree <= 0){
			goto nospace;
		}
		if ((bno = fp->s_free[--fp->s_nfree]) == 0){
			goto nospace;
		}
	} while (s5_badblock(fp, bno, dev));

	if (fp->s_nfree <= 0) {
		bp = bread(dev, LTOPBLK(bno, bsize), bsize);
		if ((bp->b_flags & B_ERROR) == 0) {
			fp->s_nfree = ((FBLKP)(bp->b_addrp))->df_nfree;
			bcopy((caddr_t)((FBLKP)(bp->b_addrp))->df_free,
			    (caddr_t)fp->s_free, sizeof(fp->s_free));
		}
		bp->b_flags &= ~B_DELWRI;
		bp->b_flags |= B_STALE|B_AGE;
		brelse(bp);
	}
	if (fp->s_nfree <= 0 || fp->s_nfree > NICFREE) {
		goto nospace;
	}
	if (fp->s_tfree)
		fp->s_tfree--;
	fp->s_fmod = 1;
	SLEEP_UNLOCK(&s5fsp->fs_sblock);
	*bnp = bno;
	return 0;

nospace:
	SLEEP_UNLOCK(&s5fsp->fs_sblock);
	fp->s_nfree = 0;
	fp->s_tfree = 0;
	delay(5*HZ);
	cmn_err(CE_NOTE, "%s: file system full, dev = %d\n", fp->s_fname, dev);
	return ENOSPC;
}

/*
 * void
 * s5_blkfree(vfs_t *vfsp, daddr_t bno)
 * 	Free a block
 *
 * Calling/Exit State:
 *	The inode rwlock is held on entry and exit.
 *
 * Description:
 *	Place the specified disk block back on the free list of the
 *  	specified device.
 */
void
s5_blkfree(vfs_t *vfsp, daddr_t bno)
{
	dev_t		dev;
	int		bsize;
	filsys_t	*fp;
	buf_t		*bp;
	daddr_t		pbno;
	s5_fs_t 	*s5fsp;

	dev = vfsp->vfs_dev;
	bsize = vfsp->vfs_bsize;
	fp = getfs(vfsp);
	s5fsp = S5FS(vfsp);
	SLEEP_LOCK(&s5fsp->fs_sblock, PRIVFS);
	fp->s_fmod = 1;
	if (s5_badblock(fp, bno, dev)){
		SLEEP_UNLOCK(&s5fsp->fs_sblock);
		return;
	}
	if (fp->s_nfree <= 0) {
		fp->s_nfree = 1;
		fp->s_free[0] = 0;
	}
	pbno = LTOPBLK(bno, bsize);
	if (fp->s_nfree >= NICFREE) {
		bp = getblk(dev, pbno, bsize, 0);
		((FBLKP)(bp->b_addrp))->df_nfree = fp->s_nfree;
		bcopy((caddr_t)fp->s_free,
		  (caddr_t)((FBLKP)(bp->b_addrp))->df_free,
		  sizeof(fp->s_free));
		fp->s_nfree = 0;
		bdwrite(bp);
	} else {
		if ((bp = getblk(dev, pbno, bsize, BG_NOMISS)) != NULL) {
		/*
		 * There may be a leftover in-core buffer for this block;
		 * if so, make sure it's marked invalid and turn off
		 * B_DELWRI so that it will not subsequently be written
		 * to disk.  Otherwise, if the block is subsequently
		 * allocated as file data, the stale data in the buffer
		 * will be aliasing the data in the page cache.
		 */
			bp->b_flags &= ~B_DELWRI;
			bp->b_flags |= B_STALE|B_AGE;
			brelse(bp);
		}
	}

	fp->s_free[fp->s_nfree++] = bno;
	fp->s_tfree++;
	fp->s_fmod = 1;
	SLEEP_UNLOCK(&s5fsp->fs_sblock);
}

/*
 * int
 * s5_badblock(filsys_t *fp, daddr_t bn, dev_t dev)
 *	Check that a specified block number is valid in
 *	the containing file system.
 *
 * Calling/Exit State:
 *	The superblock lock is  locked on entry or exit.
 *	If the block number is not valid for the file system,
 *	1 is returned; otherwise 0 is.
 *
 * Description:
 * 	Check that a block number is in the range between the I list
 * 	and the size of the device.  This is used mainly to check that
 * 	a garbage file system has not been mounted.
 *
 * 	bad block on dev x/y -- not in range
 */
/* ARGSUSED */
int
s5_badblock(filsys_t *fp, daddr_t bn, dev_t dev)
{
	if ((unsigned)bn < (daddr_t)fp->s_isize
	  || (unsigned)bn >= (daddr_t)fp->s_fsize) {
		/*
		 *+ A out-of-range block number was encountered while the
		 *+ kernel was doing a filesystem block allocation or freeing
		 *+ a filesystem block. This probably indicates a corrupted
		 *+ filesystem. Corrective action: run fsck on the filesystem.
		 */
		cmn_err(CE_WARN, "bad block %d, %s: bad block\n",
			bn, fp->s_fname);
		return 1;
	}
	return 0;
}

/*
 * int
 * s5_ialloc(vfs_t *vfsp, o_mode_t mode, int nlink, dev_t rdev,
 *	     int uid, int gid, inode_t **ipp)
 * 	Allocate an unused inode on the specified device.
 *
 * Calling/Exit State:
 *	On successful returns, <*ipp->i_rwlock> is held exclusive.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		ENOSPC  The file system is out of inodes.
 *   
 * Description:
 *	The algorithm keeps up to NICINOD spare inodes
 * 	in the super-block.  When this runs out, a linear search through
 * 	the i-list is instituted to pick up NICINOD more.
 */
int
s5_ialloc(vfs_t *vfsp, o_mode_t mode, int nlink, dev_t rdev, int uid,
	  int gid, inode_t **ipp)
{
	dev_t		dev;
	int		bsize;
	filsys_t	*fp;
	vnode_t		*vp;
	inode_t 	*ip;
	int		i;
	buf_t		*bp;
	dinode_t	*dp;
	u_short		ino;
	daddr_t		adr;
	int		error;
	s5_fs_t	*s5fsp;
	pl_t	s;

	ip = NULL;
	dev = (dev_t)vfsp->vfs_dev;
	bsize = vfsp->vfs_bsize;
	s5fsp = S5FS(vfsp);
	fp = getfs(vfsp);

	SLEEP_LOCK(&s5fsp->fs_sblock, PRIVFS);
loop:
	if (fp->s_ninode > 0 && (ino = fp->s_inode[--fp->s_ninode])) {
		error = s5_iget(vfsp, fp, ino, IG_EXCL, ipp);
		if (error) {
			SLEEP_UNLOCK(&s5fsp->fs_sblock);
			return error;
		}
		/*
		 * (*ipp)->i_rwlock is held *exclusive* here
		 */
		ip = *ipp;
		vp = ITOV(ip);
		if (ip->i_mode == 0) {
			/* Found inode: update now to avoid races */
			enum vtype type;

			vp->v_type = type = IFTOVT((int)mode);
			ip->i_mode = mode;
			ip->i_nlink = (o_nlink_t)nlink;
			ip->i_uid = (o_uid_t)uid;
			ip->i_gid = (o_gid_t)gid;
			ip->i_size = 0;
			s = S5_ILOCK(ip);
			IMARK(ip, IACC|IUPD|ICHG|ISYN);
			S5_IUNLOCK(ip, s);
			for (i = 0; i < NADDR; i++)
				ip->i_addr[i] = 0;
			/*
			 * Must set rdev after address fields are
			 * zeroed because rdev is defined to be the
			 * first address field (inode.h).
			 */
			if (type == VCHR || type == VBLK) { 
				ip->i_rdev = rdev;
				/* update i_addr components */
				ip->i_major = getemajor(rdev);
				ip->i_minor = geteminor(rdev);
				ip->i_bcflag |= NDEVFORMAT;
				/*
				 * To preserve backward compatibility we store
				 * dev in old format if it fits, otherwise
				 * O_NODEV is assigned.
				 */
				if (cmpdev_fits(rdev))
					ip->i_oldrdev = (daddr_t)_cmpdev(rdev);
				else
					ip->i_oldrdev = (daddr_t)O_NODEV;

			} else if (type == VXNAM) {
				/*
				 * Believe it or not. XENIX stores 
				 * semaphore info in rdev.
				 */
				ip->i_rdev = rdev;
				ip->i_oldrdev = rdev; /* need this for iupdat */
			}

			vp->v_rdev = ip->i_rdev;
			if (fp->s_tinode)
				fp->s_tinode--;
			fp->s_fmod = 1;
			SLEEP_UNLOCK(&s5fsp->fs_sblock);
			s5_iupdat(ip);
			*ipp = ip;
			return 0;
		}
		/*
		 *+ Inode was allocated after all. Search for
		 *+ another one.
		 */
		cmn_err(CE_NOTE, "ialloc: inode was already allocated\n");
		SLEEP_UNLOCK(&s5fsp->fs_sblock);
		s5_iupdat(ip);
		s5_iput(ip);
		SLEEP_LOCK(&s5fsp->fs_sblock, PRIVFS);
		goto loop;
	}
	/*
	 * Only try to rebuild freelist if there are free inodes.
	 */

	if (fp->s_tinode > 0) {
		fp->s_ninode = NICINOD;
		ino = FsINOS(s5fsp, fp->s_inode[0]);
		for (adr = FsITOD(s5fsp, ino); adr < (daddr_t)fp->s_isize;
		  adr++) {
			bp = bread(dev, LTOPBLK(adr, bsize), bsize);
			if (bp->b_flags & B_ERROR) {
				brelse(bp);
				ino += s5fsp->fs_inopb;
				continue;
			}
			dp = (dinode_t *)bp->b_addrp;
			for (i = 0; i < s5fsp->fs_inopb; i++, ino++, dp++) {
				if (fp->s_ninode <= 0) {
					break;
				}
				if (dp->di_mode == 0)
					fp->s_inode[--fp->s_ninode] = ino;
			}
			brelse(bp);
			if (fp->s_ninode <= 0){
				break;
			}
		}
		if (fp->s_ninode > 0) {
			fp->s_inode[fp->s_ninode-1] = 0;
			fp->s_inode[0] = 0;
		}
		if (fp->s_ninode != NICINOD) {
			fp->s_ninode = NICINOD;
			goto loop;
		}
	}

	fp->s_ninode = 0;
	fp->s_tinode = 0;
	SLEEP_UNLOCK(&s5fsp->fs_sblock);
	return ENOSPC;
}

/*
 * void
 * s5_ifree(inode_t *ip)
 * 	Free the specified inode on the specified device.
 *
 * Calling/Exit State:
 *	Inode rwlock is locked on entry and on exit.
 *
 * Description :
 *	The algorithm stores up to NICINOD inodes in the
 *	super-block and throws away any more.
 */
void
s5_ifree(inode_t *ip)
{

	filsys_t	*fp;
	u_short		ino;
	vnode_t		*vp;
	s5_fs_t	*s5fsp;

	vp = ITOV(ip);
	s5fsp = S5FS(vp->v_vfsp);
	fp = getfs(vp->v_vfsp);
	ino = ip->i_number;

	SLEEP_LOCK(&s5fsp->fs_sblock, PRIVFS);
	fp->s_tinode++;
	fp->s_fmod = 1;
	if (fp->s_ninode >= NICINOD || fp->s_ninode == 0) {
		if (ino < fp->s_inode[0])
			fp->s_inode[0] = ino;
	} else {
		fp->s_inode[fp->s_ninode++] = ino;
	}
	SLEEP_UNLOCK(&s5fsp->fs_sblock);

}


#ifdef DEBUG
void
s5_brange_check(inode_t *ip, daddr_t bn)
{
	filsys_t	*fp;

	fp = getfs((vfs_t *)(ITOV(ip))->v_vfsp);
	if (bn < (daddr_t)fp->s_isize)
		cmn_err(CE_WARN, "bn out of range, ip = 0x%x, bn = 0x%x\n",
						ip, bn);

}

#endif
