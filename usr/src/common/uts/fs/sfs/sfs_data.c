/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/sfs/sfs_data.c	1.7"

#include <fs/sfs/sfs_inode.h>
#include <util/ipl.h>
#include <util/ksynch.h>

inode_t	*sfs_inode;		/* the inode table itself */

/*
 * Free Lists
 */
struct inode_marker sfs_totally_free;	/* totally free inodes */
struct inode_marker sfs_partially_free;	/* partially free inodes */
clock_t sfs_scan_time;			/* last time sfs_free_scan() ran */
sv_t sfs_inode_sv;

/*
 *+ SFS Inode table lock
 */
LKINFO_DECL(sfs_inode_table_lkinfo, "FS:SFS:sfs inode table lock", 0);
lock_t sfs_inode_table_mutex;

/*
 * Update lock
 */
sleep_t sfs_updlock;
LKINFO_DECL(sfs_updlock_lkinfo, "FS:SFS:sfs update lock", 0);

LKINFO_DECL(sfs_ino_rwlock_lkinfo, "FS:SFS:per-inode rwlock lock", 0);
LKINFO_DECL(sfs_ino_spin_lkinfo, "FS:SFS:per-inode spin lock", 0);
LKINFO_DECL(sfs_renamelock_lkinfo, "FS:SFS:superblock rename mutex", 0);

