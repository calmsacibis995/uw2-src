/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_S5FS_S5FBLK_H	/* wrapper symbol for kernel use */
#define _FS_S5FS_S5FBLK_H	/* subject to change without notice */

#ident	"@(#)kern:fs/s5fs/s5fblk.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */
#include <fs/s5fs/s5param.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */
#include <sys/fs/s5param.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef	struct	fblk {
	int	df_nfree;
	daddr_t	df_free[NICFREE];
} fblk_t;

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_S5FS_S5FBLK_H */
