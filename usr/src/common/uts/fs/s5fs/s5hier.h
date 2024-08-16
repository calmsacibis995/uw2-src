/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_S5_HIER_H	/* wrapper symbol for kernel use */
#define _FS_S5_HIER_H	/* subject to change without notice */

#ident	"@(#)kern:fs/s5fs/s5hier.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ipl.h>	/* REQUIRED */
#include <fs/fs_hier.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * This header file has all the hierarchy and minipl information 
 * pertaining to the s5 file system. Note that all lock hierarchies in 
 * this file will be expressed as an offset from a base hierarchy value that
 * will be associated with the s5 file system. Clearly, locks that can be 
 * held across subsystem boundaries need to be dealt with separately.
 * These "global" locks will have their hierarchy values defined in the 
 * ghier.h file under the util directory.
 *
 */

#define S5FS_HIER_BASE	FS_HIER_BASE

/*
 * Hierarchy values used by the file system subsystem:
 *
 *	PLHI:	S5_HIER_BASE to S5_HIER_BASE + 10
 *	PL6:	S5_HIER_BASE to S5_HIER_BASE + 5
 */

/*
 * The hierarchy values will be checked amongst locks that have identical
 * minipl and hence the hierarchy namespace can be shared among locks that
 * have different minipls.
 *
 */

#define FS_S5LISTHIER	S5FS_HIER_BASE + 5 	/* s5:s5_table_mutex */
#define FS_S5LISTPL	PLFS

#define FS_S5INOHIER	S5FS_HIER_BASE + 5	/* s5:i_mutex */
#define FS_S5INOPL	PLFS

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_S5_HIER_H */
