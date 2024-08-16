/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/specfs/specvfsops.c	1.5"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <fs/buf.h>
#include <util/debug.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <proc/cred.h>
#include <fs/specfs/snode.h>
#include <fs/specfs/specdata.h>
#include <fs/fs_subr.h>

STATIC int spec_sync(vfs_t *, int , cred_t *);

struct vfsops spec_vfsops = {
	(int (*)())fs_nosys,		/* mount */
	(int (*)())fs_nosys,		/* unmount */
	(int (*)())fs_nosys,		/* root */
	(int (*)())fs_nosys,		/* statvfs */
	spec_sync,
	(int (*)())fs_nosys,		/* vget */
	(int (*)())fs_nosys,		/* mountroot */
	(int (*)())fs_nosys,		/* swapvp */
	(int (*)())fs_nosys,		/* filler */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};

/*
 * int
 * spec_sync(vfs_t *vfsp, int flag, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Run though all the snodes and force write-back
 *	of all dirty pages on the block devices.
 */
/* ARGSUSED */
STATIC int
spec_sync(vfs_t *vfsp, int flag, cred_t *cr)
{

	if (SLEEP_TRYLOCK(&spec_updlock) != B_TRUE)
		return 0;

#ifdef SYNC_ONCLOSE
	if (flag & SYNC_CLOSE)
		(void) strpunlink(cr);
#endif

	/* 
	 * spec_flush is called to traverse the hash list, it must lock it
	 * during traversal
	 */
	if (!(flag & SYNC_ATTR)) {
		spec_flush();
	}
	SLEEP_UNLOCK(&spec_updlock);
	return 0;
}
