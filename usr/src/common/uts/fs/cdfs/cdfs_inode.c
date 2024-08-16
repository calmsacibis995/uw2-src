/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/cdfs/cdfs_inode.c	1.12"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/cdfs/cdfs_hier.h>
#include <fs/cdfs/cdfs.h>
#include <fs/cdfs/cdfs_data.h>
#include <fs/cdfs/cdfs_fs.h>
#include <fs/cdfs/cdfs_inode.h>
#include <fs/cdfs/cdfs_susp.h>
#include <fs/cdfs/iso9660.h>
#include <fs/dnlc.h>
#include <fs/fs_subr.h>
#include <fs/mode.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/as.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/seg.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/utsname.h>
#include <util/inline.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

STATIC int cdfs_BldInode (struct vfs *, struct cdfs_iobuf *,
	 	struct cdfs_inode *);

STATIC int cdfs_AllocDrec (struct cdfs_drec **);

STATIC int cdfs_MergeDrec(cdfs_inode_t *, cdfs_drec_t *);

STATIC int cdfs_GetXar (struct vfs *, struct cdfs_drec *,
		 struct cdfs_xar *);
extern int cdfs_MergeRrip (struct cdfs_inode *, struct cdfs_rrip *);

/*
 * int
 * cdfs_InitInode(cdfs_inode_t	*ip, boolean_t init);
 * 	Initialize an Inode structure.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */
int
cdfs_InitInode(cdfs_inode_t  *ip, boolean_t init)
{
        /*
         * Set all Inode fields to zero/NULL, except
         * for the fields that require special values.
         */
	if (!init)
		bzero((caddr_t)ip, sizeof(*ip));
	else {
		ip->i_UserID = ip->i_GroupID = ip->i_Mode =ip->i_Size = ip->i_LinkCnt = 0;
		ip->i_DRcount = 0;
		ip->i_mapsz = ip->i_mapcnt = ip->i_NextByte = 0;
		ip->i_vfs = 0;
		ip->i_Xar = NULL;
		ip->i_Rrip = NULL;
		ip->i_Flags = 0;
		bzero((caddr_t)&ip->i_AccessDate, sizeof(timestruc_t));
		bzero((caddr_t)&ip->i_ModDate, sizeof(timestruc_t));
		bzero((caddr_t)&ip->i_CreateDate, sizeof(timestruc_t));
		bzero((caddr_t)&ip->i_ExpireDate, sizeof(timestruc_t));
		bzero((caddr_t)&ip->i_EffectDate, sizeof(timestruc_t));
		bzero((caddr_t)&ip->i_AttrDate, sizeof(timestruc_t));
		bzero((caddr_t)&ip->i_BackupDate, sizeof(timestruc_t));
		bzero((caddr_t)&ip->i_SymLink, sizeof(struct pathname));
		ip->i_DirOffset = ip->i_VerCode = ip->i_ReadAhead = 0;
		bzero((caddr_t)&ip->i_DirRecStorage, sizeof(struct cdfs_drec));
		bzero((caddr_t)&ip->i_XarStorage, sizeof(struct cdfs_xar));
		bzero((caddr_t)&ip->i_RripStorage, sizeof(struct cdfs_rrip));
	}

        ip->i_Fid = CDFS_NULLFID;
        ip->i_ParentFid = CDFS_NULLFID;
        ip->i_DevNum = NODEV;
        ip->i_Vnode = &ip->i_VnodeStorage;
	ip->i_Flags |= IFREE;
        return(RET_OK);
}

/*
 * cdfs_CleanInode(cdfs_inode_t *ip, boolean_t init)
 * 	Clean up an Inode structure for reuse.
 * 
 * Calling/Exit State:
 *	Inode list lock is held on entry or exit.
 *
 * Description:
 *	 Removes Inode from Free list and Hash Table, as well as
 *	clean up auxiliary data structures of the inode.
 */
void
cdfs_CleanInode(cdfs_inode_t *ip, boolean_t init)
{
	cdfs_drec_t		*drec;
	cdfs_rrip_t		*rrip;
	struct pathname		*pn;
	
	/*
	 * Make sure the Inode is removed from the Free list and Hash table.
	 */
	if ((ip->i_FreeFwd != NULL) ||
		(ip->i_FreeBack != NULL))
		cdfs_IrmFree(ip);

	if ((ip->i_HashFwd != NULL) ||
		(ip->i_HashBack != NULL)) {
		cdfs_IrmHash(ip);
	}

	/*
	 * Release Inode's Dir Rec structures.
	 * Note: The first Dir Rec is allocated as part of the
	 * Inode struct so it doesn't get freed.
	 */
	if (ip->i_DirRec != NULL) {
		drec = ip->i_DirRec->drec_PrevDR;
		while (drec != ip->i_DirRec) {
			cdfs_DrecRm(&ip->i_DirRec, drec);
			cdfs_DrecPut(&cdfs_DrecFree, drec);
			drec = ip->i_DirRec->drec_PrevDR;
		}
		ip->i_DirRec = NULL;
	}

	/*
	 * Release Inode's XAR structure and its sub-structures.
	 * Note: Since the main structure is allocated as part of
	 * the Inode structure, it gets deallocated implicitly.
	 * However, all sub-structures need to be released explicitly.
	 *
	 * Note - Any allocated sub-structures of the XAR (e.g. Escape
	 * Sequence and/or Application Use Data buffers) need to
	 * be released if they are allocated.  Currently, they are
	 * not allocated.
	 */

	/*
	 * Release Inode's RRIP structure and its sub-structures.
	 * Note: Since the main structure is allocated as part of
	 * the Inode structure, it gets deallocated implicitly.
	 * However, all sub-structures need to be released explicitly.
	 */
	rrip = ip->i_Rrip;
	if (rrip != NULL) {
		pn = &rrip->rrip_SymLink;
		if (pn->pn_buf != NULL) {
			pn_free(pn);
		}

		pn = &rrip->rrip_AltName;
		if (pn->pn_buf != NULL) {
			pn_free(pn);
		}
		ip->i_Rrip = NULL;
	}

	/*
	 * Release Inode's VNODE structure and its sub-structures.
	 * Note: Since the main structure is allocated as part of
	 * the Inode structure, it gets deallocated implicitly.
	 * However, all sub-structures need to be released explicitly.
	 */
	cdfs_InitInode(ip, init);
	return;
}


/*
 * STATIC cdfs_inode_t
 * cdfs_FindInode(cdfs_inode_t *hash, vfs_t *vfsp, cdfs_fid_t *fid)
 * 	Search the Inode cache for a specific Inode.
 *
 * Calling/Exit State: 
 *	Inode list lock is held on entry and exit.
 */
