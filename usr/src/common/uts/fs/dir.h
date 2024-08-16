/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_DIR_H	/* wrapper symbol for kernel use */
#define _FS_DIR_H	/* subject to change without notice */

#ident	"@(#)kern:fs/dir.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * WARNING!!  This file provided for compatibility only, and may be removed
 * in a later release.  Please be advised to change any source code using
 * this header file to include <sys/fs/s5dir.h> directly.
 */

#ifndef _KERNEL  

#include <sys/fs/s5dir.h> /* SVR3.2COMPAT */

#endif

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_DIR_H */
