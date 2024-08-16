/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/s5fs/s5inode.c	1.45"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/file.h>
#include <fs/s5fs/s5data.h>
#include <fs/s5fs/s5dir.h>
#include <fs/s5fs/s5filsys.h>
#include <fs/s5fs/s5hier.h>
#include <fs/s5fs/s5ino.h>
#include <fs/s5fs/s5inode.h>
#include <fs/s5fs/s5macros.h>
#include <fs/s5fs/s5param.h>
#include <fs/dnlc.h>
#include <fs/fs_subr.h>
#include <fs/mode.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/open.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/autotune.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/metrics.h>
#include <util/mod/moddefs.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

/*
 * inode hashing.
 */

#define ihash(X)	(&hinode[(int) (X) & (NHINO-1)])
struct	hinode	hinode[NHINO];	/* S5 inode hash table */

#ifdef DEBUG

struct s5_inostats {
	int s5_totalinodes;
	int s5_freeinodes;
} s5_inostats;

#endif

int s5_hash_timestamp = 0;

extern	void s5_ifree(inode_t *);
extern	void s5_blkfree(vfs_t *, daddr_t);
extern struct inode_marker s5_totally_free;
extern struct inode_marker s5_partially_free;
extern clock_t s5_scan_time;
extern sv_t	s5_inode_sv;
extern void   s5_release();

void	s5_iupdat(inode_t *);
void	s5_iput(inode_t *);
int 	s5_syncip(inode_t *, int, enum iupmode);
int 	s5_itrunc(inode_t *, uint_t);
int	s5_iremount(vfs_t *);
void	s5_remque(inode_t *);
void	s5_insque(inode_t *, struct hinode *);
int     s5_idestroy(inode_t *);
int     s5_igrab(inode_t *);

STATIC boolean_t s5_pfree_scan(void);
STATIC int s5_load(void);
STATIC int s5_unload(void);

int s5_fsflags = 0;     /* to initialize vswp->vsw_flags */

MOD_FS_WRAPPER(s5, s5_load, s5_unload, "Loadable s5 FS Type");

extern int s5_stickyhack;
extern struct vfsops s5_vfsops;

extern int s5_tflush;	/* flush time parameter */

extern struct tune_point S5NINODEcurve[];
extern int S5NINODEcurvesz;

/*
 * void 
 * s5_doinit()
 *	 Allocate and initialize inodes.
 * 
 * Calling/Exit State:
 *	Should only be called at system initialization time, i.e.,
 *	once per system lifetime. See s5init() for more.
 *
 * Description:
 *	The memory for the inode table is allocated here. It,
 *	and the hash lists, free lists, and each inode are
 *	initalized.
 */
void
s5_doinit()
{ 
	inode_t	*ip, *freefp;
	struct inode_marker *free_list;
	int	i;

	ninode = tune_calc(S5NINODEcurve, S5NINODEcurvesz);

	for (i = 0; i < NHINO; i++) {
		hinode[i].ih_forw = (inode_t *) &hinode[i];
		hinode[i].ih_back = (inode_t *) &hinode[i];
	}

	ip = (inode_t *)(&s5_totally_free);
	ip->i_freef = ip->i_freeb = ip;
	ip = (inode_t *)(&s5_partially_free);
	ip->i_freef = ip->i_freeb = ip;
	s5_scan_time = lbolt;

	free_list = &s5_totally_free;
	for (i = ninode; i != 0; i--) {
		ip = kmem_zalloc(sizeof(inode_t), KM_SLEEP);
		S5_INIT_INODE(ip, NULL, 0, 0);
		freefp = ((inode_t *)(free_list))->i_freef;  
		freefp->i_freeb = (ip);                     
		(ip)->i_freef = freefp;                    
		(ip)->i_freeb = (inode_t *)(free_list);   
		((inode_t *)(free_list))->i_freef = (ip);
		(ip)->i_state |= ITFREE;                
		(ip)->i_ftime = lbolt;	
	}
	SV_INIT(&s5_inode_sv);

	LOCK_INIT(&s5_inode_table_mutex, FS_S5LISTHIER, FS_S5LISTPL,
		  &s5_inode_table_lkinfo, KM_SLEEP);

	SLEEP_INIT(&s5_updlock, (uchar_t) 0, &s5_updlock_lkinfo, KM_SLEEP);

	MET_INODE_MAX(MET_S5, ninode);
	MET_INODE_CURRENT(MET_S5, ninode);
}


/*
 * STATIC int 
 * s5_load(void)
 *	Initialize the inode table, free list,
 *	and global s5 synchronization objects.
 * 
 * Calling/Exit State:
 *	No locks held on entry and exit.
 *
 * Description:
 *	Should be called when loading a s5 module. 
 *
 * Note : this functions is used when the s5 file system is loadable
 *	while s5_init is used when the s5 file system is static.
 */
STATIC int
s5_load(void)
{
	struct	vfssw	*vswp;

	vswp = vfs_getvfssw("s5");
	if (vswp == NULL) {
		/*
                 *+ s5 file system is not registered before
                 *+ attempting to load it.
                 */
                cmn_err(CE_NOTE, "!MOD: S5 is not registered.");
                return (EINVAL);
	}
	/* check tunable flushtime parameter */
	if (s5_tflush > v.v_autoup) {
		/*
		 *+ Invalid flush time parameter
		 */
		cmn_err(CE_NOTE, "S5FLUSH is invalid. It should be less than NAUTOUP");
                return (EINVAL);
	}
		
	s5_doinit();
	
	return(0);
}

/*
 * STATIC int 
 * s5_unload(void)
 *	Deallocate the inode table, inode table lock,
 *	and global s5 synchronization objects.
 * 
 * Calling/Exit State:
 *	No locks held on entry and exit.
 *
 * Description:
 *	Should be called when unloading a s5 module. 
 *
 */
STATIC int
s5_unload()
{
	struct inode	*ip;

	/*
	 * Free all inode storage.
	 */
	for (;;) {
		(void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);

		/*
		 * Since all s5 file systems have been unmounted at
		 * this time, all inodes have lost identity. Therefore,
		 * they are either on the free list, are SOFTHELD, or
		 * both.
		 */
		ASSERT(S5_LIST_ISEMPTY(&s5_partially_free));
		while ((ip = S5_LIST_HEAD(&s5_totally_free)) != NULL) {
			ASSERT(ip->i_state == ITFREE);
			ASSERT(ITOV(ip)->v_softcnt >= 1);
			if (ITOV(ip)->v_softcnt == 1) {
				S5_FREE_REMOVE(ip);
				S5_FREE_CHECK();
				kmem_free(ip, sizeof(inode_t));
				ninode--;
			}
		}

		/*
		 * If all inodes have been freed, then we are done. Else,
		 * other inodes may be out there some place, VN_SOFTHELD by
		 * some LWPs. Wait for them to free up as well.
		 */
		if (ninode == 0)
			break;
		SV_WAIT(&s5_inode_sv, PRINOD, &s5_inode_table_mutex);
	}
	UNLOCK(&s5_inode_table_mutex, PLBASE);
	LOCK_DEINIT(&s5_inode_table_mutex);
	SLEEP_DEINIT(&s5_updlock);
	return(0);
}

/*
 * inode_t *
 * s5_search_ilist(struct hinode *ih, ino_t ino, vfs_t *vfsp)
 *	Determine whether a specific inode belongs to a
 *	given hash bucket, i.e., has the inode been hashed
 *	already?
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode table lock.
 *
 *	On success, a pointer to an *unlocked* inode
 *	is returned. Since the calling LWP has the
 *	inode table lock, this is ok.
 *
 *	If NULL is returned, than a matching inode
 *	could not be found.
 *
 * Description:
 *	Simply search the hash bucket for a inode matching
 *	<vfsp->vfs_dev, ino>.
 */
inode_t *
s5_search_ilist(struct hinode *ih, ino_t ino, vfs_t *vfsp)
{
	inode_t *ip;

	ASSERT(LOCK_OWNED(&s5_inode_table_mutex));

	for (ip = ih->ih_forw; ip != (inode_t *)ih; ip = ip->i_forw) {
		S5_ASSERT(ip, (ip->i_state & IDENTITY) ||
				ip->i_state == IMARKER);
		/*
		 * Test for matching identity.
		 *
		 * N.B. Test of i_fs must come first due to the implicit
		 *      union with ``markers''. Markers do not have
		 *      i_number and i_dev fields.
		 */
		if (ip->i_fs != NULL &&
			ino == ip->i_number && vfsp == ITOV(ip)->v_vfsp)
			return (ip);
	}

	/*
	 * Didn't find it...
	 */
	return (NULL);
}

