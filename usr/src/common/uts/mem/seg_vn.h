/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_SEG_VN_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_VN_H	/* subject to change without notice */

#ident	"@(#)kern:mem/seg_vn.h	1.22"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/param.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <proc/mman.h>		/* REQUIRED */
#include <mem/seg_vn_f.h>	/* PORTABILITY */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/param.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/mman.h>		/* REQUIRED */
#include <vm/seg_vn_f.h>	/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

/*
 * A pointer to this structure is passed to segvn_create().
 */
struct segvn_crargs {
	struct	vnode *vp;	/* vnode mapped from */
	off_t	offset;		/* starting offset of vnode for mapping */
	struct	cred *cred;	/* credentials */
	uchar_t	type;		/* type of sharing done */
	uchar_t	prot;		/* protections */
	uchar_t	maxprot;	/* maximum protections */
};

/*
 * (Semi) private data maintained by the seg_vn driver per segment mapping.
 *
 *
 * NOTES: locking in segvn_data
 * ----------------------------
 *  Segments may become involved in a number of different types of activity
 *  which require serialization among the LWPs sharing access to the segment.
 *  To achieve this, a number of pseudo lock types are implemented, both on a
 *  per-segment and per-virtual page basis. The per-segment locks are:
 *
 *	A four state SEGLOCK. The lock is in a unique state at any one time.
 *	However, for certain states, multiple LWPs can share the lock.
 *	The states are:
 *
 * 	    WRITE state
 *
 * 		Only a single LWP can aquire the lock in WRITE state.
 *
 * 	    READ state
 *
 * 		Multiple LWPs can aquire the lock in READ state.
 *
 * 	    INTENT state
 *
 * 		Multiple LWPs can aquire the lock in INTENT state.
 *
 *	    FREE state
 *
 *		In this state, no LWPs have aquired the lock.
 *
 * 	In addition, an AMP_LOCK pseudo lock guards the instantiation of
 *	the anon map (svd_amp). The AMP_LOCK is an exclusive lock.
 *
 * 		This lock is different from the lock within the anon_map.
 *
 *  The virtual page lock is a two state pseudo lock:
 *
 * 	virtual page READ state
 *
 *		Multiple LWPs can share the lock in READ state. The segment
 *		level INTENT lock is always taken before any virtual page
 *		lock is taken. (See mem/vpage.h and mem/vm_vpage.c for more
 *		information.)
 *
 *  	virtual page WRITE state
 *
 *		Only a single LWP can hold the lock in WRITE state. The
 *		segment level INTENT lock is always taken before any
 *		virtual page lock is taken. (See mem/vpage.h and
 *		mem/vm_vpage.c for more information.)
 *
 *  The operations which require serialization, and the corresponding
 *  serialization constraints are:
 *
 *  	COW faults
 *
 * 		These faults change the page identity of the affected
 * 		virtual pages (by replacing the page with a private
 * 		copy). A new anon_t is also allocated by anon_private().
 *
 * 		These faults are serialized with all other fault and
 * 		SOFTLOCKs operations on the same virtual pages.
 *
 *		When faulting on a single page, the vpage_lock_range()
 *		operation is used to obtain either a SEGLOCK WRITE lock, or
 *		a SEGLOCK INTENT lock plus a virtual page WRITE lock.
 *
 * 	ZFOD faults
 *
 * 		These faults also change the page identity of the affected
 * 		virtual pages, actually by instantiating these pages via
 * 		anon_zero().
 *
 * 		These faults are serialized with all other fault and
 * 		SOFTLOCK operations operating on the same virtual pages.
 *
 *		When faulting on a single page, the vpage_lock_range()
 *		operation is used to obtain either a SEGLOCK WRITE lock, or
 *		SEGLOCK INTENT lock plus a virtual page WRITE lock.
 *
 * 	other faults
 *
 * 		Non-COW/ZFOD faults do not change the page identity, and
 * 		so can execute concurrently with each other, and can
 * 		execute while SOFTLOCKs are held.
 *
 *		When faulting on a single page, the vpage_lock_range()
 *		operation is used to obtain either a SEGLOCK READ lock, or
 *		SEGLOCK INTENT lock plus a virtual page READ lock.
 *
 *	loading translations for non-faulted pages
 *
 *		These translations are loaded as part of an optimization to
 *		implement klustered file fill. The serialization
 *		constraints are exactly the same as for non-COW/ZFOD
 *		faults. However, this action takes place while processing a
 *		fault on an adjacent page, so that the segment level lock has
 *		already been obtained. If a segment READ or WRITE lock is
 *		held, then no more locking is needed. If an INTENT lock is
 *		held, then a page granular READ lock is required. However,
 *		it is not possible to wait for it, as waits must occur in
 *		order of increasing virtual address. Thus, a "trylock" type
 *		of operation is performed. The optimization is skipped if
 *		the trylock fails.
 *
 * 	SOFTLOCKs
 *
 * 		The segment driver makes a guarantee to the caller that a
 * 		SOFTLOCKed page will not be changing identity. Therefore,
 * 		a SOFTLOCKed page must be immune to COW and ZFOD faults.
 *
 * 		The instantiating phase of an F_SOFTLOCK request is subject
 * 		to the same serialization constraints as the corresponding
 * 		F_INVAL/F_PROT fault (COW, ZFOD, or other).
 *
 * 		In the instantiating phase, vpage_lock_range is used to
 * 		obtain locks just as for the F_INVAL/F_PROT case.
 * 		These locks are then downgraded to READ mode for the
 * 		holding phase (see routine vpage_downgrade_range()).
 *
 * 	anon_map instantiation
 *
 * 		An anon_map instantiation can occur while the segment is
 * 		being created (i.e. is privately held), while the AS is
 * 		write locked, or during a fault. In the latter case, the
 * 		segment driver provides serialization. Anon_map
 * 		instantiation attempts are serialized with each other, but
 * 		not with other types of activity. Serialization of
 * 		anon_map instantiation attempts ensures that only one
 * 		anon_map is created for any given segment.
 *
 *		If an anon_map is instantiated by a fault, then the segment
 *		level AMP_LOCK is taken to inhibit concurrent anon_map
 *		instantiation. The AMP_LOCK is taken and released before
 *		the SEGLOCK is aquired.
 *
 *		One consequence of this method is that holders of the
 *		segment READ or WRITE locks may no longer assume that
 *		they are inhibiting the instantiation of the anon_map
 *		(this had previously been assumed by routine segvn_fault(),
 *		but now see routine segvn_pre_page() for adjustments).
 *
 * 	swap reservation
 *
 * 		In order to keep the pvn_memresv accounting in order,
 * 		swap reservation must be serialized with all SOFTLOCKs,
 * 		Swap reservation can either occur with the AS write locked,
 * 		or can occur during an F_MAXPROT_SOFTLOCK/S_WRITE fault.
 * 		If the latter case, the segment driver ensures serialization
 * 		with SOFTLOCKs.
 *
 * 		Within the F_MAXPROT_SOFTLOCK/S_WRITE fault, the
 * 		segment level WRITE lock is taken.
 *
 * 	segvn_dup protection changes
 *
 * 		A fork operation which causes writable pages to be shared
 * 		will change the protection on translations (i.e. write
 * 		protect them). This is inherently incompatible with the
 * 		guarantees given to the holders of SOFTLOCKs. Therefore,
 * 		this operation is serialized with all SOFTLOCK.
 *
 * 		This is done either with a segment level WRITE lock,
 * 		or with vpage_lock_range obtained WRITE locks.
 *
 * 	vpage lock array instantiation
 *
 * 		Concurrent instantiators of the svd_lock array are allowed
 * 		to race.  However, the svd_lock variable is mutexed by
 * 		the segment spin lock, allowing only one LWP to win.
 * 		The losers, if any, must kmem_free their arrays.
 *
 *	vpage info instantiation
 *
 *		We are still requiring either the AS WRITE lock, or the
 *		segment WRITE lock, to instantiate the vpage array (now
 *		called the "vpage info" array). This can probably be
 *		relaxed, but it is not an important performance issue.
 *
 * The following LWPs have access to the segment:
 *
 * 	=> any LWPs running in the process which owns the AS
 *
 * 	=> any LWP making access through /proc
 *
 *  Due to the existence of /proc, synchronization is required even when the
 *  process is single threaded. However, for many (if not most) segments, only
 *  one LWP will ever access them. Any non-multithreaded application not under
 *  debug will exhibit such segments. Therefore, the synchronization mechanism
 *  should be of sufficiently light weight so as not to encumber such
 *  processes. On the other hand, some multi-threaded applications might
 *  exhibit significant concurrent COW/ZFOD fautling behaviors, which we want
 *  to permit (should they be operating on non-interesecting virtual pages).
 *
 *  For this reason, an adaptive locking scheme is implemented (see function
 *  vpage_lock_range() in mem/vm_vpage.c). For the case of the non-multithreaded
 *  application not under debug, only segment level locks will be obtained. For
 *  the case of competing faults from multiple LWPs, a page granular locking
 *  array (svd_lock) will be allocated - resulting in faults taking out page
 *  granular locks.
 *
 *  A number of operations require no synchonization because they are
 *  performed with the AS write locked.  These include:
 *
 * 	protections changes
 *
 * 	memory locking/unlocking
 *
 * 	sync to backing store
 *
 *  NOTES: locks in segvn_data
 *  --------------------------
 *  svd_seglock    Per-segvn segment spinlock which mutexes the following
 * 		   data:
 *
 *  			svd_lckflag, svd_lckcnt, svd_lock, svd_lock[*],
 * 			svd_anonlck
 *
 * 		   In addition, waits on the following synchonization
 * 		   variables are initiated with the svd_seglock held:
 *
 * 			svd_segsv, svd_vpagesv
 *
 *                 In effect, this spin lock is used to synthesize the
 * 		   pseudo sleep locks implemented by the segment driver.
 *
 *  svd_lckflag    State of the per segment INTENT/READer/WRITEr pseudo
 * 		   sleeplock (protected by svd_seglock).
 *
 *  svd_lckcnt     Count of the number of readers, or intent lock holders,
 * 		   sharing the segment pseudo lock (protected by svd_seglock).
 *
 * 		   The following is the truth table for the pseudo
 * 		   intent/reader/writer lock synthesized from the
 * 		   svd_lckflag and svd_lckcnt fields:
 *
 *  		   	svd_lckflag	svd_lckcnt	state
 *  			----------      ----------      -------------
 *  			     0		     0		  unlocked
 *  			SEGVN_IXLCK        >=1            INTENT locked
 *  			SEGVN_RDLCK        >=1            READer locked
 *  			SEGVN_WRLCK          1		  WRITEr locked
 *
 * 		   All other states are illegal.
 *
 *  svd_anonlck	   Value of 1 indicates that the AMP_LOCK is held. and
 *		   0 indicates that it is available.
 *
 *  svd_segsv      Sync variable used as rendezvous for the per-segment pseudo
 *                 READer/WRITEr/INTENT sleeplock described above. Also,
 * 		   used for the per segment AMP_LOCK pseudo sleep lock.
 *
 *  svd_vpagesv    Sync variable used as rendezvous for vpage_lock array
 * 		   based locks which must block.
 *
 *  The following fields are protected by the AS lock:
 *
 *	svd_prot, svd_flags, svd_info, svd_info[*], svd_anon_index
 *
 *  Changes to the following field are protected either by the
 *  AS write lock, or by the per-segment AMP_LOCK pseudo sleep lock:
 *
 *	svd_amp
 *
 *  The following fields are not changed following creation of the
 *  segment:
 *
 *	svd_maxprot, svd_type, svd_vp, svd_offset, svd_cred
 *
 *  NOTES: lock ordering in segvn:
 *  ------------------------------
 *
 *  An LWP obtains locks in the following order:
 *
 * 	1. The per-segment spinlock (svd_seglock).
 *
 * 	2. A per-segment SEGLOCK or AMP_LOCK pseudo lock.
 *	   (must be SEGLOCK in INTENT state if proceeding to step 3)..
 *
 * 	3. A pseudo READ or WRITE lock on the virtual page.
 *
 *	4. Locks on the virtual pages are obtained in order of increasing
 *	   virtual address.
 *
 *  NOTES: other fields in segvn_data:
 *  ----------------------------------
 *
 *  The following notes expand upon the comments next to some of the
 *  segvn_data structure members defined below.
 *
 *  svd_info	Pointer to an array holding per-virtual page information
 *		(memory locks and protections).
 *
 *  svd_flags	This field holds four flags. The significance of each is:
 *
 *	SEGVN_PGPROT	When clear, indicates that page protections are being
 *			stored at the segment level. In this case, svd_prot
 *			contains valid protections, and either svd_info is
 *			not instantiated, or the vpage_info structures
 *			contain value 0 for the protections.
 *
 *			When set, indicates that page protections are being
 *			stored on a per-page basis in the svd_info array.
 *			In this case, svd_prot contains the value 0.
 *
 *	SEGVN_MEMLCK	When clear, indicates that the segment does not
 *			hold "memory locked" pages (locked via either
 *			memcntl(2), plock(2), or shmctl(2)). In this case,
 *			either svd_info is not instantiated, or the
 *			VPI_MEMLOCK bit in the vpi_flags field is clear for
 *			all pages.
 *
 *			When set, indicates that the svd_info array is
 *			instantiated, and that the VPI_MEMLOCK bit of
 *			each vpi_flags field indicates the memory locked
 *			state of each virtual page.
 *
 *	SEGVN_PROTO	When set, indicates that the segment has yet to
 *			complete creation. The segvn_data structure is
 *			allocated in automatic storage in routine
 *			segvn_create, and may not be kmem_freed.
 *
 *	SEGVN_MLCKIP	When set, indicates that a segvn_lockop(MC_LOCK)
 *			memory locking operation is in progress.
 *
 *  svd_lock	Pointer to an array holding page granular locks.
 *
 *  svd_anon_index
 *
 * 		Used when the segment has an anon_map (svd_amp is non-NULL) but
 * 		represents less than the entire contents of the anon array
 * 		(am_anon). The value stored here is added to the virtual page
 * 		number returned by a seg_page call to lookup anon pages in the
 * 		partially used array.
 *
 *  GENERAL NOTE ON F_SOFTLOCK/F_MAXPROT_SOFTLOCK processing in segvn:
 *  -----------------------------------------------------------------
 *
 *  F_SOFTLOCK and F_MAXPROT_SOFTLOCK requests share the same basic code path
 *  through segvn_fault. However, F_MAXPROT_SOFTLOCK can exercise some special
 *  code paths installed especially for it. The basic differences between
 *  these two types are:
 *
 * 	F_SOFTLOCK:
 *
 * 		F_SOFTLOCK exercises the normal fault code path, except:
 *
 * 		(a) can process multiple pages in one request
 * 		(b) reserves REAL memory for VOP_GETPAGE gotten pages
 * 		(c) downgrades vpage locks on exit (instead of releasing them)
 * 		(d) locks the translations
 * 		(e) pvn_memresv(es) the pages
 * 		(f) doesn't page_unlock() the pages
 *
 * 	F_MAXPROT_SOFTLOCK:
 *
 * 		F_MAXPROT_SOFTLOCK does all of what F_SOFTLOCK does, but
 * 		in addition, with rw == S_WRITE when operating on a read-only
 * 		page, does some things differently:
 *
 * 		(a) bypasses the usual protection check (checks protection
 * 		    against svd_maxprot only)
 * 		(b) may reserve swap space
 * 		(c) may COW a memory locked page (but never a SOFTLOCKed page)
 * 		(d) may instantiate backing store for a vnode backed page
 *
 *		These actions are needed for writing to the segment via
 *		/proc.
 */
