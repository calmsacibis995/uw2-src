/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_SEG_KVN_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_KVN_H	/* subject to change without notice */

#ident	"@(#)kern:mem/seg_kvn.h	1.13"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * MS (Segment Management) - seg_kvn routines
 *
 *	seg_kvn provides kernel vnode mappings. Unlike seg_map, seg_kvn
 *	mappings are of variable size. In addition, seg_kvn does not
 *	cache released mappings.
 */

#ifdef _KERNEL_HEADERS

#include <mem/hat.h> /* REQUIRED */
#include <mem/memresv.h> /* REQUIRED */
#include <mem/seg.h>	/* REQUIRED */
#include <util/ksynch.h> /* REQUIRED */
#include <util/param.h>	/* REQUIRED */
#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h> /* REQUIRED */
#include <sys/param.h>	/* REQUIRED */
#include <sys/seg.h>	/* REQUIRED */
#include <sys/types.h> /* REQUIRED */
#include <vm/hat.h> /* REQUIRED */
#include <vm/memresv.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

#ifdef DEBUG

/*
 * debug data for seg_kvn segments
 */
struct kmp_debug {
	uint_t			kmpd_ngetpage;	/* # of VOP_GETPAGE()s */
	uint_t			kmpd_nagewaits;	/* # of aging waits */
	uint_t			kmpd_nagings;	/* # of agings */
	uint_t			kmpd_nreserved;	/* pages REAL mem reserved */
};

#define kmp_ngetpage		kmp_debug.kmpd_ngetpage
#define kmp_nagewaits		kmp_debug.kmpd_nagewaits
#define kmp_nagings		kmp_debug.kmpd_nagings
#define kmp_nreserved		kmp_debug.kmpd_nreserved

#endif	/* DEBUG */

/*
 * The kmp structure: locus of activity for a seg_kvn mapping
 *
 *	The fields are protected as follows:
 *
 *		Fields which are constant during the life of this data
 *		structure:
 *
 *			kmp_npages, kmp_vp, kmp_off, kmp_seg, kmp_base
 *
 *		Fields mutexed by the global LOCK segkvn_mutex:
 *
 *			kmp_nlocks, kmp_nmlocks, kmp_next, kmp_last,
 *			kmp_stamp, kmp_sv, kmp_flags, kmp_cookie
 *
 *	A per-mapping pseudo sleep lock is implemented (the aging lock).
 *	Holders of this pseudo sleep lock write the SEGKVN_AGING bit into
 *	kmp_flags. Waiters (namely the segkvn_lock() function)
 *	wait at kmp_sv.
 */

struct kmp_hdr {
	struct kmp		*kmh_next;	/* forward age list link */
	struct kmp		*kmh_last;	/* backward age list link */
};

struct kmp {
	struct kmp_hdr		kmp_hdr;	/* header block */
	struct seg		kmp_seg;	/* segment structure */
	vaddr_t			kmp_base;	/* start of alloc'ed space */
	uint_t			kmp_npages;	/* pages of virtual alloc'ed */
	struct vnode		*kmp_vp;	/* mapped vnode */
	off_t			kmp_off;	/* vnode offset at map start */
	sv_t			kmp_sv;		/* for memory lock syncing */
	uint_t			kmp_nlocks;	/* number of locks */
	uint_t			kmp_nmlocks;	/* number of memory locks */
	uint_t			kmp_flags;	/* see defs below */
	clock_t			kmp_stamp;	/* aging stamp */
	TLBScookie_t		kmp_cookie;	/* hat shootdown cookie */
#ifndef NO_RDMA
	mresvtyp_t		kmp_mtype;	/* mem_resv type */
#endif
#ifdef DEBUG
	struct kmp_debug	kmp_debug;	/* debug information */
#endif	/* DEBUG */
};

#define kmp_next		kmp_hdr.kmh_next
#define kmp_last		kmp_hdr.kmh_last

#define SEGKVN_TO_KMP(seg)	((struct kmp *)(seg)->s_data)

/*
 * kmp_flags definitions
 */
#define SEGKVN_AGING		(1 << 0)	/* segment is being aged */
#define SEGKVN_HAS_COOKIE	(1 << 1)	/* TLB inconsistent */
#define SEGKVN_AGELIST		(1 << 2)	/* on aging list */
#define SEGKVN_HAS_AGED		(1 << 3)	/* aged once on aging list */
#define SEGKVN_HAT_LOAD		(1 << 4)	/* xlats may be loaded */
#define SEGKVN_KMP_KLUST	(1 << 5)	/* see SEGKVN_KLUSTER below */
#define SEGKVN_NOFLAGS		0		/* NONE */
#define SEGKVN_ALL_FLAGS	(SEGKVN_AGING|SEGKVN_HAS_COOKIE| \
				 SEGKVN_AGELIST|SEGKVN_HAS_AGED| \
				 SEGKVN_HAT_LOAD|SEGKVN_KMP_KLUST)

#define VM_SEGKVN_PRI		(PRIMEM - 1)	/* pri of LWPs blocked on the */
						/* seg_kvn segment lock */


/*
 * some times for aging purposes
 */
#define SEGKVN_INFINITY		(60 * 60 * HZ)	/* a very long time */
#define SEGKVN_DESPERATE	0

