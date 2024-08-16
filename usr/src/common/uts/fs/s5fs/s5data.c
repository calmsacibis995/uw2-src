/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/s5fs/s5data.c	1.4"

#include <util/ipl.h>
#include <util/ksynch.h>
#include <fs/s5fs/s5inode.h>

struct inode *inode;		 	/* the inode table itself */
long int ninode;

/*
 * Free lists
 */
struct inode_marker s5_totally_free;	/* totally free inodes */
struct inode_marker s5_partially_free;	/* totally free inodes */
clock_t s5_scan_time;
sv_t s5_inode_sv;

/*
 * Inode Hash List/Free List
 */
lock_t s5_inode_table_mutex;
LKINFO_DECL(s5_inode_table_lkinfo, "FS:S5FS:s5 inode table lock", 0);

/*
 * Update lock
 */
sleep_t s5_updlock;
LKINFO_DECL(s5_updlock_lkinfo, "FS:S5FS:s5 update lock", 0);

LKINFO_DECL(s5_ino_rwlock_lkinfo, "FS:S5FS:per-inode rwlock lock", 0);
LKINFO_DECL(s5_ino_spin_lkinfo, "FS:S5FS:per-inode spin lock", 0);
LKINFO_DECL(s5_renamelock_lkinfo, "FS:S5FS:superblock rename mutex", 0);
LKINFO_DECL(s5_sblock_lkinfo, "FS:S5FS:superblock inode mutex", 0);
