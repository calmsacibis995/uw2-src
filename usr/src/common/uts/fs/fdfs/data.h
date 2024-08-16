/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_FDFS_DATA_H	/* wrapper symbol for kernel use */
#define _FS_FDFS_DATA_H	/* subject to change without notice */

#ident	"@(#)kern:fs/fdfs/data.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ipl.h>
#include <util/ksynch.h>

#elif defined(_KERNEL) 

#include <sys/ipl.h>
#include <sys/ksynch.h>

#endif /* _KERNEL_HEADERS */

#define min(a,b)        ((a) <= (b) ? (a) : (b))
#define max(a,b)        ((a) >= (b) ? (a) : (b))
#define round(r)        (((r)+sizeof(int)-1)&(~(sizeof(int)-1)))
#define fdfstoi(n)      ((n)+100)

#define FDFSDIRSIZE 14
struct fdfsdirect {
        short   d_ino;
        char    d_name[FDFSDIRSIZE];
};

#define FDFSROOTINO     2
#define FDFSSDSIZE      sizeof(struct fdfsdirect)
#define FDFSNSIZE               10
#define TFDFSDIRS	2*FDFSSDSIZE

int fdfs_fstype;

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_FDFS_DATA_H */