/*
 * Flags argument values for segkvn_lock/segkvn_unlock/segkvn_vp_mapin.
 *
 * Both segkvn_lock/segkvn_unlock must be called with exactly one of
 * SEGKVN_FUTURE_LOCK or SEGKVN_MEM_LOCK, optionally OR'ed with
 * SEGKVN_NO_MEMRESV. segkvn_unlock may also have SEGKVN_DONTNEED
 * and/or SEGKVN_DISCARD OR'ed in.
 *
 * segkvn_vp_mapin acccepts the SEGKVN_NOFAULT, SEGKVN_KLUSTER,
 * and SEGKVN_DMA flags.
 */
#define SEGKVN_FUTURE_LOCK	0x00
#define SEGKVN_MEM_LOCK		0x01
#define SEGKVN_NO_MEMRESV	0x02	/* caller is responsible for M_REAL
					 * reservations, if any */
#define SEGKVN_DONTNEED		0x04	/* translations should be immediately
					 * unloaded, instead of being aged */
#define SEGKVN_DISCARD		0x08	/* don't bother to preserve data */
#define SEGKVN_NOFAULT		0x10	/* don't support HW generated faults */
#define SEGKVN_KLUSTER		0x20	/* kluster beyond segment boundaries */
#define SEGKVN_DMA		0x40	/* P_DMA pages required */

/*
 * uint_t
 * SEGKVN_DMA_FLAGS(uint_t flags)
 *	Given flags for segkvn_vp_mapin(), formulate the correct flags
 *	for memfs_create_unnamed().
 *
 * Calling/Exit State:
 *	None.
 */
#ifdef NO_RDMA
#define SEGKVN_DMA_FLAGS(flags)		(MEMFS_FIXEDSIZE)
#else /* !NO_RDMA */
#define SEGKVN_DMA_FLAGS(flags)	((flags & SEGKVN_DMA) ?		\
					 MEMFS_FIXEDSIZE|MEMFS_DMA :	\
					 MEMFS_FIXEDSIZE)
#endif /* NO_RDMA */

#ifndef NO_RDMA

/*
 * mresvtyp_t
 * SEGKVN_MEM_TYPE(uint_t flags)
 *	Given flags for segkvn_vp_mapin(), formulate the correct mresvtyp_t
 *	for locking down the pages.
 *
 * Calling/Exit State:
 *	None.
 */
#define SEGKVN_MEM_TYPE(flags)		((flags & SEGKVN_DMA) ? \
						segkvn_mtype_dma : M_REAL)
#endif /* NO_RDMA */

/*
 * mresvtyp_t
 * SEGKVN_KMP_MTYPE(struct kmp *kmp)
 *	Given a kmp, return the correct mresvtyp_t for locking down the pages.
 *
 * Calling/Exit State:
 *	None.
 */
#ifdef NO_RDMA
#define SEGKVN_KMP_MTYPE(kmp)		M_REAL
#else /* !NO_RDMA */
#define SEGKVN_KMP_MTYPE(kmp)		(kmp->kmp_mtype)
#endif /* NO_RDMA */

/*
 * external definitions
 */
extern struct seg *segkvn;
extern int segkvn_cellsize;
struct vnode;
extern void segkvn_calloc(void);
extern void segkvn_create(struct seg *);
extern void segkvn_init(struct seg *);
extern vaddr_t segkvn_vp_mapin(size_t, size_t, size_t, struct vnode *, off_t,
			       uint_t, void **);
extern void segkvn_mapout(void *);
extern int segkvn_lock(void *, uint_t);
extern void segkvn_unlock(void *, uint_t);

#ifdef DEBUG
/*
 * DEBUG only definitions
 */
#define SEGKVN_INC_STAT(f)	(++(f))
#define SEGKVN_DEC_STAT(f)	(--(f))
#define SEGKVN_ADD_STAT(f, n)	((f) += (n))
#define SEGKVN_SUB_STAT(f, n)	((f) -= (n))

struct segkvn_dbginfo {
	uint_t			kmpi_nzfod;	/* # of anon_zero()s */
	uint_t			kmpi_ngetpage;	/* # of anon_getpage()s */
	uint_t			kmpi_nagewaits;	/* # of aging waits */
	uint_t			kmpi_nanzwaits;	/* # of anon_zero() waits */
	uint_t			kmpi_nagings;	/* # of agings */
	uint_t			kmpi_nreserved;	/* pages REAL mem reserved */
	uint_t			kmpi_nincore;	/* pages REAL mem reserved */
	uint_t			kmpi_nlocks;	/* number of aging locks */
	uint_t			kmpi_nmlocks;	/* number of memory locks */
	uint_t			kmpi_flags;	/* copy of kmp_flags */
};

extern void segkvn_audit(void *, struct segkvn_dbginfo *);
extern uint_t segkvn_aging_audit(void);

#else	/* !DEBUG */

#define SEGKVN_INC_STAT(f)
#define SEGKVN_DEC_STAT(f)
#define SEGKVN_ADD_STAT(f, n)
#define SEGKVN_SUB_STAT(f, n)

#endif	/* !DEBUG */

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif  /* _MEM_SEG_KVN_H */
