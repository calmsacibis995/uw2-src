/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/specfs/specdata.c	1.3"

#include <util/ipl.h>
#include <util/ksynch.h>
#include <fs/specfs/snode.h>

snode_t *spectable[SPECTBSIZE];	/* snode hash table */
sv_t snode_sv;

/*
 * snode hash table lock
 */
lock_t spec_table_mutex;
LKINFO_DECL(spec_table_lkinfo, "FS:SPECFS:snode hash table mutex", 0);

/* snode id lock */
lock_t snode_id_mutex;
LKINFO_DECL(snode_id_lkinfo, "FS:SPECFS:spec node id mutex", 0);

sleep_t spec_updlock;
LKINFO_DECL(spec_updlock_lkinfo, "FS:SPECFS:specfs update lock", 0);

LKINFO_DECL(snode_rwlock_lkinfo, "FS:SPECFS:per-snode rwlock lock", 0);
LKINFO_DECL(snode_mutex_lkinfo, "FS:SPECFS:per-snode spin lock", 0);

int specfstype;
dev_t specdev;
long spec_lastnodeid;
