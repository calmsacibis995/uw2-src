/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_XXFS_XXDIR_H	/* wrapper symbol for kernel use */
#define _FS_XXFS_XXDIR_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/xxfs/xxdir.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <fs/xxfs/xxparam.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/fs/xxparam.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef struct xxdirect {
	o_ino_t	d_ino;		/* xx inode type */
	char	d_name[XXDIRSIZ];
} xxdirect_t;

#define XXSDSIZ	(sizeof(xxdirect_t))

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_XXFS_XXDIR_H */
