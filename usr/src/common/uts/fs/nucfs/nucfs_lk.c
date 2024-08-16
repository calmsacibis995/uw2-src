/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nucfs_lk.c	1.2.1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nucfs_lk.c,v 2.52.2.2 1994/12/19 00:08:41 stevbam Exp $"

/*
 *	nucfs_lk.c, definitions and lock initializations for nucfs
 */

#include <util/types.h>
#include <util/ksynch.h>
#include <mem/kmem.h>
#include <fs/nucfs/nucfslk.h>
#include <fs/nucfs/nwfidata.h>
#include <fs/nucfs/nwfidata.h>

/*
 * The nucfs_list_lock guards the snode lists.
 */
NWFI_LOCK_T		nucfs_list_lock;

/*
 * The NWfiNameTableLock table lock guards the names table.
 */
fspin_t			NWfiNameTableLock;

/*
 * The NWfiTimeLock guards the NWfiBolt structure.
 */
fspin_t			NWfiTimeLock;

/*
 * The Mount Volume Lock guards the mounted volume list.
 */
sleep_t			NWfiMountVolumeLock;

LKINFO_DECL(nucfs_snode_lkinfo, "NUCFS:snodeLock (per-snode)", 0);
LKINFO_DECL(nucfs_snode_rw_lkinfo, "NUCFS:snodeRwLock (per-snode)", 0);
LKINFO_DECL(nucfs_name_lkinfo, "NUCFS:nameLock (per-client_handle)", 0);

sv_t			nucfs_resource_release_sv;