STATIC cdfs_inode_t *
cdfs_FindInode(cdfs_inode_t *hash, vfs_t *vfsp, cdfs_fid_t *fid)
{
	cdfs_inode_t *ip;

	/*
	 * Scan hash list to see if Inode is already in-core.
	 */
	ip = hash;
	do {
		if ((CDFS_CMPFID(&ip->i_Fid, fid) == B_TRUE) &&
			(ip->i_vfs == vfsp)) {
			return(ip);
		}
		ip = ip->i_HashFwd;
	} while (ip != hash);
	
	/* Didn't find it */
	return(NULL);

}

/* 
 * STATIC int
 * cdfs_getfree_inode(cdfs_inode_t **ipp, vfs_t *vfsp, cdfs_fid_t *fid, pl_t s)
 *	Remove an inode from the free list (if there are
 *	any available) for re-use, or create one. May cause dnlc purges.
 *
 * Calling/Exit State:
 *	Calling LWP holds the inode table locked. The inode table lock may
 *	be released and reobtained in this routine.
 *	
 *	The inode table lock is held when 0 is returned and <*ipp>
 *	is non-NULL. In all other cases, the inode table lock is
 *	not held on exit.
 *
 *	If 0 is returned, then the inode removed from the free list or 
 *	newly allocated is placed in <*ipp>. The sleep lock of <*ipp> is held 
 *	exclusive in this case. 
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		ENFILE	The maximum number of CDFS inodes are in use.
 *			<*ipp> is NULL in this case.
 *
 * Description:
 *	Try to take an inode from the free list.
 *	If the free list is empty then purge some dnlc entries from
 * 	the file system that the inode will live on. Return ENFILE
 *	 if there aren't any inodes available.
 *
 *	Insert the new inode into the proper place on the hash list. If an
 *	inode with pages was recycled, clean any pages associated with the 
 *	inode.
 *
 *	Must be careful to re-check the hash list for the requested inode
 *	if we release the lists lock since another LWP could have entered
 *	the inode in the list.
 */
/* ARGSUSED */
STATIC int
cdfs_getfree_inode(cdfs_inode_t **ipp, vfs_t *vfsp, cdfs_fid_t *fid, pl_t s)
{
	int		error;
	cdfs_inode_t	*ip, *dupip, *hip;
	vnode_t		*vp;
	boolean_t	count = B_FALSE, dnlc_flag = B_FALSE;

	*ipp = NULL;
	error = 0;

again:

	/*
	 * If inode freelist is empty, release some name cache entries
	 * for this mounted file system in an attempt to reclaim some
	 * inodes.
	 */
	if (cdfs_InodeFree == NULL) {
		/*
		 * dnlc_purge_vfsp might go to sleep. So we have to check the
		 * hash chain again to handle possible race condition.
		 */
		UNLOCK(&cdfs_inode_table_mutex, s);
		(void)dnlc_purge_vfsp(vfsp, 10);
		dnlc_flag = B_TRUE;
		s = LOCK(&cdfs_inode_table_mutex, FS_CDFSLISTPL);
		if (cdfs_InodeFree == NULL) {
			UNLOCK(&cdfs_inode_table_mutex, s);
			return (ENFILE);
		}
	}
	/* 
	 * Remove the inode from the free list
	 */
	ip = cdfs_InodeFree;

	if (CDFS_ITRYSLEEP_LOCK(ip) != B_TRUE) {
		/* 
		 * Trylock the inode. If fails, someone raced to re-instate
		 * the inode and won. We need to move it to the end of the
		 * freelist and try to get the next one the free list.
		 */
		if (ip->i_FreeFwd)
			cdfs_InodeFree = ip->i_FreeFwd;
		if (count == B_TRUE) {
			UNLOCK(&cdfs_inode_table_mutex, s);
			return(ENFILE);
		} else
			count = B_TRUE;

		goto again;
	}

	if ((ip->i_Flags & IFREE) == 0) {
		/* Someone raced and won */
		if (ip->i_FreeFwd)
			cdfs_InodeFree = ip->i_FreeFwd;
		CDFS_SLEEP_UNLOCK(ip);
		if (count == B_TRUE) {
			UNLOCK(&cdfs_inode_table_mutex, s);
			return(ENFILE);
		} else
			count = B_TRUE;

		goto again;
	}
	cdfs_IrmFree(ip);

	ASSERT((ip->i_Flags & IFREE) != 0);
	ASSERT(ITOV(ip)->v_count == 0);
	MET_INODE_INUSE(MET_OTHER, 1);


	/* Other system info updates */
	if (ip->i_Mode != 0 && ITOV(ip)->v_pages != NULL)
		MET_INO_WO_PG(MET_OTHER, 1);
	else
		MET_INO_WO_PG(MET_OTHER, 1);

	/* remove the inode from the hash  if the inode is in the hash list*/
	if (ip->i_HashFwd || ip->i_HashBack)
		cdfs_IrmHash(ip);

	/*
 	 * If cdfs is read-write file system then we must
	 * do syncip().
	 * Now check if we called dnlc_purge_vfsp to purge
	 * inodes. If it's, make sure that someone else didn't
	 * enter the inode into the list while we were sleeping
	 * in dnlc_purge_vfsp(). If they did, then, must drop our
	 * inode (<ip>) and start over.
	 */
	if (dnlc_flag == B_TRUE) {
		hip = cdfs_InodeHash[CDFS_INOHASH(fid)]; 
		if (hip != NULL) {
			dupip = cdfs_FindInode(hip, vfsp, fid);
			if (dupip != NULL) {
				cdfs_CleanInode(ip, 1);
				cdfs_IputFree(ip);
				cdfs_InodeFree = ip;
                		UNLOCK(&cdfs_inode_table_mutex, s);
				return(0);
			}
		}
	}

	/*
	 * Clear any flags that may have been set, including
	 * IFREE. 
	 */
	ip->i_Flags = 0;

	vp = ITOV(ip);

	ASSERT(vp->v_count == 0);
	ASSERT((ip->i_Flags & IFREE) == 0);
	vp->v_flag = 0;

	ip->i_DRcount = 0;
	ip->i_DirOffset = 0;
	ip->i_NextByte = 0;
	ip->i_VerCode = 0;
	ip->i_mapcnt = 0;
	ip->i_mapsz = 0;
	ip->i_DirRec = 0;
	ip->i_ReadAhead = 0;
	ip->i_vfs = vfsp;

	*ipp = ip;

	return (error);
}

/*
 * int
 * cdfs_BldInode(vfs_t *vfsp, cdfs_iobuf *drec_buf, cdfs_inode_t *ip)
 * 	Build an Inode from the Dir Rec data on the media.
 * 
 * Calling/Exit State:
 *	Inode sleep lock is held on entry and exit.
 * 
 * Description:
 *	Convert "core" (non-SUA) DREC data to a generic DREC format.
 *	Merge generic DREC data into Inode structure.
 *	Merge generic XAR and RRIP data into Inode structure.
 */