struct	segvn_data {
	uchar_t	svd_prot;	  /* protection used if SEGVN_PGPROT not set */
	uchar_t	svd_maxprot;	  /* maximum segment protections */
	uchar_t	svd_type;	  /* type of segment (shared or private) */
	uchar_t	svd_flags;	  /* flags: see below for definitiions */
	struct	vnode *svd_vp;	  /* vnode that segment mapping is to */
	off_t	svd_offset;	  /* starting offset of vnode for mapping */
	uint_t	svd_anon_index;	  /* starting index into svd_amp->am_anon */
	struct	anon_map *svd_amp;/* pointer to anon map structure */
	struct vpage_lock *svd_lock; /* per-virtual page locks */
	struct vpage_info *svd_info; /* per-virtual page information */
	struct	cred *svd_cred;	  /* mapping credentials */
	size_t	svd_swresv;	  /* swap space reserved for this segment */

	/*
	 * pseudo read/write lock and conditions for segment.
	 * see comment above for complete description
	 */

	lock_t  svd_seglock;	/* per-segment spinlock */
	uchar_t	svd_anonlck;	/* anon_map instantiation lock */
	uchar_t	svd_lckflag;	/* lock type held */
	ushort_t svd_lckcnt;	/* count of reader or intent locks held on */
				/* segment */

