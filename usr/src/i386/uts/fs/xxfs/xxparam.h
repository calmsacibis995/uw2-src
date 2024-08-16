/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_XXFS_XXPARAM_H	/* wrapper symbol for kernel use */
#define _FS_XXFS_XXPARAM_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/xxfs/xxparam.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Filesystem parameters.
 */
#define	XXSUPERB	((daddr_t)2)	/* block number of the super block */
#define	XXDIRSIZ	14		/* max characters per directory */
#define	XXNICINOD	100		/* number of superblock inodes */
#define	XXROOTINO	2		/* i-number of all roots */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_XXFS_XXPARAM_H */
