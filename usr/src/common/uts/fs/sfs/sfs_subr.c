/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/sfs/sfs_subr.c	1.37"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <acc/dac/acl.h>
#include <fs/buf.h>
#include <fs/sfs/sfs_data.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_hier.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_tables.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>

extern int	sfs_alloc(inode_t *, daddr_t, int, daddr_t *, cred_t *);
extern int	sfs_free(inode_t *, daddr_t, off_t);
extern void 	sfs_sbupdate(vfs_t *);
extern void	sfs_iupdat(inode_t *, enum iupmode);
extern void	sfs_iput(inode_t *, cred_t *);
extern int	sfs_igrab(inode_t *);
extern void	sfs_freeblocks_sync(vfs_t *, uint_t);
extern int dnlc_purge_vfsp(vfs_t *, int);
int sfs_syncip(inode_t *, int, enum iupmode);

vfsops_t *sfs_ufs_vfsopsp;

/*
 * void
 * sfs_fsinvalid(vfs_t *vfsp)
 *	Invalidate a file system because of a error.
 *
 * Calling/Exit State:
 *	The file system in <vfsp> is not locked on entry or exit.
 *	
 * Description:
 *	We lock the file system <vfsp> and assert the SFS_INVALID
 *	bit. Subsequent operations on the file system will notice
 *	SFS_INVALID being set and return an error. Eventually, access
 *	to the file system will cease and the errors to the file system 
 *	can be repaired.
 */
void
sfs_fsinvalid(vfs_t *vfsp)
	/* vfsp - mounted file system vfs structure to invalidate */
{
	fs_t		*fsp;
	sfs_vfs_t	*sfs_vfsp;
	pl_t		s;
	
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (vfsp == rootvfs) {
		/*
		 *+ During usage, a fatal error to the root file system
		 *+ occurred. This may be for a number of reasons including
		 *+ media failure, errors accessing the device containing
		 *+ the root file system, or kernel programming errors.
		 */
		cmn_err(CE_PANIC, "Root file system corrupt\n");
	}

	fsp = getfs(vfsp);
	s = LOCK(&vfsp->vfs_mutex, FS_VFSPPL);
	sfs_vfsp->vfs_flags &= ~SFS_DOWENABLED;	/* disable dow */
	sfs_vfsp->vfs_flags |= SFS_FSINVALID;
	UNLOCK(&vfsp->vfs_mutex, s);
	/*
	 *+ During usage, a fatal error to a file system
	 *+ occurred. This may be for a number of reasons including
	 *+ media failure, errors accessing the device containing
	 *+ the file system, or kernel programming errors.
	 */
	cmn_err(CE_WARN, "Invalidating corrupt file system %s. \n",
		fsp->fs_fsmnt);
	/*
	 *+ During usage, a fatal error to the root file system
	 *+ occurred. The file system should be unmounted and
	 *+ fixed via one of the administrative file system
	 *+ repair commands.
	 */
	cmn_err(CE_WARN, "Unmount immediately and fix. \n");

	return;
}

/*
 * void
 * sfs_flushi(int flag, cred_t *cr)
 *	Each modified inode is written to disk.
 *
 * Calling/Exit State:
 *	No relevant SFS locks are held on entry or exit.
 *
 * Description:
 *	We traverse the inode table looking for idle,
 *	modified inodes. If we find one, we try to
 *	grab it. If we do grab it, we flush the inode
 *	to disk (iff flag & SYNC_ATTR) or both the
 *	inode and it's pages to disk.
 */
