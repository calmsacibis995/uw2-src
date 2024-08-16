/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/compat_subr.c	1.2"

/*
 * misc routines required for compat with SVR4 i386AT drivers
 *
 */

#include <util/param.h>
#include <fs/vfs.h>

void fshadbad(dev_t, daddr_t);


/* ARGSUSED1 */
/*
 * void
 * fshadbad(dev_t dev, daddr_t bno)
 *	called from hdb_mapbad to mark fs dirty
 *
 * Calling/Exit State:
 *	called from UP driver, bound to cpu
 *	sets VFS_BADBLOCK
 */
void
fshadbad(dev_t dev, daddr_t bno)
{
 	register struct vfs *vfsp;

        if ((vfsp = vfs_devsearch(dev)) != (struct vfs *)NULL)
		vfsp->vfs_flag |= VFS_BADBLOCK;
}
