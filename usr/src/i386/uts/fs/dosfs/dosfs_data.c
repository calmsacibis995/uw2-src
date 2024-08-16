/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:fs/dosfs/dosfs_data.c	1.1"

#include "dosfs.h"

struct denode *denode;		 	/* the inode table itself */
long int ndenode;

/*
 * Inode Hash List/Free List
 */
lock_t dosfs_denode_table_mutex;
LKINFO_DECL(dosfs_denode_table_lkinfo, "FS:DOSFS:dosfs denode table lock", 0);

/*
 * Update lock
 */
sleep_t dosfs_updlock;
LKINFO_DECL(dosfs_updlock_lkinfo, "FS:DOSFS:dosfs update lock", 0);

LKINFO_DECL(dosfs_deno_lock_lkinfo, "FS:DOSFS:per-denode sleep lock", 0);
LKINFO_DECL(dosfs_rename_lkinfo, "FS:DOSFS: rename lock", 0);