STATIC int	
cdfs_BldInode(vfs_t *vfsp, cdfs_iobuf_t *drec_buf, cdfs_inode_t *ip)
	/* File system's VFS structure	*/
	/* Dir Rec buffer structure	*/
	/* Inode struct (already hashed)*/
{
	cdfs_drec_t	*drec;		/* Dir Rec template		*/
	cdfs_xar_t	*xar;		/* XAR template			*/
	cdfs_rrip_t	*rrip;		/* RRIP template		*/
	vnode_t		*vp;		/* Vnode associated with Inode	*/
	int		RetVal;		/* Return value of called procs */

	ip->i_vfs = vfsp;

	/*
	 * Copy the relevent Dir Rec data to the Inode.
	 */
	drec = &ip->i_DirRecStorage;

	drec->drec_Loc = drec_buf->sb_sect;
	drec->drec_Offset = drec_buf->sb_offset - drec_buf->sb_sectoff;

	RetVal = cdfs_ConvertDrec(drec,
		(union media_drec *)drec_buf->sb_ptr, CDFS_TYPE(vfsp));
	if (RetVal != RET_OK) {
		return(RetVal);
	}
		
	RetVal = cdfs_MergeDrec(ip, drec);
	if (RetVal != RET_OK) {
		return(RetVal); 
	}

	/*
	 * If there this is a multi-extent file, then get the additional
	 * Dir Recs from the media and merge them into the Inode structure.
	 */
	while ((drec->drec_Flags & ISO_DREC_MULTI) != 0) {
		drec_buf->sb_ptr += drec_buf->sb_reclen;
		drec_buf->sb_offset +=  drec_buf->sb_reclen;
		RetVal = cdfs_ReadDrec(vfsp, drec_buf);
		if (RetVal != RET_OK) {
			return(RetVal);
		}

		RetVal = cdfs_AllocDrec(&drec);
		if (RetVal != RET_OK) {
			return(RetVal);
		}

		drec->drec_Loc = drec_buf->sb_sect;
		drec->drec_Offset = drec_buf->sb_offset - drec_buf->sb_sectoff;

		RetVal = cdfs_ConvertDrec(drec,
			(union media_drec *)drec_buf->sb_ptr, CDFS_TYPE(vfsp));
		if (RetVal != RET_OK) {
			return(RetVal);
		}

		RetVal = cdfs_MergeDrec(ip, drec);
		if (RetVal != RET_OK) {
			break; 
		}
	}

	/*
	 * If the last Dir Rec of the file has an XAR, then get it
	 * and merge the data into the Inode.
	 *
	 * Note: Storage for the XAR is allocated within the Inode
	 * itself but this need not be the case.
	 */
	if (drec->drec_XarLen > 0) {
		xar = &ip->i_XarStorage;

		RetVal = cdfs_GetXar(vfsp, drec, xar);
		if (RetVal != RET_OK) {
			return(RetVal);
		}

		RetVal = cdfs_MergeXar(ip, xar); 
		if (RetVal != RET_OK) {
			return(RetVal);
		}
	}
	
	/*
	 * If RRIP is active, get the RRIP data and merge it with
	 * the Inode.
	 * Note: Storage for the RRIP data is allocated with the Inode
	 * structure, but this need not be the case.
	 * Note: It's not an error if we don't find any RRIP data.
	 */
	if ((CDFS_FLAGS(vfsp) & CDFS_RRIP_ACTIVE) != 0) {
		rrip = &ip->i_RripStorage;

		RetVal = cdfs_GetRrip(vfsp, drec_buf, rrip);
		switch (RetVal) {
			case RET_OK: {
				RetVal = cdfs_MergeRrip(ip, rrip); 
				if (RetVal != RET_OK) {
					return(RetVal);
				}
				break;
			}
			case RET_EOF:
			case RET_NOT_FOUND: {
				RetVal = RET_OK;
				break;
			}
			default: {
				return(RetVal); 
			}
		}
	}
	
	/*
	 * Setup the Vnode portion of the Inode.
	 */
	vp = &ip->i_VnodeStorage;
	ip->i_Vnode = vp;

	vp->v_op = &cdfs_vnodeops;
	vp->v_vfsmountedhere = NULL;
	vp->v_vfsp = vfsp;
	vp->v_flag = 0;
	vp->v_stream = NULL;
	vp->v_pages = NULL;
	vp->v_filocks = NULL;

	vp->v_type = IFTOVT(ip->i_Mode);
	vp->v_rdev = cdfs_GetDevNum(vfsp,ip);
	vp->v_data = (caddr_t)ip;
	if (CDFS_CMPFID(&ip->i_Fid, &CDFS_ROOTFID(vfsp)) == B_TRUE) {
		vp->v_flag |= VROOT;
	}
	return(RET_OK);
}



/*
 * cdfs_GetInode(vfs_t *vfsp, cdfs_fid_t *fid, cdfs_iobuf_t *drec_buf,
 *	cdfs_inode_t  **ipp)
 * 	Get the Inode structure identified by the File ID structure.
 *
 * Calling/Exit State:
 *	On success 0 is returned, <*ipp> is returned unlocked.
 *
 * Description:
 *      Search for the inode on the hash list. If it's there, honor
 *      the locking protocol. If it's not there, try to get an inode
 *      slot by either dynamically allocating one, or by removing an
 *      inode from the free list. If we can, than initialize the
 *      inode from its specified device. Before changing the identity
 *      of an inode, free any pages it has.
 *
 * Note: Each file in a CDFS file system can be uniquely identified
 * by a File ID structure.  Using the specified File ID structure,
 * this routine locates or builds an Inode describing the file or dir.
 */
