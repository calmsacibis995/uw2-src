/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_VPAGE_H	/* wrapper symbol for kernel use */
#define _MEM_VPAGE_H	/* subject to change without notice */

#ident	"@(#)kern:mem/vpage.h	1.18"
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

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <proc/mman.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/mman.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * VM - Information per virtual page.
 */

/*
 * Per-Virtual Page serialization state information (Fine Granularity)
 *
 * The virtual identity of each page in the segment can be protected
 * using only the per-segment pseudo reader/writer lock. In fact, in
 * this implementation, these locks will be used in exactly that way
 * until a conflict between LWPs is actually observed (look in
 * routine vpage_lock_range() for the detection code). At that time,
 * an array of vpage_info structures is allocated, providing finer
 * granularity serialization, and therefore improved concurrency.
 *
 * Some operations require that the virtual page identity be unchanging:
 *
 *	1. non-COW/non-ZFOD faults
 *
 *	2. SOFTLOCKs (holding phase)
 *
 * The required serialization will be provided either by a segment level
 * READ lock (escalated case), or by a segment level INTENT lock plus a page
 * granular READ lock.
 *
 * Some operations are incompatible with these operations, and thus
 * require exclusive access to the virtual page identity.  There are:
 *
 *	1. COW and ZFOD faults (including SOFTLOCK instantiating phase)
 *
 * 		Both of these operations actually allocate a new anonymous
 * 		page, changing the HAT layer's translation. Serialization
 * 		provides for the consistency of the translation and the
 *		anon_map. It also provides for anon_map consistency (by
 *		excluding anon slot instantiation races - within the
 *		context of the AS).
 * 
 * 	2. Write protecting a page at segvn_dup() time
 * 
 *		This operation is incompatible with the semantic guarantees
 *		afforded to the holders of a SOFTLOCK. This is because the
 *		SOFTLOCK holder may have given a reference to the page to
 *		an I/O device which does not obey the page protections.
 *
 * The required serialization will be provided by either a segment level
 * WRITE lock (escalated case), or by a segment level INTENT lock plus a page
 * granular WRITE lock.
 * 
 * NOTES: formulation of the vpage_lock stucture
 * ---------------------------------------------
 * 
 * Each vpage_lock consists of a single char:
 *
 *	vpl_lcktcnt	The possible values are given in the table below:
 *
 *		Value				State
 *		-----				-----
 *		N == 0				No lock is held.
 *		0 < N <= VPAGE_MAX_SHARE	N concurrent READer locks
 *						are held.
 *		N == VPAGE_WR_LOCKED		WRITEr lock is held.
 *
 *			The code takes advantage of the fact that
 *			VPAGE_MAX_SHARE < VPAGE_WR_LOCKED.
 *
 *			This field is mutexed by the segment spin lock
 *			(svd_seglock in the segvn_data structure).
 *
 * NOTES: nominative lock ordering for vpages:
 * -------------------------------------------
 *
 * Locks are taken in the following order:
 *
 *	1. The per-segment spinlock (svd_seglock)
 *
 *	2a. A pseudo READ or WRITE write lock on the segment.
 *
 *		-- or --
 *
 *	1. The per-segment spinlock (svd_seglock)
 *
 *	2b. A pseudo INTENT lock on the segment.
 *
 *	3. A pseudo READ or WRITE lock on the virtual page.
 *
 * Locks on virtual pages are always garnered while holding the per-segment
 * spinlock (svd_seglock). No other spinlocks are acquired by the vpage
 * code (other than the FSPIN manipulated as part of SV_WAITs and BROADCASTS
 * but these are not part of the locking hierarchy).
 *
 * Multiple requests to garner overlapping ranges of vpages are guaranteed not 
 * to cause deadlocks since all such requests are processed in ascending
 * virtual address order.
 */ 

typedef struct vpage_lock {
	uchar_t		vpl_lcktcnt;	/* lock exclusive/share state */
} vpage_lock_t;

/*
 * Definitions for the vpl_lcktcnt field:
 */
#define VPAGE_WR_LOCKED		((1 << NBBY) - 1)
					/* maximum READers sharing */
#define VPAGE_MAX_SHARE (VPAGE_WR_LOCKED - 1)
					/* value indicating WRITEr locked */

/*
 * Miscellaneous per virtual-page state information:
 *
 * Notes on fields in vpage_info_t:
 * -------------------------------
 *
 * vpi_nmemlck     Gives the number of reasons for holding a HAT_LOCK on
 *		   the translation. This number is the sum of the number of
 *		   SOFTLOCKs held (or intransit) on this page, plus 1 if
 *		   the page is memory locked (indicated by the VPI_MEMLOCK
 *		   bit of vpi_flags).
 *
 *		   Note that since:
 *
 *			vpi_nmemlck <= vpl_lcktcnt + 1
 *		
 *		   Overflow of this field is not possible.
 *
 * vpi_flags	   This field stores the per-page protection information
 *		   and the "memory locked" state of the virtual-page.
 *	
 *	Protections	Stored in the lower 4 bits. If page granular locks
 *			are not in effect (indicated by the SEGVN_PGPROT
 *			bit clear in svd_flags), then these bits are 0.
 *			Otherwise, they indicate the actual protections
 *			of the virtual page.
 *
 *	VPI_MEMLOCK	Stored in the high order bit. If set, indicates that
 *			the virtual page has been locked (via plock(2),
 *			memcntl(2), or shmctl(2)).
 *
 * Notes on MP protection:
 * ----------------------
 * All fields of this data structure are protected by either:
 *
 *	 (i) the AS write lock, or
 *	(ii) the AS read lock together with the segment spin lock
 *	     (svd_seglock).
 */
typedef struct vpage_info {
	uchar_t		vpi_nmemlck;	/* number of reasons for hat lock */
	uchar_t		vpi_flags;	/* per page protection + memory lock */
} vpage_info_t;

/*
 * flags values for vpi_flags
 *
 *	Note: the lower 4 bits are used to store page protections (PROT_ALL).
 */

#define	VPI_MEMLOCK	0x80		/* page is memory locked */
#define	VPI_ALL		(VPI_MEMLOCK)	/* All VPI flags ORed together */

#if (VPI_ALL & PROT_ALL) != 0
#error overlapping usage of vpi_flags
#endif

/*
 * vpage_lock_range request types
 */
typedef enum vp_lock {
	VP_READ, 	/* page identity will not change */
	VP_WRITE,	/* page identity is expected to change */
	VP_PRIV_WR,	/* MAP_PRIVATE/S_WRITE fault */
	VP_ZFOD		/* ZFOD is possible */
} vp_lock_t;


#ifdef _KERNEL

extern boolean_t vpage_lock_range(struct seg *seg, uint_t pgnumber,
				  uint_t npages, vp_lock_t type);
extern void vpage_unlock_range(struct segvn_data *svp, uint_t pgnumber,
			       uint_t npages);
extern void vpage_downgrade_range(struct segvn_data *svp, uint_t pgnumber,
				  uint_t npages);

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _MEM_VPAGE_H */
