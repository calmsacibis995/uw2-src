/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_USTAT_H	/* wrapper symbol for kernel use */
#define _FS_USTAT_H	/* subject to change without notice */

#ident	"@(#)kern:fs/ustat.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/* WARNING: The ustat system call will become obsolete in the
** next major release following SVR4. Application code should
** migrate to the replacement system call statvfs(2).
*/

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#else

#include <sys/types.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

struct  ustat {
	daddr_t	f_tfree;	/* total free */
	o_ino_t	f_tinode;	/* total inodes free */
	char	f_fname[6];	/* filsys name */
	char	f_fpack[6];	/* filsys pack name */
};

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_USTAT_H */
