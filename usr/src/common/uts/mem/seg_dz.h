/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_MEM_SEG_DZ_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_DZ_H	/* subject to change without notice */

#ident	"@(#)kern:mem/seg_dz.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <mem/anon.h>		/* REQUIRED */
#include <util/param.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <mem/anon.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/param.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * A pointer to this structure is passed to segdz_create().
 * It is used to indicate the direction for growth of stack. The
 * valid values are AH_LO and AH_HI.
 */
struct segdz_crargs {
	char flags;
	char prot;
};

/*
 * The anon map structure is an N-ary tree data structure. This is used
 * because this segment driver needs to efficiently support large, sparse 
 * mappings.
 * The following #defines are used for the N-ary tree data structure.
 * The number of levels in the tree is dynamic and it would grow
 * vertically. The offsets from the base of the segment is used as the
 * index into the tree.
 *
 * The offsets are arrived based on how the segment gets created:
 * whether to grow from hight to low or the opposite direction.
 * If the LOW direction is set , then the offsets are derived by
 * taking the base as the segment's base + len; otherwise the
 * base is taken as the segment base itself. The offset is shifted
 * by the number of levels in the tree to traverse the tree and find its
 * leaf node. 
 *
 * Each leaf node contains LEAF_PARTITION anon pointers for the vpages.
 * This gives the tree structure efficient use of memory allocation.
 *
 * SEGDZ_NUMNODES - # of elements in one node 
 * SEGDZ_LEVELS - # of levels in the tree
 * SEGDZ_SHIFT - # of times to shift the addr. to get to the leaf 
 * LEAF_PARTITION - # of anon nodes in a leaf; for better packing
 *
 */

#define SEGDZ_LVL_SHIFT		4
#define SEGDZ_NUMNODES		(1 << SEGDZ_LVL_SHIFT)
#define SEGDZ_LVL_MASK		(SEGDZ_NUMNODES - 1)
#define SEGDZ_LEVELS		((NBITPOFF - PAGESHIFT + SEGDZ_LVL_SHIFT -1) / SEGDZ_LVL_SHIFT)
#define SEGDZ_MAXLEVELS		(SEGDZ_LEVELS - 1)
#define LEAF_PARTITION		4
#define LEAF_PARTITION_SHIFT	2
#define LEAF_PARTITION_MASK	0x3

/*
 * The following macro is used to get the index into a particular
 * level in the tree given the offset.
 */
#define SEGDZ_OFF_TO_INDEX(offset, lvl)	\
	(((lvl) == 0) ? (((offset) >> PAGESHIFT) & LEAF_PARTITION_MASK) : \
		((((offset) >> PAGESHIFT + LEAF_PARTITION_SHIFT) >> \
		((lvl) - 1) * SEGDZ_LVL_SHIFT) & SEGDZ_LVL_MASK)) \

/*
 * The vpginfo_t is the vinfo struct for each vpage. It contains of
 *  2 other fields:
 *	nmemlck - number of softlocks + 1 if page is memory locked.
 *	vpi_flags - The lower 4 bits reflects the protection of the page
 *		    if SEGDZ_PGPROT is set and the most significant 4 bits
 *		    indicates whether the page is memory locked.
 */
typedef struct vpg_info {
	uchar_t vpi_nmemlck;
	uchar_t vpi_flags;
} vpginfo_t;

/*
 * The leaf node contains an array of LEAF_PARTITION number of pointers to
 * to the anon struct and the vinfo for those vpages. This gives the 
 * best possible packing of data structure.
 */
typedef struct segdz_leaf {
	anon_t *am_anon[LEAF_PARTITION];
	vpginfo_t vpi_info[LEAF_PARTITION];
} segdz_leaf_t;

/*
 * A node in the tree is either pointing to the next node or is pointing to the
 * leaf node if the level of the tree is SEGDZ_LEVELS - 1.
 */
typedef union segdz_node {
	union segdz_node *next;
	segdz_leaf_t *leaf;
} segdz_node_t;

/*
 * Anon map struct for segdz. It contains a pointer to the root and number of
 * swap reservations. These fields are mutexed by the segment r/w sleep lock.
 */ 
typedef struct segdz_anonmap {
	segdz_node_t *am_root;
	size_t am_cowcnt;	/* used for fork/fault synchronization */
	size_t am_swresv;
	int am_level;	/* levels in the N-ary tree */
	unsigned long am_off_range; /* range of offset supported by the tree */
} segdz_amp_t;

