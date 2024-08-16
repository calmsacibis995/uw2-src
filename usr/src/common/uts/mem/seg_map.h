/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_SEG_MAP_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_MAP_H	/* subject to change without notice */

#ident	"@(#)kern:mem/seg_map.h	1.22"
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

#include <mem/hat.h>		/* REQUIRED */
#include <mem/seg.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/param.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <vm/hat.h>		/* REQUIRED */
#include <vm/seg.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/param.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Each smap struct represents a MAXBSIZE sized mapping to the
 * <sm_vp, sm_off> given in the structure.  The location of the
 * the structure in the array gives the virtual address of the
 * mapping.
 */

/*
 * The following computes the number of pages covered by an smap (see below).
 * Also, SM_PGARRAYSZ is computed, which is the length of a bit array
 * large enough to hold one bit per page.
 */

#ifndef PGPERSMAP
#define PGPERSMAP	((MAXBSIZE + PAGEOFFSET) / PAGESIZE)
#endif
#define SM_PGARRAYSZ	((PGPERSMAP + NBITPW - 1) / NBITPW)

/*
 * Each smap struct represents a MAXBSIZE sized mapping to the
 * <sm_vp, sm_off> given in the structure.  The location of the
 * the structure in the array gives the virtual address of the
 * mapping.
 *
 * The following fields are protected by smd_list_lock:
 *	sm_refcnt
 *	sm_hash
 *	sm_next, sm_prev
 *	sm_flags
 *
 * The following fields are constant when sm_refcnt is non-zero, and therefore
 * are indirectly protected by smd_list_lock; however, we can take advantage of
 * the fact that it's illegal to operate directly on segmap addresses (such as
 * with segmap_fault) without holding a reference count, and read these fields
 * without acquiring the lock:
 *	sm_vp, sm_off
 *
 * The following fields are constant when sm_refcnt is zero, and therefore is
 * indirectly protected by smd_list_lock:
 *	sm_last_use
 *	sm_last_time
 *
 * The following field is protected by smd_fspin if sm_refcnt is non-zero:
 *	sm_pgflags
 */
struct smap {
	struct vnode *sm_vp;		/* vnode pointer (if mapped) */
	off_t sm_off;			/* file offset for mapping */
	ushort_t sm_refcnt;		/* reference count for uses */
	uchar_t sm_flags;		/* flags (see below) */
	lock_t sm_lock;			/* mutex for sm_lockcnt */
	uint_t sm_pgflag[SM_PGARRAYSZ];	/* per-page flags */
	TLBScookie_t sm_last_use;	/* "time" this chunk was last used */
	clock_t sm_last_time;		/* actual time of last use */
	struct smap *sm_hash;		/* hash pointer */
	struct smap *sm_next;		/* next pointer */
	struct smap *sm_prev;		/* previous pointer */
};

/* Flag values for sm_flags */

#define SMP_TLBI		0x01	/* translations have been modified,
					 * but TLBs have not been flushed,
					 * leaving them inconsistent
					 */
#define SMP_UNLOADED		0x02	/* translations have been unloaded,
					 * and not used since then,
					 * but the smap identity is retained
					 */
#define SMP_PRIVATE_RESV	0x04	/* this smap holds the private
					 * memory reservation
					 * (segmap_resv_lock)
					 */
#define SMP_WRITING		0x08	/* this smap is currently being used
					 * for writing
					 */

/*
 * The SM_WRITING() macro indicates that the smap chunk is currently being
 * used for writing.
 */
#define SM_WRITING(smp)	((smp)->sm_flags & SMP_WRITING)

/*
 * The following macros test, set, and clear the "mustfault" flag
 * for a given page within an smap chunk.  This flag indicates that
 * segmap_getmap() must call segmap_fault() to generate a call to
 * VOP_GETPAGE(), since a previous call to VOP_GETPAGE() indicated
 * that not all of the page's backing store was allocated.
 */
#if (SM_PGARRAYSZ == 1)

#define SM_PG_MUSTFAULT(sm, pg) \
		BITMASK1_TEST1((sm)->sm_pgflag, pg)
#define SM_SET_PG_MUSTFAULT(sm, pg) \
		BITMASK1_SET1((sm)->sm_pgflag, pg)
