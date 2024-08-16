/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/cdfs/cdfs_bmap.c	1.2"

#include <util/types.h>
#include <util/param.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <proc/user.h>
#include <fs/vnode.h>
#include <fs/buf.h>
#include <proc/disp.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <mem/seg.h>
#include <mem/page.h>
#include <fs/cdfs/cdfs_hier.h>
#include <fs/cdfs/cdfs_data.h>
#include <fs/cdfs/cdfs_inode.h>
#include <fs/cdfs/cdfs_fs.h>
#include <fs/cdfs/cdfs.h>
#include <svc/errno.h>
#include <util/sysmacros.h>
#include <fs/vfs.h>
#include <util/cmn_err.h>
#include <util/debug.h>

/*
 * int
 * cdfs_bmappage(vnode_t *vp, off_t off, size_t len, page_t **ppp,
 *	daddr_t *io_list, int rw)
 * 	Convert the File-releative byte offset to the
 * 	corresponding Media-relative physical block #/offset. 
 *
 * Calling/Exit State:
 * 	No locks are held on entry or exit.
 *	
 *	Returns 0 on success, or a non-zero errrno if an error occurs.
 *
 * Description:
 *
 *      If the caller is cdfs_getapage, the existing block numbers are
 *      returned via io_list for the caller to do io from the backing
 *      store to fill the page.
 *
 *      Note: we are passing the exact range to bmappage(). Therefore,
 *      'off' is not page-aligned, nor is it block-aligned.
 *
 * Note: The last byte of each File Section need not
 * 	end on a block boundry.  Thus, for multi-extent files,
 * 	a File-relative logical block boundry do not necessarily
 * 	correspond to a physical block boundry.
 */
/* ARGSUSED */
int
cdfs_bmappage(vnode_t *vp, off_t off, size_t len, page_t **ppp,
	daddr_t *io_list, int rw)
{
	cdfs_drec_t	*drec;		/* Dir Rec structure	*/
	uint_t		bytecnt;	/* Cummulative byte count */
	uint_t		blkcnt;		/* Cummulative blk cnt in D-Rec	*/
	uint_t		unit_size;	/* # bytes in a file unit */
	uint_t		cmpl_units;	/* # of complete File Units */
	uint_t		end;		/* End of continuous file data*/
	daddr_t		lbn, lastlbn;
	long		bsize, bshift;
	off_t		eoff;

	vfs_t		*vfsp;
	cdfs_inode_t	*ip = VTOI(vp);


	ASSERT(rw == S_READ || rw == S_OTHER);
	
	vfsp = vp->v_vfsp;
	bshift = CDFS_BLKSHFT(vfsp);
	bsize = CDFS_BLKSZ(vfsp);
	ASSERT((bsize > PAGESIZE) ? off >> bshift == (off + len -1) >> bshift
			 : off >> PAGESHIFT == (off + len -1) >> PAGESHIFT);
	
	lbn = pagerounddown(off) >> bshift;
	lastlbn = lbn + (PAGEOFFSET >> bshift);
	eoff = off + len;
	/*
	 * Round eoff up to the isize so we can use it to 
	 * compute the end of the backing store allocation.
	 */
	 if (eoff < ip->i_Size) {
		 eoff = ip->i_Size;
	 }

	drec = ip->i_DirRec;
	ASSERT(drec != NULL);

	bytecnt = 0;
	while (lbn <= lastlbn) {
		for (;;) {
			if (bytecnt + drec->drec_DataLen >= off)
				break;
			bytecnt += drec->drec_DataLen;
			drec = drec->drec_NextDR;
			if (drec == ip->i_DirRec)
				return(RET_EOF);
		}

		/*
	 	* We've found the Dir Rec that contains the desired data.
	 	* - Initialize the block counter to the # of blocks in the
	 	*	 Extent preceeding the file data.
	 	* - Initialize the ending file offset to the end of the
	 	*	 continuous area that contains the file data, i.e. the
	 	*	 end of this File Section.
	 	*/
		blkcnt = drec->drec_XarLen;
		end = bytecnt + drec->drec_DataLen;
	
		/*
	 	* If this is an interleaved Extent, locate the File Unit that
	 	* contains the desired location.
	 	* - Compute the # of bytes in a File Unit.
	 	* - Compute the # of complete File Units to skip.
	 	* - Increment the byte count by the # bytes in the complete/full
	 	*   File Units. Note: Interleave blocks do not contain file data.
	 	* - Recompute the File Offset of the end of the continuous area
	 	*   that contains the file data.  This continuous area will be
	 	*   limited by the end of the current File Unit.
	 	* - Increment the block count by the # of blocks consumed by the
	 	*    complete/full File Units, including the interleave blocks.
	 	*/
		if (drec->drec_UnitSz != 0) {
			unit_size = drec->drec_UnitSz << bshift;
			cmpl_units = (off - bytecnt) / unit_size;
			bytecnt += (cmpl_units * unit_size);
			end = MIN(end, bytecnt + unit_size);
			blkcnt += ((drec->drec_UnitSz + drec->drec_Interleave) * cmpl_units);
		}
	
		/*
	 	* Now that we've located a continuous set of physical blocks
	 	* that contains the desired offset, the desired physical block #
	 	* and offset are straight-forward:
	 	* - Compute the physical block #.
	 	* - Since the current byte counter is pointing to the beginning
	 	*	 of a block, the final block offset is simply the
		* 	 non-block portion of the remaining # of byte.
	 	*/
		if (io_list != NULL) {
			*io_list = drec->drec_ExtLoc + blkcnt +
				((off - bytecnt) >> bshift);
		}
	
		off += bsize;
		io_list++;
		lbn++;
		if (off >= eoff) {
			*io_list = DB_HOLE;
			break;
		}
	}

	return(RET_OK);
}