/*
 * Locking info for segdz private data:
 *	There is one read/write sleep lock that mutexs the fields in
 *	the segdz_data as well as the anon map structure. This lock is held 
 *	in reader mode to stabilize the anon map and is held in writer mode when
 *	changing the anon map. Finer grained locks (such as the vpage locks
 *	in segvn) were not considered neccessary for this segment driver
 *	since not much concurrent intra-AS activity is likely (such as 
 *	concurrent faulting). Thus concurrent faults within an address space
 *	is not supported by this driver. Concurrent forks are possible since
 *	the sleep lock is held in read mode for this operation.
 *
 *	Wherever the AS lock is held in writer mode, the segment lock is not
 *	held since the lock serializes any changes either to private segment
 *	data fields or to the anon map.
 *
 *	The segment writer lock is held across F_SOFTLOCK and F_SOFTUNLOCk.
 *
 *	The maximum allowable protection for this segment is PROT_ALL all the
 *	time.
 *
 */
typedef struct	segdz_data {
	uchar_t sdz_prot;	/* protection used if SEGDZ_PGPROT not set */
	uchar_t	sdz_flags;	/* flags: see below for definitiions */
	segdz_amp_t sdz_amp;	/* pointer to anon map structure */
	rwsleep_t  sdz_seglock;	/* per-segment r/w sleep lock */
} segdz_data_t;

#ifdef	_KERNEL

/*
 * Pushdown stack algorithms are used for several manipulations of the
 * N-ary tree. Each element in the stack consists of a pointer to a node.
 * The stack also contains an offset value and top of the stack value.
 */
typedef struct segdz_stack {
	int tos;
	ulong_t amp_index;
	segdz_node_t *snodes[SEGDZ_LEVELS - 1];
} segdz_stack_t;

/*
 * The following macros are stack operators.
 * 	SEGDZ_PUSH pushes an element on the stack.
 *	SEGDZ_POP pops an element from the stack.
 *	SEGDZ_STACK_TOP returns the top of the stack element.
 */

#define SEGDZ_PUSH(stack, nnode)  { \
	ASSERT((stack).tos < SEGDZ_MAXLEVELS); \
	(stack).snodes[(stack).tos] = (nnode); \
	(stack).tos++; \
}

#define SEGDZ_POP(stack, nnode) { \
	ASSERT((stack).tos != 0); \
	--(stack).tos;	\
	(nnode) = ((stack).snodes[(stack).tos]);  \
}

#define SEGDZ_STACK_TOP(stack, nnode) { \
	ASSERT(stack.tos != 0); \
	(nnode) = ((stack).snodes[(stack).tos - 1]); \
}

/*
 * flags values for vpi_flags
 *
 *	Note: the lower 4 bits are used to store page protections (PROT_ALL).
 */
#define VPI_MEMLOCK     0x80            /* page is memory locked */

#if (VPI_MEMLOCK & PROT_ALL) != 0
#error overlapping usage of vpi_flags
#endif

/*
 * The following macros are based on the stack growth direction.
 *
 * SEGDZ_ADDR_TO_OFF() - Given an addr it converts to the offset that will
 *			 used to index into the tree.
 * SEGDZ_OFF_TO_ADDR() - Given an offset it will convert to the corresponding
 *			 addr.
 * SEGDZ_LOW_OFFSET()  - Find the lowest offset for the anon map for the
 *			 given range.
 * SEGDZ_HI_OFFSET()   - Find the highest offset for the anon map for the
 * 			 given range.
 * SEGDZ_ADDR_RANGE()  - Find the highest and lowest addresses in a range
 *			 given the offsets.
 */
#if (STK_GROWTH_DIR == HI_TO_LOW)
#define SEGDZ_ADDR_TO_OFF(seg, addr)	\
	((seg)->s_base + (seg)->s_size - PAGESIZE - (addr))

#define SEGDZ_OFF_TO_ADDR(seg, offset)	\
	((seg)->s_base + (seg)->s_size - PAGESIZE - (offset))

#define SEGDZ_LOW_OFFSET(seg, addr, len)	\
	SEGDZ_ADDR_TO_OFF(seg, (addr) + (len) - PAGESIZE)

#define SEGDZ_HI_OFFSET(seg, addr, len)	SEGDZ_ADDR_TO_OFF((seg), (addr))