#define SM_CLR_PG_MUSTFAULT(sm, pg) \
		BITMASK1_CLR1((sm)->sm_pgflag, pg)
#define SM_CLRALL_MUSTFAULT(sm) \
		BITMASK1_CLRALL((sm)->sm_pgflag)
#define SM_ANY_MUSTFAULT(sm) \
		BITMASK1_TESTALL((sm)->sm_pgflag)

#elif (SM_PGARRAYSZ == 2)

#define SM_PG_MUSTFAULT(sm, pg) \
		BITMASK2_TEST1((sm)->sm_pgflag, pg)
#define SM_SET_PG_MUSTFAULT(sm, pg) \
		BITMASK2_SET1((sm)->sm_pgflag, pg)
#define SM_CLR_PG_MUSTFAULT(sm, pg) \
		BITMASK2_CLR1((sm)->sm_pgflag, pg)
#define SM_CLRALL_MUSTFAULT(sm) \
		BITMASK2_CLRALL((sm)->sm_pgflag)
#define SM_ANY_MUSTFAULT(sm) \
		BITMASK2_TESTALL((sm)->sm_pgflag)

#else /* SM_PGARRAYSZ > 2 */

#define SM_PG_MUSTFAULT(sm, pg) \
		BITMASKN_TEST1((sm)->sm_pgflag, pg)
#define SM_SET_PG_MUSTFAULT(sm, pg) \
		BITMASKN_SET1((sm)->sm_pgflag, pg)
#define SM_CLR_PG_MUSTFAULT(sm, pg) \
		BITMASKN_CLR1((sm)->sm_pgflag, pg)
#define SM_CLRALL_MUSTFAULT(sm) \
		BITMASKN_CLRALL((sm)->sm_pgflag, SM_PGARRAYSZ)
#define SM_ANY_MUSTFAULT(sm) \
		BITMASKN_TESTALL((sm)->sm_pgflag, SM_PGARRAYSZ)

#endif

/*
 * (Semi) private data maintained by the segmap driver per SEGMENT mapping
 */
struct segmap_data {
	struct smap *smd_sm;		/* array of smap structures */
	struct smap *smd_free;		/* free list head pointer */
	struct smap *smd_age;		/* pointer into free list for next smap
					 * to age */
	sv_t smd_sv;			/* wait here on empty free list */
	uint_t smd_resv_pool;		/* reserve pool of M_BOTH */
					/* reservations */
	sv_t smd_resv_sv;		/* wait here on emtpy mem_resv pool */
	lock_t smd_list_lock;		/* this lock covers the free list
					 * (including smd_age) and the hash
					 * table contents */
	fspin_t smd_fspin;		/* mutex for setting sm_pgflags */
	uint_t smd_hashsz;		/* hash table size (# slots) */
	struct smap **smd_hash;		/* pointer to hash table */
};

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

/*
 * These are flags used on release.  Some of these might get handled
 * by segment operations needed for msync (when we figure them out).
 * SM_ASYNC modifies SM_WRITE.  SM_INVAL implies SM_WRITE and SM_NOCACHE.
 */
#define	SM_WRITE	0x01	/* write back the pages upon release */
#define	SM_ASYNC	0x02	/* do the write asynchronously */
#define	SM_INVAL	0x04	/* remove from page and segmap caches */
#define SM_NOCACHE	0x08	/* remove from segmap cache only */
#define	SM_DONTNEED	0x10	/* less likely to be needed soon */
#define SM_SETMOD	0x20	/* mark pages as modified */

/*
 * The segmap_data structure contains a private pool of cached M_BOTH
 * reservations for use when the mem_resv() pool has been depleted.
 * The initial pool size (in units of SMAP chunks) is defined here.
 */
#define SEGMAP_RESV_POOL_SIZE	4

/* the kernel generic mapping segment */
extern struct seg *segkmap;

extern void segmap_calloc(struct seg *);
extern void segmap_create(struct seg *);
extern void segmap_init(struct seg *);

extern void *segmap_getmap(struct seg *, struct vnode *, off_t, size_t,
			   enum seg_rw rw, boolean_t mustfault, int *errorp);
extern int segmap_release(struct seg *, void *, uint_t);
extern off_t segmap_abort_create(struct seg *, void *, off_t, size_t);

extern void segmap_age(struct seg *);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_SEG_MAP_H */