int
cdfs_GetInode(vfs_t *vfsp, cdfs_fid_t *fid, cdfs_iobuf_t *drec_buf,
	cdfs_inode_t  **ipp)
	/* File system's VFS structure	*/
	/* Unique File ID of Inode	*/
	/* Dir Rec (if set) of 1st D.Rec*/
	/* Ret addr of Inode pointer	*/
{
	cdfs_inode_t	*ip;		/* Inode pntr to be returned	*/
	cdfs_inode_t	*tmp_ip;	/* Temporary Inode pntr		*/
	cdfs_inode_t	*hip;		/* Head of Inode hash list	*/
	cdfs_fid_t 	tmp_fid;	/* Temporary FID		*/
	cdfs_iobuf_t	tmp_drec_buf;	/* Temp. Dir Rec buffer struct	*/
	int 		retval;		/* Return value of called procs	*/
	pl_t		s;
	vnode_t		*vp;
	page_t		*pages;


	/*
	 * Update system accounting statistics.
	 */
	MET_IGET(MET_OTHER);
	*ipp = NULL;
	/*
         * Locate the Hash list that should contain the Inode.
         */
	hip = cdfs_InodeHash[CDFS_INOHASH(fid)];
loop:
	s = LOCK(&cdfs_inode_table_mutex, FS_CDFSLISTPL);
	if (hip != NULL) {
	    ip = cdfs_FindInode(hip, vfsp, fid);
	    /*
	     * It may take many attempts before complete success or failure.
	     */
	    if (ip != NULL) {
                CDFS_SLEEPLOCK_RELLOCK(ip);
                /* 
		 * We assume that an inode on an hash chain does      
                 * have its vnode and other pointers sane!           
		 */
                vp = ITOV(ip);
                ASSERT(vp == ip->i_Vnode);
		if (CDFS_CMPFID(&ip->i_Fid, fid) == B_FALSE) {
			/*
                         * If someone races with you to get the inode
                         * release the inode and go to look again.
                         */
                        CDFS_SLEEP_UNLOCK(ip);
                        goto loop;
                }
		/* found the inode */
                s = LOCK(&cdfs_inode_table_mutex, FS_CDFSLISTPL);
                if ((ip->i_Flags & IFREE) != 0) {
			/*
			 * Inode is on the free list. Remove it from freelist.
			 */
			cdfs_IrmFree(ip);
			ip->i_Flags &= ~IFREE;
			MET_INODE_INUSE(MET_OTHER, 1);
		}
		UNLOCK(&cdfs_inode_table_mutex, s);

		VN_HOLD(vp);
		ASSERT((ip->i_Flags & IFREE) == 0);

		*ipp = ip;
		CDFS_SLEEP_UNLOCK(ip);
		return (0);
 	    }
	}
	
	/*
	 * We hold the hash list lock at this point. Take a free inode
	 * and re-assign it to our inode.
	 */
	retval = cdfs_getfree_inode(&ip, vfsp, fid, s);
	if (retval != RET_OK) {
		if (retval == ENFILE) {
			MET_INODE_FAIL(MET_OTHER);
			/*
			 *+ The inode table has filled up.
			 *+ The correction is to reconfigure the kernel
			 *+ to increase the inode table size.
			 */
			cmn_err(CE_WARN, "cdfs_GetInode: inode table overflow");
		}
		
		return(retval);
	}
				
	if (ip == NULL)
		goto loop;
	/*
	 * Fillin the Unique File ID structure.
	 */
	ASSERT(ip->i_Flags == 0);
	ip->i_Fid = *fid;

	/* 
	 * Put the inode on the chain for its new (fid, vfs) pair.
	 */
	cdfs_IputHash(ip);

	/*
         * Can safely release the inode table lock at this point
         * since the executing LWP has already established its
         * reference to the inode and the inode is in the hash
         * chain. Regardless of how the caller asked for the
         * lock to be obtained, it is always obtained the inode sleep lock 
         * in cdfs_getfree_inode to prevent another LWP from using
         * the inode before it's fully initialized.
        */
	UNLOCK(&cdfs_inode_table_mutex, s);

	/*
	 * If caller didn't pass in a Dir Rec buffer for the
	 * file/dir, we need to create the buffer ourselves.
	 */
	if (drec_buf == NULL) {
		/*
		 * Read the Dir Rec of the file/dir from the media.
		 *
		 * Note: Since we don't have access to the Inode/Vnode of
		 * the Parent directory, we have to do the I/O directly
		 * from the driver using the 'struct buf' (bp) interface.
		 */
		drec_buf = &tmp_drec_buf;
		CDFS_SETUP_IOBUF(drec_buf, CDFS_BUFIO);
		drec_buf->sb_dev = CDFS_DEV(vfsp);
		drec_buf->sb_sect = fid->fid_SectNum;
		drec_buf->sb_offset = fid->fid_Offset;
		retval = cdfs_ReadDrec(vfsp, drec_buf);
		if (retval != RET_OK) {
			goto Cleanup;
		}
	}
	
	/*
	 * Build the desired Inode using the allocated Inode stucture
	 * and the Dir Rec data from the media.
	 */
	retval = cdfs_BldInode(vfsp, drec_buf, ip);
	if (retval != RET_OK) {
		goto Cleanup;
	}


	/*
	 * If the Inode has been "relocated" (via a RRIP 'CL' or
	 * 'PL' SUF) then build a new Inode from the "relocated"
	 * data and fix things up with the desired FID.  Otherwise
	 * we "hold" on to the vnode we already have and return it.
	 *
	 * Note: A "relocated" inode/vnode is implicitly "locked and held"
	 * via the (recursive) call to cdfs_GetInode().
	 */
	if ((ip->i_Flags & CDFS_INODE_RRIP_REL) != 0) {

		tmp_fid.fid_SectNum = (ip->i_DirRec)->drec_ExtLoc;
		tmp_fid.fid_Offset = ISO_DOT_OFFSET;
		retval = cdfs_GetInode(vfsp, &tmp_fid, NULL, &tmp_ip);

		/*
		 * If the new Inode was successfully built, then
		 * abandon the original Inode and fixup the new Inode
		 * (including the Hash Table) with the desired FID.
		 *
		 * Note: Since we've gotten here, we are building
		 * the original Inode from scratch, which means that
		 * no one else is referencing it, except for someone
		 * sleeping on the inode lock,
		 * via cdfs_FindInode().  Therefore, as long as
		 * cdfs_FindInode() verifies that the Inode's ID has
		 * not change while sleeping, then there is no
		 * problem shuffling Inode ID's and cleaning up
		 * the original Inode and shuffling
		 * around Inode ID's.
		 */
		if (retval == RET_OK) {
			s = LOCK(&cdfs_inode_table_mutex, FS_CDFSLISTPL);
			cdfs_IrmHash(tmp_ip);
			tmp_ip->i_Fid = ip->i_Fid;
			tmp_ip->i_ParentFid = ip->i_ParentFid;
			cdfs_IputHash(tmp_ip);
			vp = ITOV(ip);
			pages = vp->v_pages;
			cdfs_CleanInode(ip, 1);
			cdfs_IputFree(ip);
			cdfs_InodeFree = ip;
			UNLOCK(&cdfs_inode_table_mutex, s);
			CDFS_SLEEP_UNLOCK(ip);
			if (pages != NULL){
				vp->v_pages = pages;
				pvn_abort_range(vp, 0, 0);
			}
			ip = tmp_ip;
		}
	} else {
		VN_HOLD(ITOV(ip));
		CDFS_SLEEP_UNLOCK(ip);
	}

Cleanup:
	/*
	 * If a temp IOBUF was allocated, then release it.
	 */
	if (drec_buf == &tmp_drec_buf) {
		CDFS_RELEASE_IOBUF(drec_buf);
	}

	/*
	 * The inode is not valid, so clean it up and
	 * put it at HEAD of Free list.
	 */
	if (retval != RET_OK) {
		s= LOCK(&cdfs_inode_table_mutex, FS_CDFSLISTPL);
		vp = ITOV(ip);
		pages = vp->v_pages;
		cdfs_CleanInode(ip, 1);
		cdfs_IputFree(ip);
		cdfs_InodeFree = ip;
		UNLOCK(&cdfs_inode_table_mutex, s);
		CDFS_SLEEP_UNLOCK(ip);
		if (pages != NULL) {
			vp->v_pages = pages;
			pvn_abort_range(vp, 0, 0);
		}
		ip = NULL; 
	}

	*ipp = ip;
	return(retval);
}