	/*
  	 * sync variables for managing pseudo sleep locks
	 */

	sv_t	svd_segsv;	/* sync variable for segment level lock */
	sv_t	svd_vpagesv;	/* sync variable for vpage level locks */
};

/*
 * definitions for field svd_flags
 */
#define SEGVN_PGPROT	(1 << 0)	/* using page granular protections */
#define SEGVN_MEMLCK	(1 << 1)	/* memory pages were locked */
#define SEGVN_PROTO	(1 << 2)	/* too early to really be alive */
#define SEGVN_MLCKIP	(1 << 3)	/* memory locking in progress */

/*
 * definition for field svd_lckflag
 */

#define SEGVN_IXLCK	(1 << 0) 	/* intent lock(s) are held */
#define SEGVN_RDLCK	(1 << 1)	/* read lock(s) are held */
#define SEGVN_WRLCK	(1 << 2)	/* write lock is held */

#ifdef _KERNEL

#define VM_SEGLCK_PRI	(PRIMEM - 1) /* pri of LWPs blocked on segment locks */
#define VM_VPAGE_PRI	(PRIMEM - 1) /* pri of LWPs blocked behind faults */
#define VM_ANONMAP_PRI  (PRIMEM - 1) /* pri of LWPs blocked behind am_lock(s) */

