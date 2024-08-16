/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nucfs_init.c	1.1.1.5"
#ident  "@(#)kern-nuc:fs/nucfs/nucfs_init.c     1.1.1.2"
#ident  "$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nucfs_init.c,v 2.52.2.2 1995/01/11 04:27:34 stevbam Exp $"

/*
**  Netware Unix Client
**
**	MODULE:
**		nucfs_init.c -	The Virtual File System Interface layer (NWfi)
**				for SRV4.2MP module initialization.
**
**	ABSTRACT:
**		The nucfs_vnops.c contains the NetWare UNIX Client File System
**		vnode operations of the Virtual File System Interface layer
**		(NWfi) for SVR4.2MP VFS/VNODE Architecture Kernels. This
**		file contains initialization code.
*/

#include <net/nuc/nwctypes.h>
#include <net/nuc/requester.h>

#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/pathname.h>
#include <util/types.h>
#include <fs/nucfs/nucfslk.h>
#include <util/cmn_err.h>
#include <util/ksynch.h>
#include <mem/kmem.h>
#include <net/nuc/spilcommon.h>
#include <fs/nucfs/nwfschandle.h>
#include <fs/nucfs/nucfscommon.h>
#include <fs/nucfs/nwfidata.h>
#include <fs/nucfs/nwfsnode.h> 
#include <fs/nucfs/nwfsops.h> 
#include <fs/nucfs/nwficommon.h>
#include <fs/nucfs/nucfsglob.h>
#include <proc/lwp.h>
#include <svc/clock.h>
#include <util/nuc_tools/trace/nwctrace.h>

/*
 * Define the tracing mask.
 */
#define NVLT_ModMask	NVLTM_fs

NWFI_LKINFODECL(nucfs_mount_lkinfo, "NUCFS:NWfiMountVolumeLock", 0);
NWFI_LKINFODECL(nucfs_list_lkinfo, "NUCFS:nucfs_list_lock", 0);

extern	NWFI_FSPIN_T	NWfiNameTableLock, NWfiTimeLock;
extern	NWFI_SLEEP_T	NWfiMountVolumeLock;

STATIC	toid_t	nucfs_timeoutid;
STATIC	void 	NWfiPagePushDeInit(NWFI_VOLFLUSH_DATA_T *);
STATIC	void 	NWfiAttFlushDeInit(NWFI_VOLFLUSH_DATA_T *);


void
nucfs_lock_init(void)
{
        NWFI_LOCKINIT(&nucfs_list_lock, NUCFS_HIERLIST, PLTIMEOUT,
                                &nucfs_list_lkinfo, KM_NOSLEEP);
	NWFI_FSPININIT(&NWfiNameTableLock);
	NWFI_FSPININIT(&NWfiTimeLock);
	NWFI_SLEEPINIT(&NWfiMountVolumeLock, (uchar_t) 0,
				&nucfs_mount_lkinfo ,KM_NOSLEEP);
	NUCFS_RELEASE_INIT();	/* initialize release event */
}

void
nucfs_lock_deinit(void)
{
        NWFI_LOCKDEINIT(&nucfs_list_lock);
	NWFI_SLEEPDEINIT(&NWfiMountVolumeLock);
}

/*
 * void
 * nucfs_timeout_start(void)
 *  -	Global (not per volume) timeout service to signal attribute
 *	flush daemons, to process nodes on volume timedNode lists.
 *	To be called from nucfs_init(), at start time.
 *
 * Because this is called at startup time, there is no lock protection
 * necessary for this function.
 */
void
nucfs_timeout_start(void)
{
	void *co = itimeout_allocate(KM_SLEEP);
	nucfs_timeoutid = itimeout_a(nucfs_timeout,
		(void *)NULL, ((HZ/10) | TO_PERIODIC), PLTIMEOUT, co);
}

/*
 * void
 * nucfs_timeout_stop(void)
 *  - To be called from nucfs_deinit().
 *
 * At the point that this function is called, the nucfs file system
 * module is in the process of being unloaded/de-inited. As a result,
 * this function does not need any lock protection.
 */
void
nucfs_timeout_stop(void)
{
	untimeout(nucfs_timeoutid);
}

/*
 * int
 * NWfiFlushInit(void)
 *	Initialize the flush daemon/s for a nucfs volume being mounted.
 *	Call spawn_sys_lwp to create our daemons as LWPs of sysproc.
 *
 * Calling/Exit State:
 *      No locks held on entry and exit. 
 *
 * Return value:
 *       0: is returned on success.
 *       -1: is returned on failure, or if we can't
 *		start the daemons.  
 *
 * At the point that this function is called, volFlushData is privately
 * held by the LWP that is performing mount. So no lock cover is needed.
 */
