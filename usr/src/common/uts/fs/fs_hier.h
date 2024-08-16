/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_FS_HIER_H	/* wrapper symbol for kernel use */
#define _FS_FS_HIER_H	/* subject to change without notice */

#ident	"@(#)kern:fs/fs_hier.h	1.13"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ghier.h>
#include <util/ipl.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * This header file has all the hierarchy and minipl information 
 * pertaining to the file system subsystem. Note that all lock hierarchies in 
 * this file will be expressed as an offset from a base hierarchy value that
 * will be associated with the file system. Clearly, locks that can be 
 * held across subsystem boundaries need to be dealt with separately.
 * These "global locks will have their hierarchy defined in the ghier.h file
 * under util.
 */

#ifdef _KERNEL

/*
 * Hierarchy values used by the file system subsystem:
 *
 *	PLHI:	FS_HIER_BASE to FS_HIER_BASE + 10
 *	PL6:	FS_HIER_BASE to FS_HIER_BASE + 5
 */

/*
 * The hierarchy values will be checked amongst locks that have identical
 * minipl and hence the hierarchy namespace can be shared among locks that
 * have different minipls.
 *
 */

#define	PLBUF		PL6	
#define	PLFS		PL6	

#define FS_BUFLISTHIER	FS_HIER_BASE + 5 	/* buf_lists */
#define FS_BUFLISTPL	PLBUF

#define FS_BUFASYNCHIER	FS_HIER_BASE + 5	/* buf_async_cnt */
#define FS_BUFASYNCPL	PLBUF

#define FS_BUFPGIOHIER	FS_HIER_BASE + 10 	/* buf:pageio_lists_lock*/
#define FS_BUFPGIOPL	PLHI

#define FS_VFSPHIER	FS_HIER_BASE + 5 	/* vfs:vfs_mutex */
#define FS_VFSPPL	PLFS

#define FS_VPFRLOCKHIER	FS_HIER_BASE + 3	/* vp:v_filocks_mutex */
#define FS_VPFRLOCKPL	PLFS

#define FS_DNLCHIER	FS_HIER_BASE + 4	/* dnlc:dnlc_mutex */
#define FS_DNLCPL	PLFS

#define FS_SLPLOCKHIER	FS_HIER_BASE + 5	/* frlock:sleeplcks_mutex */
#define FS_SLPLOCKPL	PLFS

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_FS_HIER_H */
