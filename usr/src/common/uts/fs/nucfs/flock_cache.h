/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/flock_cache.h	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/flock_cache.h,v 2.1.2.4 1995/01/29 20:33:39 mdash Exp $"

#ifndef _FS_NUCFS_FLOCK_CACHE_H
#define _FS_NUCFS_FLOCK_CACHE_H

#ifdef _KERNEL_HEADERS
#include <fs/nucfs/nwfslock.h>
#elif defined(_KERNEL) || defined(_KMEMUSER)
#include <sys/nwfslock.h>
#endif /* _KERNEL_HEADERS */

/*
 * Declarations for the NUCFS frlock request cache.
 */

/*
 * Much of the complexity of maintaining frlocks in NUCFS originates in
 * differences between System V and NetWare lock semantics.  One large
 * difference is that NetWare sees requests as originating from virtual DOS
 * clients, which in NUCFS are all processes sharing a set of credentials,
 * while UNIX sees requests as originating from individual processes.  This
 * situation compels NUCFS to resolve intra-credential, inter-process lock
 * conflicts locally, before handing them off to the server.  This further
 * implies that NUCFS must maintain local lock state, unlike many other
 * distributed file system clients, which punt all lock requests and I/O on
 * locked files to the server.
 *
 * Another disparity is the lack of direct support for advisory locks in
 * NetWare.  Without time to implement a protocol for advisory locks between
 * NUCFS and NUCNLM, we have made the difficult choice of promoting all locks
 * on files that are not mandatory lock-enabled to mandatory status, as was
 * done in UnixWare 1.1.  (Some kind of enforcement of advisory was found
 * necessary to the correct functioning of the UnixWare desktop late in 2.0.1.)
 * It should be noted that, because there is no server-side enforcement,
 * advisory locks will generally not work outside the scope of a single client
 * host.
 *
 * Finally, underlying lock semantics have subtle ramifications that pervade
 * the impelementation of locks in NUCFS.  For an explanation of UNIX
 * semantics, see the official documentation.  The following information about
 * the NetWare side was derived by experiment, with some fine points provided
 * by Kyle Powell.
 *
 * Semantics of shared locks.
 *
 * (1) The single holder of a NetWare read lock cannot write the locked range,
 * but the single holder of a UNIX lock can.  (In NetWare, a lock can reduce
 * your access rights; in UNIX, it cannot.)
 *
 * Interaction between multiple lock requests from a single client.
 *
 * (1) If two locks have identical dimensions, the second will effectively
 * displace the first, except that an attempt to exclusively lock exactly the
 * same range twice will fail with an error.
 *
 * (2) If two overlapping locks do not match in dimension, the second
 * effectively masks the overlapped region of the first (with no effect, if
 * they are of the same type).  Unlocking the second restores the effect of the
 * first.
 *
 * Unlock requests.
 *
 * (1) An unlock request reaps only those locks that have the same offset as
 * the request.  An unlock request for an offset that has no locks produces an
 * error.
 *
 * (2) If only one lock starts at the given offset, the length of the unlock
 * request is ignored, and the lock is reaped.
 *
 * (3) If more than one lock starts at the given offset, the unlock length must
 * match the length of one lock, and only that lock is reaped.  If the length
 * does not match that of a lock at the given offset, an error is returned.
 */

/*
 * Structure to cache lock requests honored by the server.  Iff cachePidCount >
 * 1, then cachePidChain is non-empty, pid information is kept there, and
 * cachePid should be ignored.  (To avoid multiple processes within a
 * credential displacing each other's identical, shared locks on the server, we
 * reference count such locks here, and represent the aggregate of such locks
 * with a single lock on the server.)
 */
typedef struct NUCfsFlockCache {
	NWFI_LIST_T	cacheChain;
	uint32		cachePidCount;
	NWFI_LIST_T	cachePidChain;
 	NUCFS_LOCK_T	cacheState;
} NUCFS_FLOCK_CACHE_T;

#define	cacheCommand	cacheState.lockCommand
#define cacheType	cacheState.lockType
#define cacheOffset	cacheState.lockOffset
#define cacheEnd	cacheState.lockEnd
#define cachePid	cacheState.lockPid
#define cacheCred	cacheState.lockCred

#define CHAIN_TO_CACHE(c)	\
	((NUCFS_FLOCK_CACHE_T *)((char *)(c) - \
		offsetof(NUCFS_FLOCK_CACHE_T, cacheChain)))

/*
 * Entries on cachePidChain have this form.
 */
typedef struct NUCfsFlockPid {
	NWFI_LIST_T	chain;
	uint32		pid;	
} NUCFS_FLOCK_PID_T;

#define CHAIN_TO_PID(c)	\
	((NUCFS_FLOCK_PID_T *)((char *)(c) - \
		offsetof(NUCFS_FLOCK_PID_T, chain)))

extern void NWfsFlockCacheExtractRange(NWFI_LIST_T *, NUCFS_LOCK_T *, boolean_t,
				       uint16, NWFI_LIST_T *);
extern ccode_t NWfsFlockCacheClip(NWFS_CLIENT_HANDLE_T *, uint32,
				  NUCFS_LOCK_T *, NWFI_LIST_T *,
				  enum NUC_DIAG *);
extern void NWfsFlockCacheSplice(NWFI_LIST_T *, NWFI_LIST_T *);
extern NUCFS_FLOCK_CACHE_T *NWfsFlockCacheHold(NWFI_LIST_T *, NUCFS_LOCK_T *);
extern void NWfsFlockCacheRelease(NUCFS_FLOCK_CACHE_T *, uint32);
extern NUCFS_FLOCK_CACHE_T *NWfsFlockCacheFind(NWFI_LIST_T *, NUCFS_LOCK_T *,
					       boolean_t, uint16);
extern boolean_t NWfsFlockPidFind(NUCFS_FLOCK_CACHE_T *, uint32);
extern uint32 NWfsFlockPidNext(NUCFS_FLOCK_CACHE_T *, NWFI_LIST_T **);

#endif /* _FS_NUCFS_FLOCK_CACHE_H */