/* ARGSUSED */
void
sfs_flushi(unsigned int flag, cred_t *cr)
{
	inode_t	*ip, *ipx;
	union ihead *ih;
	int	cheap;
	struct inode_marker *mp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Write back each (modified) inode.  Ignore busy inodes.
	 *
	 * For each inode, if it's modified and available (not locked),
	 * *then* lock the inode (sfs_igrab) and update it.  This avoids 
	 * locking for uninteresting inodes.  Losing a race is ok (we 
	 * don't modify anything), since this is heuristic update.
	 */

	cheap = flag & SYNC_ATTR;
	SFS_CREATE_MARKER(mp);

	for (ih = &sfs_ihead[0]; ih < &sfs_ihead[INOHSZ]; ih++) {
	    (void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
	    for (ip = (struct inode *)ih->ih_chain[0]; 
				ip != (struct inode *)ih; ip = ipx){
		ipx = ip->i_forw;
		if (ip->i_state == IMARKER) /* skip markers */
			continue;
		ASSERT(ITOV(ip)->v_op == &sfs_vnodeops ||
		       ITOV(ip)->v_op == &ufs_vnodeops);

		/*
		 * The following unlocked tests of vpages and i_flag are
		 * acceptable. If we read a stale value, and thus fail to
		 * sync, we will catch it the next time through.
		 */
		if (((ITOV(ip))->v_pages == NULL &&
		    (ip->i_flag & (IUPD|IACC|ICHG|IMOD)) == 0) || 
		    (ITOV(ip)->v_flag & VNOSYNC) ||	/* no-sync fs */
		    !SFS_IRWLOCK_IDLE(ip)) {		/* locked  */
			continue;				/* ==> ignore */
		}
		if (sfs_igrab(ip)) {				/* try for it */
			SFS_INSERT_MARKER(mp, ipx);
			UNLOCK(&sfs_inode_table_mutex, PLBASE);
			/*
			 * We have the inode locked exclusively at this
			 * point.
			 */
			if (cheap || ip->i_swapcnt > 0 || IS_STICKY(ip)) {
				sfs_iupdat(ip, IUP_DELAY);
			} else {
				(void) sfs_syncip(ip, B_ASYNC, IUP_DELAY);
			}

			SFS_IRWLOCK_UNLOCK(ip);
			VN_SOFTRELE(ITOV(ip));
		        (void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
			ipx = mp->im_forw;
			SFS_REMOVE_MARKER(mp);
		}
	    }
	    UNLOCK(&sfs_inode_table_mutex, PLBASE);
	}
	SFS_DESTROY_MARKER(mp);

	return;
}

/*
 * int
 * sfs_syncip(inode_t *ip, int flags, enum iupmode mode)
 *	Flush all the pages associated with an inode using the given flags,
 * 	then force inode information to be written back using the given flags.
 *
 * Calling/Exit State:
 *	The inode's rwlock lock must be held on entry and exit.
 */
/* ARGSUSED */
int
sfs_syncip(inode_t *ip, int flags, enum iupmode mode)
{
	int	error;
	vnode_t *vp = ITOV(ip);

	if (ip->i_fs == NULL) {
		return (0);			/* not active */
	}

	if (vp->v_pages == NULL || vp->v_type == VCHR) {
		error = 0;
	} else {
		error = VOP_PUTPAGE(vp, 0, 0, flags, sys_cred);
	}

	if (ip->i_flag & (IUPD | IACC | ICHG | IMOD)) {
			sfs_iupdat(ip, mode);
	}
	return (error);
}

/*
 * int
 * sfs_iremount(vfs_t *vfsp)
 *	This function checks the consistency of in-core inodes
 *	with disk inodes and updates the in-core version if
 *	necessary.
 *
 * Calling/Exit State:
 *	No specific locking is required. Usually the root vnode
 *	shared/exclusive lock is held exclusive as part of the
 *	locking requirement for "mount".	
 *
 * Description:
 * 	Assumptions:
 *	- remount only valid on a read-only fs
 *	- quotas not on for read-only fs
 *
 * 	Processing:
 *	- invalidate pages associated with inodes
 *	- invalidate inodes that are not in use
 *	- check inuse incore inodes with disk counterparts
 *
 * 	Return:
 *	- 0 on success
 *	- EINVAL if any active incore inode is not in sync with
 *		disk counterpart
 */
int
sfs_iremount(vfs_t *vfsp)
{
	inode_t *ip;
	inode_t *ipx;
	union	ihead *ih;
	vnode_t *vp;
	buf_t *bp;
	struct dinode *dp;
	fs_t *fs;
	dev_t dev;
	int error;
	struct inode_marker *mp;
	char tpath[SHORTSYMLINK + 1];
	boolean_t isshortlink = B_FALSE;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	
	/*
	 * File system is mounted read-only at present.
	 */
	fs = getfs(vfsp);
	ASSERT(fs->fs_ronly);

#ifdef QUOTA
	/*
	 * Therefore, quotas are not yet enabled, either.
	 */
	ASSERT((((struct sfs_vfs *)(vfsp->vfs_data))->vfs_qflags & MQ_ENABLED)
		== 0);
#endif

	/*
	 * Clear dnlc entries for this file system to minimize
	 * active inodes.
	 */
	dnlc_purge_vfsp(vfsp, 0);

	/*
	 * Invalidate buffer cache entries for this file system
	 * so that the disk inodes are read in for comparison
	 * with the incore copies (active inodes only).
	 */
	dev = vfsp->vfs_dev;
	binval(dev);

	SFS_CREATE_MARKER(mp);

	/*
	 * This search runs through the hash chains (rather
	 * than the entire inode table) so that we examine
	 * inodes that we know are currently valid.
	 */
	for (ih = &sfs_ihead[0]; ih < &sfs_ihead[INOHSZ]; ih++) {
		(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
		for (ip = (inode_t *)ih->ih_chain[0]; ip != (inode_t *)ih;
								ip = ipx) {
			ipx = ip->i_forw;
			if (ip->i_state == IMARKER) /* skip markers */
				continue;

			ASSERT(ITOV(ip)->v_op == &sfs_vnodeops ||
			       ITOV(ip)->v_op == &ufs_vnodeops);

			/*
			 * Process inodes on a particular fs only.
			 */
			if (ip->i_dev != dev || ip->i_fs == NULL)
				continue;

			vp = ITOV(ip);

			if (!sfs_igrab(ip)) {
				UNLOCK(&sfs_inode_table_mutex, PLBASE);
				/*
				 *+ During a remount, an active inode is locked.
				 *+ The remount will abend and the system will
				 *+ reboot automatically.
				 */
				cmn_err(CE_NOTE, "inode locked, ip = 0x%x, UFS/SFS sfs_iremount abandoned\n", ip);
				error = EINVAL;
				goto out1;
			}
			SFS_INSERT_MARKER(mp, ipx);
			UNLOCK(&sfs_inode_table_mutex, PLBASE);
			/*
			 * Invalidate pages.
			 *
			 * This unlocked test of v_pages is correct, since
			 * it is only necessary to invalidate any pages which
			 * were read in before the remount operation began.
			 */
			if (vp->v_pages) {
				error = VOP_PUTPAGE(vp, 0, 0, B_INVAL, sys_cred);
				if (error)
					goto out2;
			}
			/*
			 * Compare incore inode with disk inode.
			 * Don't continue if they are different.
			 * Make an exception for the access time.
			 */
			bp = bread(ip->i_dev, (daddr_t)fsbtodb(fs, itod(fs, ip->i_number)), (int)fs->fs_bsize);
			if (bp->b_flags & B_ERROR) {
				/*
				 *+ During a remount, failed to read an inode
				 *+ from disk.
				 *+ The remount will aband and the system wil
				 *+ reboot automatically.
				 */
				cmn_err(CE_NOTE, 
					"UFS/SFS sfs_iremount: cannot read disk inode %d\n",
					ip->i_number);
				error = EINVAL;
				goto out3;
			}
			dp = (struct dinode *)bp->b_addrp;
			dp += itoo(fs, ip->i_number);
			dp->di_atime = ip->i_atime;
			/*
			 * If this is a short symbolic link and the path
			 * is stashed in i_db, save the path and zero i_db
			 * before doing the comparison.
			 */
			if ((vp->v_type == VLNK) &&
			    (ip->i_size <= SHORTSYMLINK) &&
			    (ip->i_db[1] != 0)) {
				strncpy(tpath, (char *)&ip->i_db[1], ip->i_size);
				bzero((char *)&ip->i_db[1], ip->i_size);
				isshortlink = B_TRUE;
			}

			if (bcmp((caddr_t)&dp->di_ic, (caddr_t)&ip->i_ic,
				 sizeof(struct dinode))
			    || (!UFSVFSP(vfsp) && bcmp((caddr_t)&(dp+1)->di_ic,
				 (caddr_t)&ip->is_ic,
				 sizeof(struct dinode)))) {
				/*
				 *+ During a remount, an active in-core inode
				 *+ is out of sync with the disk inode.
				 *+ The remount will aband and the system wil
				 *+ reboot automatically.
				 */
				cmn_err(CE_NOTE, 
					"UFS/SFS sfs_iremount: incore and disk inodes are not in sync, fs = %s, inumber = %d\n",
					fs->fs_fsmnt, ip->i_number);
				error = EINVAL;
				goto out3;
			}
			brelse(bp);

			/* Restore symlink, if necessary. */
			if (isshortlink) {
				strncpy((char *)&ip->i_db[1], tpath, ip->i_size);
				isshortlink = B_FALSE;
			}
			SFS_IRWLOCK_UNLOCK(ip);
			VN_SOFTRELE(vp);
			(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
			ipx = mp->im_forw;
			SFS_REMOVE_MARKER(mp);
		}
		UNLOCK(&sfs_inode_table_mutex, PLBASE);
	}

	SFS_DESTROY_MARKER(mp);
	return 0;

out3:
	brelse(bp);
out2:
	(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
	SFS_REMOVE_MARKER(mp);
	UNLOCK(&sfs_inode_table_mutex, PLBASE);
	SFS_IRWLOCK_UNLOCK(ip);
	VN_SOFTRELE(vp);
out1:
	SFS_DESTROY_MARKER(mp);
	return (error);
}

/*
 * void
 * sfs_update(cred_t *cr)
 *	Performs the SFS component of 'sync'. 
 *
 * Calling/Exit State:
 *	Called from sync(). No SFS locks are held on entry or exit.
 *
 * Description:
 *	We go through the disk queues to initiate sandbagged I/O. It
 *	goes through the list of inodes and writes all modified ones.
 *	Modified superblocks of SFS file systems are written to disk.
 */
void
sfs_update(cred_t *cr)
{
	vfs_t *vfsp;
	sfs_vfs_t *sfs_vfsp;
	fs_t *fs;

	/*
	 * Avoid performing a sync if there's one in progress
	 */
	if (SLEEP_TRYLOCK(&sfs_updlock) != B_TRUE) {
		return;
	}

	/*
	 * Write back modified superblocks.
	 * Consistency check that the superblock of
	 * each file system is still in the buffer cache.
	 */
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	for (vfsp = rootvfs; vfsp != NULL; vfsp = vfsp->vfs_next) {
		if (vfsp->vfs_op == &sfs_vfsops ||
		    (sfs_ufs_vfsopsp && vfsp->vfs_op == sfs_ufs_vfsopsp)) {
			fs = getfs(vfsp);
			if (fs->fs_fmod == 0) {
				continue;
			}
			if (fs->fs_ronly != 0) {
				/*
				 *+ During an update operation, an UFS/SFS
				 *+ file system that was mounted read-only
				 *+ was found to be modified. This indicates
				 *+ a kernel programming error. 
				 */
				cmn_err(CE_WARN,
					"UFS/SFS sfs_update: fs = %s  read-only filesystem modified\n",
					fs->fs_fsmnt);
				sfs_fsinvalid(vfsp);
				continue;
			}
			sfs_freeblocks_sync(vfsp, 0);
			sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
			FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
			fs->fs_fmod = 0;
			fs->fs_time = hrestime.tv_sec;
			FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
			sfs_sbupdate(vfsp);
		}
	}

	SLEEP_UNLOCK(&vfslist_lock);
	SLEEP_UNLOCK(&sfs_updlock);

	sfs_flushi((unsigned int)0, cr);
	/*
	 * Force stale buffer cache information to be flushed,
	 * for all devices.  This should cause any remaining control
	 * information (e.g., cg and inode info) to be flushed back.
	 */
	bflush((dev_t)NODEV);
	return;
}

/*
 * int
 * sfs_aclget(inode_t *ip, struct acl *aclp, int defaults)
 *	Get the File's extended ACLs
 *
 * Calling/Exit State:
 *	pointer to an inode
 *	pointer to kernel buffer for storing default ACL
 *	defaults flag: if set, get default entries only
 */

int
sfs_aclget(inode_t *ip, struct acl *aclp, int defaults)
{
        struct aclhdr	*ahdrp;         /* ACL header ptr */
        uint		bsize;
        buf_t		*bp = NULL;
        fs_t		*fsp = ip->i_fs;
        struct acl	*src_aclp;      /* ptr to source ACL */
        daddr_t		aclblk;
        long		dentries;       /* default entries */
        long		aentries;       /* non-default entries */
        long		bentries;       /* entries in current buffer */
        long		b_aentries;     /* non-default entries in */
					/*    current buffer      */
        long		b_dentries = 0; /* default entries in */
					/*    current buffer  */
        long		entries;        /* total entries in ACL */
        uint		error = 0;

	ASSERT(!UFSIP(ip));
        entries = ip->i_aclcnt;
        dentries = ip->i_daclcnt;
        aentries = entries - dentries;          /* non-default entries */

        /* Set up to copy ACL entries from the secure inode first */
        bentries = (entries > NACLI) ? NACLI : entries;
        src_aclp = ip->i_acl;
        aclblk = ip->i_aclblk;

        /*
         * loop while default entries remain to be copied.
         * At entry to loop, we'll copy any inode based entries.
         * If any extended ACL blocks exist, read them and
         * copy them to temp buffer also.
         */
        while (entries > 0) {
                if (bentries > aentries) {      /* buffer has defaults */
                        b_dentries = bentries - aentries;
                        b_aentries = aentries;
                } else                          /* only non-defaults */
                        b_aentries = bentries;
                aentries -= b_aentries;
                dentries -= b_dentries;

                if (b_aentries > 0) {           /* non-defaults in buffer? */
			if (!defaults) {
				/* copy non-defaults */
				bcopy((caddr_t)src_aclp, (caddr_t)aclp,
					b_aentries * sizeof(struct acl));
				aclp += b_aentries;

			}
                        src_aclp += b_aentries; /* then skip past them */
		}

		/*
		 * If the defaults are in the buffer, copy them
		 */
                if (b_dentries > 0) {
                        bcopy((caddr_t)src_aclp, (caddr_t)aclp,
                              b_dentries * sizeof(struct acl));
                        src_aclp += b_dentries;
                        aclp     += b_dentries;
                }

                entries -= bentries;
                if (bp)                /* release last block if necessary */
                        brelse(bp);

                /* Read the entries from the ACL blocks  */
                if (aclblk) {
                        ASSERT(entries > 0);
                        bsize = fragroundup(fsp, entries * sizeof(struct acl) +
                                        sizeof (struct aclhdr));
                        if (bsize > fsp->fs_bsize)
                                bsize = fsp->fs_bsize;
                        bp = pbread(ip->i_dev, NSPF(fsp) * aclblk, bsize);
			if (bp->b_flags & B_ERROR) {
				error = bp->b_error ? bp->b_error : EIO;
                                brelse(bp);
                                return (error);
                        }

                        /* point at block header */
                        ahdrp = (struct aclhdr *)(bp->b_addrp);
                        /* point at actual ACL entries in the fragment */
                        src_aclp = (struct acl *)(ahdrp + 1);
                        bentries = ahdrp->a_size;
                        aclblk = ahdrp->a_nxtblk;
                }       /* end "if (aclblk)" */
        }       /* end "while(entries)" */

        return (error);
}

/*
 * int
 * sfs_aclstore(inode_t *ip, struct acl *aclp, int nentries,
 *           long dentries, cred_t *cr)
 *	Store a given ACL buffer on the file
 *
 * Calling/Exit State:
 *      Pointer to the file's inode
 *      Pointer to buffer of ACL entries
 *      Number of ACL entries to save umber of default ACL entries
 *
 * Description:
 *	This routine sets an ACL exactly from the given buffer, without
 *	the overhead of computing file modes, and determining whether all
 *	base entries should be stored with the ACL or not.
 */
int
sfs_aclstore(inode_t *ip, struct acl *aclp, int nentries,
             long dentries, cred_t *cr)
{
        struct aclhdr	*ahdrp;
        uint		bsize;
        vnode_t		*vp = ITOV(ip);
        vfs_t		*vfsp = vp->v_vfsp;
        sfs_vfs_t	*sfs_vfsp;
        buf_t		*bp = NULL;
        buf_t		*lbp = NULL;
        fs_t		*fsp = ip->i_fs;
        struct aclhdr	*lahdrp=NULL;
        daddr_t		aclblk = (daddr_t)0;
        daddr_t		laclblk=0;
        long		acls;           /* working count */
        uint		lbsize;
        uint		error = 0;

        sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	ASSERT(!UFSIP(ip));
        ip->i_aclblk = aclblk;

        /* Copy Entries into Secure Inode */
        ip->i_aclcnt  = nentries;
        ip->i_daclcnt = dentries;
        acls          = (nentries > NACLI) ? NACLI : nentries;
        nentries     -= acls;
        bcopy((caddr_t)aclp, (caddr_t)&ip->i_acl, acls * sizeof(struct acl));
        aclp         += acls;

        /* Allocate any necessary ACL blocks, one at a time */

        while (nentries) {
                bsize = fragroundup(fsp, (nentries * sizeof(struct acl)) +
                                        sizeof(struct aclhdr));
                if (bsize > fsp->fs_bsize)
                        bsize = fsp->fs_bsize;

                error = sfs_alloc(ip, (daddr_t)0, bsize, &aclblk, cr);
                if (error!= 0)
			/* no space on disk, deallocate all blocks */
                        goto dealloc;

                bp = pbread(ip->i_dev, NSPF(fsp) * aclblk, bsize);
		if (bp->b_flags & B_ERROR) {
                        error = bp->b_error ? bp->b_error : EIO;
                        goto dealloc;
		}

                /* point at ACL block header in correct fragment */
                ahdrp         = (struct aclhdr *)(bp->b_addrp);
                ahdrp->a_ino  = (ino_t)ip->i_number;
                acls          = (bsize - sizeof(struct aclhdr)) /
				 sizeof(struct acl);
                ahdrp->a_size = nentries > acls ? acls : nentries;

                /* Copy ACL Entries out to disk block buffer */
                bcopy((caddr_t)aclp, (caddr_t)ahdrp + sizeof(struct aclhdr),
                        ahdrp->a_size * sizeof(struct acl));

                nentries -= ahdrp->a_size;
                aclp     += ahdrp->a_size;
                /*
                 * ACL Block Copied Successfully. Set up correct
                 * back-chaining of ACL blocks for recovery purposes
                 */
                if (lbp) {
                        /* set previous blk's next block ptr to current block */
			lahdrp->a_nxtblk = aclblk;
                        /* set current blk's prev. block ptr to last block */
                        ahdrp->a_lstblk  = laclblk;
                        bwrite(lbp);
                } else  {
                        /*
                         * this is the first (and possibly only) ACL block
                         * make it's previous block ptr zero, and
                         * set inode ACL block ptr to block number.
                         */
                        ahdrp->a_lstblk = (daddr_t)0;
                        ip->i_aclblk    = aclblk;
                }
                lbp     = bp;
                lahdrp  = ahdrp;
                laclblk = aclblk;
                lbsize  = bsize;
        }       /* end "while (nentries)" */

        if (aclblk) {
                /* Write out last block (if it exists) */
                ahdrp->a_nxtblk = (daddr_t)0;
                bwrite(bp);
        }
        return (0);

dealloc:
	ip->i_aclcnt  = 0;
	ip->i_daclcnt = 0;
	ip->i_aclblk  = (daddr_t)0;

        /* release all buffers & deallocate all blocks if an error occurred */
        if (bp) {
                sfs_free(ip, aclblk, bsize);
                brelse(bp);
                aclblk = (daddr_t)0;
                /* if this is the only ACL block, nothing else to do */
       	}
        if (lbp) {
                if (lahdrp->a_lstblk != (daddr_t)0) {
                        /*
                         * if not the 1st ACL block, setup
                         * to free all remaining blocks
                         */
                        aclblk = lahdrp->a_lstblk;
                        /* get count of entries successfully stored in blks */
                        nentries = ip->i_aclcnt - nentries - NACLI -
                                        lahdrp->a_size;
                }
                sfs_free(ip, laclblk, lbsize);
                brelse(lbp);
        }

	if ((sfs_vfsp->vfs_flags & SFS_FSINVALID) != 0)
		return (error);

        while (aclblk) {
                ASSERT(nentries > 0);
                bsize = fragroundup(fsp, (nentries * sizeof(struct acl)) +
                                        sizeof(struct aclhdr));
                if (bsize > fsp->fs_bsize)
                        bsize = fsp->fs_bsize;
                bp = pbread(ip->i_dev, NSPF(fsp) * aclblk, bsize);
		if (bp->b_flags & B_ERROR) {
                        sfs_free(ip, aclblk, bsize);
                        brelse(bp);
                        return (error);
                }
                ahdrp     = (struct aclhdr *)(bp->b_addrp);
                laclblk   = ahdrp->a_lstblk;
                nentries -= ahdrp->a_size;
                sfs_free(ip, aclblk, bsize);
                brelse(bp);
		if ((sfs_vfsp->vfs_flags & SFS_FSINVALID) != 0)
			return (error);
                aclblk = laclblk;
        }
        return (error);
}
