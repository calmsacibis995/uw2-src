/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:fs/xxfs/xxdata.c	1.1"

#include <util/ipl.h>
#include <util/ksynch.h>

struct inode *xx_inode;		 /* the inode table itself */
struct inode *xx_ifreeh, **xx_ifreet;
int	xx_tflush;
long int xx_ninode;

/*
 * Inode Hash List/Free List
 */
lock_t xx_inode_table_mutex;
LKINFO_DECL(xx_inode_table_lkinfo, "FS:XXFS:xx inode table lock", 0);

/*
 * Update lock
 */
sleep_t xx_updlock;
LKINFO_DECL(xx_updlock_lkinfo, "FS:XXFS:xx update lock", 0);

LKINFO_DECL(xx_ino_rwlock_lkinfo, "FS:XXFS:per-inode rwlock lock", 0);
LKINFO_DECL(xx_ino_spin_lkinfo, "FS:XXFS:per-inode spin lock", 0);
LKINFO_DECL(xx_renamelock_lkinfo, "FS:XXFS:superblock rename mutex", 0);
LKINFO_DECL(xx_sblock_lkinfo, "FS:XXFS:superblock inode mutex", 0);
