/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nwfidata.h	1.7"
#ident  "@(#)nwfidata.h	1.3"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nwfidata.h,v 2.6.2.5 1995/01/24 18:14:56 mdash Exp $"

#ifndef _NWFIDATA_H	/* wrapper symbol for kernel use */
#define _NWFIDATA_H	/* subject to change without notice */

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/buf.h>		/* REQUIRED */
#include <fs/nucfs/nucfslk.h>	/* PORTABILITY */
#include <fs/vnode.h>		/* REQUIRED */
#include <proc/proc.h>		/* PORTABILITY */
#include <proc/user.h>		/* PORTABILITY */
#include <svc/systm.h>		/* PORTABILITY */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/listasm.h>	/* PORTABILITY */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/buf.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/listasm.h>	/* PORTABILITY */
#include <sys/nucfslk.h>	/* PORTABILITY */
#include <sys/proc.h>		/* PORTABILITY */
#include <sys/systm.h>		/* PORTABILITY */
#include <sys/types.h>		/* REQUIRED */
#include <sys/user.h>		/* PORTABILITY */
#include <sys/vnode.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Data structure for defining doubly linked lists.
 */

typedef struct NWfiList {
	struct NWfiList		*flink;
	struct NWfiList		*rlink;
} NWFI_LIST_T;

#define NWFI_NEXT_ELEMENT(list)		((list)->flink)
#define NWFI_PREV_ELEMENT(list)		((list)->rlink)

typedef struct buf		NWFI_BUF_T;
typedef off_t			NWFI_OFF_T;



#define NWFI_CLOCK_AGED(nodeTime, ageTicks)	\
	(lbolt - (nodeTime) >= (ageTicks))



/*
 * List ops for the NWFI_HBUF_T (per volume asyncIoList) lists.
 */
#define NWFI_BUF_FORW(x)	((NWFI_BUF_T *)(x))->av_forw
#define NWFI_BUF_BACK(x)	((NWFI_BUF_T *)(x))->av_back
#define NWFI_BUF_EMPTY(x)	((x) == (void *)NWFI_BUF_FORW(x))
#define NWFI_BUF_INIT(x)		{				\
	NWFI_BUF_FORW(x)	= (void *)(x);				\
	NWFI_BUF_BACK(x)	= (void *)(x);				\
	}

#define NWFI_BUF_REMOVE(x)	{					\
	NWFI_BUF_FORW(NWFI_BUF_BACK(x)) = (void *)(NWFI_BUF_FORW(x));	\
	NWFI_BUF_BACK(NWFI_BUF_FORW(x)) = (void *)(NWFI_BUF_BACK(x)); 	\
	NWFI_BUF_INIT(x);						\
	}

/* Add x to the tail of list headed by y */
#define NWFI_BUF_ADD(x, y)	{					\
	NWFI_BUF_BACK(x) 		= (void *)NWFI_BUF_BACK(y);	\
	NWFI_BUF_FORW(NWFI_BUF_BACK(y))	= (void *)(x);			\
	NWFI_BUF_FORW(x) 		= (void *)(y);			\
	NWFI_BUF_BACK(y) 		= (void *)(x);			\
	}

/*
 * vnode definition
 */
typedef vnode_t NWFI_NODE_T;

/*
 * A nucfs time stamp (in local time).
 */
typedef struct {
	clock_t		ticks;
	uint_t		seqNo;
} NWFI_TIME_STAMP_T;

/*
 * Time stamp used by the name cache.
 */
typedef clock_t NWFI_CLOCK_T;

/*
 * Lock/Synchronization definitions
 */
typedef struct {
	lock_t		spinLock;
	k_pl_t		savedPl;
} NWFI_LOCK_T;

typedef sv_t		NWFI_SV_T;
typedef rwsleep_t	NWFI_RWLOCK_T;
typedef	sleep_t		NWFI_SLEEP_T;
typedef	fspin_t		NWFI_FSPIN_T;

#define NWFI_LKINFODECL		LKINFO_DECL
#define	NWFI_LOCKINIT(nwLockP, a, b, c, d) \
	LOCK_INIT(&((nwLockP)->spinLock), a, b, c, d)
#define	NWFI_SLEEPINIT		SLEEP_INIT
#define	NWFI_FSPININIT		FSPIN_INIT
#define	NWFI_LOCKDEINIT(nwLockP) \
	LOCK_DEINIT(&((nwLockP)->spinLock))
#define	NWFI_SLEEPDEINIT	SLEEP_DEINIT

#ifdef _KERNEL
/*
 * Insert an element onto a double linked list.
 */
#define NWfiInsque(e, l)	insque(e, l)

/*
 * Remove an element from a double linked list.
 */