struct seg;
extern int segvn_create(struct seg *, const void * const);
extern struct seg_ops segvn_ops;

#define IS_SEGVN(seg)	((seg)->s_ops == &segvn_ops)

/*
 * SEGLOCK_READ(sp)
 * 	Acquire a read lock on the specified segment.
 *
 * Calling/Exit State:
 *	Invoked holding svd_seglock for the specified segment and returns
 *	with it held.  svd_seglock can be dropped in the interim if we
 *	must wait for the lock.
 */

#define SEGLOCK_READ(sp) { \
	ASSERT(LOCK_OWNED(&(sp)->svd_seglock)); \
	ASSERT(KS_HOLD1LOCK()); \
        while ((sp)->svd_lckflag & (SEGVN_IXLCK|SEGVN_WRLCK)) { \
                SV_WAIT(&(sp)->svd_segsv, VM_SEGLCK_PRI, &(sp)->svd_seglock); \
                (void)LOCK(&(sp)->svd_seglock, VM_SEGVN_IPL); \
        } \
	(sp)->svd_lckflag = SEGVN_RDLCK; \
        (sp)->svd_lckcnt++; \
	}

/*
 * SEGLOCK_INTENT(sp)
 * 	Acquire an intent lock on the specified segment.
 *
 * Calling/Exit State:
 *	Invoked holding svd_seglock for the specified segment and returns
 *	with it held.  svd_seglock can be dropped in the interim if we
 *	must wait for the lock.
 */

