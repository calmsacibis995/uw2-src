/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_SFS_SFS_DATA_H	/* wrapper symbol for kernel use */
#define _FS_SFS_SFS_DATA_H	/* subject to change without notice */

#ident	"@(#)kern:fs/sfs/sfs_data.h	1.12"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/sfs/sfs_inode.h>
#include <util/ipl.h>
#include <util/ksynch.h>

#elif defined(_KERNEL) 

#include <sys/fs/sfs_inode.h>
#include <sys/ipl.h>
#include <sys/ksynch.h>

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

extern struct inode *sfs_inode;		/* the inode table itself */

/*
 * Inode Hash List/Free List
 */
extern lock_t sfs_inode_table_mutex;
extern lkinfo_t sfs_inode_table_lkinfo;
extern struct inode_marker sfs_totally_free;
extern struct inode_marker sfs_partially_free;

/*
 * Update lock
 */
extern sleep_t sfs_updlock;
extern lkinfo_t sfs_updlock_lkinfo;

/*
 * Per-Inode Lockinfo Structures
 */
extern lkinfo_t sfs_ino_rwlock_lkinfo;
extern lkinfo_t sfs_ino_spin_lkinfo;

/*
 * Per Mounted-fs Lockinfo Structure
 */
extern lkinfo_t sfs_renamelock_lkinfo;

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_SFS_SFS_DATA_H */