/*
 * int
 * s5_getfree_inode(inode_t **ipp, struct hinode *ih, ino_t ino, filsys_t *fs,
 *		    vfs_t *vfsp)
 *	Remove an inode from the free list (if there are
 *	any available) for re-use. May cause dnlc purges.
 *
 * Calling/Exit State:
 *	Calling LWP holds the inode table locked. The inode table lock may
 *	be released and reobtained in this routine if the inode free list
 *	is initially empty and the calling LWP tries to free some inodes by
 *	purging the dnlc of some entries.
 *
 *	The inode table lock is held when 0 is returned and <*ipp>
 *	is non-NULL. In all other cases, the inode table lock is
 *	not held on exit.
 *
 *	If 0 is returned, then the inode removed from the free list is
 *	placed in <*ipp>. The rwlock of <*ipp> is held exclusive in this
 *	case.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routin
 *		ENFILE  All available s5 inodes are in use.
 *			<*ipp> is NULL in this case.
 *
 * Description:
 *	Try to take an inode from the free list. If the free list
 *	is empty then purge some dnlc entries from the file system
 *	that the inode will live on. Return ENFILE if there aren't
 *	any inodes available.
 *
 *	If there is an available inode, then remove it from the free list
 *	and insert it into the proper place on the hash list. Clean any
 *	pages associated with the inode.
 *
 *	Must be careful to re-check the hash list for the requested inode
 *	if we release the lists lock since another LWP could have entered
 *	the inode in the list.
 */