/*
 * int
 * cdfs_AllocDrec(cdfs_drec_t **drec)
 * 	Allocate an empty  Dir Rec structure from the free list.
 *
 * Calling/Exit State:
 * 	No locks are held on entry or exit.
 */
STATIC int
cdfs_AllocDrec(cdfs_drec_t **drec)
{
	pl_t	s;

	if (cdfs_DrecFree == NULL) {
		/*
		 *+ the system is out of memory.
		 */
		cmn_err(CE_WARN,
			"cdfs_AllocDrec(): No more multi-extent Directory Records.");
		cmn_err(CE_CONT,
			"The CDFS Directory Record Cache may be too small\n\n");
		return(ENOMEM);
	}

	*drec = cdfs_DrecFree; 
	s  =LOCK(&cdfs_inode_table_mutex, FS_CDFSLISTPL);
	cdfs_DrecRm(&cdfs_DrecFree, *drec);
	UNLOCK(&cdfs_inode_table_mutex, s);
	return(RET_OK);
}



/*
 * STATIC int
 * cdfs_MergeDrec(cdfs_inode_t *ip, cdfs_drec_t *drec)
 * 	Merge the Dir Rec data into an Inode structure.
 *
 * Calling/Exit State:
 * 	Inode sleep lock is held on entry and exit.
 */
STATIC int
cdfs_MergeDrec(cdfs_inode_t *ip, cdfs_drec_t *drec)
{
	pl_t	s;
	/*
	 * Add the Dir Rec to tail of the Inode's Dir Rec list.
	 */
	s = LOCK(&cdfs_inode_table_mutex, FS_CDFSLISTPL);
	cdfs_DrecPut(&ip->i_DirRec, drec);
	UNLOCK(&cdfs_inode_table_mutex, s);
	CDFS_ILOCK(ip);
	ip->i_DRcount++;

	if (ip->i_DRcount == 1) {
		/*
		 * For the first Dir Rec, use the Dir Rec data to
		 * set the appropriate Inode fields.
		 * - Set the appropriate Inode flags.
		 * - Set the file type.
		 * - Set # of bytes in file.
		 * - Set Access/Modify/Create date and time.
		 */
		ip->i_Flags |= (
			(((drec->drec_Flags & CDFS_DREC_EXIST) == 0) ?
				CDFS_INODE_HIDDEN : 0) ||
			(((drec->drec_Flags & CDFS_DREC_ASSOC) == 0) ?
				0 : CDFS_INODE_ASSOC)
		);

		if ((drec->drec_Flags & CDFS_DREC_DIR) == 0) {
			ip->i_Mode = IFREG;
			ip->i_LinkCnt = 1;
		} else {
			ip->i_Mode = IFDIR;
			ip->i_LinkCnt = 2;
		}

		ip->i_Size = drec->drec_DataLen;

		ip->i_AccessDate = drec->drec_Date;
		ip->i_ModDate = drec->drec_Date;
		ip->i_CreateDate = drec->drec_Date;

	} else {
		/*
		 * Check for consistency between Dir Recs of a multi-extent file.
		 * - Verify the flags fields are identical.
		 * - Update the size of the file.
		 * - Update the time-stamps to the most recent dates. 
		 *
		 * - Additional checks should be added to ensure
		 * consistency between multi-extent files.
		 */
		if ((drec->drec_Flags & ~CDFS_DREC_MULTI) != 
			((drec->drec_PrevDR)->drec_Flags & ~CDFS_DREC_MULTI)) {
			/* 
			 *+ Inconsistency between Dir Recs of a multi-extent
			 *+ file while building the DREC of the inode.
			 */ 
			cmn_err(CE_NOTE,
				"cdfs_MergeDrec(): Inconsistent Directory Record type:");
			cmn_err(CE_CONT,"Device= 0x%x     Sector=%d     Offset=%d\n\n",
				CDFS_DEV(ip->i_vfs), ip->i_Fid.fid_SectNum,
				ip->i_Fid.fid_Offset);
		}

		ip->i_Size += drec->drec_DataLen;

		/*
		 * Use the latest date of all Dir Recs in this file.
		 */
		if ((drec->drec_Date.tv_sec > ip->i_AccessDate.tv_sec) ||
			(drec->drec_Date.tv_nsec > ip->i_AccessDate.tv_nsec)) {
			ip->i_AccessDate = drec->drec_Date;
			ip->i_ModDate = drec->drec_Date;
			ip->i_CreateDate = drec->drec_Date;
		}
	}

	CDFS_IUNLOCK(ip);
	return(RET_OK);
}



/*
 * STATIC int
 * cdfs_GetXar(vfs_t *vfsp, cdfs_drec *drec, cdfs_xar *xar)
 * 	Get the XAR associated with a Dir Rec (File Section) and
 * 	merge its data into the Inode structure.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */
STATIC int
cdfs_GetXar(vfs_t *vfsp, cdfs_drec_t *drec, cdfs_xar_t *xar)
{
	union media_xar		*xar_m;
	struct buf		*bp;

	/*
	 * Read in XAR sector.
	 *
	 * Currently we only care about the "fixed" portion
	 * of the media-resident XAR.  Hence, we only read in the
	 * one block's worth of data from the media.  If we ever
	 * need to worry about the "variable-length" portion of the
	 * XAR (e.g. Escape Sequence/Application Use Data), then
	 * additional buffer space needs to be allocated (and release
	 * via cdfs_CleanInode()) and additional block of data need
	 * to be read.
	 */
	bp = bread(CDFS_DEV(vfsp), LTOPBLK(drec->drec_ExtLoc,CDFS_BLKSZ(vfsp)),
				CDFS_BLKSZ(vfsp));
	if ((bp->b_error & B_ERROR) != 0) {
		brelse(bp);
		return(EIO);
	}
	
	xar_m = (union media_xar *)bp->b_un.b_addr;
	cdfs_ConvertXar(xar, xar_m, CDFS_TYPE(vfsp));
	
	brelse(bp);
	return(RET_OK);
}




