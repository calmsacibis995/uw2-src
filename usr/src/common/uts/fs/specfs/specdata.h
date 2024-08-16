/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_SPECFS_SPECDATA_H	/* wrapper symbol for kernel use */
#define _FS_SPECFS_SPECDATA_H	/* subject to change without notice */

#ident	"@(#)kern:fs/specfs/specdata.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/specfs/snode.h>
#include <util/ipl.h>
#include <util/ksynch.h>

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

extern struct snode *spectable[];	/* snode hash table */

/*
 * snode hash table lock
 */
extern lock_t spec_table_mutex;
extern lkinfo_t spec_table_lkinfo;

/* snode id lock */
extern lock_t snode_id_mutex;
extern lkinfo_t snode_id_lkinfo;

/* snode lockinfo */
extern lkinfo_t snode_mutex_lkinfo;
extern lkinfo_t snode_rwlock_lkinfo;

extern sleep_t spec_updlock;
extern lkinfo_t spec_updlock_lkinfo;

extern int specfstype;
extern dev_t specdev;
extern ulong_t spec_lastnodeid;

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* FS_SPECFS_SPECDATA_H */