/* ARGSUSED */
int
s5_getfree_inode(inode_t **ipp, struct hinode *ih, ino_t ino, filsys_t *fs,
		 vfs_t *vfsp)
{
	inode_t	*ip;
	inode_t	*dupip = NULL;
	vnode_t	*vp;
	int 	count = 0;
	int 	softcnt;
	int 	save_timestamp;
	int	error;

	ASSERT(KS_HOLD1LOCK());
        ASSERT(LOCK_OWNED(&s5_inode_table_mutex));
        ASSERT(getpl() == FS_S5LISTPL);

	/*
         * Save the hash timestamp before dropping the inode table lock.
         */
        save_timestamp = s5_hash_timestamp;
	*ipp = NULL;
	error = 0;

	/*
         * First, try to grab a inode off the totally free list.
         *
         * Second, try to shake some inodes off the partially free list and
         * try to grab one.
 	 * Thirt, try to shake some inodes off the partially free list and
	 * try to grab one which has aged sufficiently long.
         */
again:

	count++;
	if (!S5_LIST_ISEMPTY(&s5_totally_free) || S5_PFREE_SCAN()) {
                ASSERT(!S5_LIST_ISEMPTY(&s5_totally_free));
                ip = S5_LIST_HEAD(&s5_totally_free);
                ASSERT(ip->i_state == (IDENTITY|ITFREE) ||
                       ip->i_state == ITFREE);
                if (ip->i_state == ITFREE) {
                        S5_FREE_REMOVE(ip);
                        ip->i_state = (INVALID|ILCLFREE);
                        S5_FREE_CHECK();
                        MET_INODE_INUSE(MET_S5, 1);
                        goto i_initialize;
                }
		
		/*
                 * Be careful about not looping too long on an
                 * inode whose identity cannot be destroyed.
                 */
                if (count < 5) {
                        MET_INODE_INUSE(MET_S5, 1);
                        goto i_destroy;
                }
        }
	/*
	 * Take from the head of the partially free list if pages
	 * have ``ages'' sufficiently long.
	 */
	if ((ip = S5_LIST_HEAD(&s5_partially_free)) != NULL) {
		goto i_destroy;
	}


	/* Purge some DNLC entries for this mounted file system
	 * in an attempt to reclaim some inodes.
	 * dnlc_purge_vfsp might go to sleep. So we have to check the
	 * hash chain again to handle possible race condition.
	 */
	UNLOCK(&s5_inode_table_mutex, PLBASE);
	(void)dnlc_purge_vfsp(vfsp, 10);
	(void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
	/*
	 * The s5_inode_table_mutex was dropped, so recheck the totally
	 * free list.
	 */
	if ((ip = S5_LIST_HEAD(&s5_totally_free)) != NULL) {
		ASSERT(ip->i_state == (IDENTITY|ITFREE) ||
			ip->i_state == ITFREE);
		if (ip->i_state == ITFREE) {
			S5_FREE_REMOVE(ip);
			ip->i_state = (INVALID|ILCLFREE);
			S5_FREE_CHECK();
			MET_INODE_INUSE(MET_S5, 1);
			goto i_initialize;
		}
		MET_INODE_INUSE(MET_S5, 1);
		goto i_destroy;
	}
	/*
	 * Try to remove an inode from the partially free list.
	 */
	if (S5_LIST_ISEMPTY(&s5_partially_free)) {
		UNLOCK(&s5_inode_table_mutex, PLBASE);
		return (ENFILE);
	}

	ip = S5_LIST_HEAD(&s5_partially_free);
        ASSERT(ip->i_state == (IDENTITY|IPFREE));

i_destroy:
	/*
	 * Remove the inode from the free list.
	 */
	S5_FREE_REMOVE(ip);
	ASSERT(ip->i_state == IDENTITY);
	ip->i_state = (IDENTITY|INVALID);
        S5_FREE_CHECK();
        UNLOCK(&s5_inode_table_mutex, PLBASE);

	/*
         * Gather more inode metrics (these are per-processor so
         * we don't need the lock). v_pages doesn't need locking,
         * as metrics don't need to be exact.
         */
	if (ip->i_mode != 0 && ITOV(ip)->v_pages != NULL) {
		MET_INO_W_PG(MET_S5, 1);
	} else {
		MET_INO_WO_PG(MET_S5, 1);
	}


	/*
	 * The inode has an old identity which must now be destroyed.
	 */
	S5_IRWLOCK_WRLOCK(ip);
	error = s5_idestroy(ip);
	if (error) {
		if (++count > 5) /* too many tries */
			return (ENFILE);
		(void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
		goto again;
	}

	/*
	 * Wait for all SOFTHOLDs to disappear on the old identity.
	 */
	vp = ITOV(ip);
	for (;;) {
		(void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
		VN_LOCK(vp);
		softcnt = vp->v_softcnt;
		VN_UNLOCK(vp);
		if (softcnt == 0)
			break;
		SV_WAIT(&s5_inode_sv, PRINOD, &s5_inode_table_mutex);
	}
	ip->i_state = INVALID|ILCLFREE;
	vp->v_softcnt = 1; /* no locking needed, vp is privately held */

i_initialize:
	/*
	 * Make sure that someone else didn't enter the inode into
	 * the list while we dropped the table lock. If they
	 * did, then, must release our inode (<ip>) and start over.
	 */
	ASSERT(ip->i_state == (INVALID|ILCLFREE));
	vp = ITOV(ip);
	if (save_timestamp != s5_hash_timestamp) {
		dupip = s5_search_ilist(ih, ino, vfsp);
	}
	if (dupip != NULL) {
		UNLOCK(&s5_inode_table_mutex, PLBASE);
		/* no locking needed, vp is privately held */
                vp->v_softcnt = 0;
		s5_release(vp);
		return (0);
	}

	/*
         * Clear any flags that may have been set.
         */
        ip->i_flag = 0;
	vp->v_flag = 0;
	vp->v_macflag = 0;

	ASSERT(vp->v_count == 0);
	ASSERT(vp->v_pages == NULL);
	ASSERT(vp->v_softcnt == 1);

	/*
	 * Put the inode on the chain for its new (ino, dev) pair and
	 * destroy the mapping if any. 
	 */
	s5_insque(ip, ih);
	s5_hash_timestamp++;

	ip->i_fs = fs;
	ip->i_number = (o_ino_t)ino;
	UNLOCK(&s5_inode_table_mutex, PLBASE);
	
	/*
         * The inode is now visible to s5_iget via the table.
         * However, it is marked INVALID, so that any potential
         * LWP looking for this inode will block in s5_iget.
         */

	ip->i_dev = vfsp->vfs_dev;
	ip->i_nextr = 0;
	ip->i_vcode = 0;
	ip->i_mapcnt = 0;
	ip->i_swapcnt = 0;

	/*
	 * Return the inode write locked.
	 */
	ASSERT(S5_IRWLOCK_IDLE(ip));
	S5_IRWLOCK_WRLOCK(ip);
	*ipp = ip;

	return (error);
}

/*
 * int
 * s5_init_inode(inode_t *ip, vfs_t *vfsp, ino_t ino, lkmode)
 *	Initialize an inode from it's on-disk copy to complete
 *	the initialization of an inode.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock *exclusive* on
 *	entry and exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. There are no errnos returned directly
 *	by this routine.
 *
 * Description:
 *	The inode is initialized from the on-disk copy. If there's
 *	an error, the inode number (ip->i_number) is set to 0
 *	so any LWPs blocked on the inode do not use it (see s5_iget).
 */
int
s5_init_inode(inode_t *ip, vfs_t *vfsp, ino_t ino, enum igmode lkmode)
{
	int		i;
	int		error;
	buf_t		*bp;
	dinode_t	*dp;
	s5_fs_t	*s5fsp;
	vnode_t		*vp = ITOV(ip);
	char		*p1;
	char		*p2;

	ASSERT(ip->i_state == (IDENTITY|INVALID|ILCLFREE));
	ASSERT(vp->v_count == 0);
	ASSERT(vp->v_softcnt == 1);
	ASSERT(vp->v_pages == NULL);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(!S5_ITRYRDLOCK(ip));

	s5fsp = S5FS(vfsp);
	bp = bread(ip->i_dev, LTOPBLK(FsITOD(s5fsp, ino), vfsp->vfs_bsize),
			 vfsp->vfs_bsize);
	/*
	 * Check I/O errors and get vcode
	 */
	error = (bp->b_flags & B_ERROR) ? EIO : 0;
	if (error != 0) {
		goto error_exit;
	}

	dp = (struct dinode *)bp->b_addrp;
	dp += FsITOO(s5fsp, ino);
	ip->i_nlink = dp->di_nlink;
	ip->i_uid = dp->di_uid;
	ip->i_gid = dp->di_gid;
	ip->i_size = dp->di_size;
	ip->i_mode = dp->di_mode;
	ip->i_atime = dp->di_atime;
	ip->i_mtime = dp->di_mtime;
	ip->i_ctime = dp->di_ctime;
	ip->i_number = (o_ino_t)ino;
	ip->i_gen = dp->di_gen;

	if (lkmode == IG_NCREATE && ip->i_nlink == 0) {
		error = ENOENT;
		goto error_exit;
	}
	p1 = (char *) ip->i_addr;
	p2 = (char *) dp->di_addr;
	for (i = 0; i < NADDR; i++) {
		l3tolone(p1, p2);
	}

	if (ip->i_mode & IFBLK || ip->i_mode == IFCHR) {
		if (ip->i_bcflag & NDEVFORMAT)
			ip->i_rdev = makedevice(ip->i_major, ip->i_minor);
		else
			ip->i_rdev = expdev(ip->i_oldrdev);
	} else if (ip->i_mode & IFNAM)
		ip->i_rdev = ip->i_oldrdev;

	vp = ITOV(ip);
	if (IFTOVT((uint)ip->i_mode) == VREG) {
		error = fs_vcode(vp, &ip->i_vcode);
		if (error != 0) {
			goto error_exit;
		}
	}

	brelse(bp);
	/*
	 * Fill in the rest.
	 */
	vp->v_lid = (lid_t)0;
	vp->v_vfsmountedhere = NULL;
	vp->v_op = &s5vnodeops;
	vp->v_vfsp = vfsp;
	vp->v_stream = NULL;
	vp->v_data = (caddr_t)ip;
	vp->v_filocks = NULL;
	vp->v_type = IFTOVT((uint)ip->i_mode);
	vp->v_rdev = ip->i_rdev;

	/*
	 * Now, we wish to make the inode visible to the rest of the world.
	 * Remember to wakeup any potential waiters.
	 *
	 * As soon as we release the VN_LOCK, the vnode can be grabbed
	 * by the VN_TRY_HOLD macro. Therefore, initialization must be
	 * complete at this time.
	 */
	(void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
	ip->i_state = IDENTITY;
	VN_LOCK(vp);
	ASSERT(vp->v_count == 0);
	ASSERT(vp->v_softcnt == 1);
	vp->v_count = 1;
	vp->v_softcnt = 0;
	VN_UNLOCK(vp);
	UNLOCK(&s5_inode_table_mutex, PLBASE);
	if (SV_BLKD(&s5_inode_sv))
		SV_BROADCAST(&s5_inode_sv, 0);

	return (0);


error_exit:
	brelse(bp);
	/*
	 * Initialization failed. Destroy the inode identity. 
	 */
	(void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
	s5_remque(ip);
	UNLOCK(&s5_inode_table_mutex, PLBASE);
	S5_IRWLOCK_UNLOCK(ip);

	/*
	 * At this point, we hold a soft count on the vnode,
	 * but not a hard count. All we need to do is release our
	 * soft count, and the vnode will go away.
	 */
	VN_SOFTRELE(vp);

	return (error);
}

/*
 * int
 * s5_iget(vfs_t *vfsp, s5_fs_t *fs, int ino, enum igmode lkmode, inode_t **ipp)
 * 	Look up an inode by vfs and i-number.
 * 
 * Calling/Exit State:
 *	On success 0 is returned, <*ipp> is returned locked according to
 *	<excl>. If <excl> is non-zero, it's held exclusive; otherwise,
 *	the lock is held shared.
 *
 *	On failure, an errno value is returned.
 *
 * Description:
 * 	Search for the inode on the hash list. If it's in core, honor
 * 	the locking protocol. If it's not in core, read it in from the
 * 	associated device. In all cases, a pointer to a locked inode
 *	structure is returned.
 */
int
s5_iget(vfs_t *vfsp, filsys_t *fs, int ino, enum igmode lkmode, inode_t **ipp)
{
	inode_t		*ip;
	struct hinode	*hip;
	vnode_t		*vp;
	int		error;

        ASSERT(getfs(vfsp) == fs);
        ASSERT(KS_HOLD0LOCKS());
        ASSERT(getpl() == PLBASE);

	MET_IGET(MET_S5);
	*ipp = NULL;
	hip = ihash(ino);
loop:
	(void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
	ip = s5_search_ilist(hip, ino, vfsp);
	if (ip != NULL) {
		/*
		 * If some LWP is creating or destroying the identity of
		 * this inode, then wait till it is finished.
		 */
		ASSERT(ip->i_state & IDENTITY);
		if (ip->i_state & INVALID) {
			SV_WAIT(&s5_inode_sv, PRINOD, &s5_inode_table_mutex);
			goto loop;
		}
		/*
		 * Remove the inode from the free list (if necessary).
		 */
		vp = ITOV(ip);
		switch(ip->i_state) {
		case IDENTITY|ITFREE:
			MET_INODE_INUSE(MET_S5, 1);
			/* FALLTHROUGH */
		case IDENTITY|IPFREE:
			S5_FREE_REMOVE(ip);
			VN_LOCK(vp);
			++vp->v_count;
			--vp->v_softcnt;
			VN_UNLOCK(vp);
			break;
		default:
			ASSERT(ip->i_state == IDENTITY);
			VN_HOLD(vp);
		}
		S5_FREE_CHECK();
		UNLOCK(&s5_inode_table_mutex, PLBASE);

		switch (lkmode) {
                case IG_SHARE:
                        S5_IRWLOCK_RDLOCK(ip);
                        break;
                case IG_EXCL:
                        S5_IRWLOCK_WRLOCK(ip);
                        break;
                default:
                        ASSERT(lkmode == IG_NONE || lkmode == IG_NCREATE);
                        break;
                }
		*ipp = ip;

		return (0);
	}

	/*
	 * Take a free inode and re-assign it to our inode.
	 *
	 * We hold the inode table lock at this point.
	 * s5_getfree_inode() drops the lock in all cases.
	 *
	 * When s5_getfree_inode returns an inode, the new inode has been
	 * added to the table, is no longer free, and has both the INVALID
	 * and IDENTITY bits set.
	 */
	error = s5_getfree_inode(&ip, hip, ino, fs, vfsp);
	ASSERT(KS_HOLD0LOCKS());
        ASSERT(getpl() == PLBASE);
	if (error != 0) {
		if (error == ENFILE) {
			MET_INODE_FAIL(MET_S5);
			/*
			 *+ The inode table has filled up.
			 *+ Corrective action: reconfigure
			 *+ the kernel to increase the inode
			 *+ table size.
			 */
			cmn_err(CE_WARN,
				"s5_iget: inode table overflow");
		}
		return (error);
	}

	if (ip == NULL) {
		goto loop;
	}

	error = s5_init_inode(ip, vfsp, ino, lkmode);
	if (error)
		return(error);

	/*
	 * The inode is always returned write locked by s5_getfree_inode.
	 * Therefore, we might need to downgrade the lock, depending upon
	 * the requirements of the caller.
	 */
	switch(lkmode){
	case IG_SHARE:
		S5_IRWLOCK_UNLOCK(ip);
		S5_IRWLOCK_RDLOCK(ip);
		break;
	case IG_EXCL:
		break;
	case IG_NCREATE:
	default:
		ASSERT(lkmode == IG_NONE || lkmode == IG_NCREATE);
		S5_IRWLOCK_UNLOCK(ip);
		break;
	}

	ASSERT(ip->i_number == ino);
	*ipp = ip;
	return (0);
}
	

/*
 * void
 * s5_iput(inode_t *ip)
 *	Unlock inode and vrele associated vnode
 *
 * Calling/Exit State:
 *	The caller must hold the inode's rwlock in *exclusive* mode
 *	on entry. On exit, the calling LWP's reference to the inode
 *	is removed and the caller may not reference the inode anymore.
 */
void
s5_iput(inode_t *ip)
{
	vnode_t *vp;
	boolean_t       totally_free;
	struct inode_marker *free_list;
	int	flags;

	vp = ITOV(ip);
	ASSERT(!S5_IRWLOCK_IDLE(ip));

	/*
	 * At this point, a new reference to the inode can be still be
	 * generated by one of:
	 * 
	 *	=> VN_TRY_HOLD
	 *	=> s5_iget
	 *
	 * In all two cases, the LWP generating the reference holds
	 * either the VN_LOCK or the s5_inode_table_mutex. So, acquire both
	 * locks now. We also acquire the s5_inode_table_mutex so that we
	 * can transition the free/destroy state of the inode atomically with
	 * changing its v_count/v_softcnt fields.
	 */
	(void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
	ASSERT(ip->i_state == IDENTITY);
	VN_LOCK(vp);
	ASSERT(vp->v_count != 0);
	if (vp->v_count != 1) {
		/*
                 * Someone generated a new reference before we acquired
                 * both locks.
                 *
                 * The new reference is still extant, so that inactivating
                 * the vnode will become someone else's responsibility. Give
                 * up our reference and return.
                 */

		vp->v_count--;
		VN_UNLOCK(vp);
		UNLOCK(&s5_inode_table_mutex, PLBASE);
		S5_IRWLOCK_UNLOCK(ip);
		return;
	} 
	/*
	 * The reference count is exactly 1, so that now we can be sure
	 * that we really hold the last hard reference.
	 *
	 * We exchange our hard count (v_count) for a soft count (v_softcnt)
	 * in order to suppress any new references via VN_TRY_HOLD once we
	 * give up the VN_LOCK. Even after we give up the VN_LOCK, we will
	 * still hold the s5_inode_table_mutex, thus inhibiting any
	 * new VN_HOLDs via s5_iget.
	 *
	 * Note: the pages remain visible to the pageout and fsflush daemons.
	 */
	vp->v_count = 0;
	++vp->v_softcnt;
	VN_UNLOCK(vp);

	/*
	 * If we are removing the file, or if we wish to kmem_free the
	 * storage (because we are over the low water mark), then we
	 * commit to destroying the inode's identity.
	 *
	 * Otherwise, we just free the inode and let the fsflush/pageout
	 * daemons clean the pages/inode.
	 */
	totally_free = (vp->v_softcnt == 1 && vp->v_pages == NULL);
	if (ip->i_nlink <= 0) {
		ip->i_state = (IDENTITY|INVALID|ILCLFREE);
	} else {
		/*
		 * Freeing the inode but not destroying it.
		 */
		if (totally_free) {
			free_list = &s5_totally_free;
			flags = ITFREE;
			MET_INODE_INUSE(MET_S5,-1);
		} else {
			free_list = &s5_partially_free;
			flags = IPFREE;
		}
		S5_FREE_TAIL(free_list, ip, flags);
		ASSERT(ip->i_state == (IDENTITY|flags));
		S5_FREE_CHECK();
		UNLOCK(&s5_inode_table_mutex, PLBASE);
		S5_IRWLOCK_UNLOCK(ip);
		return;
	}
	UNLOCK(&s5_inode_table_mutex, PLBASE);

	/*
	 * At this point, we are committed to destroying the
	 * inode's identity. If destruction fails, then the inode
	 * will go onto the free list. No error handling is needed
	 * here.
	 */
	(void) s5_idestroy(ip);
}

/*
 * int
 * s5_icmp(inode_t *ip, dinode_t *dp)
 *	This function compares incore inode to disk inode.
 *
 * Calling/Exit State:
 *	No specific locking is required.
 *
 * Description:
 *	Compare incore inode to disk inode. Return 1 if not in sync,
 *	and 0 otherwise.
 */
STATIC int
s5_icmp(inode_t *ip, dinode_t *dp)
{
	char *p1, *p2;
	int i;

	if ( ip->i_nlink != dp->di_nlink || ip->i_uid != dp->di_uid ||
		ip->i_gid != dp->di_gid || ip->i_size != dp->di_size ||
		ip->i_mode != dp->di_mode || ip->i_atime != dp->di_atime || 
		ip->i_mtime != dp->di_mtime || ip->i_ctime != dp->di_ctime ||
		ip->i_gen != dp->di_gen )

			return 1;
	
	p1 = (char *) ip->i_addr;
	p2 = (char *) dp->di_addr;

	for (i = 0; i < NADDR; i++) {
		if ( *p1++ != *p2++ || *p1++ != *p2++ || *p1++ != *p2++ )
			return 1;
		p1++;
	}
	return 0;
}

/*
 * int
 * s5_iremount(vfs_t *vfsp)
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
s5_iremount(vfs_t *vfsp)
{
	inode_t *ip, *ipx;
	vnode_t *vp;
	buf_t *bp;
	struct dinode *dp;
	s5_fs_t *s5vfsp = S5FS(vfsp);
	dev_t dev;
	int error, i;
	int bsize = vfsp->vfs_bsize;
	struct inode_marker *mp;
	struct hinode   *ih;

	ASSERT(KS_HOLD0LOCKS());
        ASSERT(getpl() == PLBASE);

	/*
         * File system is mounted read-only at present.
         */
        ASSERT(getfs(vfsp)->s_ronly);

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
	
	S5_CREATE_MARKER(mp);
	/*
	 * This search runs through the hash chains (rather
	 * than the entire inode table) so that we examine
	 * inodes that we know are currently valid.
	 * Note that: this code is a bit too trusting of the user than it
	 *	 should be, since the user could exchange a hold in
	 *	 front of the scan for one behind it. However, since
	 *	 this operation only occurs at boot time, when no real
	 *	 users are running, we let the flaw pass for now. This should
	 *	 be fixed together with s5_iflush (which has the opposite
	 *	 problem of holding the table lock for too long).
	 */
	(void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
	for (i = 0; i < NHINO; i++) {
		ih = &hinode[i];
		for (ip = ih->ih_forw; ip != (inode_t *)ih; ip = ip->i_forw) {

			ipx = ip->i_forw;
			vp = ITOV(ip);

			ASSERT(vp->v_op == &s5vnodeops);
			if (ip->i_state == IMARKER)	/* skip marker */
				continue;
			/*
		 	* Process inodes on a particular fs only.
		 	*/
			if (ip->i_dev != dev || vp->v_vfsp == NULL || ip->i_fs == NULL)
				continue;
	
			if (!s5_igrab(ip)) {
				UNLOCK(&s5_inode_table_mutex, PLBASE);
				/*
                         	*+ During a remount, an active inode is locked.
                         	*+ The remount will abend and the system will
                         	*+ reboot automatically.
                         	*/
				cmn_err(CE_NOTE, "inode locked, ip = 0x%x, s5_iremount abandoned\n", ip);
				error = EINVAL;
				goto out1;
			}
			S5_INSERT_MARKER(mp, ipx);
			UNLOCK(&s5_inode_table_mutex, PLBASE);
			/*
			 * Invalidate pages.
			 * This unlocked test of v_pages is correct, since
       		          * it is only necessary to invalidate any pages which
       		          * were read in before the remount operation began.
			 */
			if (vp->v_pages) {
				error = VOP_PUTPAGE(vp, 0, 0, B_INVAL, sys_cred);
				if (error) {
					goto out2;
				}
			}
			/*
			 * Compare incore inode with disk inode.
			 * Don't continue if they are different.
			 * Make an exception for the access time.
			  */
			bp = bread(ip->i_dev, LTOPBLK(FsITOD(s5vfsp,
				ip->i_number), bsize), bsize);
			if (bp->b_flags & B_ERROR) {
				/*
				 *+ During a remount, failed to read an inode
			  	 *+ from disk.
				 *+ The remount will aband and the system wil
				 *+ reboot automatically.
				 */
				cmn_err(CE_NOTE,
				"s5_iremount: cannot read disk inode %d\n",
								ip->i_number);
				error = EINVAL;
				goto out2;
			}
			dp = (struct dinode *)bp->b_addrp;
			dp += FsITOO(s5vfsp, ip->i_number);
			dp->di_atime = ip->i_atime;
			if (s5_icmp(ip, dp)) {
				/*
				 *+ During a remount, an active in-core inode
				 *+ is out of sync with the disk inode.
				 *+ The remount will aband and the system wil
				 *+ reboot automatically.
				 */
				cmn_err(CE_NOTE, "s5iremount: incore and disk copies of inode %d are not in sync\n", ip->i_number);
				error = EINVAL;
				goto out3;
			}
			brelse(bp);
	
			S5_IRWLOCK_UNLOCK(ip);
			VN_SOFTRELE(vp);
			(void)LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
			ipx = mp->im_forw;
			S5_REMOVE_MARKER(mp);
		}
	}

	UNLOCK(&s5_inode_table_mutex, PLBASE);
	S5_DESTROY_MARKER(mp);
	return 0;
out3:
	brelse(bp);
out2:
	(void)LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
	S5_REMOVE_MARKER(mp);
	UNLOCK(&s5_inode_table_mutex, PLBASE);
	S5_IRWLOCK_UNLOCK(ip);
	VN_SOFTRELE(vp);
out1:
	S5_DESTROY_MARKER(mp);
	return (error);
}


/* 
 * int
 * s5_iflush(vfs_t *vfsp, int force)
 *	Remove any inodes in the inode cache belonging to dev
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode hash list locked.
 *
 * Description:
 *	There should not be any active ones, return error if any are found but
 *	still invalidate others (N.B.: this is a user error, not a system
 *	error).
 *
 * Remarks:
 *	This is called from umount()/s5vfsops.c when dev is being
 *	unmounted and from mountfs() when a REMOUNT occurs.
 *
 *	Note the code must be modified to clean pages
 */
/* ARGSUSED */
int
s5_iflush(vfs_t *vfsp, int force)
{
	inode_t	*ip = NULL, *ipx;
	struct hinode	*ih;
	vnode_t	*vp;
	vnode_t	*rvp;
	int	i, open = 0;
	dev_t	dev;
	boolean_t	just_sync;
	boolean_t	lock_dropped;
	struct inode_marker *mp;

	ASSERT(KS_HOLD0LOCKS());
        ASSERT(getpl() == PLBASE);

	S5_CREATE_MARKER(mp);
	dev = vfsp->vfs_dev;
	rvp = S5FS(vfsp)->fs_root;
	ASSERT(rvp != NULL);

rescan:
	/*
	 * This search should run through the hash chains (rather
	 * than the entire inode table) so that we only examine
	 * inodes that we know are currently valid.
	 */
	(void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
	lock_dropped = B_FALSE;
	for (i = 0; i < NHINO; i++) {
		ih = &hinode[i];
		for (ip = ih->ih_forw; ip != (inode_t *)ih; ip = ipx) {
			ipx = ip->i_forw;
			/*
			 * Test of i_fs comes first for the ``markers''.
			 */
			if (ip->i_fs != NULL && ip->i_dev == dev) {
				vp = ITOV(ip);
				if (vp == rvp) {
				/*
                               	 * The unlocked reference to v_count is
                               	 * legal here because (i) no new hard hold
                               	 * could have been made to VP since
                               	 * we last acquired the s5_inode_table_mutex,
                               	 * and (ii) we rely upon ATOMIC access to
                               	 * ints.
                               	 */
					if (vp->v_count > 2 && force == 0) {
						open = -1;
					}
				continue;
				}
				/*
				 * If the file is truly open, then set error
				 * indicator, but sync the file and
				 * continue to move on.
				 */
				just_sync = B_FALSE;
                                VN_LOCK(vp);
				if (vp->v_count != 0) {
					open = -1;
					just_sync = B_TRUE;
				}
				VN_UNLOCK(vp);

			    /*
			     * Try to grab the inode. If we fail to grab it,
			     * then one of several things is happening:
			     *
			     * (a) fslush (i.e. s5_flushi) has its
			     *     hooks on it, or
			     *
			     * (b) an LWP is destroying the identity
			     *     for us in s5_idestroy, or
			     *
			     * (c) an LWP is creating the identity
			     *     in s5_getfree_inode.
			     *
			     * (d) an LWP is closing the file in
			     *     s5_inactive.
			     *
			     * In cases (b) or (c), the inode is
			     * INVALID, so it suffices to wait on
			     * s5_inode_sv. In cases (a) and (d),
			     * it suffices to take a soft hold and
			     * wait on the S5_IRWLOCK_WRLOCK.
			     */
			    lock_dropped = B_TRUE;
			    S5_INSERT_MARKER(mp, ip);
			    if (!s5_igrab(ip)) {
				if (ip->i_state & INVALID) {
					SV_WAIT(&s5_inode_sv, PRINOD,
						&s5_inode_table_mutex);
				} else {
					VN_SOFTHOLD(vp);
					UNLOCK(&s5_inode_table_mutex, PLBASE);
					S5_IRWLOCK_WRLOCK(ip);
					S5_IRWLOCK_UNLOCK(ip);
					VN_SOFTRELE(vp);
				}
				(void) LOCK(&s5_inode_table_mutex,
					    FS_S5LISTPL);
				ipx = mp->im_forw;
				S5_REMOVE_MARKER(mp);
				continue;
			    }

			    if (just_sync) {
				UNLOCK(&s5_inode_table_mutex, PLBASE);
				s5_syncip(ip, 0, IUP_FORCE_DELAY);
				(void) LOCK(&s5_inode_table_mutex,
					    FS_S5LISTPL);
				/*
				 * skip over the file
				 */
				ASSERT(ip->i_state & IDENTITY);
				ipx = ip->i_forw;
				S5_REMOVE_MARKER(mp);
				S5_INSERT_MARKER(mp, ipx);
				UNLOCK(&s5_inode_table_mutex, PLBASE);
				S5_IRWLOCK_UNLOCK(ip);
				VN_SOFTRELE(vp);
			    } else {
				/*
				 * Remove the inode from the free list (if
				 * necessary).
				 */
				switch(ip->i_state) {
				case IDENTITY|ITFREE:
					MET_INODE_INUSE(MET_S5, 1);
					/* FALLTHROUGH */
				case IDENTITY|IPFREE:
					S5_FREE_REMOVE(ip);
					VN_SOFTRELE(vp);
					break;
				}
				S5_FREE_CHECK();
				ASSERT(ip->i_state == IDENTITY);

				/*
				 * Okay, destroy the identity.
				 */
				ip->i_state = (IDENTITY|INVALID|ILCLFREE);
				UNLOCK(&s5_inode_table_mutex, PLBASE);

				/*
				 * s5_idestroy will release both the
				 * inode RWLOCK and the SOFTHOLD.
				 */
				s5_syncip(ip, 0, IUP_FORCE_DELAY);
				if (s5_idestroy(ip))
				    open = -1;
			   }
			   (void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
			   ipx = mp->im_forw;
			   S5_REMOVE_MARKER(mp);
			}
		}
	}
	UNLOCK(&s5_inode_table_mutex, PLBASE);

	/*
	 * While we were scanning the table, somebody could have exchanged
	 * a hold ahead of the scan for one behind the scan. So therefore,
	 * we must rescan the table if the lock was dropped.
	 *
	 * PERF: The lock hold time associated with this approach is not
	 *	 good. However, it is better to hold the lock than to
	 *	 allow vnodes to be owned by an unmounted file system -
	 *	 which will lead to strange memory stepping phenomena.
	 */
	if (open == 0 && lock_dropped)
		goto rescan;

	S5_DESTROY_MARKER(mp);

	return (open);
}

/*
 * s5_itruncup(inode_t *ip, size_t nsize)
 *	Truncate the inode ip up to nsize size.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock
 *	exclusive on entry.
 */
s5_itruncup(inode_t *ip, size_t nsize)
{
	vnode_t *vp = ITOV(ip);
	s5_fs_t *s5fsp = S5FS(vp->v_vfsp);
	size_t osize;
	size_t zsize;
	int err;
	pl_t s;

	osize = ip->i_size;
	ASSERT(nsize > osize);

	zsize = blkroundup(s5fsp, osize);

	if (zsize >= nsize)
		zsize = nsize;
	else if (zsize & PAGEOFFSET)
		page_find_unload(vp, zsize & PAGEMASK);

	/*
	 * Call pvn_trunczero to zero till end of
	 * block. 
	 */
	err = pvn_trunczero(vp, osize, zsize - osize);
	if (err) {
		/*
		 * Need to restore old isize because
		 * isize may have been changed.
		 */
		s = S5_ILOCK(ip);
		if (ip->i_size != osize) {
			ip->i_size = osize;
		}
		S5_IUNLOCK(ip, s);
		return (err);
	}

	s = S5_ILOCK(ip);
	ip->i_size = nsize;
	IMARK(ip, IUPD|ICHG);
	S5_IUNLOCK(ip, s);
	s5_iupdat(ip);

	return 0;
}

/*
 * void
 * s5_indirtrunc(vfs_t *vfsp, daddr_t bn, daddr_t lastbn, int level)
 *	Release blocks associated with the inode ip and
 *	stored in the indirect block bn.
 *
 * Calling/Exit State:
 *	May be called recursively.
 *
 *	The calling LWP hold's the inode's rwlock in *exclusive* mode
 *	on entry and exit.
 *
 * Description:
 *	Blocks are free'd in LIFO order up to (but not including) <lastbn>.
 *	If level is greater than SINGLE, the block is an indirect
 *	block and recursive calls to s5_indirtrunc must be used to
 *	cleanse other indirect blocks.
 */
void
s5_indirtrunc(vfs_t *vfsp, daddr_t bn, daddr_t lastbn, int level)
{
	int		i;
	buf_t		*bp;
	buf_t		*copy;
	daddr_t		*bap;
	daddr_t		nb;
	long		bsize;
	long		factor;
	long		nindir;
	daddr_t		last;
	s5_fs_t	*s5fsp;

	s5fsp = S5FS(vfsp);
	/*
	 * Calculate index in current block of last block (pointer) to be kept.
	 * A lastbn of -1 indicates that the entire block is going away, so we
	 * need not calculate the index.
	 */

	bsize = vfsp->vfs_bsize;
	nindir = s5fsp->fs_nindir;
	factor = 1;
	for (i = SINGLE; i < level; i++)
		factor *= nindir;
	last = lastbn;
	if (lastbn > 0)
		last /= factor;

	/*
	 * Get buffer of block pointers, zero those entries corresponding to
	 * blocks to be freed, and update on-disk copy first.  (If the entire
	 * block is to be discarded, there's no need to zero it out and
	 * rewrite it, since there are no longer any pointers to it, and it
	 * will be freed shortly by the caller anyway.)
	 * Note potential deadlock if we run out of buffers.  One way to
	 * avoid this might be to use statically-allocated memory instead;
	 * you'd have to make sure that only one process at a time got at it.
	 */

	copy = ngeteblk(bsize);
	bp = bread(vfsp->vfs_dev, LTOPBLK(bn, bsize), bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(copy);
		brelse(bp);
		return;
	}
	bap = bp->b_un.b_daddr;
	bcopy((caddr_t)bap, copy->b_un.b_daddr, bsize);
	bzero((caddr_t)&bap[last+1],
		(int)(nindir - (last+1)) * sizeof(daddr_t));
	bwrite(bp);
	bap = copy->b_un.b_daddr;

	/*
	 * Recursively free totally unused blocks.
	 */

	for (i = nindir-1; i > last; i--) {
		nb = bap[i];
		if (nb != 0) {
			if (level > SINGLE)
				s5_indirtrunc(vfsp, nb, (daddr_t)-1, level-1);
			s5_blkfree(vfsp, nb);
		}
	}

	/*
	 * Recursively free last partial block.
	 */

	if (level > SINGLE && lastbn >= 0) {
		last = lastbn % factor;
		nb = bap[i];
		if (nb != 0) {
			s5_indirtrunc(vfsp, nb, last, level-1);
		}
	}

	brelse(copy);
}

/*
 * int
 * s5_itrunc(inode_t *ip, uint_t length)
 *	Truncate the inode ip to at most length size.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock exclusive on entry and exit.
 */
int
s5_itrunc(inode_t *ip, uint_t length)
{
	vnode_t		*vp = ITOV(ip);
	vfs_t		*vfsp = vp->v_vfsp;
	s5_fs_t	*s5fsp = S5FS(vfsp);
	daddr_t		lastblock;
	daddr_t		bn;
	daddr_t		lastiblock[NIADDR];
	daddr_t		save[NADDR];
	int		level;
	int		type;
	int		i;
	int		err;
	int		bsize;
	size_t		osize;
	pl_t s;

	/*
	 * We only allow truncation of regular files and directories
	 * to arbritary lengths here. In addition, we allow symbolic
	 * links to be truncated only to zero length. Other inode
	 * types cannot have their length set here, especially device
	 * inodes where ip->i_rdev is actually being stored in ip->i_db[1]!
	 */
	type = ip->i_mode & IFMT;

	if (type == IFIFO)
		return (0);

	if (type != IFREG && type != IFDIR && !(type == IFLNK && length == 0)) 
		return (0);
	else if (type == IFLNK) {
		if (length != 0)
                        return (EINVAL);
	}	

	osize = ip->i_size;

	/*
	 * If file size is not changing, mark it as changed for
	 * POSIX. Since there's no space change, no need to write
	 * inode synchronously.
	 */
	if (length == osize) {
		s = S5_ILOCK(ip);
		IMARK(ip, ICHG | IUPD);
		S5_IUNLOCK(ip, s);
		s5_iupdat(ip);
		return (0);
	}

	/* Truncate-up case. */
	if (length > osize)
		return (s5_itruncup(ip, length));

	/* Truncate-down case. */
	/*
	 * If file is currently in use for swap, disallow truncate-down
	 */

	if (ip->i_swapcnt > 0)
		return (EBUSY);

	/*
	 * Update the pages of the file.  If the file is not being
	 * truncated to a page boundary, the contents of the
	 * page following the end of the file must be zero'ed
	 * in case it is looked at through mmap'ed address.
	 */
	if ((length & PAGEOFFSET) != 0) {
		int zbytes;

		zbytes = MIN((pageroundup(length) - length),
				(blkroundup(s5fsp, length) - length));
		zbytes = MIN(zbytes, ip->i_size);
		err = pvn_trunczero(vp, length, zbytes);
		if (err)
			return (err);
	}

	s = S5_ILOCK(ip);

	bsize = VBSIZE(vp);

	/*
	 * Calculate index into inode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep. Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	lastblock =
		((length + bsize - 1) >> s5fsp->fs_bshift) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR(s5fsp);
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR(s5fsp)
				* NINDIR(s5fsp);

	/*
	 * Update file and block pointers
	 * on disk before we start freeing blocks.
	 * If we crash before free'ing blocks below,
	 * the blocks will be returned to the free list.
	 * lastiblock values are also normalized to -1
	 * for calls to s5_indirtrunc below.
	 */

	for (i = NADDR - 1; i >= 0; i--) {
		save[i] = ip->i_addr[i];
	}

	for (level = TRIPLE; level >= SINGLE; level--) {
		if (lastiblock[level] < 0) {
			ip->i_addr[IB(level)] = 0;
			lastiblock[level] = -1;
		}
	}
	for (i = NDADDR - 1; i > lastblock; i--) {
		ip->i_addr[i] = 0;
	}

	/*
	 * Fix the file size before abort pages past new EOF.
	 */
	ip->i_size = length;

	IMARK(ip, ICHG|IUPD);
	S5_IUNLOCK(ip, s);

	s5_iupdat(ip);

	/*
	 * Call pvn_abort_range to abort all pages past the new eof.
	 */
	pvn_abort_range(vp, length, 0);

	/*
	 * If the new eof falls in the middle of a page, we need
	 * to wait for any pending io to complete before freeing
	 * any backing store for the portion of the page past eof.
	 */
	if ((length & PAGEOFFSET) != 0)
		page_find_iowait(vp, length);

	/*
	 * Indirect blocks first.
	 */
	for (level = TRIPLE; level >= SINGLE; level--) {
		bn = save[IB(level)];
		if (bn != 0) {
			s5_indirtrunc(vfsp, bn, lastiblock[level], level);
			if (lastiblock[level] < 0) {
				s5_blkfree(vfsp, bn);
			}
		}
		if (lastiblock[level] >= 0)
			goto done;
	}

	/*
	 * Direct blocks.
	 */
	for (i = NDADDR - 1; i > lastblock; i--) {
		if ((bn = save[i]) != 0)
			s5_blkfree(vfsp, bn);
	}

done:
	s = S5_ILOCK(ip);
	IMARK(ip, ICHG);
	S5_IUNLOCK(ip, s);
	return (0);
}

/*
 * void
 * s5init(vfssw_t *vswp)
 *	Initialize the S5 vfs structure, inode table, free list,
 *	and global S5 synchronization objects.
 *
 * Calling/Exit State:
 *	Should only be called when initializing S5. Currently, this
 *	only happens at system initialization time. If S5 is made
 *	into a loadable file system, then this code will have to be
 *	reworked.
 */
void
s5init(vfssw_t *vswp)
{

	/* check tunable flushtime parameter */
	if (s5_tflush > v.v_autoup) {
		/*
		 *+ Invalid flush time parameter
		 */
		cmn_err(CE_NOTE, "S5FLUSH is invalid. It should be less than NAUTOUP");
                return;
	}
	s5_doinit();

	/*
	 * Associate vfs operations
	 */
	vswp->vsw_vfsops = &s5_vfsops;

}

#define	TST_GROUP	3
#define	TST_OTHER	6

/*
 * int
 * s5_iaccess(inode_t *ip, mode_t mode, cred_t *cr)
 * 	Check mode permission on inode.
 *
 * Calling/Exit State:
 *	the calling LWP must hold the inode's rwlock at minimum
 *	share mode.
 *
 * Description:
 *	Mode is READ, WRITE or EXEC.
 *	in the case of WRITE, the read-only status of the file system
 * 	is checked. Also in WRITE, prototype text segments cannot be
 * 	written. The mode is shifted to select the owner/group/other
 * 	fields.
 */
int
s5_iaccess(inode_t *ip, mode_t mode, cred_t *cr)
{
	vnode_t	*vp;
	int	denied_mode;
	int	lshift;
	int	i;

	vp = ITOV(ip);
	if (mode & IWRITE) {
		/*
                 * Disallow write attempts on read-only
                 * file systems, unless the file is a block
                 * or character device or a FIFO.
                 */
                if (vp->v_vfsp->vfs_flag & VFS_RDONLY) { 
                        if ((ip->i_mode & IFMT) != IFCHR &&
                            (ip->i_mode & IFMT) != IFBLK &&
                            (ip->i_mode & IFMT) != IFIFO) {
                                return (EROFS);
                        }
                }
        }
	if (cr->cr_uid == ip->i_uid)
		lshift = 0;			/* TST OWNER */
	else if (groupmember(ip->i_gid, cr)) {
		mode >>= TST_GROUP;
		lshift = TST_GROUP;
	}
	else {
		mode >>= TST_OTHER;
		lshift = TST_OTHER;
	}
	if ((i = (ip->i_mode & mode)) == mode) {
		return 0;
	}
	denied_mode = (mode & (~i));
	denied_mode <<= lshift;
	if ((denied_mode & (IREAD | IEXEC)) && pm_denied(cr, P_DACREAD))
		return (EACCES);
	if ((denied_mode & IWRITE) && pm_denied(cr, P_DACWRITE))
		return (EACCES);
	return (0);
}

/*
 * int
 * s5_syncip(inode_t *ip, int pflags, enum iupmode iupflag)
 * 	Flush all the pages associated with an inode using the given flags,
 * 	then force inode information to be written back.
 *
 * Calling/Exit State:
 *	The inode's rwlock lock must be held on entry and exit.
 */
int
s5_syncip(inode_t *ip, int pflags, enum iupmode iupflag)
{
	int	error;
	vnode_t	*vp = ITOV(ip);

	if (vp->v_pages == NULL || vp->v_type == VCHR)
		error = 0;
	else {
		error = VOP_PUTPAGE(vp, 0, 0, pflags, sys_cred);
	}

	if (ip->i_flag & (IUPD|IACC|ICHG|IMOD)) {
		if (((pflags & B_ASYNC) == 0) && (iupflag == IUP_SYNC))
			ip->i_flag |= ISYN;
		s5_iupdat(ip);
	}
	return error;
}

/*
 * s5_igrab(inode_t *ip)
 *	Attemp to lock and SOFTHOLD an inode that caller thinks is currently
 *	referenced but unlocked.
 *
 * Calling/Exit State:
 *      If the inode does not have IDENTITY, is INVALID, or cannot be write
 *	locked, return 0, else return 1. [with the inode locked
 *      and with an added soft reference].
 *
 *      Called with the s5_inode_table_mutex held and returns that
 *      way.
 */
int
s5_igrab(inode_t *ip)
{
	vnode_t *vp;
	
	/*
	 * Race analysis:
	 *
	 * The caller needs to know that upon successful return, the
	 * IDENTITY of the ``grabbed'' vnode will not go away. This is
	 * guaranteed by the fact that s5_idestroy is the only function which
	 * can destroy identity, and that s5_idestroy needs to acquire the
	 * inode write lock.
	 *
	 * The second problem is that the caller needs to safely unlock
	 * the inode without the storage disappearing beneath the unlock.
	 * This is guaranteed by the SOFTHOLD.
	 *
	 * In order to release the ``grabbed inode'', the caller must first
	 * unlock the inode, and then VN_SOFTRELE the vnode.
	 */

	vp = ITOV(ip);
	switch (ip->i_state) {
	case IDENTITY:
	case IDENTITY|ITFREE:
	case IDENTITY|IPFREE:
		if (S5_ITRYWRLOCK(ip)) {
			VN_SOFTHOLD(vp);
			return (1);
		}
		break;
	}
	return (0);
}


/*
 * int
 * s5_idestroy(inode_t *ip)
 *	Destroy an inode's identity.
 *
 * Calling/Exit State:
 *	The inode is INVALID, has IDENTITY, is not free, and has a zero
 *	v_count. The caller holds a SOFTHOLD on the vnode. The callers
 *	holds the S5_IRWLOCK_WRLOCK At exit, both thethe SOFTHOLD and the
 *	S5_IRWLOCK_WRLOCK are released.
 *
 * 	No SPIN locks are held on entry or exit from this function.
 *
 *	Returns 0 on success and an error code on failure.
 */
int
s5_idestroy(inode_t *ip)
{
	vnode_t	*vp = ITOV(ip);
	int	err;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(vp->v_count == 0);
	ASSERT(vp->v_softcnt != 0);
	ASSERT(ip->i_state == (INVALID|IDENTITY|ILCLFREE) ||
	       ip->i_state == (INVALID|IDENTITY));

	/*
	 * Since the INVALID bit is set, and since vp->v_count == 0,
	 * neither fs_iget nor VN_TRY_HOLD can grab a hard hold on
	 * this inode. However, fsflush/pageout are still able to grab
	 * the pages, and hence can grab a softhold.
	 */
	if (!ip->i_fs->s_ronly) {
		if (ip->i_nlink <= 0) {
			/* free inode */
			ip->i_gen++;
			ASSERT(ip->i_swapcnt == 0);
			err = s5_itrunc(ip, 0);
			if (err != 0)
				goto fs_invalid;
			ip->i_mode = 0;
			ip->i_rdev = 0;
			ip->i_oldrdev = 0;
			ip->i_flag |= IUPD|ICHG;

			s5_ifree(ip);
			s5_iupdat(ip);
		} else {
			/*
			 * The unlocked reference to vpages is legal here
			 * because no LWP could have instantiated a
			 * page since the INVALID bit was set (i.e.
			 * v_pages is being mutexed by the INVALID
			 * pseudo lock). Since pages might be in
			 * the process of being aborted, it does assume
			 * ATOMIC reference to pointers.
			 */
			if (vp->v_pages != NULL) {
				/*
				 * Synchronously write and abort the pages
				 * of the vnode and put the pages on the free
				 * list when we are done. This action will
				 * cause all the pages to be written back for
				 * the file now and will allow s5_update()
				 * to skip over inodes that are on the free
				 * list.
				 */
				pvn_unload(vp);
				err = s5_syncip(ip, B_INVAL, IUP_FORCE_DELAY);
				if (err)
					goto backout;
			} else {
				s5_iupdat(ip);
			}
		}
	} else {
		/* for read-only fs abourt pages */
		pvn_abort_range(vp, 0, 0);
	}
	

fs_invalid:
	/* Remove from hash and put it in the freelist. */
	(void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
	s5_remque(ip);
	UNLOCK(&s5_inode_table_mutex, PLBASE);

	/*
	 * Unlock the inode.
	 */
	S5_IRWLOCK_UNLOCK(ip);

	VN_SOFTRELE(vp);
	return 0;

backout:

	/*
	 * Take an extra SOFTHOLD on the vnode. We need two SOFTHOLDs here:
	 *
	 *	(1) To protect the vnode storage during the
	 *	    S5_IRWLOCK_UNLOCK below.
	 *	(2) For the free list.
	 */
	VN_SOFTHOLD(vp);
	ASSERT(vp->v_softcnt >= 2);

	/*
	 * Recover from a failure to sync the pages of an inode we
	 * were attempting to destroy. Since the inode still has
	 * identity, we just put in on the free list.
	 */
	(void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
	ip->i_state = IDENTITY;
	S5_FREE_TAIL(&s5_partially_free, ip, IPFREE);
	S5_FREE_CHECK();
	UNLOCK(&s5_inode_table_mutex, PLBASE);

	/*
	 * Unlock the inode. This must be done after sending it to the
	 * free list, in order to prevent races with s5_igrab.
	 */
	S5_IRWLOCK_UNLOCK(ip);
	VN_SOFTRELE(vp);

	/*
	 * Remember to wake up anybody waiting.
	 */
	if (SV_BLKD(&s5_inode_sv))
		SV_BROADCAST(&s5_inode_sv, 0);

	return err;
}

/*
 * STATIC boolean_t
 * s5_pfree_scan(void)
 *	Scan the partially-free list in hopes of rebuild the
 *	totally-free list.
 *
 * Calling/Exit State:
 *	The s5_inode_table_mutex is held on entry to this function,
 *	throughout execution, and at the exit.
 *
 * Remarks:
 *	This routine can take the number of totally-free inodes
 *	over the low water mark (s5_inode_lwm). This is acceptable,
 *	since the low water mark is only a guideline. This condition
 *	is expected to be rare.
 */
STATIC boolean_t
s5_pfree_scan(void)
{
	inode_t *ip, *nip;
	vnode_t *vp;
	boolean_t ret = B_FALSE;

	ASSERT(LOCK_OWNED(&s5_inode_table_mutex));

	ip = ((inode_t *)(&s5_partially_free))->i_freeb;
	while (ip != (inode_t *)(&s5_partially_free)) {
		ASSERT(ip != NULL);
		ASSERT(ip->i_state == (IDENTITY|IPFREE));
		nip = ip->i_freeb;
		ASSERT(nip != NULL);
		vp = ITOV(ip);
		/*
		 * The following unlocked tests of the vnode fields
		 * are legal, since the division of inodes between the
		 * partially and totally free lists is only an optimization.
		 */
		if (vp->v_pages == NULL && vp->v_softcnt == 1) {
			S5_FREE_REMOVE(ip);
			MET_INODE_INUSE(MET_S5, -1);
			S5_FREE_TAIL(&s5_totally_free, ip, ITFREE);
			ret = B_TRUE;
		}
		ip = nip;
	}
	s5_scan_time = lbolt;

	S5_FREE_CHECK();

	return ret;
}

/*
 * void
 * s5_iupdat(inode_t *ip)
 * 	Flush inode to disk, updating timestamps if requested.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode's rwlock in *exclusive* mode.
 */
void
s5_iupdat(inode_t *ip)
{
	buf_t		*bp;
	vnode_t		*vp;
	s5_fs_t	*s5fsp;
	dinode_t	*dp;
	char		*p1;
	char		*p2;
	unsigned	i;
	int		bsize;
	boolean_t issync = B_FALSE;
	pl_t s;

	vp = ITOV(ip);
	s5fsp = S5FS(vp->v_vfsp);
	bsize = VBSIZE(vp);
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
		return;
	bp = bread(ip->i_dev, LTOPBLK(FsITOD(s5fsp, ip->i_number), bsize),
			 bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return;
	}
	dp = (struct dinode *)bp->b_addrp;
	dp += FsITOO(s5fsp, ip->i_number);
	dp->di_mode = ip->i_mode;
	dp->di_nlink = ip->i_nlink;
	dp->di_uid = ip->i_uid;
	dp->di_gid = ip->i_gid;
	dp->di_size = ip->i_size;
	dp->di_gen = ip->i_gen;
	p1 = (char *)dp->di_addr;
	p2 = (char *)ip->i_addr;
	for (i = 0; i < NADDR; i++) {
		ltol3one(p1, p2);
	}

	/*
	 * Update the inode's times if necessary
	 */
	s = S5_ILOCK(ip);
	IMARK(ip, ip->i_flag);
	if (ip->i_flag & ISYN)
		issync = B_TRUE;
	ip->i_flag &= ~(IACC|IUPD|ICHG|IMOD|ISYN);
	S5_IUNLOCK(ip, s);
	dp->di_atime = ip->i_atime;
	dp->di_mtime = ip->i_mtime;
	dp->di_ctime = ip->i_ctime;
	if (issync == B_TRUE)
		bwrite(bp);
	else
		bdwrite(bp);
}


/*
 * void
 * s5_remque(inode_t *ip)
 *	Remove an inode from the hash chain it's on.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode table lock.
 */
void
s5_remque(inode_t *ip)
{
	ASSERT(LOCK_OWNED(&s5_inode_table_mutex));
	ASSERT(ip->i_state & IDENTITY);
	ASSERT(!(ip->i_state & (ITFREE|IPFREE)));

	ip->i_back->i_forw      = ip->i_forw;
	ip->i_forw->i_back      = ip->i_back;
	ip->i_state &= ~IDENTITY;
	return;
}

/*
 * void
 * s5_insque(inode_t *ip, struct hinode *hip)
 *	Insert an inode into a hash chain.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode table lock.
 */
void
s5_insque(inode_t *ip, struct hinode *ih)
{
	ASSERT(LOCK_OWNED(&s5_inode_table_mutex));
	ASSERT(!(ip->i_state & (IDENTITY|ITFREE|IPFREE)));

	ih->ih_forw->i_back = ip;
        ip->i_forw = ih->ih_forw;
        ih->ih_forw = ip;
        ip->i_back = (struct inode *) ih;
	ip->i_state |= IDENTITY;

	return;
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_s5_inode(const inode_t *ip)
 *	Print an s5 inode.
 *
 * Calling/Exit State:
 *	No locking.
 *
 * Remarks:
 *	Intended for use from a kernel debugger.
 */
void
print_s5_inode(const inode_t *ip)
{
	if (ip->i_state == IMARKER) {
		debug_printf("ip = 0x%x (INODE MARKER)\n", ip);
		return;
	}
	debug_printf("ip = 0x%x, iflag = %8x, istate = %8x, inum = %d\n",
		     ip, ip->i_flag, ip->i_state, ip->i_number);
	debug_printf("\tvcount = %d, vsoftcnt = %d vdata = %x vfsp=%x\n",
		     ITOV(ip)->v_count, ITOV(ip)->v_softcnt, &ip->i_vnode,ITOV(ip)->v_vfsp);
	debug_printf("\tlinks = %d, imode = %06o, isize = %d, addr = %x\n",
		     ip->i_nlink, ip->i_mode, ip->i_size, ip->i_addr);
}

void
find_inode(const inode_t *fip)
{
	int i;
        struct hinode	*ih;
	inode_t	*ip;

	for (i = 0; i < NHINO; i++) {
	    ih = &hinode[i];
	    for (ip = ih->ih_forw; ip != (inode_t *)ih; ip = ip->i_forw) {
			if (ip == fip) {
				debug_printf("\tfound from the hash list\n");
				print_s5_inode(fip);
				return;
			} 
			if (debug_output_aborted())
				return;
	    }
	}
	ip = ((inode_t*)(&s5_totally_free))->i_freef;
	while (ip != (inode_t*)(&s5_totally_free)) {
		if (ip == fip) {
			debug_printf("\tfound from the totally free list\n");
			print_s5_inode(ip);
			return;
		}
		if (debug_output_aborted())
			return;
		ip = ip->i_freef;
	}

	ip = ((inode_t*)(&s5_partially_free))->i_freef;
	while (ip != (inode_t*)(&s5_partially_free)) {
		if (ip == fip) {
			debug_printf("\tfound from  the partially free list\n");
			print_s5_inode(ip);
			return;
		}
		if (debug_output_aborted())
			return;
		ip = ip->i_freef;
	}
}
/*
 * void
 * print_s5_hash(void)
 *	Dumps the inode cache.
 *
 * Calling/Exit State:
 *	No locking.
 */
void
print_s5_hash(void)
{
	int i, j=0;
	struct hinode	*ih;
	inode_t	*ip;

	for (i = 0; i < NHINO; i++) {
	    ih = &hinode[i];
	    for (ip = ih->ih_forw; ip != (inode_t *)ih; ip = ip->i_forw) {
		print_s5_inode(ip);
		j++;
		if (debug_output_aborted())
			return;
	    }
	}
	debug_printf("===========%d inodes :\n",j);
}

/*
 * void
 * print_s5_freelist(void)
 *	Dumps the inode free list.
 *
 * Calling/Exit State:
 *	No locking.
 */
void
print_s5_freelist(void)
{
	inode_t	*ip;


	debug_printf("===== totally free list:\n");

	ip = ((inode_t*)(&s5_totally_free))->i_freef;
	while (ip != (inode_t*)(&s5_totally_free)) {
		print_s5_inode(ip);
		if (debug_output_aborted())
			return;
		ip = ip->i_freef;
	}
	debug_printf("===== partially free list:\n");

	ip = ((inode_t*)(&s5_partially_free))->i_freef;
	while (ip != (inode_t*)(&s5_partially_free)) {
		print_s5_inode(ip);
		if (debug_output_aborted())
			return;
		ip = ip->i_freef;
	}
}

/*
 * int
 * s5_assfail(inode_t *ip, const char *a, const char *f, int l)
 *      Panic when an S5_ASSERT fails, but also capture ip.
 *
 * Calling/Exit State:
 *      None.
 */
int
s5_assfail(inode_t *ip, const char *a, const char *f, int l)
{
	/*
	 *+ An inode failure occurred.
	 */
        cmn_err(CE_WARN, "INODE FAILURE at 0x%x\n", (long)ip);
        assfail(a, f, l);
        /* NOTREACHED */
}



/*
 * void
 * s5_free_check(void)
 *	Perform basic checks on the free list.
 *
 * Calling/Exit State:
 *	The s5_inode_table_mutex is held.
 */
void
s5_free_check(void)
{
	inode_t	*ip, *fip, *lip;
	int count = 0;

	ASSERT(LOCK_OWNED(&s5_inode_table_mutex));

	fip = lip = ip = (inode_t *)(&s5_totally_free);
	ip = ip->i_freef;
	while (ip != fip) {
		ASSERT(ip->i_state == ITFREE ||
		       ip->i_state == (IDENTITY|ITFREE));
		ASSERT(ip->i_freeb == lip);
		lip = ip;
		ip = ip->i_freef;
		++count;
	}
	ASSERT(ip->i_freeb == lip);

	fip = lip = ip = (inode_t *)(&s5_partially_free);
	ip = ip->i_freef;
	while (ip != fip) {
		ASSERT(ip->i_state == (IDENTITY|IPFREE));
		ASSERT(ip->i_freeb == lip);
		lip = ip;
		ip = ip->i_freef;
		++count;
	}
	ASSERT(ip->i_freeb == lip);

}

#endif /* DEBUG || DEBUG_TOOLS */
