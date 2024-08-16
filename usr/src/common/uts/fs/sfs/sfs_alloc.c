/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/sfs/sfs_alloc.c	1.26"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <util/types.h>
#include <util/debug.h>
#include <util/param.h>
#include <acc/priv/privilege.h>
#include <svc/systm.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <fs/buf.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/sfs/sfs_hier.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_quota.h>
#include <fs/sfs/sfs_tables.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <util/sysmacros.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/flock.h>
#include <fs/fs_subr.h>
#include <util/cmn_err.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif

extern void sfs_fsinvalid(vfs_t *);
extern void sfs_iput(inode_t *, cred_t *);
extern int sfs_iget(vfs_t *, fs_t *, ino_t, inode_t **, int, cred_t *);
extern int sfs_skpc(char, u_int, char *);
extern int sfs_scanc(u_int, u_char *, u_char *, u_char);
extern void sfs_setblock(fs_t *, u_char *, daddr_t h);
extern void sfs_clrblock(fs_t *, u_char *, daddr_t);
extern int sfs_isblock(fs_t *, u_char *, daddr_t);
extern int sfs_badblock(fs_t *, daddr_t);
extern void sfs_fragacct(fs_t *, int, long *, int);
extern int sfs_chkdq(inode_t *, long, int, cred_t *);
extern int sfs_chkiq(sfs_vfs_t *, inode_t *, uid_t, int, cred_t *);
extern void sfs_freeblocks_sync(vfs_t *, int);

STATIC int sfs_hashalloc(inode_t *, int, long, int, u_long *, int (*)(), int *);
STATIC daddr_t sfs_fragextend(inode_t *, int, long, int, int);
STATIC int sfs_alloccg(inode_t *, int, daddr_t, int , daddr_t *);
STATIC int sfs_alloccgblk(fs_t *, cg_t *, daddr_t, vfs_t *, daddr_t *);
int sfs_realloccg(inode_t *, daddr_t, daddr_t, int, int, daddr_t *, cred_t *);
STATIC int sfs_ialloccg(inode_t *, int, daddr_t, int, ino_t *, int *);
STATIC int sfs_mapsearch(fs_t *, cg_t *, daddr_t, int, vfs_t *, daddr_t *);
int sfs_free(inode_t *, daddr_t, off_t);
int sfs_ifree(inode_t *, ino_t, mode_t);


/*
 * fsfull(fs_t *fs)
 *	Print a warning message about running out of disk space
 *	ona file system.
 *
 * Calling/Exit State:
 *	Only locks that can be held while blocking
 *	may be held by the calling process.
 */
void
fsfull(fs_t *fs)
{
	/*
	 *+ A write to the filesystem failed because there
	 *+ were no disk blocks available on that filesystem.
	 *+ Corrective action: remove some files to free up disk blocks. 
	 */
	cmn_err(CE_NOTE, "%s: file system full\n", fs->fs_fsmnt);
	return;
}

/*
 * int
 * sfs_alloc(ip, bpref, size, bnp, cr)
 *	Allocate a block in the file system.
 *
 * Calling/Exit State:
 *	No inode lock is necessary since the caller conditionally
 *	assigns <*bnp> to the inode.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EINVAL	<size> is 0.
 *		ENOSPC	There is not enough space on the file
 *			system to satisfy the request.
 *		EIO	<size> is an invalid block size for the
 *			file system.
 *
 * Description:
 *	The size of the requested block is given, which must be some
 *	multiple of fs_fsize and <= fs_bsize.
 *	A preference may be optionally specified. If a preference is given
 *	the following hierarchy is used to allocate a block:
 *		1) Allocate the requested block.
 *		2) Allocate a rotationally optimal block in the same cylinder.
 *		3) Allocate a block in the same cylinder group.
 *		4) Quadradically rehash into other cylinder groups, until an
 *		   available block is located.
 *	If no block preference is given the following heirarchy is used
 *	to allocate a block:
 *		1) Allocate a block in the cylinder group that contains the
 *		   inode for the file.
 *		2) Quadradically rehash into other cylinder groups, until an
 *		   available block is located.
 */
int
sfs_alloc(inode_t *ip, daddr_t bpref, int size, daddr_t *bnp, cred_t *cr)
{
	fs_t	*fs;
	daddr_t	bno = 0;
	int	cg;
	int	error;

	ASSERT(((ip->i_number & 1) == 0) || (UFSIP(ip)));

	if (size == 0) {
		return(EINVAL);
	}

	fs = ip->i_fs;
	if (((unsigned)size > fs->fs_bsize) || (fragoff(fs, size) != 0)) {
		/*
		 *+ A request was made to allocate a disk block in a
		 *+ filesystem; however, the request specified an invalid
		 *+ block size.
		 */
		cmn_err(CE_WARN, 
		   "UFS/SFS sfs_alloc: bad size, dev = 0x%x, bsize = %d, size = %d, fs = %s\n",
		   ip->i_dev, fs->fs_bsize, size, fs->fs_fsmnt);
		sfs_fsinvalid(ITOV(ip)->v_vfsp);
		return (EIO);
	}

	/*
	 * Do a quick check for space availability - no locking is
	 * used since this is a quick read.  If no blocks are available,
	 * or if the total amount of free space is less than two times the
	 * percentage reservation, then free everything from the deferred
	 * free block list.  The deferred free block list consists of
	 * blocks which were part of recently freed inodes; however, these
	 * blocks can't be reallocated until the inode they were part
	 * of is written to disk with its data block pointers zeroed out.
	 */
	if ((fs->fs_cstotal.cs_nbfree == 0) ||
			(freespace(fs, 2 * fs->fs_minfree) <= 0))
		sfs_freeblocks_sync(ITOV(ip)->v_vfsp, 0);

	/*
	 * Do another quick check for space availability.  If space is
	 * still low, the file system will run out of space soon anyways.
	 */
	if (size == fs->fs_bsize && fs->fs_cstotal.cs_nbfree == 0) {
		goto nospace;
	}

	if (UFSIP(ip) && (freespace(fs, fs->fs_minfree) <= 0) &&
	    pm_denied(cr, P_FILESYS)) {
		goto nospace;
	}
	if (ip->i_dquot != NULL) {
		error = sfs_chkdq(ip, (long)btodb(size), 0, CRED());
		if (error) {
			return (error);
		}
	}

	if (bpref >= fs->fs_size) {
		bpref = (daddr_t)0;
	}

	if (bpref == (daddr_t)0) {
		cg = (int)itog(fs, ip->i_number);
	} else {
		cg = dtog(fs, bpref);
	}

	error = sfs_hashalloc(ip, cg, (long)bpref, size,
			     (u_long *)&bno, sfs_alloccg, NULL);
	/* check if operation is in error */
	if (error)
		return(error);
	if (bno > (daddr_t)0) {
		*bnp = bno;
		return (0);
	}

nospace:
	fsfull(fs);
	delay(5*HZ);
#ifdef CC_PARTIAL
	CC_COUNT(CC_RE_DB, CCBITS_RE_DB);
#endif
	return (ENOSPC);
}

/*
 * int
 * sfs_realloccg(ip, bprev, bpref, osize, nsize, bnp, cr)
 *	Reallocate a fragment to a bigger size
 *
 * Calling/Exit State:
 *	No inode lock is necessary since the caller conditionally
 *	assigns <*bnp> to the inode.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		ENOSPC	There is not enough space on the file system
 *			to satisfy the request.
 *		EIO	Either the old fragment size or newfragment size
 *			is invalid. The file system is sealed in this case.
 *		EIO	An attempt was made to extend a fragment from an
 *			invalid file system block number. The file system is
 *			sealed in this case.
 *
 * Description:
 *	The number and size of the old block is given, and a preference
 *	and new size is also specified.  The allocator attempts to extend
 *	the original block.  Failing that, the regular block allocator is
 *	invoked to get an appropriate block.
 */