#define SEGLOCK_INTENT(sp) { \
	ASSERT(LOCK_OWNED(&(sp)->svd_seglock)); \
	ASSERT(KS_HOLD1LOCK()); \
        while ((sp)->svd_lckflag & (SEGVN_RDLCK|SEGVN_WRLCK)) { \
                SV_WAIT(&(sp)->svd_segsv, VM_SEGLCK_PRI, &(sp)->svd_seglock); \
                (void)LOCK(&(sp)->svd_seglock, VM_SEGVN_IPL); \
        } \
	(sp)->svd_lckflag = SEGVN_IXLCK; \
        (sp)->svd_lckcnt++; \
	}

/*
 * SEGLOCK_WRITE(sp)
 * 	Acquire a write lock on the specified segment.
 *
 * Calling/Exit State:
 *	Invoked holding svd_seglock for the specified segment and returns
 *	with it held.  svd_seglock can be dropped in the interim if we
 *	must wait for the lock.
 */

#define SEGLOCK_WRITE(sp) { \
	ASSERT(LOCK_OWNED(&(sp)->svd_seglock)); \
	ASSERT(KS_HOLD1LOCK()); \
        while((sp)->svd_lckflag) { \
                SV_WAIT(&(sp)->svd_segsv, VM_SEGLCK_PRI, &(sp)->svd_seglock); \
                (void)LOCK(&(sp)->svd_seglock, VM_SEGVN_IPL); \
        } \
        ASSERT((sp)->svd_lckcnt == 0); \
        (sp)->svd_lckflag = SEGVN_WRLCK; \
        (sp)->svd_lckcnt = 1; \
	}

/*
 * SEGLOCK_DOWNGRADE(sp, type)
 * 	Atomically downgrade a lock already held for write to reader or
 *	intent lock.
 *
 * Calling/Exit State:
 *	Invoked holding svd_seglock of the specified segment and return
 *	with it held. This condition is asserted because it is required
 *	to avoid missed wakeups.
 *
 *	We also issue a sync-variable broadcast to any other LWPs blocked
 *	on svd_segsv (if necessary).
 */
