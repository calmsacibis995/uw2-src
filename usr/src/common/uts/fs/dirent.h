/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_DIRENT_H	/* wrapper symbol for kernel use */
#define _FS_DIRENT_H	/* subject to change without notice */
#define _SYS_DIRENT_H	/* SVR4.0COMPAT */

#ident	"@(#)kern:fs/dirent.h	1.10"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * File-system independent directory entry.
 */
struct dirent {
	ino_t		d_ino;		/* "inode number" of entry */
	off_t		d_off;		/* file offset of next directory entry */
	ushort_t	d_reclen;	/* length of this record */
	char		d_name[1];	/* name of file */
};

typedef	struct	dirent	dirent_t;

#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)

#define DIRCOMPCMP(a) \
	((a[0] == '.')  && (a[1] == '.') && (a[2] == '\0'))

#if defined(__STDC__) && !defined(_KERNEL)
int getdents(int, struct dirent *, unsigned);
#else
int getdents( );
#endif

#endif /* !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) */ 

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_DIRENT_H */
