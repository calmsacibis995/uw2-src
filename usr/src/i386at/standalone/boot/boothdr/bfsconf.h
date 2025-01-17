/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _BFSCONF_H
#define _BFSCONF_H

#ident	"@(#)stand:i386at/standalone/boot/boothdr/bfsconf.h	1.2"
#ident  "$Header: $"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ifndef _FS_BFS_BFS_H
#include <sys/fs/bfs.h>	/* REQUIRED */
#endif

#define OFF2BLK(x)	((x) / BFS_BSIZE)	/* disk offset to blk # */
#define BUFOFF(x)	((x) & (BFS_BSIZE - 1))	/* buffer offset	*/

#define BLK_GBUF        (GBUFSZ / BFS_BSIZE)    /* max blk in global buf*/

#define NBFSFLST	10	/* # of cached file list entries	*/
#define BFS_DIRSZ	(sizeof(struct bfs_dirent)) /* size of dirent	*/
#define BFS_FLSTSZ	(sizeof(struct bfs_ldirs))  /* size of file lst	*/

struct bfsroot {
	int	brt_nflst;		/* # of file list entries	*/
	struct	bfs_dirent brt_ino; 	/* root inode struct		*/
};
#define BRTINOP	(brtp->brt_ino)

struct bfsflst {
	int	bfl_cnt;		/* # of cached file list entries*/
	struct	bfs_ldirs bfl_ldirs[NBFSFLST]; /* cached file list ent	*/
};
#define BFLSTP 	(bflp->bfl_ldirs)

#endif /* _BOOT_BFSCONF_H */