int
NWfiFlushInit(NWFI_VOLFLUSH_DATA_T	**volFlushData, int volIndex)
{
	k_lwpid_t pagePushLwpId;
	k_lwpid_t attFlushLwpId;
	int retval = (-1);
	
	NVLT_ENTER(2);

	(*volFlushData) = kmem_zalloc(sizeof(NWFI_VOLFLUSH_DATA_T), KM_SLEEP);
	
	NWFI_EV_INIT(&((*volFlushData)->pagePushEvent));
	NWFI_EV_INIT(&((*volFlushData)->attFlushEvent));

	(*volFlushData)->volumeIndex = volIndex;

	/* 
	 * We don't set LWP_DETACHED so we can wait for the daemons to exit.
	 */
	if (spawn_sys_lwp(&pagePushLwpId, 0, (void (*)(void *))nuc_pagepushd, 
			(void *)(*volFlushData))) {
		goto fail1;
	}
	if (spawn_sys_lwp(&attFlushLwpId, 0, (void (*)(void *))nuc_attflushd, 
			(void *)(*volFlushData))) {
		/* 
		 * need to signal nuc_pagepushd, and wait for it to
		 * exit before we can return.
		 */
		goto fail2;
	}

	(*volFlushData)->pagePushLwpId = pagePushLwpId;
	(*volFlushData)->attFlushLwpId = attFlushLwpId;
	retval = 0;
	NVLT_LEAVE((uint_t)retval);
	return (retval);
fail2:
	NWfiPagePushDeInit((*volFlushData));
fail1:
	kmem_free(*volFlushData, sizeof(NWFI_VOLFLUSH_DATA_T));
	NVLT_LEAVE((uint_t)retval);
	return (retval);
}

/*
 * At the point that this function is called, volFlushData is privately
 * held by the LWP that is performing mount. So no lock cover is needed.
 */

void
NWfiFlushActivate(
	struct NWfsServerVolume	*netwareVolume, 
	NWFI_VOLFLUSH_DATA_T	*volFlushData)
{
	NVLT_ENTER(2);
	volFlushData->serverVolume = netwareVolume;
	NWFI_GET_CLOCK(volFlushData->lastStaleCheckTime);	
	NWFI_EV_BROADCAST(&(volFlushData->pagePushEvent));
	NWFI_EV_BROADCAST(&(volFlushData->attFlushEvent));
	NVLT_VLEAVE();
	return;
}


/*
 * void
 * NWfiFlushDeInit( NWFI_VOLFLUSH_DATA_T	*volFlushData)
 *	Kill the volume flush daemons and release the memory associated
 *	with its volFlushData.
 *
 * Calling/Exit State:
 *      No locks held on entry and exit. None required, since this
 *	function would only be executed under circumstances in which
 *	racing accesses to the data would not occur.
 */
void
NWfiFlushDeInit(NWFI_VOLFLUSH_DATA_T *volFlushData)
{
	NVLT_ENTER(1);
	NWfiPagePushDeInit(volFlushData);
	NWfiAttFlushDeInit(volFlushData);
	kmem_free(volFlushData, sizeof(NWFI_VOLFLUSH_DATA_T));
	NVLT_VLEAVE();
	return;
}

/*
 * Kill the page-push daemon associated with a volume. 
 */
STATIC	void
NWfiPagePushDeInit(NWFI_VOLFLUSH_DATA_T *volFlushData)
{
	k_lwpid_t pagePushLwpId;

	NVLT_ENTER(1);

	pagePushLwpId = volFlushData->pagePushLwpId;
	volFlushData->flags |= VOL_FLUSH_DEINIT;
	NWFI_EV_BROADCAST(&(volFlushData->pagePushEvent));
	(void)wait_sys_lwp(pagePushLwpId);
	NVLT_VLEAVE();
	return;
}

/*
 * Kill the attribute flush daemon associated with a volume.
 */
STATIC	void
NWfiAttFlushDeInit(NWFI_VOLFLUSH_DATA_T *volFlushData)
{
	k_lwpid_t attFlushLwpId;

	NVLT_ENTER(1);
	attFlushLwpId = volFlushData->attFlushLwpId;
	volFlushData->flags |= VOL_FLUSH_DEINIT;
	NWFI_EV_BROADCAST(&(volFlushData->attFlushEvent));
	(void)wait_sys_lwp(attFlushLwpId);
	NVLT_VLEAVE();
	return;
}
