/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_INO_H	/* wrapper symbol for kernel use */
#define _FS_INO_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/ino.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * WARNING.  This stub will be removed in the next major UNIX release.
 * Be advised to change any source code using this header file.
 */

#ifndef	_KERNEL

#include <sys/fs/s5ino.h>	/* SVR4.0COMPAT */

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_INO_H */
