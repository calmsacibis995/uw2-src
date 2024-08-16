/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_S5FS_S5DATA_H	/* wrapper symbol for kernel use */
#define _FS_S5FS_S5DATA_H	/* subject to change without notice */

#ident	"@(#)kern:fs/s5fs/s5data.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ipl.h>
#include <util/ksynch.h>

#elif defined(_KERNEL) 

#include <sys/ipl.h>
#include <sys/ksynch.h>

#endif /* _KERNEL_HEADERS */

extern struct inode *inode;                 /* the inode table itself */
extern long int ninode;

/*
 * Inode Hash List/Free List
 */
extern lock_t s5_inode_table_mutex;
extern struct inode_marker s5_totally_free;
extern struct inode_marker s5_partially_free;
extern clock_t s5_scan_time;
extern sv_t s5_inode_sv;

extern lkinfo_t s5_inode_table_lkinfo;
/*
 * Update lock
 */
extern sleep_t s5_updlock;
extern lkinfo_t s5_updlock_lkinfo;

/*
 * Per-Inode Lockinfo Structures
 */
extern lkinfo_t s5_ino_rwlock_lkinfo;
extern lkinfo_t s5_ino_spin_lkinfo;

/*
 * Per Mounted-fs Lockinfo Structure
 */
extern lkinfo_t s5_renamelock_lkinfo;
extern lkinfo_t s5_sblock_lkinfo;

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_S5FS_S5DATA_H */