/*
 * STATIC int
 * cdfs_MergeXar(cdfs_inode_t *ip, cdfs_xar_t *xar) 
 * 	Merge the XAR information into the Inode structure.
 *
 * Calling/Exit State:
 *	Inode sleep lock is held on entry and exit.
 */
int
cdfs_MergeXar(cdfs_inode_t *ip, cdfs_xar_t *xar)
{
	ip->i_Xar = xar;

	/*
	 * Merge the XAR's UID, GID, and Permission bits into the inode.
	 *
	 * Note: Per ISO-9660 Section 9.1.6, the XAR's UID, GID, and PERMS
	 * are only valid if the PROTECTION bit is set in the Dir Rec
	 * associated with this XAR (assumed to be the last Dir Rec processed).
	 */
	if (((ip->i_DirRec->drec_PrevDR)->drec_Flags & CDFS_DREC_PROTECT) != 0) {
		/*
		 * Merge in the XAR's UID and GID.
		 * Note: Per ISO-9660 specification Section 9.5.1 and 9.5.2,
		 * a UID/GID of 0 means the UID/GID is undefined.  Additionally,
		 * if the GID is undefined, then the UID is also not undefined.
		 */
		CDFS_ILOCK(ip);
		if (xar->xar_GroupID != 0) {
			if (xar->xar_UserID != 0) {
				ip->i_Flags |= CDFS_INODE_UID_OK;
				ip->i_UserID = xar->xar_UserID;
			}
			ip->i_Flags |= CDFS_INODE_GID_OK;
			ip->i_GroupID = xar->xar_GroupID;
		}

		/*
		 * Merge in the XAR permissions.
		 */
		ip->i_Flags |= CDFS_INODE_PERM_OK;
		CDFS_IUNLOCK(ip);
		ip->i_Mode &= ~(
			IREAD_USER | IWRITE_USER | IEXEC_USER |
			IREAD_GROUP | IWRITE_GROUP | IEXEC_GROUP |
			IREAD_OTHER | IWRITE_OTHER | IEXEC_OTHER
		);
		ip->i_Mode |= (
			(((xar->xar_Perms & CDFS_XAR_OWNREAD) == 0) ? 0 : IREAD_USER) |
			(((xar->xar_Perms & CDFS_XAR_OWNEXEC) == 0) ? 0 : IEXEC_USER) |
			(((xar->xar_Perms & CDFS_XAR_GROUPREAD) == 0) ? 0 : IREAD_GROUP) |
			(((xar->xar_Perms & CDFS_XAR_GROUPEXEC) == 0) ? 0 : IEXEC_GROUP) |
			(((xar->xar_Perms & CDFS_XAR_OTHERREAD) == 0) ? 0 : IREAD_OTHER) |
			(((xar->xar_Perms & CDFS_XAR_OTHEREXEC) == 0) ? 0 : IEXEC_OTHER)
		);
	}

	/*
	 * Merge in the XAR's time-stamp values.
	 * Note: Per ISO-9660 section 8.4.26.1, a value of zero means
	 * the time-stamp is not specified.
	 */
	if ((xar->xar_CreateDate.tv_sec != 0) ||
		(xar->xar_CreateDate.tv_nsec != 0)) {
		ip->i_CreateDate = xar->xar_CreateDate;
	}

	if ((xar->xar_ModDate.tv_sec != 0) ||
		(xar->xar_ModDate.tv_nsec != 0)) {
		ip->i_ModDate = xar->xar_ModDate;
	}

	if ((xar->xar_EffectDate.tv_sec != 0) ||
		(xar->xar_EffectDate.tv_nsec != 0)) {
		ip->i_EffectDate = xar->xar_EffectDate;
	}

	if ((xar->xar_CreateDate.tv_sec != 0) ||
		(xar->xar_CreateDate.tv_nsec != 0)) {
		ip->i_ExpireDate = xar->xar_ExpireDate;
	}

	return(RET_OK);
}


/*
 * int
 * cdfs_iinactive(cdfs_inode_t *ip)
 * 	Vnode is no longer referenced
 * 	deallocate the file.
 * 
 * Calling/Exit State:
 *	
 */
void
cdfs_iinactive(cdfs_inode_t *ip)
{
	vnode_t		*vp;
	pl_t		s;
	page_t		*pages;

	vp = ITOV(ip);

	CDFS_SLEEP_LOCK(ip);
	/*
	 * It's now impossible for any new reference to be acquired via
	 * iget(), since any attempt to generate such a reference will
	 * block. Futhermore, if the reference count on the vnode is 
	 * exactly 1, we are the only one who has a hold on the vnode,
	 * and no one can established as long as hold the inode lock. 
	 */
	VN_LOCK(vp);
	if (vp->v_count != 1){
		/*
		 * The new reference is still extant and the vnode has
		 * become someone else's responsibility. Give up our reference.
		 */
		vp->v_count--;
		VN_UNLOCK(vp);
		CDFS_SLEEP_UNLOCK(ip);
		return;
	}
	/*
 	 * The reference count is exactly 1, and once again we hold
	 * the last reference.
	 */
	/*
	 * CDFS_WRITE_SUPPORT:
	 * Mark iinactive in progress.	This allow VOP_PUTPAGE to abort
	 * a concurrent attempt to flush a page due to pageout/fsflush.
	 */
	vp->v_count = 0;
	VN_UNLOCK(vp);
	ASSERT(vp->v_count == 0);
	
	/*
	 * We hold the last reference and it cannot be changed while we
	 * hold the inode sleep lock. Once the reference is clear, we need
	 * to hold the inode list lock to check if any one is waiting for it. 
	 */
	s = LOCK(&cdfs_inode_table_mutex, FS_CDFSLISTPL);
	/*
	 * Someone may previously have blocked in iget() attempting to
	 * accquired the inode lock; If so, they will establish a new 
	 * reference to the inode and we can leave them unmolested to do so.
	 * Buf if not, we return the inode to the freelist and cleanup.
	 * Note that as long as we hold the inode list lock no one can find
	 * the inode but as soon as we drop the inode list lock iget()
	 * may find the inode but will block until we release the inode
	 * sleep lock. The worst that can happen is that someone may 
	 * acquire the inode after cleanup but this is benign.
	 */
	if (CDFS_SLEEPLOCK_BLKD(ip)) {
		UNLOCK(&cdfs_inode_table_mutex, s);
		CDFS_SLEEP_UNLOCK(ip);
		return;
	}
		
	pages = vp->v_pages;

	/*
	 * Release the Inode:
	 * - put the inode on the hash-list.
	 * - Put the Inode on the Free-list.
	 * If the inode requires a small amount of cleanup (it has
	 * no data pages, or it is not a complete/valid inode), then
	 * put it at the head of the freelist.
	 */
	cdfs_CleanInode(ip, 1);
	cdfs_IputFree(ip);
	if (pages == NULL) {
		cdfs_InodeFree = ip;
	}	
	ip->i_Flags |= IFREE;

	MET_INODE_INUSE(MET_OTHER, -1);
	UNLOCK(&cdfs_inode_table_mutex, s);

	CDFS_SLEEP_UNLOCK(ip);

	/* Invalidate all pages for this vnode */
	if (pages != NULL) {
		vp->v_pages = pages;
		pvn_abort_range(vp, 0 ,0 );
	}
	ASSERT(vp->v_pages == NULL);

	return;
}