#define NWfiRemque(e)		remque(e)

/*
 * Initialize an NWFI_LIST_T list.
 */
#define NWFI_LIST_INIT(l)	((l)->flink = (l)->rlink = (l))

/*
 * Is this is empty?
 */
#define NWFI_LIST_EMPTY(l)	((l)->flink == (l))

/*
 * Name Comparison Operators
 */
#define NWfiNameCmp		bcmp
#define NWfiNameCopy		bcopy

/*
 * Buffer operations.
 */
#define NWfiBufInsque(bp, dp)	binshash(bp, dp)
#define NWfiBufRemque(bp)	bremhash(bp)

/*
 * Local Time Maintenence
 */

/*
 * Extern data to maintain the time for nucfs.
 */
extern fspin_t			NWfiTimeLock;
extern NWFI_TIME_STAMP_T	NWfiBolt;
extern clock_t			NWfiStaleTicks;
extern NWFI_CLOCK_T		nwfsRHCacheQuantum; 
extern NWFI_CLOCK_T		nwfsNodeCacheQuantum;

/*
 * Macro to get the time.
 */
#define NWFI_GET_CLOCK(clock)		((clock) = lbolt)

#define NWFI_GET_TIME(timep) {			\
	clock_t ticks;				\
						\
	NWFI_GET_CLOCK(ticks);			\
	FSPIN_LOCK(&NWfiTimeLock);		\
	if (ticks != NWfiBolt.ticks) {		\
		NWfiBolt.ticks = ticks;		\
		NWfiBolt.seqNo = 0;		\
	} else {				\
		++NWfiBolt.seqNo;		\
	}					\
	*(timep) = NWfiBolt;			\
	FSPIN_UNLOCK(&NWfiTimeLock);		\
}

/* 
 * NWFI_CLOCK_T's can be read/written atomically on many platforms
 * without the need for lock protection -- assuming that readers can
 * tolerate stale but self-consistent snapshots of the values. 
 *
 * Because this is a platform specific property, the following two
 * aliases are provided for reading and writing the NWFI_CLOCK_T
 * values. 
 */
#define NWFI_CLOCK_READ(x)		(x)
#define NWFI_CLOCK_WRITE(x, ticks)	((x) = (ticks))
 

/*
 * Is a given time stale?
 */
#define NUCFS_CLOCK_STALE(clock)	(lbolt - (clock) >= NWfiStaleTicks)
#define NUCFS_STALE_ATTRIBS(timep)	NUCFS_CLOCK_STALE((timep)->ticks)

/*
 * Is time1 in advance of time2?
 */
#define NWFI_GT(time1p, time2p) 				\
	((time1p)->ticks - (time2p)->ticks > 0 ||		\
	 ((time1p)->ticks == (time2p)->ticks &&			\
	  (time1p)->seqNo > (time2p)->seqNo))

/*
 * Lock/Synchronization definitions
 */
#define NWFI_LOCK(lockp)	\
	((lockp)->savedPl = (k_pl_t) LOCK(&(lockp)->spinLock, PLTIMEOUT))
#define NWFI_UNLOCK(lockp)	\
	UNLOCK(&(lockp)->spinLock, (lockp)->savedPl)
#ifdef DEBUG
#define NWFI_LOCK_OWNED(lockp)	\
	LOCK_OWNED(&(lockp)->spinLock)
#else /* !DEBUG */
#define NWFI_LOCK_OWNED(lockp)	(B_TRUE)
#endif /* DEBUG */

#define NWFI_RD_LOCK(lockp)	RWSLEEP_RDLOCK(lockp, PRINOD)
#define NWFI_WR_LOCK(lockp)	RWSLEEP_WRLOCK(lockp, PRINOD)
#define NWFI_RW_UNLOCK(lockp)	RWSLEEP_UNLOCK(lockp)

/*
 * Wait/Wakeup Operations
 */
#define NWFI_WAIT(lockp, svp)	\
	SV_WAIT((svp), PRIVFS, &((lockp)->spinLock))

#define NWFI_WAKEUP(svp) {					\
	if (SV_BLKD(svp)) {					\
		SV_BROADCAST((svp), 0);				\
	}							\
}

/*
 * NWFI layer aliases for event operations
 */
#define NWFI_EV_WAIT(evp)	EVENT_WAIT((evp), PRIVFS)
#define NWFI_EV_WAIT_SIG(evp) 	EVENT_WAIT_SIG((evp), PRIVFS)
#define	NWFI_EV_INIT(evp)	EVENT_INIT((evp))
#define	NWFI_EV_SIGNAL(evp)	EVENT_SIGNAL((evp), 0)
#define	NWFI_EV_BROADCAST(evp)	EVENT_BROADCAST((evp), 0)
#define	NWFI_EV_ALLOC(slpflag)	EVENT_ALLOC((slpflag))
#define	NWFI_EV_DEALLOC(evp)	EVENT_DEALLOC((evp))