#define SEGDZ_ADDR_RANGE(seg, loff, hoff, laddr, haddr)	{  \
	((laddr) = SEGDZ_OFF_TO_ADDR((seg), (hoff)),	\
	(haddr) = SEGDZ_OFF_TO_ADDR((seg), (loff)));	\
}

#else	/* stack growth high to low */

#define SEGDZ_ADDR_TO_OFF(seg, addr)	((addr) - (seg)->s_base)

#define SEGDZ_OFF_TO_ADDR(seg, offset)	(offset + (seg)->s_base)

#define SEGDZ_LOW_OFFSET(seg, addr, len)	\
	SEGDZ_ADDR_TO_OFF(seg, (addr))

#define SEGDZ_HI_OFFSET(seg, addr, len)	\
	SEGDZ_ADDR_TO_OFF((seg), (addr) + (len) - PAGESIZE)

#define SEGDZ_ADDR_RANGE(seg, loff, hoff, laddr, haddr)	{  \
	((laddr) = SEGDZ_OFF_TO_ADDR((seg), (loff)),	\
	(haddr) = SEGDZ_OFF_TO_ADDR((seg), (hoff)));	\
}

#endif	/* (STK_GROWTH_DIR == HI_TO_LOW) */

/*
 * definition for field svd_lckflag
 */
#define SEGDZ_RDLCK	(1 << 0)
#define SEGDZ_WRLCK	(1 << 1)

/*
 * definitions for field svd_flags
 * Caveat : SEGDZ_LOW and SEGDZ_HIGH cannot be 0.
 */
#define SEGDZ_PGPROT	(1 << 0)	/* using page granular protections */
#define SEGDZ_MEMLCK	(1 << 1)	/* memory pages were locked */
#define SEGDZ_MLCKFLT	(1 << 2)	/* memory lock pages at fault time */
#define SEGDZ_HI_TO_LOW	(1 << 3)	/* grow high to low address */
#define SEGDZ_LOW_TO_HI	(1 << 4)	/* grow low to high address */

#ifdef DEBUG
#define VPAGE_RDONLY(sdp, vpi_flags)	\
	(!((((sdp)->sdz_flags & SEGDZ_PGPROT) ? \
	(vpi_flags) : (sdp)->sdz_prot) & PROT_WRITE))
#endif

#define VPAGE_WRITABLE(sdp, vpi_flags)	\
	((((sdp)->sdz_flags & SEGDZ_PGPROT) ? \
	(vpi_flags) : (sdp)->sdz_prot) & PROT_WRITE)

extern int segdz_create(struct seg *, const void * const);
extern struct seg_ops segdz_ops;

/*
 * flags used in functions segdz_insnode().
 */
#define SEGDZ_NOFLAGS	0
#define SEGDZ_NOANON	1

/*
 * flags used for sedz_crargs.
 */
#define SEGDZ_CONCAT	0x1
#define SEGDZ_NOCONCAT	0x2

/*
 * operation specifiers when call segdz_amp_traverse().
 */
#define SEGDZ_LOCKOP	(1 << 0)
#define SEGDZ_LOCKASOP	(1 << 1)
#define SEGDZ_UNLOCKOP	(1 << 2)
#define SEGDZ_SETPROT	(1 << 3)
#define SEGDZ_INCORE	(1 << 4)
#define SEGDZ_RELEASE	(1 << 5)
#define SEGDZ_XFER	(1 << 6)

/*
 * argument structure that is parsed by the segdz_amp_traverse() function
 * for different operation requests.
 */
typedef struct segdz_lockop_args {
	u_char check;
	u_char mask;
} segdz_lockop_args_t;

typedef struct segdz_setprot_args {
	u_int prot;
	off_t srange, erange;
	boolean_t install;
	vaddr_t start_addr;
} segdz_setprot_args_t;

typedef struct segdz_relop_args {
	segdz_amp_t *amp;
	size_t	skip_swfree;
	struct as *as;
} segdz_relop_args_t;

typedef struct segdz_xferop_args {
	struct seg *nseg;
	size_t *swresv;
	struct as *as;
} segdz_xferop_args_t;

typedef union segdz_args {
	char *incore_vec;
	u_int prot;
	segdz_lockop_args_t lockargs;
	segdz_relop_args_t relargs;
	segdz_xferop_args_t xferargs;
} segdz_args_t;

#define SEGDZ_RELEASE_NODE(seg, len, node) { \
	if  ((len) == (seg)->s_size) \
		kmem_free((node), SEGDZ_NUMNODES * sizeof(segdz_node_t)); \
}

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_SEG_DZ_H */