/*
 * int
 * cdfs_FlushInodes(vfs_t *vfsp)
 * 	Remove any inodes in the inode cache belonging to dev
 *
 * Calling/Exit State:
 *	
 * 	There should be only one active inode which is rootinode,
 *	return error if any are found but still invalidate others
 * 	(N.B.: this is a user error, not a system error).
 *
 * Also, count the references to dev by block devices - this really
 * has nothing to do with the object of the procedure, but as we have
 * to scan the inode table here anyway, we might as well get the
 * extra benefit.
 *
 * This is called from umount1()/cdfs_vfsops.c when dev is being unmounted.
 */
int
cdfs_FlushInodes(vfs_t *vfsp)
{
	struct cdfs_inode	*ip, *ipx, *hip;
	struct vnode		*vp;
	struct vnode		*rvp;
	uint_t			i;
	pl_t			s;
	int			retval =0;
	page_t			*pages;

	rvp = CDFS_ROOT(vfsp);

	for (i = 0; i < cdfs_IhashCnt; i++) {
	    hip = cdfs_InodeHash[i];
	    s = LOCK(&cdfs_inode_table_mutex, FS_CDFSLISTPL);
loop:
	    if ((ip = hip) != NULL) {
	        do {
			ipx = ip->i_HashFwd;
			/*
		 	* Examine all Inode belonging to this CDFS instance.
		 	* All Inode of this CDFS should not be referenced.
		 	*/
			if (ip->i_vfs == vfsp) {
				/*
			 	 * Note: The ROOT Inode will still be
				 * referenced and shoule have a 
			 	 * reference count of exactly 2.
			 	 */
				vp = ITOV(ip);
				if (vp == rvp) {
					if (vp->v_count > 2) {
						UNLOCK(&cdfs_inode_table_mutex, s);
						return (RET_ERR);
					}
					continue;
				}
				if ((ip->i_Flags & IFREE) == 0) {
					/*
				 	* Set error indicator for return value,
				 	* but continue invalidating other inodes
				 	*/
					retval = -1;
				} else {
	
					/*
			 		* Cleanup the Inode
			 		*/
					pages = vp->v_pages;

					cdfs_CleanInode(ip, 1);
					cdfs_IputFree(ip);
	
					if (pages != NULL){
						UNLOCK(&cdfs_inode_table_mutex, s);
						vp->v_pages = pages;
						pvn_abort_range(vp, 0, 0);
						s = LOCK(&cdfs_inode_table_mutex, FS_CDFSLISTPL);
						if (ip->i_HashFwd != ipx)
							goto loop;
					}
				}
			}

		} while (ip != hip);
	    }
	    UNLOCK(&cdfs_inode_table_mutex, s);
	}
	return(retval);
}



/*
 * int
 * cdfs_iaccess(vfs_t *vfs, cdfs_inode_t *ip, mode_t mode, cred_t *cr)
 * 	Check mode permission on inode.
 * 
 * Calling/Exit State:
 *
 * Description: 
 *	Mode is READ, WRITE or EXEC.
 * 	In the case of WRITE, the read-only status of the file system
 * 	is checked.  The mode is shifted to select the owner/group/other
 * 	fields.  The super user is granted all permissions except
 * 	writing to read-only file systems.
 */
int
cdfs_iaccess(vfs_t *vfs, cdfs_inode_t *ip, mode_t mode, cred_t *cr)
{
	mode_t		imode;
	int		lshift, i, denied_mode;

	/*
	 * Disallow write attempts on read-only
	 * file systems, unless the file is a block
	 * or character device or a FIFO.
	 * Note: Currently, CDFS is ALWAYS Read-Only.
	 */
	if ((mode & IWRITE) != 0) {
		if ((vfs->vfs_flag & VFS_RDONLY) != 0) { 
			if ((ip->i_Mode & IFMT) != IFCHR &&
			    (ip->i_Mode & IFMT) != IFBLK &&
			    (ip->i_Mode & IFMT) != IFIFO) {
				return(EROFS);
			}
		}
	}
	

	/*
	 * Access check is based on only one of owner, group, public.
	 * If not owner, then check group perms.
	 * If not a member of the group, then
	 * check public access.
	 */
	if (cr->cr_uid == cdfs_GetUid(vfs, ip)) {
		mode >>= IUSER_SHIFT;
		lshift = 0;
	} else if (groupmember(cdfs_GetGid(vfs,ip), cr) != 0) {
		mode >>= IGROUP_SHIFT;
		lshift = IGROUP_SHIFT;
	} else {
		mode >>= IOTHER_SHIFT;
		lshift = IOTHER_SHIFT;
	}

	imode = cdfs_GetPerms(vfs, ip);
	if ((i = (imode & mode)) == mode) {
		return(RET_OK);
	}
#ifdef is286EMU
	if ((is286EMUL != 0) && ((imode & IEXEC) == IEXEC)) {
		return(RET_OK);
	}

#endif	/* is286EMU */

	denied_mode = (mode & (~i));
	denied_mode <<= lshift;
	if ((denied_mode & (IREAD | IEXEC)) && pm_denied(cr, P_DACREAD))
		return (EACCES);
	if ((denied_mode & IWRITE) && pm_denied(cr, P_DACWRITE))
		return (EACCES);
	return(RET_OK);
}






/*
 * void
 * cdfs_IputFree(cdfs_inode_t *ip)
 * 	Put an Inode into the Free-list.
 * 
 * Calling/Exit State:
 *	Inode list lock is held on entry and exit.
 */
void
cdfs_IputFree(cdfs_inode_t *ip)
	/* Inode to be added			*/
{
	CDFS_LIST_PUT(&cdfs_InodeFree, ip, i_FreeFwd, i_FreeBack);
	return;
}

/*
 * void
 * cdfs_IrmFree(cdfs_inode_t *ip)
 * 	Remove an Inode from the Free-list.
 * 
 * Calling/Exit State:
 *	Inode list lock is held on entry and exit.
 */
void
cdfs_IrmFree(cdfs_inode_t *ip)
	/* Inode to be removed			*/
{
	CDFS_LIST_RM(&cdfs_InodeFree, ip, i_FreeFwd, i_FreeBack);
	return;
}

