/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_KMA_H	/* wrapper symbol for kernel use */
#define _MEM_KMA_H	/* subject to change without notice */

#ident	"@(#)kern:mem/kma.h	1.11"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Definitions for implementation aspects of KMA needed to be referenced
 * outside of kma.c.  Interface aspects of KMA are defined in kmem.h.
 */

#ifdef _KERNEL_HEADERS

#include <mem/kma_p.h> /* PORTABILITY */
#include <util/ksynch.h> /* REQUIRED */
#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h> /* REQUIRED */
#include <sys/types.h> /* REQUIRED */
#include <vm/kma_p.h> /* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/* per-engine local info stored in the plocal structure */
typedef struct kmlocal {
	struct kmfree *lfree;	/* local per-engine free list */
	uint_t lnfree;		/* # buffers on this free list */
} kmlocal_t;

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

/* Functions called from system initialization and online/offline: */
extern void kma_calloc(void);
extern void kma_init(void);
struct engine;
extern int kma_online(struct engine *);
extern void kma_offline(struct engine *);
extern void kma_offline_self(void);
#ifndef NO_RDMA
extern void kma_switch_small(boolean_t);
#endif /* NO_RDMA */

/*
 * The following declarations are for the macros below and should not be
 * used directly.
 */

extern fspin_t kma_lock;	/* Mutex for the following fields: */
extern uint_t kma_waitcnt;	/* Count of waiters on all freelists */
extern uint_t kma_shrinktime;	/* How long til time to shrink KMA pools */
extern clock_t kma_lastgive;	/* Time of last forced giveback */

extern void kma_do_giveback(void);

/* Min and max times between KMA pool shrinks, in seconds */
#define KMA_MAXSHRINKTIME	(30 * 60)
#define KMA_MINSHRINKTIME	(45)
/* Min time between forced givebacks, in ticks */
#define KMA_MINGIVETIME		(HZ / 2)

/*
 * boolean_t kma_force_shrink(void)
 *
 * Calling/Exit State:
 *	Forces the next call to kma_refreshpools() to shrink the pools,
 *	if they haven't been shrunk recently.  Returns B_TRUE if refresh
 *	daemon should be kicked off.
 */
#define kma_force_shrink() \
	( kma_shrinktime <= KMA_MAXSHRINKTIME - KMA_MINSHRINKTIME ? \
		( FSPIN_LOCK(&kma_lock), \
		  kma_shrinktime = 0, \
		  FSPIN_UNLOCK(&kma_lock), \
		  B_TRUE \
		) : \
		( lbolt - kma_lastgive >= KMA_MINGIVETIME ? \
			( FSPIN_LOCK(&kma_lock), \
			  ( lbolt - kma_lastgive >= KMA_MINGIVETIME ? \
				( kma_lastgive = lbolt, \
				  FSPIN_UNLOCK(&kma_lock), \
				  kma_do_giveback() \
				) : \
				( FSPIN_UNLOCK(&kma_lock) ) \
			  ), \
			  B_FALSE \
			) : \
			( B_FALSE ) \
		) \
	)

/*
 * int kma_waiters(void)
 *
 * Calling/Exit State:
 *	Returns non-zero if anyone is waiting on KMA memory.
 *	This is a stale snapshot.
 */
#define kma_waiters()	kma_waitcnt

#endif /* _KERNEL */

#if defined(_KERNEL) || defined(_KMEMUSER)

#ifndef NO_RDMA
/*
 * Under the restricted DMA models RMDA_SMALL, KMA supports three sets of
 * pools, one for non-DMAable memory, one for preferentially DMAable memory,
 * and one for preferentially non-DMAable memory. This puts an upper bound
 * on the number of pool types needed.
 */
#define KMEM_POOL_TYPES		3

#else /* NO_RDMA */

#define KMEM_POOL_TYPES		1

#endif /* NO_RDMA */

#endif /* defined(_KERNEL) || defined(_KMEMUSER) */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_KMA_H */
