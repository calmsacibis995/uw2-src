/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_XX_XXDATA_H	/* wrapper symbol for kernel use */
#define _FS_XX_XXDATA_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/xxfs/xxdata.h	1.2"
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

extern struct inode *xxinode;                 /* the inode table itself */
extern struct inode *xx_ifreeh, **xx_ifreet;
extern int xx_tflush;
extern long int xx_ninode;

/*
 * Inode Hash List/Free List
 */
extern lock_t xx_inode_table_mutex;
extern lkinfo_t xx_inode_table_lkinfo;

/*
 * Update lock
 */
extern sleep_t xx_updlock;
extern lkinfo_t xx_updlock_lkinfo;

/*
 * Per-Inode Lockinfo Structures
 */
extern lkinfo_t xx_ino_rwlock_lkinfo;
extern lkinfo_t xx_ino_spin_lkinfo;

/*
 * Per Mounted-fs Lockinfo Structure
 */
extern lkinfo_t xx_renamelock_lkinfo;
extern lkinfo_t xx_sblock_lkinfo;

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_XX_XXDATA_H */
