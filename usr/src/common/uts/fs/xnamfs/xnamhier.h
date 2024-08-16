/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_XNAMFS_XNAMHIER_H	/* wrapper symbol for kernel use */
#define _FS_XNAMFS_XNAMHIER_H	/* subject to change without notice */

#ident	"@(#)kern:fs/xnamfs/xnamhier.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ipl.h>	/* REQUIRED */
#include <fs/fs_hier.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * This header file has all the hierarchy and minipl information 
 * pertaining to the xnamfs file system. Note that all lock hierarchies in 
 * this file will be expressed as an offset from a base hierarchy value that
 * will be associated with the xnamfs file system. 
 */

#define XNAM_HIER_BASE	FS_HIER_BASE

/*
 * The hierarchy values will be checked amongst locks that have identical
 * minipl and hence the hierarchy namespace can be shared among locks that
 * have different minipls.
 *
 */
#define XNAM_HIER	XNAM_HIER_BASE

#define FS_XNTBLHIER	XNAM_HIER_BASE + 5	/* xnamfs:xnam_table_mutex */
#define FS_XNTBLPL	PLFS

#define FS_XNAMHIER	XNAM_HIER_BASE + 5	/* xnamfs:x_mutex */
#define FS_XNAMPL	PLFS

#define FS_XSEMHIER	XNAM_HIER_BASE + 5	/* xnamfs:xsem_mutex */
#define FS_XSEMPL	PLFS

#define FS_XFREEHIER	XNAM_HIER_BASE + 5	/* xnamfs:xs_freelist and
						 * xsd_global_lock
						 */
#define FS_XFREEPL	PLFS

#define FS_XSDPRI	PLFS               	/* xnamfs:xsd:x_sv priority */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_XNAMFS_XNAMHIER_H */
