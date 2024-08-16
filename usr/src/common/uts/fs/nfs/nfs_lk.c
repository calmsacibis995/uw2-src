/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_lk.c	1.16"
#ident	"$Header: $"

/*
 *	nfs_lk.c, definitions and lock initializations for nfs
 */

#include <mem/kmem.h>
#include <util/ksynch.h>
#include <util/types.h>

/*
 * lock used to protect the nfs_mnt_list
 */
lock_t			nfs_mnt_lock;

/*
 * lock used to protect the client handle table
 */
lock_t			chtable_lock;

/*
 * lock for the protection of the rnode free list
 */
lock_t			rpfreelist_lock;

/*
 * lock for the protection of the server side statistics
 */
fspin_t			svstat_mutex;

/*
 * lock for the protection of the client side statistics
 */
fspin_t			clstat_mutex;

/*
 * lock for free space management for nfs servers
 */
fspin_t			rfsfreesp_mutex;

fspin_t			unixauthtab_mutex;
fspin_t			desauthtab_mutex;
fspin_t			newnum_mutex;
fspin_t			minmap_mutex;

#ifdef NFSESV

fspin_t			esvauthtab_mutex;

#endif

/*
 * sleep lock for protecting the rnode hash table
 */
rwsleep_t		nfs_rtable_lock;

/*
 * sleep lock to protect the list of exportinfo structs
 */
rwsleep_t		exported_lock;
rwsleep_t		sync_busy_lock;

/*
 * sync vars used for async I/O by async lwps and master async daemon.
 */
sv_t			nfs_asynclwp_sv;
sv_t			nfs_asyncd_sv;

/*
 * sync var user by attr lwp
 */
sv_t			nfs_mmaplwp_sv;

/*
 * sync var and lock used in xdr_rrok() to wait for memory when
 * allocb() fails
 */
lock_t			nfs_rrok_lock;
sv_t			nfs_rrok_sv;

LKINFO_DECL(rtable_lkinfo, "NFS:nfs_rtable_lock:rtable (global)", 0);
LKINFO_DECL(rlock_rw_lkinfo, "NFS:r_rwlock:rnode (per-rnode)", 0);
LKINFO_DECL(r_statelock_lkinfo, "NFS:r_statelock:rnode (per-rnode)", 0);
LKINFO_DECL(exi_lkinfo, "NFS:exi_lock:exportinfo (per-exportinfo)", 0);
LKINFO_DECL(exi_list_lkinfo, "NFS:exi_list:exportlist (global)", 0);
LKINFO_DECL(nfs_mnt_lkinfo, "NFS:nfs_mnt_lock:(global)", 0);
LKINFO_DECL(mi_async_lkinfo, "NFS:mi_async_lock:(per-mntinfo)", 0);
LKINFO_DECL(nfs_sp_lkinfo, "NFS:sp_lock:per svc_param", 0);
LKINFO_DECL(mi_lock_lkinfo, "NFS:mi_lock:(per-mntinfo)", 0);
LKINFO_DECL(chtable_lkinfo, "NFS:chtable_lock:(global)", 0);
LKINFO_DECL(sync_busy_lkinfo, "NFS:sync_busy_lock:nfs_sync (global)", 0);
LKINFO_DECL(rp_freelist_lkinfo, "NFS:rpfreelist_lock:(global)", 0);
LKINFO_DECL(nfs_rrok_lkinfo, "NFS:nfs_rrok_lock:(global)", 0);
LKINFO_DECL(chtab_lkinfo, "NFS:chtab_lock:(global)", 0);
