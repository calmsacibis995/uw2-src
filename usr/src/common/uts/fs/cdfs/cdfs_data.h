/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_CDFS_CDFS_DATA_H      /* swrappe- symbol for kernel use */
#define _FS_CDFS_CDFS_DATA_H      /* subject to change without notice */

#ident	"@(#)kern:fs/cdfs/cdfs_data.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef	_KERNEL_HEADERS

#include <fs/cdfs/cdfs_inode.h>
#include <util/ipl.h>
#include <util/ksynch.h>

#elif defined(_KERNEL)

#include <sys/fs/cdfs_inode.h>
#include <sys/ipl.h>
#include <sys/ksynch.h>

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL


extern  struct vnodeops         cdfs_vnodeops;

/*
 * Inode Hash List/Free List
 */
extern lock_t cdfs_inode_table_mutex;
extern lkinfo_t cdfs_inode_table_lkinfo;

/*
 * Per-Inode Lockinfo Structures
 */
extern lkinfo_t cdfs_ino_splock_lkinfo;

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_CDFS_CDFS_DATA_H */