/*
 * void
 * cdfs_IputHash(cdfs_inode_t *ip)
 * 	Added the inode the hashlist.
 * 
 * Calling/Exit State:
 *	Inode list lock is held on entry and exit.
 */
void
cdfs_IputHash(cdfs_inode_t *ip)
	/* Inode to be added			*/
{
	cdfs_inode_t	**hash;		/* Hash-list Head	*/
	
	hash = &cdfs_InodeHash[CDFS_INOHASH(&ip->i_Fid)];

	CDFS_LIST_PUT(hash, ip, i_HashFwd, i_HashBack);
	return;
}

/*
 * void
 * cdfs_IrmHash(cdfs_inode_t *ip)
 * 	Remove an Inode from the specified Hash-list.
 * 
 * Calling/Exit State:
 *	Inode list lock is held on entry and exit.
 */
void
cdfs_IrmHash(cdfs_inode_t *ip)
	/* Inode to be removed			*/
{
	struct cdfs_inode	**hash;		/* Hash-list Head	*/

	hash = &cdfs_InodeHash[CDFS_INOHASH(&ip->i_Fid)];
	CDFS_LIST_RM(hash, ip, i_HashFwd, i_HashBack);
	return;
}



/*
 * void
 * cdfs_DrecPut(cdfs_drec_t **head, cdfs_drec_t *drec)
 *
 * Calling/Exit State:
 * 	The inode list lock on entry and exit.
 */
void
cdfs_DrecPut(cdfs_drec_t **head, cdfs_drec_t *drec)
	/* Head of Dir Rec list			*/
	/* Dir Rec to be freed			*/
{
	CDFS_LIST_PUT(head, drec, drec_NextDR, drec_PrevDR);
	return;
}



/*
 * void
 * cdfs_DrecRm(cdfs_drec_t **head, cdfs_drec_t *drec)
 * 	Remove a Dir Rec from the Free-list.
 *
 * Calling/Exit State:
 * 	The inode list lock on entry and exit.
 */
void
cdfs_DrecRm(cdfs_drec_t **head, cdfs_drec_t *drec)
	/* Head of Dir Rec list			*/
	/* Dir Rec to be removed		*/
{
	CDFS_LIST_RM(head, drec, drec_NextDR, drec_PrevDR);
	return;
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_cdfs_hash(void)
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger.
 */
void
print_cdfs_hash(void)
{
	cdfs_inode_t *ip, *hash;
	int i;

	for (i = 0; i < cdfs_IhashCnt; i++) {
		hash = cdfs_InodeHash[i];
		if ((ip = hash) == NULL)
			continue;
		do { 
			debug_printf("ip = 0x%x, iflag = %8x,"
				      " fid = %d, vcount = %d\n",
				     ip, ip->i_Flags, ip->i_Fid.fid_SectNum,
				     ip->i_VnodeStorage.v_count);
			debug_printf("\tlinks = %d, imode = %06o, isize = %d,"
				      " drcount = %d\n",
				     ip->i_LinkCnt, ip->i_Mode, ip->i_Size,
				     ip->i_DRcount);
			debug_printf("\tNextBytes = %d, DirOffset = %d\n",
				     ip->i_NextByte, ip->i_DirOffset); 
			if (debug_output_aborted())
				break;
			ip = ip->i_HashFwd;
		} while (ip != hash);
	}
	return;
}

/*
 * void
 * print_cdfs_freelist(void)
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger.
 */
void
print_cdfs_freelist(void)
{
	cdfs_inode_t	*ip;
	int i = 0;
	
	ip = cdfs_InodeFree;
	if (ip != NULL) {
	    do {
		i++;
		debug_printf("ip = 0x%x, iflag = %8x, fid = %d, vcount = %d\n",
			     ip, ip->i_Flags, ip->i_Fid.fid_SectNum,
			     ip->i_VnodeStorage.v_count);
		debug_printf("\tlinks = %d, imode = %06o, isize = %d,"
			      " drcount = %d\n",
			     ip->i_LinkCnt, ip->i_Mode, ip->i_Size,
			     ip->i_DRcount);
		if (debug_output_aborted())
			break;
		ip = ip->i_FreeFwd;
	    } while (ip != cdfs_InodeFree);
	}
	return;
}


static char *vtype_name[] = {
        "NON", "REG", "DIR", "BLK", "CHR", "LNK", "FIFO", "XNAM", "BAD"
};
#define N_VTYPE (sizeof(vtype_name) / sizeof(char *))

/*
 * void
 * print_cdfs_inode(print_cdfs_inode *ip)
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger.
 */
/* ARGSUSED */
void
print_cdfs_inode(const struct cdfs_inode *ip)
{
	struct	vnode	*vp;
	vtype_t	vtype;


	debug_printf("Flags:   %8x   Sector:    %8d   Offset: %8d    Vnode: %8x\n",
	     ip->i_Flags, ip->i_Fid.fid_SectNum, ip->i_Fid.fid_Offset,
		 ip->i_Vnode);

	debug_printf( "links:   %8x   UserID:   %8x   GroupID: %8x   Size: %8x\n",
	     ip->i_LinkCnt, ip->i_UserID, ip->i_GroupID, ip->i_Size);

	debug_printf("mode:      %06o   Version:  %8x   DirOffset:  %8x\n",
	     ip->i_Mode, ip->i_VerCode, ip->i_DirOffset);

	debug_printf("mapcnt:  %8x   NextByte: %8x   DRcount:  %d\n",
	     ip->i_mapcnt, ip->i_NextByte, ip->i_DRcount);

	debug_printf("DirRec:  %8x   Xar:      %8x    Rrip:    %8x\n",
	     ip->i_DirRec, ip->i_Xar, ip->i_Rrip);

	vp = ip->i_Vnode;
	vtype = vp->v_type;
	debug_printf("vnode struct @ %x:\n", vp);

	debug_printf("type: %-2x - %-8s count:   %8x   vdata:   %8x   flag:    %8x\n",
		vtype, (u_int)vtype >= N_VTYPE ? "???" : vtype_name[vtype],
		vp->v_count, vp->v_data, vp->v_flag);

	debug_printf("vfsp:    %8x   vfsmountedhere:  %8x               stream:  %8x\n",
		vp->v_vfsp, vp->v_vfsmountedhere, vp->v_stream);
	debug_printf("pages:   %8x   filocks: %8x   rdev: %d,%d\n",
		vp->v_pages, vp->v_filocks, getemajor(vp->v_rdev), geteminor(vp->v_rdev));

	debug_printf("lid:     %8x   macflag: %8x    softcnt:   %8x\n",
			vp->v_lid, vp->v_macflag, vp->v_softcnt);
}

#endif /* DEBUG || DEBUG_TOOLS */
