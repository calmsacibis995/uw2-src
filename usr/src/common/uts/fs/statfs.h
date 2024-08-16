/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_STATFS_H	/* wrapper symbol for kernel use */
#define _FS_STATFS_H	/* subject to change without notice */

#ident	"@(#)kern:fs/statfs.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Structure returned by statfs(2) and fstatfs(2).
 * This structure and associated system calls have been replaced
 * by statvfs(2) and fstatvfs(2) and will be removed from the system
 * in a near-future release.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

struct	statfs {
	short	f_fstyp;	/* File system type */
	long	f_bsize;	/* Block size */
	long	f_frsize;	/* Fragment size (if supported) */
	long	f_blocks;	/* Total number of blocks on file system */
	long	f_bfree;	/* Total number of free blocks */
	ino_t	f_files;	/* Total number of file nodes (inodes) */
	ino_t	f_ffree;	/* Total number of free file nodes */
	char	f_fname[6];	/* Volume name */
	char	f_fpack[6];	/* Pack name */
};

#if defined(__STDC__) && !defined(_KERNEL)
int statfs(const char *, struct statfs *, int, int);
int fstatfs(int, struct statfs *, int, int);
#endif

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_STATFS_H */
