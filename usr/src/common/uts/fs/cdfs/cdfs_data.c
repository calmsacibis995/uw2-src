/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/cdfs/cdfs_data.c	1.1"

#include <util/ipl.h>
#include <util/ksynch.h>

/*
 * Inode Hash List/Free List
 */
lock_t cdfs_inode_table_mutex;
LKINFO_DECL(cdfs_inode_table_lkinfo, "FS:CDFS:cdfs inode table lock", 0);


/*
 * Inode info locks
 */
LKINFO_DECL(cdfs_ino_splock_lkinfo, "FS:CDFS:cdfs inode sleep lock", 0);