/*
 * int cdfs_bmap_sect(vfs, ip, offset, blkno, blkoff, lastcont)
 *	Map the offset to logical block number.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exits.
 */
int
cdfs_bmap_sect(vfs_t *vfs, cdfs_inode_t *ip, ulong_t offset,
	 daddr_t *blkno, uint_t *blkoff, uint_t *lastcont)
{
	cdfs_drec_t	*drec;		/* Dir Rec structure	*/
	uint_t		bytecnt;	/* Cummulative byte count */
	uint_t		blkcnt;		/* Cummulative blk cnt in D-Rec	*/
	uint_t		unit_size;	/* # bytes in a file unit */
	uint_t		cmpl_units;	/* # of complete File Units */
	uint_t		end; 		/* End of continuous file data*/
	long		bshift;

	/*
	 * Locate the Dir Rec that contains the desired location.
	 */
	if (offset >= ip->i_Size) {
		return(RET_EOF);
	}

	drec = ip->i_DirRec;
	ASSERT(drec != NULL);

	bshift = CDFS_BLKSHFT(vfs);
	bytecnt = 0;
	for (;;) {
		if (bytecnt + drec->drec_DataLen > offset) {
			break;
		}

		bytecnt += drec->drec_DataLen;
		drec = drec->drec_NextDR;

		if (drec == ip->i_DirRec) {
			return(RET_EOF);
		}
	}

	/*
	 * We've found the Dir Rec that contains the desired data.
	 * - Initialize the block counter to the # of blocks in the
	 *	 Extent preceeding the file data.
	 * - Initialize the ending file offset to the end of the
	 *	 continuous area that contains the file data, i.e. the
	 *	 end of this File Section.
	 */
	blkcnt = drec->drec_XarLen;
	end = bytecnt + drec->drec_DataLen;

	/*
	 * If this is an interleaved Extent, locate the File Unit that
	 * contains the desired location.
	 * - Compute the # of bytes in a File Unit.
	 * - Compute the # of complete File Units to skip.
	 * - Increment the byte count by the # bytes in the complete/full
	 *	 File Units.  Note: Interleave blocks do not contain file data.
	 * - Recompute the File Offset of the end of the continuous area
	 *	 that contains the file data.  This continuous area will be
	 *	 limited by the end of the current File Unit.
	 * - Increment the block count by the # of blocks consumed by the
	 *	 complete/full File Units, including the interleave blocks.
	 */
	if (drec->drec_UnitSz != 0) {
		unit_size = drec->drec_UnitSz << bshift;
		cmpl_units = (offset - bytecnt) / unit_size;
		bytecnt += (cmpl_units * unit_size);
		end = MIN(end, bytecnt + unit_size);
		blkcnt += ((drec->drec_UnitSz + drec->drec_Interleave) * cmpl_units);
	}

	/*
	 * Now that we've located a continuous set of physical blocks
	 * that contains the desired offset, the desired physical block #
	 * and offset are straigh-forward:
	 * - Compute the physical block #.
	 * - Since the current byte counter is pointing to the beginning
	 *	 of a block, the final block offset is simply the non-block
	 *	 portion of the remaining # of byte.
	 */
	if (blkno != NULL) {
		*blkno = drec->drec_ExtLoc + blkcnt +
			((offset - bytecnt) >>  bshift);
	}

	if (blkoff != NULL) {
		*blkoff = ((offset - bytecnt) & CDFS_BLKMASK(vfs));
	}

	if (lastcont != NULL) {
		*lastcont = end;
	}

	return(RET_OK);
}