int
sfs_realloccg(inode_t *ip, daddr_t bprev, daddr_t bpref,
	      int osize, int nsize, daddr_t *bnp, cred_t *cr)
{
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;
	fs_t		*fs;
	daddr_t		bno = 0;
	int		cg;
	int		request;
	int		error = 0;


	ASSERT(((ip->i_number & 1) == 0) || (UFSIP(ip)));

	fs = ip->i_fs;
	if ((unsigned)osize > fs->fs_bsize || fragoff(fs, osize) != 0 ||
	    (unsigned)nsize > fs->fs_bsize || fragoff(fs, nsize) != 0) {
		/*
		 *+ A request was made to resize a filesystem block
		 *+ fragment to a larger size; however, either the old
		 *+ fragment size or the new fragment size is
		 *+ invalid.
		 */
		cmn_err(CE_WARN, 
		    "UFS/SFS sfs_realloccg: bad size, dev = 0x%x, bsize = %d, osize = %d, nsize = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, osize, nsize, fs->fs_fsmnt);
		sfs_fsinvalid(ITOV(ip)->v_vfsp);
		return (EIO);
	}

	/*
	 * Do a quick check for space availability - no locking is
	 * used since this is a quick read.  If no blocks are available,
	 * or if the total amount of free space is less than two times the
	 * percentage reservation, then free everything from the deferred
	 * free block list.  The deferred free block list consists of
	 * blocks which were part of recently freed inodes; however, these
	 * blocks can't be reallocated until the inode they were part
	 * of is written to disk with its data block pointers zeroed out.
	 */
	if ((fs->fs_cstotal.cs_nbfree == 0) ||
			(freespace(fs, 2 * fs->fs_minfree) <= 0))
		sfs_freeblocks_sync(ITOV(ip)->v_vfsp, 0);

	if (UFSIP(ip) && (freespace(fs, fs->fs_minfree) <= 0) &&
	    pm_denied(cr, P_FILESYS)) {
		goto nospace;
	}

	if (bprev == (daddr_t)0) {
		/*
		 *+ An attempt was made to extend a fragment
		 *+ in an invalid filesystem block number.
		 */
		cmn_err(CE_WARN, 
		    "UFS/SFS sfs_realloccg: bad bprev, dev = 0x%x, bsize = %d, bprev = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, bprev, fs->fs_fsmnt);
		sfs_fsinvalid(ITOV(ip)->v_vfsp);
		return (EIO);
	}
	if (ip->i_dquot != NULL) {
		error = sfs_chkdq(ip, (long)btodb(nsize - osize), 0, CRED());
		if (error) {
			return (error);
		}
	}
	cg  = dtog(fs, bprev);
	bno = sfs_fragextend(ip, cg, (long)bprev, osize, nsize);
	if (bno != (daddr_t)0) {
		*bnp = bno;
		return (0);
	}

	vfsp = ITOV(ip)->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
	switch ((int)fs->fs_optim) {
	case FS_OPTSPACE:
		/*
		 * Allocate an exact sized fragment. Although this makes 
		 * best use of space, we will waste time relocating it if 
		 * the file continues to grow. If the fragmentation is
		 * less than half of the minimum free reserve, we choose
		 * to begin optimizing for time.
		 */
		request = nsize;
		if (fs->fs_minfree < 5 ||
		    (fs->fs_cstotal.cs_nffree >
		     fs->fs_dsize * fs->fs_minfree / (2 * 100))) {
			FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
			break;
		}
		fs->fs_optim = FS_OPTTIME;
		FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
		/*
		 *+ The fragmentation on the file system is less than
		 *+ half of the minimum free reserve so the kernel has
		 *+ chosen to begin TIME optimized allocations.
		 *+
		 */
		cmn_err(CE_NOTE, "%s: optimization changed from SPACE to TIME\n",
			fs->fs_fsmnt);
		break;
	default:
		/*
		 * Old file systems.
		 */
		fs->fs_optim = FS_OPTTIME;
		FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
		/*
		 *+ An unknown value for whether to optimize allocations
		 *+ by SPACE or TIME was noticed. This means that an old
		 *+ file system is being used. The default optimization 
		 *+ for allocations is chosen (TIME)
		 */
		cmn_err(CE_NOTE, "%s: bad optimization, defaulting to TIME\n",
			fs->fs_fsmnt);
		FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
		/* FALLTHRU */
	case FS_OPTTIME:
		/*
		 * At this point we have discovered a file that is trying
		 * to grow a small fragment to a larger fragment. To save
		 * time, we allocate a full sized block, then free the 
		 * unused portion. If the file continues to grow, the 
		 * `sfs_fragextend' call above will be able to grow it in place
		 * without further copying. If aberrant programs cause
		 * disk fragmentation to grow within 2% of the free reserve,
		 * we choose to begin optimizing for space.
		 */
		request = fs->fs_bsize;
		if (fs->fs_cstotal.cs_nffree <
		    fs->fs_dsize * (fs->fs_minfree - 2) / 100) {
			FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
			break;
		}
		fs->fs_optim = FS_OPTSPACE;
		FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
		/*
		 *+ The file system is running out of fragments and
		 *+ has chosen to SPACE optimize allocations.
		 */
		cmn_err(CE_NOTE, "%s: optimization changed from TIME to SPACE\n",
			fs->fs_fsmnt);
		break;
	}

	if ((long)bpref >= fs->fs_size) {
		bpref = (daddr_t)0;
	}

	error = sfs_hashalloc(ip, cg, (long)bpref, request,
				(u_long *)&bno, sfs_alloccg, NULL);
	if (error)
               return (error);
	if (error == 0 && bno > (daddr_t)0) {
		if (nsize < request) {
			error = sfs_free(ip, bno + numfrags(fs, nsize),
					 (off_t)(request - nsize));
			if (error)
                                return (error);
		}

		*bnp = bno;
		return (0);
	}
	/* there are no blocks available */
nospace:
	fsfull(fs);
	delay(5*HZ);
#ifdef CC_PARTIAL
	CC_COUNT(CC_RE_DB, CCBITS_RE_DB);
#endif
	return (ENOSPC);
}

/*
 * sfs_ialloc(pip, ipref, mode, ipp, cr, iskipped)
 *	Allocate an inode in the file system.
 *	
 * Calling/Exit State:
 *	<pip> need not be locked, but caller must hold a reference to it.
 *
 *	On successful returns, <*ipp->i_rwlock> is held exclusive.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		ENOSPC	The file system is out of inodes.
 *
 * Description:
 *	A preference may be optionally specified. If a preference is given
 *	the following hierarchy is used to allocate an inode:
 *		1) Allocate the requested inode.
 *		2) Allocate an inode in the same cylinder group.
 *		3) Quadradically rehash into other cylinder groups, until an
 *		   available inode is located.
 *	If no inode preference is given the following heirarchy is used
 *	to allocate an inode:
 *		1) Allocate an inode in cylinder group 0.
 *		2) Quadradically rehash into other cylinder groups, until an
 *		   available inode is located.
 */
int
sfs_ialloc(inode_t *pip, ino_t ipref, mode_t mode,
	   inode_t **ipp, cred_t *cr, int *iskipped)
{
	inode_t		*ip;
	fs_t		*fs;
	int		cg;
	ino_t		ino = 0;
	int		error;
	pl_t		pl;
	sfs_vfs_t	*sfs_vfsp;

	fs = pip->i_fs;
	/*
	 * no locking here since this is a heuristic
	 */
	if (fs->fs_cstotal.cs_nifree == 0) {
		goto noinodes;
	}
	sfs_vfsp = (sfs_vfs_t *)(ITOV(pip))->v_vfsp->vfs_data;
	if (UFSIP(pip)) {
		pl = QLIST_LOCK();
		if (sfs_vfsp->vfs_qinod != NULL) {
			QLIST_UNLOCK(pl);
			error = sfs_chkiq(sfs_vfsp, (inode_t *)NULL, cr->cr_uid, 0, cr);
			if (error) {
				return (error);
			}
		} else {
			QLIST_UNLOCK(pl);
		}
	}
	if (ipref >= (u_long)(fs->fs_ncg * fs->fs_ipg)) {
		ipref = (ino_t)0;
	}
	cg = itog(fs, ipref);
	error = sfs_hashalloc(pip, cg, (long)ipref, mode,
				(u_long *)&ino, sfs_ialloccg, iskipped);
	if (error) {
		return (error);
	}

	ASSERT(((ino & 1) == 0) || (UFSIP(pip)));
	if (ino == (ino_t)0) {
		goto noinodes;
	}

	error = sfs_iget((ITOV(pip))->v_vfsp, fs, ino, ipp, IG_EXCL, cr);
	if (error) {
		sfs_ifree(pip, ino, 0);
		return (error);
	}

	/*
	 * (*ipp)->i_rwlock is held *exclusive* here
	 */
	ip = *ipp;
	if (ip->i_mode) {
		/*
		 *+ A "free" inode on the filesystem was found to have
		 *+ been already allocated.  This indicates an
		 *+ inconsistency between an inode's state in the inode
		 *+ bit map and its image in the inode free list.
		 */
		cmn_err(CE_NOTE, 
		    "UFS/SFS sfs_ialloc: mode = 0%o, inum = %d, fs = %s\n",
		    ip->i_mode, ip->i_number, fs->fs_fsmnt);
		ip->i_mode = 0;
	}
	/*
         * Start with zero blocks.  If there were blocks associated
         * with the inode, they are recovered on the next file system
         * check (fsck).
         */
	if (ip->i_blocks) {
		/*
		 *+ While the kernel was allocating an inode, it found
		 *+ a "free" inode with blocks allocated to it. This
		 *+ is an inconsistent state.  
		 */
		cmn_err(CE_NOTE, "free inode %s/%d had %d blocks\n",
		        fs->fs_fsmnt, ino, ip->i_blocks);
		ip->i_blocks = 0;
	}
	return (0);
noinodes:
	/*
	 *+ An attempt to create a symbolic link or some type of file
	 *+ or node failed because there were no free inodes available
	 *+ in that filesystem.  Corrective action:  remove some files 
	 *+ to free some inodes.
	 */
	cmn_err(CE_NOTE, "%s: out of inodes\n", fs->fs_fsmnt);
#ifdef CC_PARTIAL
	CC_COUNT(CC_RE_INODE, CCBITS_RE_INODE);
#endif
	return (ENOSPC);
}

/*
 * ino_t
 * sfs_dirpref(fs_t *fs)
 *	Find a cylinder to place a directory.
 *
 * Calling/Exit State:
 *	Caller guarantees validity of <fs>.
 *
 *	We don't lock the fs structure due to the heuristic and read-only
 *	nature of this algorithm.
 *
 * Description:
 *	The policy implemented by this algorithm is to select from
 *	among those cylinder groups with above the average number of
 *	free inodes, the one with the smallest number of directories.
 *
 */
ino_t
sfs_dirpref(fs_t *fs)
{
	int	cg;
	int	minndir;
	int	mincg;
	int	avgifree;

	avgifree = fs->fs_cstotal.cs_nifree / fs->fs_ncg;
	minndir = fs->fs_ipg;
	mincg = 0;
	for (cg = 0; cg < fs->fs_ncg; cg++)
		if (fs->fs_cs(fs, cg).cs_ndir < minndir &&
		    fs->fs_cs(fs, cg).cs_nifree >= avgifree) {
			mincg = cg;
			minndir = fs->fs_cs(fs, cg).cs_ndir;
		}
	return ((ino_t)(fs->fs_ipg * mincg));
}

/*
 * daddr_t
 * sfs_blkpref(ip, lbn, indx, bap)
 *	Select the desired position for the next block in a file.
 *
 * Calling/Exit State:
 *	No inode locks are required since the block is not assigned
 *	here - the caller will conditionally assign the block to
 *	the inode.
 *
 * Description:
 *	The file is logically divided into sections. The first section is
 *	composed of the direct blocks. Each additional section contains
 *	fs_maxbpg blocks.
 *	 
 *	If no blocks have been allocated in the first section, the policy
 *	is to request a block in the same cylinder group as the inode
 *	that describes the file. If no blocks have been allocated in any
 *	other section, the policy is to place the section in a cylinder
 *	group with a greater than average number of free blocks.  An
 *	appropriate cylinder group is found by using a rotor that sweeps
 *	the cylinder groups. When a new group of blocks is needed, the
 *	sweep begins in the cylinder group following the cylinder group
 *	from which the previous allocation was made. The sweep continues
 *	until a cylinder group with greater than the average number of free
 *	blocks is found. If the allocation is for the first block in an
 *	indirect block, the information on the previous allocation is
 *	unavailable; here a best guess is made based upon the logical block
 *	number being allocated.
 *	 
 *	If a section is already partially allocated, the policy is to
 *	contiguously allocate fs_maxcontig blocks.  The end of one of these
 *	contiguous blocks and the beginning of the next is physically separated
 *	so that the disk head will be in transit between them for at least
 *	fs_rotdelay milliseconds.  This is to allow time for the processor to
 *	schedule another I/O transfer.
 */
daddr_t
sfs_blkpref(inode_t *ip, daddr_t lbn, int indx, daddr_t *bap)
{
	fs_t	*fs;
	int	cg;
	int	avgbfree;
	int	startcg;
	daddr_t	nextblk;

	fs = ip->i_fs;
	if (((indx % fs->fs_maxbpg) == 0) || (bap[indx - 1] == 0)) {
		if (lbn < NDADDR) {
			cg = itog(fs, ip->i_number);
			return ((fs->fs_fpg * cg) + fs->fs_frag);
		}
		/*
		 * Find a cylinder with greater than average
		 * number of unused data blocks.
		 */
		if (indx == 0 || bap[indx - 1] == 0) {
			startcg = itog(fs, ip->i_number) + lbn / fs->fs_maxbpg;
		} else {
			startcg = dtog(fs, bap[indx - 1]) + 1;
		}
		startcg %= fs->fs_ncg;
		avgbfree = fs->fs_cstotal.cs_nbfree / fs->fs_ncg;
		for (cg = startcg; cg < fs->fs_ncg; cg++) {
			if (fs->fs_cs(fs, cg).cs_nbfree >= avgbfree) {
				fs->fs_cgrotor = cg;
				return (fs->fs_fpg * cg + fs->fs_frag);
			}
		}
		for (cg = 0; cg <= startcg; cg++) {
			if (fs->fs_cs(fs, cg).cs_nbfree >= avgbfree) {
				fs->fs_cgrotor = cg;
				return (fs->fs_fpg * cg + fs->fs_frag);
			}
		}
		return (NULL);
	}
	/*
	 * One or more previous blocks have been laid out. If less
	 * than fs_maxcontig previous blocks are contiguous, the
	 * next block is requested contiguously, otherwise it is
	 * requested rotationally delayed by fs_rotdelay milliseconds.
	 */
	nextblk = bap[indx - 1] + fs->fs_frag;
	if (indx > fs->fs_maxcontig &&
	    bap[indx - fs->fs_maxcontig] + blkstofrags(fs, fs->fs_maxcontig)
	    != nextblk) {
		return (nextblk);
	}
	if (fs->fs_rotdelay != 0) {
		/*
		 * Here we convert ms of delay to frags as:
		 * (frags) = (ms) * (rev/sec) * (sect/rev) /
		 *	((sect/frag) * (ms/sec))
		 * then round up to the next block.
		 */
		nextblk += roundup(fs->fs_rotdelay * fs->fs_rps * fs->fs_nsect /
		    (NSPF(fs) * 1000), fs->fs_frag);
	}

	return (nextblk);
}

/*
 * int
 * sfs_free(inode_t ip, daddr_t bno, off_t size)
 *	Free a block or fragment.
 *
 * Calling/Exit State:
 *	No inode lock is necessary here since the block has already
 *	been removed from the inode by the time we're called.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EIO	The file system containing <ip> has been
 *			invalidated and is unaccessible.
 *		EIO	<size> is incorrect; either greater than
 *			the file system's block size or not at a
 *			block boundary. The file system is invalidated
 *			in this case.
 *		EIO	A block which has already been freed is
 *			being freed again. The file system is invalidated
 *			in this case.
 *		EIO	A fragment which overlaps with a free
 *			fragment is being freed. The file system
 *			is invalidated in this case.
 *
 * Description:
 *	The specified block or fragment is placed back in the
 *	free map. If a fragment is deallocated, a possible 
 *	block reassembly is checked.
 *
 */
int
sfs_free(inode_t *ip, daddr_t bno, off_t size)
{
	fs_t		*fs;
	cg_t		*cgp;
	buf_t		*bp;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;
	daddr_t		newbno;
	int		cg;
	int		blk;
	int		frags;
	int		bbase;
	int		i;

	vfsp = ITOV(ip)->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	newbno = bno;
	fs = ip->i_fs;
	if ((unsigned)size > fs->fs_bsize || fragoff(fs, size) != 0) {
		/*
		 *+ The kernel freed a block having an incorrect size.
		 *+ This indicates either a filesystem software bug
		 *+ or corrupted data structures on disk in the filesystem.
		 */
		cmn_err(CE_WARN, 
		    "UFS/SFS sfs_free: bad size, dev = 0x%x, bsize = %d, size = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, size, fs->fs_fsmnt);
		sfs_fsinvalid(vfsp);
		return (EIO);
	}
	cg = dtog(fs, newbno);
	if (sfs_badblock(fs, newbno)) {
		/*
		 *+ The kernel freed a block having an invalid block
		 *+ number.  This indicates possible corruption of
		 *+ data structures on disk in the filesystem.
		 */
		cmn_err(CE_NOTE, "UFS/SFS sfs_free: bad block %d, ino %d\n",
			newbno, ip->i_number);
		return (0);
	}
	bp = bread(ip->i_dev, (daddr_t)fsbtodb(fs, cgtod(fs, cg)),
		   (int)fs->fs_bsize);

	cgp = (cg_t *)bp->b_addrp;
	if ((bp->b_flags & B_ERROR) || (cgp->cg_magic != CG_MAGIC)) {
		brelse(bp);
		return (0);
	}
	cgp->cg_time = hrestime.tv_sec;
	newbno = dtogd(fs, newbno);
	if (size == fs->fs_bsize) {
		if (sfs_isblock(fs, cgp->cg_free,
		                (daddr_t)fragstoblks(fs, newbno))) {
			/*
			 *+ The kernel freed a filesystem block that was
			 *+ already on the free list.
			 */
			cmn_err(CE_WARN,
			    "freeing free blk, block = %d, fs = %s, cg = %d\n",
			    newbno, fs->fs_fsmnt, cg);
			brelse(bp);
			sfs_fsinvalid(vfsp);
			return (EIO);
		}
		sfs_setblock(fs, cgp->cg_free, (daddr_t)fragstoblks(fs, newbno));
		cgp->cg_cs.cs_nbfree++;
		i = cbtocylno(fs, newbno);
		cgp->cg_b[i][cbtorpos(fs, newbno)]++;
		cgp->cg_btot[i]++;
		FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
		fs->fs_cstotal.cs_nbfree++;
		fs->fs_cs(fs, cg).cs_nbfree++;
		fs->fs_fmod++;
		FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
	} else {
		bbase = newbno - fragnum(fs, newbno);
		/*
		 * Decrement the counts associated with the old frags
		 */
		blk = blkmap(fs, cgp->cg_free, bbase);
		sfs_fragacct(fs, blk, cgp->cg_frsum, -1);
		/*
		 * Deallocate the fragment
		 */
		frags = numfrags(fs, size);
		for (i = 0; i < frags; i++) {
			if (isset(cgp->cg_free, newbno + i)) {
				/*
				 *+ The kernel freed a filesystem fragment
				 *+ that overlapped with a fragment already
				 *+ on the free list.
				 */
				cmn_err(CE_WARN,
				"freeing free frag, block = %d, fs = %s, cg = %d\n",
				   newbno + i, fs->fs_fsmnt, cg); 
				brelse(bp);
				sfs_fsinvalid(vfsp);
				return (EIO);
			}
			setbit(cgp->cg_free, newbno + i);
		}
		cgp->cg_cs.cs_nffree += i;
		FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
		fs->fs_cstotal.cs_nffree += i;
		fs->fs_cs(fs, cg).cs_nffree += i;
		fs->fs_fmod++;
		FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
		/*
		 * Add back in counts associated with the new frags
		 */
		blk = blkmap(fs, cgp->cg_free, bbase);
		sfs_fragacct(fs, blk, cgp->cg_frsum, 1);
		/*
		 * If a complete block has been reassembled, account for it.
		 * We set fs_mod again here since an intervening
		 * sbupdate will have cleared it.
		 */
		if (sfs_isblock(fs, cgp->cg_free,
				(daddr_t)fragstoblks(fs,bbase))) {
			cgp->cg_cs.cs_nffree -= fs->fs_frag;
			cgp->cg_cs.cs_nbfree++;
			FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
			fs->fs_cstotal.cs_nffree -= fs->fs_frag;
			fs->fs_cs(fs, cg).cs_nffree -= fs->fs_frag;
			fs->fs_cstotal.cs_nbfree++;
			fs->fs_cs(fs, cg).cs_nbfree++;
			fs->fs_fmod++;
			FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
			i = cbtocylno(fs, bbase);
			cgp->cg_b[i][cbtorpos(fs, bbase)]++;
			cgp->cg_btot[i]++;
		}
	}
	bdwrite(bp);
	return (0);
}

/*
 * sfs_ifree(ip, ino, mode)
 *	Free an inode to the cylinder's groups free list.
 *
 * Calling/Exit State:
 *	No inode locking is required.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EIO	<ino> is not in the range of valid inode numbers
 *			for this file system. The file system is invalidated
 *			in this case.
 *
 * Description:
 *	The specified inode is placed back in the free map. Buffer cache
 *	mutexing of cylinder group data provides synchronization
 *	among multiple ifree/ialloc calls.
 */
int
sfs_ifree(inode_t *ip, ino_t ino, mode_t mode)
{
	fs_t		*fs;
	cg_t		*cgp;
	buf_t		*bp;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;
	int		cg;
	ino_t		inot;
	ino_t		newino = ino;

	fs = ip->i_fs;
	if ((unsigned)newino >= (fs->fs_ipg * fs->fs_ncg)) {
		/*
		 *+ The kernel attempted to free an inode number that
		 *+ is outside the range of inode numbers in that
		 *+ filesystem.
		 */
		cmn_err(CE_WARN, 
		    "UFS/SFS sfs_ifree: range, dev = 0x%x, ino = %d, fs = %s\n",
		    ip->i_dev, newino, fs->fs_fsmnt);
		sfs_fsinvalid(ITOV(ip)->v_vfsp);
		return (EIO);
	}

	cg = itog(fs, newino);
	bp = bread(ip->i_dev, (daddr_t)fsbtodb(fs, cgtod(fs, cg)),
		(int)fs->fs_bsize);

	cgp = (cg_t *)bp->b_addrp;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC) {
		brelse(bp);
		return (0);
	}
	cgp->cg_time = hrestime.tv_sec;
	inot = newino % (u_long)fs->fs_ipg;
	if (isclr(cgp->cg_iused, inot)) {
		/*
		 *+ The kernel freed an inode that was already on the list
		 *+ of free inodes for that filesystem.
		 */
		cmn_err(CE_NOTE, 
		    "UFS/SFS sfs_ifree: freeing free inode, mode= %o, ino = %d, fs = %s, cg = %d\n",
		    ip->i_mode, newino, fs->fs_fsmnt, cg);
		brelse(bp);
		return (0);
	}
	clrbit(cgp->cg_iused, inot);
	if (inot < (u_long)cgp->cg_irotor) {
		cgp->cg_irotor = inot;
	}
	cgp->cg_cs.cs_nifree++;
	vfsp = ITOV(ip)->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
	fs->fs_cstotal.cs_nifree++;
	fs->fs_cs(fs, cg).cs_nifree++;
	if ((mode & IFMT) == IFDIR) {
		cgp->cg_cs.cs_ndir--;
		fs->fs_cstotal.cs_ndir--;
		fs->fs_cs(fs, cg).cs_ndir--;
	}
	fs->fs_fmod++;
	FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
	bdwrite(bp);
	return (0);
}


/*
 * sfs_hashalloc(ip, cg, pref, size, allocp, allocator, alloccost)
 *	Implement the cylinder overflow algorithm.
 *
 * Calling/Exit State:
 *	No inode locking required. Caller of alloc routine will conditionally
 *	assign block to inode.
 *
 * Description:
 *	The policy implemented by this algorithm is:
 *	   1) Allocate the block in the requested cylinder group.
 *	   2) Quadradically rehash on the cylinder group number.
 *	   3) Brute force search for a free block.
 */
STATIC int
sfs_hashalloc(inode_t *ip, int cg, long pref, int size,
	      u_long *allocp, int (*allocator)(), int *alloccost)
{
	fs_t	*fs;
	int	i;
	u_long	alloc = 0;
	int	error;
	int	icg = cg;

	if (alloccost) {
		*alloccost = 0;
	}

	fs = ip->i_fs;
	/*
	 * 1: preferred cylinder group
	 */
	error = (*allocator)(ip, cg, pref, size, &alloc, alloccost);
	if ((error != 0) || (alloc != 0)) {
		/* 
		 * Either we successfully allocated something,
		 * or there was an error.
		 */
		*allocp = alloc;
		return (error);
	}
	/*
	 * 2: quadratic rehash
	 */
	for (i = 1; i < fs->fs_ncg; i *= 2) {
		cg += i;
		if (cg >= fs->fs_ncg)
			cg -= fs->fs_ncg;
		error = (*allocator)(ip, cg, 0, size, &alloc, alloccost);
		if ((error != 0) || (alloc != 0)) {
			/* 
			 * either we successfully allocated something, 
			 * or there was an error 
			 */
			*allocp = alloc;
			return (error);
		}
	}
	/*
	 * 3: brute force search
	 * Note that we start at i == 2, since 0 was checked initially,
	 * and 1 is always checked in the quadratic rehash.
	 */
	cg = (icg + 2) % fs->fs_ncg;
	for (i = 2; i < fs->fs_ncg; i++) {
		error = (*allocator)(ip, cg, 0, size, &alloc, alloccost);
		if ((error != 0) || (alloc != 0)) {
			/* 
			 * either we successfully allocated something, 
			 * or there was an error 
			 */
			*allocp = alloc;
			return (error);
		}
		cg++;
		if (cg == fs->fs_ncg) {
			cg = 0;
		}
	}
	*allocp = 0;
	return (0);
}

/*
 * daddr_t
 * sfs_fragextend(inode_t ip, int cg, long bprev, int osize, int nsize)
 *	Determine whether a fragment can be extended.
 *
 * Calling/Exit State:
 *	No inode lock required; caller will contionally allocate block
 *	to inode.
 *
 *	A non-NULL return value indicates success; in this case, a valid
 *	block number is returned. NULL indicates that the fragment
 *	could not be extended.
 *
 * Description:
 *	Check to see if the necessary fragments are available, and 
 *	if they are, allocate them.
 */
STATIC daddr_t
sfs_fragextend(inode_t *ip, int cg, long bprev, int osize, int nsize)
{
	fs_t		*fs;
	buf_t		*bp;
	cg_t		*cgp;
	sfs_vfs_t	*sfs_vfsp;
	long		bno;
	int		frags;
	int		ofrags;
	int		delta;
	int		bbase;
	int		i;
	vfs_t		*vfsp;

	fs = ip->i_fs;
	if (fs->fs_cs(fs, cg).cs_nffree < numfrags(fs, nsize - osize)) {
		return (NULL);
	}
	frags = numfrags(fs, nsize);
	ofrags = numfrags(fs, osize);

	ASSERT(frags >= ofrags);

	bbase = fragnum(fs, bprev);
	if (bbase > fragnum(fs, (bprev + frags - 1))) {
		/* cannot extend across a block boundary */
		return (NULL);
	}
	bp = bread(ip->i_dev, (daddr_t)fsbtodb(fs, cgtod(fs, cg)),
		(int)fs->fs_bsize);

	cgp = (cg_t *)bp->b_addrp;
	if ((bp->b_flags & B_ERROR) || (cgp->cg_magic != CG_MAGIC)) {
		brelse(bp);
		return (NULL);
	}
	cgp->cg_time = hrestime.tv_sec;
	bno = dtogd(fs, bprev);
	for (i = ofrags; i < frags; i++) {
		if (isclr(cgp->cg_free, bno + i)) {
			brelse(bp);
			return (NULL);
		}
	}
	/*
	 * The current fragment can be extended, deduct the count
	 * on fragment being extended into and increase the count on the
	 * remaining fragment (if any) allocate the extended piece.
	 */
	for (i = frags; i < fs->fs_frag - bbase; i++) {
		if (isclr(cgp->cg_free, bno + i)) {
			break;
		}
	}

	cgp->cg_frsum[i - ofrags]--;

	if (i != frags) {
		cgp->cg_frsum[i - frags]++;
	}

	for (i = ofrags; i < frags; i++) {
		clrbit(cgp->cg_free, bno + i);
		cgp->cg_cs.cs_nffree--;
	}
	delta = frags - ofrags;
	vfsp = ITOV(ip)->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
	fs->fs_cstotal.cs_nffree -= delta;
	fs->fs_cs(fs, cg).cs_nffree -= delta;
	fs->fs_fmod++;
	FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
	bdwrite(bp);
	return (bprev);
}

/*
 * int
 * sfs_alloccg(inode_t *ip, int cg, daddr_t bpref, int size, daddr_t *bnop)
 *	Determine whether a block can be allocated.
 *
 * Calling/Exit State:
 *	No inode lock required; caller will conditionally assign block
 *	to inode.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. No errnos are directly returned by this
 *	routine.
 *
 * Description:
 *	Check to see if a block of the apprpriate size
 *	is available, and if it is, allocate it.
 */
STATIC int
sfs_alloccg(inode_t *ip, int cg, daddr_t bpref, int size, daddr_t *bnop)
{
	fs_t		*fs;
	buf_t		*bp;
	cg_t		*cgp;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;
	daddr_t		bno;
	int		frags;
	int		allocsiz;
	int		i;
	int		error;

	*bnop = (daddr_t)0;
	fs = ip->i_fs;
	if (fs->fs_cs(fs, cg).cs_nbfree == 0 && size == fs->fs_bsize) {
		return (0);
	}

	vfsp = ITOV(ip)->v_vfsp;
	bp = bread(ip->i_dev, (daddr_t)fsbtodb(fs, cgtod(fs, cg)),
		   (int)fs->fs_bsize);

	cgp = (cg_t *)bp->b_addrp;
	if ((bp->b_flags & B_ERROR) || (cgp->cg_magic != CG_MAGIC) ||
	    (cgp->cg_cs.cs_nbfree == 0 && size == fs->fs_bsize)) {
		brelse(bp);
		return (0);
	}

	cgp->cg_time = hrestime.tv_sec;
	if (size == fs->fs_bsize) {
		error = sfs_alloccgblk(fs, cgp, bpref, vfsp, bnop);
		bdwrite(bp);
		return (error);
	}
	/*
	 * Check to see if any fragments are already available
	 * allocsiz is the size which will be allocated, hacking
	 * it down to a smaller size if necessary.
	 */
	frags = numfrags(fs, size);
	for (allocsiz = frags; allocsiz < fs->fs_frag; allocsiz++) {
		if (cgp->cg_frsum[allocsiz] != 0) {
			break;
		}
	}

	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (allocsiz == fs->fs_frag) {
		/*
		 * No fragments were available, so a block
		 * will be allocated and hacked up.
		 */
		if (cgp->cg_cs.cs_nbfree == 0) {
			brelse(bp);
			return (0);
		}
		error = sfs_alloccgblk(fs, cgp, bpref, vfsp, &bno);
		*bnop = bno;
		if (error != 0) {
			brelse(bp);
			return (error);
		}

		bpref = dtogd(fs, bno);
		for (i = frags; i < fs->fs_frag; i++) {
			setbit(cgp->cg_free, bpref + i);
		}

		i = fs->fs_frag - frags;
		cgp->cg_frsum[i]++;
		cgp->cg_cs.cs_nffree += i;
		FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
		fs->fs_cstotal.cs_nffree += i;
		fs->fs_cs(fs, cg).cs_nffree += i;
		fs->fs_fmod++;
		FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
		bdwrite(bp);
		return (error);
	}
	/*
         * The bno returned from sfs_mapsearch() is relative to a
         * cylinder group, so prior to returning success the block
         * number needs to be adjusted.
	 */

	error = sfs_mapsearch(fs, cgp, bpref, allocsiz, vfsp, &bno);
	if (error != 0) {
		brelse(bp);
		return (error);
	}
	/*
         * Remove the necessary fragments from free map.
         * Update the count on the remaining fragments (if any).
         */
	for (i = 0; i < frags; i++) {
		clrbit(cgp->cg_free, bno + i);
	}
	cgp->cg_frsum[allocsiz]--;
	if (frags != allocsiz) {
		cgp->cg_frsum[allocsiz - frags]++;
	}
	cgp->cg_cs.cs_nffree -= frags;
	FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
	fs->fs_cstotal.cs_nffree -= frags;
	fs->fs_cs(fs, cg).cs_nffree -= frags;
	fs->fs_fmod++;
	FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
	bdwrite(bp);
	*bnop = cg * fs->fs_fpg + bno;
	return (error);
}

/*
 * int
 * sfs_alloccgblk(fs_t *fs, cg_t *cgp, daddr_t bpref, vfs_t *vfsp,daddr_t *bnop)
 *	Allocate a block in a cylinder group.
 *
 * Calling/Exit State:
 *	Caller arranged mutexing of <cgp>
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EIO	The rotationally optimal block selected based
 *			on cylinder group information is invalid. The
 *			file system is sealed in this case.
 *		EIO	The rotationally optimal block selected could
 *			is valid but could not be found in the cylinder
 *			group free list. The file system is sealed in 
 *			this case.
 *
 * Description:
 *	This algorithm implements the following policy:
 *	   1) Allocate the requested block.
 *	   2) Allocate a rotationally optimal block in the same cylinder.
 *	   3) Allocate the next available block on the block rotor for
 *	      the specified cylinder group.
 *
 *	Note that this routine only allocates fs_bsize blocks; these
 *	blocks may be fragmented by the routine that allocates them.
 *
 */
STATIC int
sfs_alloccgblk(fs_t *fs, cg_t *cgp, daddr_t bpref, vfs_t *vfsp, daddr_t *bnop)
{
	short		*cylbp;
	sfs_vfs_t	*sfs_vfsp;
	daddr_t		bno;
	int		cylno;
	int		pos;
	int		delta;
	int		i;
	int		error;

	/*
  	 * If no preference is specified, start search with the
	 * last allocated block.
	 */
	if (bpref == 0) {
		bpref = cgp->cg_rotor;
		goto norot;
	}
	bpref = blknum(fs, bpref);
	bpref = dtogd(fs, bpref);
	/*
	 * If the requested block is available, use it.
	 */
	if (sfs_isblock(fs, cgp->cg_free, (daddr_t)fragstoblks(fs, bpref))) {
		bno = bpref;
		goto gotit;
	}
	/*
	 * Check for a block available on the same cylinder.
	 */
	cylno = cbtocylno(fs, bpref);
	if (cgp->cg_btot[cylno] == 0) {
		goto norot;
	}
	if (fs->fs_cpc == 0) {
		/*
		 * Block layout info is not available, so just
		 * have to take any block in this cylinder.
		 */
		bpref = howmany(fs->fs_spc * cylno, NSPF(fs));
		goto norot;
	}
	/*
	 * Check the summary information to see if a block is 
	 * available in the requested cylinder starting at the
	 * requested rotational position and proceeding around.
	 */
	cylbp = cgp->cg_b[cylno];
	pos = cbtorpos(fs, bpref);
	for (i = pos; i < NRPOS; i++) {
		if (cylbp[i] > 0) {
			break;
		}
	}

	if (i == NRPOS) {
		for (i = 0; i < pos; i++) {
			if (cylbp[i] > 0) {
				break;
			}
		}
	}
	if (cylbp[i] > 0) {
		/*
		 * Found a rotational position, now find the actual
		 * block. Invalidate the file system if if none is
		 * actually there. Note: fs_postbl[] and fs_rotbl[]
		 * are read-only.
		 */
		pos = cylno % fs->fs_cpc;
		bno = (cylno - pos) * fs->fs_spc / NSPB(fs);
		if (fs->fs_postbl[pos][i] == -1) {
			/*
			 *+ When allocating a filesystem block within
			 *+ a cylinder group, the kernel discovered
			 *+ that the cylinder group data structures
			 *+ contained inconsistent data.
			 */
			cmn_err(CE_WARN, 
			    "UFS/SFS sfs_alloccgblk: cyl groups corrupted, pos = %d, i = %d, fs = %s\n",
			    pos, i, fs->fs_fsmnt);
			sfs_fsinvalid(vfsp);
			return (EIO);
		}
		for (i = fs->fs_postbl[pos][i];; ) {
			if (sfs_isblock(fs, cgp->cg_free, (daddr_t)(bno + i))) {
				bno = blkstofrags(fs, (bno + i));
				goto gotit;
			}
			delta = fs->fs_rotbl[i];
			if (delta <= 0 || delta > MAXBPC - i)
				break;
			i += delta;
		}
		/*
		 *+ When allocating a filesystem block within
		 *+ a cylinder group, the kernel discovered
		 *+ that the cylinder group data structures
		 *+ contained inconsistent data.
		 */
		cmn_err(CE_WARN, 
		    "UFS/SFS sfs_alloccgblk: can't find blk in cyl, pos = %d, i = %d, fs = %s\n",
		    pos, i, fs->fs_fsmnt);
		sfs_fsinvalid(vfsp);
		return (EIO);
	}
norot:
	/*
	 * No blocks in the requested cylinder, so take
	 * next available one in this cylinder group.
	 * The bno returned from sfs_mapsearch() is relative to a
	 * cylinder group, so prior to returning success the block
	 * number needs to be adjusted.
	 */
	
	error = sfs_mapsearch(fs, cgp, bpref, (int)fs->fs_frag, vfsp, &bno);
	if (error != 0)
		return (error);

	cgp->cg_rotor = bno;

gotit:
	sfs_clrblock(fs, cgp->cg_free, (long)fragstoblks(fs, bno));
	cgp->cg_cs.cs_nbfree--;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
	fs->fs_cstotal.cs_nbfree--;
	fs->fs_cs(fs, cgp->cg_cgx).cs_nbfree--;
	fs->fs_fmod++;
	FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
	cylno = cbtocylno(fs, bno);
	cgp->cg_b[cylno][cbtorpos(fs, bno)]--;
	cgp->cg_btot[cylno]--;
	*bnop = cgp->cg_cgx * fs->fs_fpg + bno;
	return (0);
}

/*
 * int
 * sfs_ialloccg(inode_t *ip, int cg, daddr_t ipref, int mode, ino_t *inop
 *              int *skipped)
 *	Determine whether an inode can be allocated from a
 *	specified cylinder group.
 *
 * Calling/Exit State:
 *	<ip> need not be locked, but caller must hold a reference.
 *
 *	On successful returns, if <*inop> is non-zero, than it
 *	contains the number of the inode allocated. If <*inop>
 *	is zero, than an inode could not be allocated.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EIO	The cylinder group indicates that there's
 *			an available inode but the cylinder group
 *			bitmap doesn't contain any free bits.
 *			The file system is invalidated in this case.
 *		EIO	The cylinder group bitmap indicates that
 *			there's an available slot, but there are
 *			not any free bits. The file system is
 *			is invalidated in this case.
 *
 * Description:
 *	Check to see if an inode is available. If there is an
 *	available inode, then allocate it using the following policy:
 *		1) Allocate the requested inode.
 *		2) Allocate the next available inode after
 *		   the requested inode in the specified cylinder group.
 *
 */
STATIC int
sfs_ialloccg(inode_t *ip, int cg, daddr_t ipref,
	     int mode, ino_t *inop, int *skipped)
{
	fs_t		*fs;
	cg_t		*cgp;
	buf_t		*bp;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;
	int		start;
	int		len;
	int		loc;
	int		map;
	int		i;
	int		nsearched;

	ASSERT(((ipref & 1) == 0) || (UFSIP(ip)));

	*inop = (ino_t)0;

	/*
	 * Make sure that an additional inode can
	 * be allocated from this cylinder group
	 */
	fs = ip->i_fs;
	if (fs->fs_cs(fs, cg).cs_nifree == 0) {
		if (skipped) {
			*skipped += fs->fs_ipg;
		}
		return (0);
	}

	bp = bread(ip->i_dev, (daddr_t)fsbtodb(fs, cgtod(fs, cg)),
		(int)fs->fs_bsize);

	cgp = (cg_t *)bp->b_addrp;
	if (bp->b_flags & B_ERROR || cgp->cg_magic != CG_MAGIC || 
	    cgp->cg_cs.cs_nifree == 0) {
		brelse(bp);
		return (0);
	}
	cgp->cg_time = hrestime.tv_sec;
	nsearched = 0;

	/*
	 * Check ipref, if it's available, then
	 * allocate it. If it's not available, start
	 * the search from the next available position.
	if (ipref) {
		ipref %= fs->fs_ipg;
		if (isclr(cgp->cg_iused, ipref)) {
			goto gotit;
		}
	}

	/*
	 * Scan the cylinder group bitmap for an available
	 * inode. Initially, search the range from
	 * [irotor, end-of-bitmap]. If an available inode
	 * can't be found in that range, try [0,irotor).
	 */
	start = cgp->cg_irotor / NBBY;
	len = howmany(fs->fs_ipg - cgp->cg_irotor, NBBY);
	loc = sfs_skpc(0xff, (u_int)len, &cgp->cg_iused[start]);
	nsearched = len - loc;
	if (loc == 0) {
		len = start + 1;
		start = 0;
		loc = sfs_skpc(0xff, (u_int)len, &cgp->cg_iused[0]);
		nsearched += len - loc;
		if (loc == 0) {
			/*
			 *+ A free inode could not be found in this
			 *+ cylinder group. However, the cylinder group
			 *+ information indicated that an inode was 
			 *+ available. Corrective action: run fsck.
			 */
			cmn_err(CE_WARN, 
			    "UFS/SFS sfs_ialloccg: map corrupted, cg = %d, irotor = %d, fs = %s\n",
			    cg, cgp->cg_irotor, fs->fs_fsmnt);
			brelse(bp);
			sfs_fsinvalid(ITOV(ip)->v_vfsp);
			*inop = (ino_t)0;
			return (EIO);
		}
	}
	nsearched *= NBBY;
	i = start + len - loc;
	map = cgp->cg_iused[i];
	ipref = i * NBBY;
	for (i = 1; i < (1 << NBBY); i <<= 1, ipref++) {
		if ((map & i) == 0) {
			cgp->cg_irotor = ipref;
			goto gotit;
		}
		nsearched++;
	}

	/*
	 *+ A cylinder group was expected to contain a free inode, but
	 *+ no free inode were found during a search of the cylinder
	 *+ group free inode list. This implies inconsistent data structures.
	 */
	cmn_err(CE_WARN, 
	    "UFS/SFS sfs_ialloccg: block not in map fs = %s, cg = %d\n", 
	    fs->fs_fsmnt, cg);
	brelse(bp);
	sfs_fsinvalid(ITOV(ip)->v_vfsp);
	return (EIO);

gotit:

	/*
	 * Allocate the inode by setting the bit indicated by
	 * ipref in the bitmap. Update the number of free inodes
	 * in the cylinder group and superblock. If a directory
	 * inode is being allocated, update the number of directories
	 * for both the cylinder group and file system.
	 */
	if (skipped && nsearched) {
		*skipped += nsearched;
	}
	setbit(cgp->cg_iused, ipref);
	cgp->cg_cs.cs_nifree--;
	vfsp = ITOV(ip)->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
	fs->fs_cstotal.cs_nifree--;
	fs->fs_cs(fs, cg).cs_nifree--;
	fs->fs_fmod++;
	if ((mode & IFMT) == IFDIR) {
		cgp->cg_cs.cs_ndir++;
		fs->fs_cstotal.cs_ndir++;
		fs->fs_cs(fs, cg).cs_ndir++;
	}
	FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);

	/*
	 * Start an asynchronous write of the cylinder group block
	 * and return.
	 */
	bdwrite(bp);
	*inop = (cg * fs->fs_ipg) + ipref;
	return (0);
}

/*
 * int
 * sfs_mapsearch(fs_t *fs, cg_t *cgp, daddr_t bpref, int allocsiz,
 *	      vfs_t *vfsp, daddr_t *bnop)
 *	Find a block of the specified size in the specified cylinder group.
 *
 * Calling/Exit State:
 *	Calling process has exclusive access to <cgp> (mutexing
 *	provided through buffer cache) and has determined that
 *	there it contains free blocks. If no free blocks could
 *	be found, the file system is sealed and -1 is returned.
 *
 * Description:
 *	A free block is returned to the caller. The free block is
 *	selected using the following policy:
 *		1) See if <bpref> is available, if so, allocate
 *		   it.
 *		2) Start searching for a free block from 
 *		   <cgp->cg_frotor>.
 *
 */
STATIC int
sfs_mapsearch(fs_t *fs, cg_t *cgp, daddr_t bpref, int allocsiz,
	      vfs_t *vfsp, daddr_t *bnop)
{
	daddr_t	bno;
	int	i;
	int	blk;
	int	len;
	int	loc;
	int	pos;
	int	field;
	int	start;
	int	subfield;

	/*
	 * Find the fragment by searching through the
	 * free block map for an appropriate bit pattern.
	 */
	if (bpref) {
		start = dtogd(fs, bpref) / NBBY;
	} else {
		start = cgp->cg_frotor / NBBY;
	}

	/*
	 * Search the free block map for the cylinder group, potentially
	 * in two passes. The first pass scans [cg_free[start], end-of-bitmap].
	 * The second scans [0, cg_free[start]).
	 */
	len = howmany(fs->fs_fpg, NBBY) - start;
	loc = sfs_scanc((unsigned)len, (u_char *)&cgp->cg_free[start],
	    (u_char *)sfs_fragtbl[fs->fs_frag],
	    (u_char)(1 << (allocsiz - 1 + (fs->fs_frag % NBBY))));
	if (loc == 0) {
		len = start + 1;
		start = 0;
		loc = sfs_scanc((unsigned)len, (u_char *)&cgp->cg_free[0],
		    (u_char *)sfs_fragtbl[fs->fs_frag],
		    (u_char)(1 << (allocsiz - 1 + (fs->fs_frag % NBBY))));
		if (loc == 0) {
			/*
			 *+ A free block could not be found in this
			 *+ cylinder group. However, the cylinder group
			 *+ information indicated that an block was 
			 *+ available. Corrective action: run fsck.
			 */
			cmn_err(CE_WARN, 
			    "UFS/SFS sfs_mapsearch: map corrupted, start = %d, len = %d, fs = %s\n",
			    start, len, fs->fs_fsmnt);
			sfs_fsinvalid(vfsp);
			return (EIO);
		}
	}
	bno = (start + len - loc) * NBBY;
	cgp->cg_frotor = bno;
	/*
	 * Found the byte in the map, sift
	 * through the bits to find the selected frag
	 */
	for (i = bno + NBBY; bno < i; bno += fs->fs_frag) {
		blk = blkmap(fs, cgp->cg_free, bno);
		blk <<= 1;
		field = sfs_around[allocsiz];
		subfield = sfs_inside[allocsiz];
		for (pos = 0; pos <= fs->fs_frag - allocsiz; pos++) {
			if ((blk & field) == subfield) {
				*bnop = bno + pos;
				return (0);
			}
			field <<= 1;
			subfield <<= 1;
		}
	}
	/*
	 *+ A cylinder group was expected to contain a free block, but
	 *+ no free blocks were found during a search of the cylinder
	 *+ group free list. This implies inconsistent data structures.
	 */
	cmn_err(CE_WARN, 
	    "UFS/SFS sfs_mapsearch: block not in map, bno = %d, fs = %s\n",
	    bno, fs->fs_fsmnt);
	sfs_fsinvalid(vfsp);
	return (EIO);
}

/*
 * int
 * sfs_badblock(fs_t *fs, daddr_t bn)
 *	Check that a specified block number is valid in
 *	the containing file system.
 *
 * Calling/Exit State:
 *	The file system is not locked on entry or exit.
 *	If the block number is not valid for the file system,
 *	1 is returned; otherwise 0 is.
 */
int
sfs_badblock(fs_t *fs, daddr_t bn)
{
	if ((unsigned)bn >= fs->fs_size) {
		/*
		 *+ A out-of-range block number was encountered while the
		 *+ kernel was doing a filesystem block allocation or freeing
		 *+ a filesystem block. This probably indicates a corrupted
		 *+ filesystem. Corrective action: run fsck on the filesystem.
		 */
		cmn_err(CE_WARN, "bad block %d, %s: bad block\n",
			bn, fs->fs_fsmnt);
		return (1);
	}
	return (0);
}


/*
 * void
 * sfs_fragacct(fs_t *fs, int fragmap, long fraglist[], int cnt)
 *	Update the frsum fields to reflect addition or deletion
 *	of some frags.
 *
 * Calling/Exit State:
 *	<fraglist> is part of a cylinder-group - caller provides
 *	mutexing on this (via buffer cache mechanism)
 */
void
sfs_fragacct(fs_t *fs, int fragmap, long fraglist[], int cnt)
{
	int inblk;
	int field;
	int subfield;
	int siz;
	int pos;

	inblk = (int)(sfs_fragtbl[fs->fs_frag][fragmap]) << 1;
	fragmap <<= 1;
	for (siz = 1; siz < fs->fs_frag; siz++) {
		if ((inblk & (1 << (siz + (fs->fs_frag % NBBY)))) == 0)
			continue;
		field    = sfs_around[siz];
		subfield = sfs_inside[siz];
		for (pos = siz; pos <= fs->fs_frag; pos++) {
			if ((fragmap & field) == subfield) {
				fraglist[siz] += cnt;
				pos           += siz;
				field        <<= siz;
				subfield     <<= siz;
			}
			field    <<= 1;
			subfield <<= 1;
		}
	}
	return;
}

/*
 * Block operations
 */

/*
 * int
 * sfs_isblock(fs_t *fs, u_char *cp, daddr_t bn)
 *	Determine whether a a block is available on a given
 *	file system.
 *
 * Calling/Exit State:
 *	The file system is not locked on entry or exit.
 *	The cylinder group's bitmap <cp> is owned exclusively
 *	by the caller.
 *	
 *	If 1 is returned then the specified block is
 *	available; 0 is returned if the block is not available.
 *	
 */
int
sfs_isblock(fs_t *fs, u_char *cp, daddr_t bn)
{
	unsigned char mask;

	switch ((int)fs->fs_frag) {
	case 8:
		return (cp[bn] == 0xff);
	case 4:
		mask = 0x0f << ((bn & 0x1) << 2);
		return ((cp[bn >> 1] & mask) == mask);
	case 2:
		mask = 0x03 << ((bn & 0x3) << 1);
		return ((cp[bn >> 2] & mask) == mask);
	case 1:
		mask = 0x01 << (bn & 0x7);
		return ((cp[bn >> 3] & mask) == mask);
	default:
		/*
		 *+ On attempting to allocate a fileystem block, the kernel
		 *+ discovered that the superblock contained an illegal
		 *+ value for the number of fragments in a block.
		 */
		cmn_err(CE_WARN,
		       "UFS/SFS sfs_isblock: fs=%s, invalid fragment size %d\n",
			fs->fs_fsmnt, fs->fs_frag);
		ASSERT((fs->fs_frag != 0) || (fs->fs_frag == 0));
		return (NULL);
	}
}

/*
 * void
 * sfs_clrblock(fs_t *fs, u_char *cp, daddr_t bn)
 *	Allocate a block in the containing file system
 *	by clearing the corresponding block in the cylinder
 *	group bitmap.
 *
 * Calling/Exit State:
 *	The file system isn't locked on entry or exit. The
 *	calling LWP has the cylinder group bitmap <cp> locked
 *	exclusive on entry and exit.
 *	
 */
void
sfs_clrblock(fs_t *fs, u_char *cp, daddr_t bn)
{

	switch ((int)fs->fs_frag) {
	case 8:
		cp[bn] = 0;
		break;
	case 4:
		cp[bn >> 1] &= ~(0x0f << ((bn & 0x1) << 2));
		break;
	case 2:
		cp[bn >> 2] &= ~(0x03 << ((bn & 0x3) << 1));
		break;
	case 1:
		cp[bn >> 3] &= ~(0x01 << (bn & 0x7));
		break;
	default:
		/*
		 *+ On attempting to clear a fileystem block, the kernel
		 *+ discovered that the superblock contained an illegal
		 *+ value for the number of fragments in a block.
		 */
		cmn_err(CE_WARN,
		      "UFS/SFS sfs_clrblock: fs=%s, invalid fragment size %d\n",
		      fs->fs_fsmnt, fs->fs_frag);
		ASSERT((fs->fs_frag != 0) || (fs->fs_frag == 0));
		break;
	}
	return;
}

/*
 * void
 * sfs_setblock(fs_t *fs, u_char *cp, daddr_t bn)
 *	Deallocate a block in a file system by setting
 *	the corresponding bit in the cylinder group bitmap.
 *
 * Calling/Exit State:
 *	The file system isn't locked on entry or exit. The
 *	calling LWP has the cylinder group bitmap locked 
 *	exclusively on entry and exit.
 */
void
sfs_setblock(fs_t *fs, u_char *cp, daddr_t bn)
{

	switch ((int)fs->fs_frag) {

	case 8:
		cp[bn] = 0xff;
		break;
	case 4:
		cp[bn >> 1] |= (0x0f << ((bn & 0x1) << 2));
		break;
	case 2:
		cp[bn >> 2] |= (0x03 << ((bn & 0x3) << 1));
		break;
	case 1:
		cp[bn >> 3] |= (0x01 << (bn & 0x7));
		break;
	default:
		/*
		 *+ On attempting to free a fileystem block, the kernel
		 *+ discovered that the superblock contained an illegal
		 *+ value for the number of fragments in a block.
		 */
		cmn_err(CE_WARN,
		   "UFS/SFS sfs_setblock: fs=%s, invalid fragment size %d\n",
		    fs->fs_fsmnt, fs->fs_frag);
		ASSERT((fs->fs_frag != 0) || (fs->fs_frag == 0));
		break;
	}
	return;
}

/*
 * int
 * sfs_scanc(u_int size, u_char *cp, u_char table[], u_char mask)
 *	Scan a cylinder group's free map for a block.
 *
 * Calling/Exit State:
 *	Caller provides mutexing to <cp> via buffer cache mechanism.
 */
int
sfs_scanc(u_int size, u_char *cp, u_char table[], u_char mask)
{
	u_char *end = &cp[size];

	while (cp < end && (table[*cp] & mask) == 0)
		cp++;

	return (end - cp);
}

/*
 * int
 * sfs_skpc(char c, u_int len, char *cp)
 *
 * Calling/Exit State:
 *	Caller provides mutexing to <cp> via buffer cache mechanism.
 */
sfs_skpc(char c, u_int len, char *cp)
{

	if (len == 0) {
		return (0);
	}

	while (*cp++ == c && --len)
		continue;

	return (len);
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_sfs_cg(const cg_t *cgp)
 *	Dumps the cylinder group struct.
 *
 * Calling/Exit State:
 *	No locking.
 */
void
print_sfs_cg(const cg_t *cgp)
{
	debug_printf("\trotor = %d, frotor = %d, irotor = %d, magic = %d\n",
					cgp->cg_rotor,
					cgp->cg_frotor,
					cgp->cg_irotor,
					cgp->cg_magic);
	debug_printf("\tfrsum = %d %d %d %d %d %d %d %d\n",
					cgp->cg_frsum[0],
					cgp->cg_frsum[1],
					cgp->cg_frsum[2],
					cgp->cg_frsum[3],
					cgp->cg_frsum[4],
					cgp->cg_frsum[5],
					cgp->cg_frsum[6],
					cgp->cg_frsum[7]);
	debug_printf("\tiused = 0x%x, cg_free = 0x%x\n", cgp->cg_iused[0],
					cgp->cg_free[0]);
}

#endif /* DEBUG || DEBUG_TOOLS */
