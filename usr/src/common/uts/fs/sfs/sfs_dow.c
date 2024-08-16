/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/sfs/sfs_dow.c	1.5"
#ident	"$Header: $"

#include <util/types.h>
#include <fs/sfs/sfs_dow.h>
#include <fs/sfs/sfs_hier.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/fbuf.h>
#include <mem/seg_map.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/sysmacros.h>

/*
 * dow utility routines used by sfs.  The routines in this file, as
 *	well as sfs generally, follow certain conventions:
 *
 *	(1) A global variable, sfs_dow_enabled, controls whether
 *		or not dow allocation is enabled for sfs file
 *		systems in general.
 *
 *	(2) Dow allocation for an individual sfs file system is enabled
 *		only if the SFS_DOWENABLED flag is set for that file
 *		system.  This flag is normally set at mount time and
 *		cleared at unmount time, but is will also be cleared
 *		if or when:
 *			(a) the SFS_FSINVALID flag is set.
 *			(b) the file system is mounted read-only.  Such
 *				file systems throw away their modifications,
 *				so there is no point in allocating dows
 *				for such a file system.
 *			(c) the file system is mounted soft (VNOSYNC flag
 *				is set).  For such a file system, the order
 *				in which data is written to disk is unimportant.
 *
 *	(3) If a dow cannot be allocated to correspond to an action,
 *		then sfs handles the action synchronously.  The routines
 *		in this file utilize that fact to handle actions synchronously
 *		when a dow cannot be allocated.  For example, in sfs_dow_iupdat,
 *		the inode will be written synchronously if a dow cannot
 *		be allocated for it; the caller will receive a dowid of
 *		DOW_NONE, and thus will know the inode was written
 *		synchronously.  Similarly, in sfs_dow_order, if the
 *		depedendent dow is DOW_NONE, then the antecedent dow
 *		will be handled synchronously prior to returning from
 *		sfs_dow_order, thus enforcing the ordering with respect
 *		to whatever action is to be done next.
 *
 *	(4) There are several functions which have a dowid_t pointer
 *		as an argument, and on success, return a dowid_t stored
 *		in the area addressed by the pointer.  That dowid_t
 *		could either be a valid dowid, or DOW_NONE.  These functions
 *		do not return any kind of valid dowid on error; the caller
 *		should only use the dowid value when the function  returns
 *		success.  This convention is always followed.
 *
 *	(5) Dows are created using ip's rather than vp's.
 */
boolean_t sfs_dow_enabled = B_TRUE;

struct sfs_hardening sfs_hardening = {
	0 * HZ,		/* directory hardening in ticks */
	10 * HZ,	/* file hardening in ticks */
	10 * HZ		/* remove hardening in ticks */
};

/*
 * dowid_t
 * sfs_dow_create_inode(inode_t *ip)
 *	Allocate a dow structure corresponding to the disk buffer for the
 *	specified inode and return it.
 *
 * Calling/Exit State:
 *	No locking requirements
 *	
 * Remarks:
 *	Normally, invokes dow_create_buf with no sleeping.  If ip is on a
 *	soft-mount or read-only file system, then it simply returns DOW_NONE
 *	without trying to allocate a dow.
 *
 *	If successful, marks the inode with the IDOW flag, so the buffer
 *	will be written out using delayed ordered writes; see sfs_iupdat.
 */
dowid_t
sfs_dow_create_inode(inode_t *ip)
{
	fs_t *fp;
	dowid_t ipdow;
	pl_t opl;

	if (!sfs_dow_enabled || ((SFS_VFS_PRIV(ITOV(ip)->v_vfsp)->vfs_flags &
			SFS_DOWENABLED) == 0))
		return (DOW_NONE);
	fp = ip->i_fs;
	ipdow = dow_create_buf(ip->i_dev,
			(daddr_t)fsbtodb(fp, itod(fp, ip->i_number)),
			(int)fp->fs_bsize, DOW_NO_RESWAIT);
	if (ipdow != DOW_NONE) {
		opl = SFS_ILOCK(ip);
		ip->i_flag |= IDOW;
		SFS_IUNLOCK(ip, opl);
	}
	return (ipdow);
}