/*
 * List lock.
 */
extern NWFI_LOCK_T		nucfs_list_lock;
#define NUCFS_LIST_LOCK()	NWFI_LOCK(&nucfs_list_lock)
#define NUCFS_LIST_UNLOCK()	NWFI_UNLOCK(&nucfs_list_lock)
#define NUCFS_LIST_LOCK_OWNED()	NWFI_LOCK_OWNED(&nucfs_list_lock)

/*
 * resource synch variable for server node recycling
 */
extern sv_t			nucfs_resource_release_sv;
#define NUCFS_RELEASE_WAIT()	{					\
	NWFI_WAIT(&nucfs_list_lock, &nucfs_resource_release_sv);	\
}

#define	NUCFS_RELEASE_WAKE()	NWFI_WAKEUP(&nucfs_resource_release_sv)
#define	NUCFS_RELEASE_INIT()	SV_INIT(&nucfs_resource_release_sv)

/*
 * Implementation specific SNODE operations.
 */
#define NWFI_SNODE_INIT(sp)	{					\
        LOCK_INIT(&(sp)->snodeLock.spinLock, NUCFS_HIERSTATE,		\
		PLTIMEOUT, &nucfs_snode_lkinfo, KM_SLEEP);		\
        RWSLEEP_INIT(&(sp)->snodeRwLock, NUCFS_HIERRW,			\
		     &nucfs_snode_rw_lkinfo, KM_SLEEP);			\
	NWFI_CHANDLE_INIT(&(sp)->clientHandle);				\
	SV_INIT(&(sp)->snodeSync);					\
}

#define NWFI_SNODE_DEINIT(sp)	{					\
	LOCK_DEINIT(&(sp)->snodeLock.spinLock);				\
	RWSLEEP_DEINIT(&(sp)->snodeRwLock);				\
	NWFI_CHANDLE_DEINIT(&(sp)->clientHandle);			\
}

extern lkinfo_t		nucfs_snode_lkinfo;
extern lkinfo_t		nucfs_snode_rw_lkinfo;

/*
 * Implementation specific client handle operations.
 */
#define NWFI_CHANDLE_INIT(chp)	{					\
	LOCK_INIT(&(chp)->nameLock.spinLock, NUCFS_HIERNAME,		\
		PLTIMEOUT, &nucfs_name_lkinfo, KM_SLEEP);		\
}

#define NWFI_CHANDLE_DEINIT(chp)    {					\
	LOCK_DEINIT(&(chp)->nameLock.spinLock);				\
}

extern lkinfo_t		nucfs_name_lkinfo;

/*
 * Cred handling.
 */
#define NWfiFsToNucCred(nwfsCred, nucCredentials) {		\
	(nucCredentials)->userID = (nwfsCred)->userId;		\
	(nucCredentials)->groupID = (nwfsCred)->groupId;	\
	(nucCredentials)->pid = u.u_procp->p_epid;		\
	(nucCredentials)->flags = NWC_OPEN_PUBLIC;		\
}

#define NWfiUnixToFsCred(unixCredentials, nwfsCred) {		\
	(nwfsCred)->userId = (unixCredentials)->cr_uid;		\
	(nwfsCred)->groupId = (unixCredentials)->cr_gid;	\
}

#define NWFI_CRED_MATCH(cred1, cred2) 		\
	((cred1)->userId == (cred2)->userId &&	\
	 (cred1)->groupId == (cred2)->groupId)

/*
 * Prototypes for NVLT functions.
 *
 * XXX: They live here because NVLT didn't provide them.
 */
extern void	NVLTenter(uint_t type, int argc);
extern uint_t 	NVLTleave(uint_t type, uint_t retcode);
extern void	NVLTvleave(uint_t type);
extern void	NVLTstring(uint_t mask, char *string);
extern void	NVLTtrace(uint_t type, uint_t v1, uint_t v2,
			  uint_t v3, uint_t v4);
extern void	NVLTtrace_off(void);
extern void	NVLTstrlog_off(void);
extern int	NVLTassfail(uint_t mid, const char *a, const char *f, int l);

/*
 * Some other exported functions.
 */
struct NWfiVolumeFlushData;

extern void	nucfs_lock_init(void);
extern void	nucfs_lock_deinit(void);
extern void	nuc_pagepushd(struct NWfiVolumeFlushData *);
extern void 	nuc_attflushd(struct NWfiVolumeFlushData *);

#endif /* KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NWFIDATA_H */
