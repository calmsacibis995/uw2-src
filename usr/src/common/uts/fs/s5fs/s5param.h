/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_S5FS_S5PARAM_H	/* wrapper symbol for kernel use */
#define _FS_S5FS_S5PARAM_H	/* subject to change without notice */

#ident	"@(#)kern:fs/s5fs/s5param.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Filesystem parameters.
 */
#define	SUPERB	((daddr_t)1)	/* block number of the super block */
#define	DIRSIZ	14		/* max characters per directory */
#define	NICINOD	100		/* number of superblock inodes */
#define	NICFREE	50		/* number of superblock free blocks */
#define	S5ROOTINO	2	/* i-number of all roots */

#define SUPERBOFF	512	/* superblock offset */

#ifndef _KERNEL

/*
 * Parameters used by fsck and mkfs.
 */
#define STEPSIZE	7	/* default step for freelist spacing */
#define CYLSIZE		400	/* default cyl size for spacing */
#define MAXCYL		1600	/* maximum cylinder size */

#endif	/* #ifndef _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_S5FS_S5PARAM_H */