/*
 * dowid_t
 * sfs_dow_create_page(inode_t *ip, off_t offset, uint_t len)
 *	Allocate a dow structure for a page corresponding to the specified
 *	offset and length.
 *
 * Calling/Exit State:
 *	No locking requirements.
 *	
 * Remarks:
 *	Normally, invokes dow_create_page with no sleeping.  If ip is on a
 *	soft-mount or readonly file system, then it simply returns DOW_NONE
 *	without trying to allocate a dow.
 */
dowid_t
sfs_dow_create_page(inode_t *ip, off_t offset, uint_t len)
{
	off_t roff, rlen, size, mask;

	if (!sfs_dow_enabled || ((SFS_VFS_PRIV(ITOV(ip)->v_vfsp)->vfs_flags &
			SFS_DOWENABLED) == 0))
		return (DOW_NONE);
	size = MIN(ip->i_fs->fs_bsize, PAGESIZE);
	mask = ~(size - 1);
	roff = offset & mask;
	rlen = (((offset + len) - roff) + (size - 1)) & mask;
	if (rlen != size) {
#ifdef	DEBUG
		cmn_err(CE_NOTE,
			"sfs_dow_create_page error: offset = %d,len = %d\n",
			offset, len);
#endif
		return (DOW_NONE);
	}
	return (dow_create_page(ITOV(ip), roff, rlen, DOW_NO_RESWAIT));
}

/*
 * dowid_t
 * sfs_dow_iupdat(inode_t *ip)
 *	Do a delayed ordered update of the specified inode.
 *
 * Calling/Exit State:
 *	Returns a dowid corresponding to the inode update.
 *
 *	The calling LWP must hold the inode's rwlock in *exclusive* mode.
 *	
 * Remarks:
 *	Normally, this routine performs the following steps:
 *		(1) allocate a dow structure for the disk buffer corresponding
 *			to the inode
 *		(2) call sfs_iupdat to do a delayed inode update
 *		(3) call dow_setmod to mark the inode buffer dow as modified
 *		(4) return the dowid for the inode buffer dow
 *
 *	Two sub-cases are handled:
 *		(a) the dow cannot be allocated: in this case, the I/O is
 *			converted to synchronous I/O.
 *		(b) ip is on a soft-mounted FS: then all the I/O is delayed
 *			and no ordering is established.  See sfs_create_inode
 *			and sfs_iupdat to see how this is handled.
 */
dowid_t
sfs_dow_iupdat(inode_t *ip)
{
	dowid_t ipdow;
	extern void sfs_iupdat(inode_t *, enum iupmode);

	ipdow = sfs_dow_create_inode(ip);
	dow_startmod(ipdow);
	sfs_iupdat(ip, ipdow == DOW_NONE ? IUP_SYNC : IUP_FORCE_DELAY);
	dow_setmod(ipdow, FLUSHDELTAMAX);
	return (ipdow);
}

/*
 * void
 * sfs_dow_order(dowid_t depdow, dowid_t antdow)
 *	Establish an ordering between two dows, such that depdow will
 *	follow antdow in the graph.
 *
 * Calling/Exit State:
 *	depdow and antdow must either be DOW_NONE or represent valid
 *	dows which the caller has a hold on.
 *
 * Remarks:
 *	(1) If antdow is DOW_NONE, then presumably the action which
 *		must precede depdow has already been done synchronously;
 *		thus, the ordering has already been enforced, and this
 *		operation does nothing.
 *
 *	(2) If depdow is DOW_NONE, and antdow is not, then the action
 *		specified by antdow will be done synchronously in this
 *		routine.  The rationale is as follows: If depdow is
 *		DOW_NONE, then it is assumed to represent a failure to
 *		allocate a dow structure; in such cases, sfs converts
 *		ordered writes to synchronous writes.  In order to
 *		assure ordering, it is thus necessary to perform the
 *		antdow operation prior to returning to the caller.
 */
void
sfs_dow_order(dowid_t depdow, dowid_t antdow)
{

	if ((depdow == DOW_NONE) || (dow_order(depdow, antdow,
			DOW_NO_RESWAIT) != 0))
		dow_handle_sync(antdow);
}
