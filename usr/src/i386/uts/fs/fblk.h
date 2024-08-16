/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_FBLK_H	/* wrapper symbol for kernel use */
#define _FS_FBLK_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/fblk.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * WARNING.  This stub will be removed in the next major UNIX release.
*  Please be advised to change any source code using this header file.
 */

#ifdef	_KERNEL_HEADERS

#include <fs/s5fs/s5fblk.h>	/* REQUIRED */

#else

#include <sys/fs/s5fblk.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_FBLK_H */