#define SEGLOCK_DOWNGRADE(sp, type) { \
        ASSERT(LOCK_OWNED(&(sp)->svd_seglock)); \
        ASSERT((sp)->svd_lckflag == SEGVN_WRLCK && (sp)->svd_lckcnt == 1); \
        (sp)->svd_lckflag = type; \
        if (SV_BLKD(&(sp)->svd_segsv)) { \
		SV_BROADCAST(&(sp)->svd_segsv, 0); \
	} \
	}

/*
 * SEGLOCK_UNLOCK(sp)
 * 	Drop the lock (intent, read or write) held by caller on the specified
 *	segment; wake up anyone blocked on it.
 *
 * Calling/Exit State:
 *	Invoked holding svd_seglock for the specified segment and returns
 *	with it held.
 *
 *	We also issue a sync-variable broadcast to any other LWPs blocked
 *	on svd_segsv (if necessary).
 *
 * Remarks:
 * 	NOTE: the design of the segment-level pseudo read/write lock makes
 *	this code work whether the lock is currently owned in INTENT, READ or
 *	WRITE mode with just one test.
 */
#define SEGLOCK_UNLOCK(sp) { \
        ASSERT(LOCK_OWNED(&(sp)->svd_seglock)); \
        if (--(sp)->svd_lckcnt == 0) { \
		(sp)->svd_lckflag = 0; \
		if (SV_BLKD(&(sp)->svd_segsv)) { \
			SV_BROADCAST(&(sp)->svd_segsv, 0); \
		} \
	} \
	}

/*
 * SEGTRYLOCK_READ(sp)
 * 	Try to acquire a read lock on the specified segment, but do not
 *	block.
 *
 * Calling/Exit State:
 * 	Invoked and returns holding svd_seglock for the specified segment.
 *	Returns non-zero iff the lock is obtained.
 */

#define SEGTRYLOCK_READ(sp) \
        (ASSERT(LOCK_OWNED(&(sp)->svd_seglock)), \
	(((sp)->svd_lckflag & (SEGVN_IXLCK|SEGVN_WRLCK)) == 0) ? \
	((sp)->svd_lckflag = SEGVN_RDLCK, (sp)->svd_lckcnt++, 1) : 0)

/*
 * SEGTRYLOCK_WRITE(sp)
 * 	Try to acquire a write lock on the specified segment, but do not
 *	block.
 *
 * Calling/Exit State:
 * 	Invoked and returns holding svd_seglock for the specified segment.
 *	Returns non-zero iff the lock is obtained.
 */

#define SEGTRYLOCK_WRITE(sp) \
        (ASSERT(LOCK_OWNED(&(sp)->svd_seglock)), \
        ((sp)->svd_lckflag == 0) ? \
	((sp)->svd_lckflag = SEGVN_WRLCK, (sp)->svd_lckcnt = 1, 1) : 0)

/*
 * SEG_AMP_LOCK(sp)
 * 	Acquire the AMP_LOCK on the specified segment.
 *
 * Calling/Exit State:
 *	Invoked holding svd_seglock for the specified segment and returns
 *	with it held.  svd_seglock can be dropped in the interim if we
 *	must wait for the lock.
 */

#define SEG_AMP_LOCK(sp) { \
	ASSERT(LOCK_OWNED(&(sp)->svd_seglock)); \
	ASSERT(KS_HOLD1LOCK()); \
        while((sp)->svd_anonlck) { \
                SV_WAIT(&(sp)->svd_segsv, VM_SEGLCK_PRI, &(sp)->svd_seglock); \
                (void)LOCK(&(sp)->svd_seglock, VM_SEGVN_IPL); \
        } \
        (sp)->svd_anonlck = 1; \
	}

/*
 * SEG_AMP_UNLOCK(sp)
 * 	Drop the AMP_LOCK on the specified segment.
 *
 * Calling/Exit State:
 *	Invoked holding svd_seglock for the specified segment and returns
 *	with it held.
 */
#define SEG_AMP_UNLOCK(sp) { \
        ASSERT(LOCK_OWNED(&(sp)->svd_seglock)); \
	(sp)->svd_anonlck = 0; \
	if (SV_BLKD(&(sp)->svd_segsv)) { \
		SV_BROADCAST(&(sp)->svd_segsv, 0); \
	} \
	}

/*
 * Provided as shorthand for creating user zfod segments.
 */
extern void *zfod_argsp;

#endif /* _KERNEL) */

#if defined(__cplusplus)
	}
#endif

#endif	/* _MEM_SEG_VN_H */
