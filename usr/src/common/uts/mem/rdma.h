/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_RDMA_H	/* wrapper symbol for kernel use */
#define _MEM_RDMA_H	/* subject to change without notice */

#ident	"@(#)kern:mem/rdma.h	1.12"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Support for the restricted DMA property, for systems which use DMA
 * but cannot access all of physical memory via DMA.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */


#if defined(_KERNEL)

/*
 * Three RDMA modes are currently supported:
 *
 *	RDMA_DISABLED:	The VM system has no support for DMA restictions
 *			(typically because all of the memory is DMAable).
 *
 *	RDMA_MICRO:	All pages are DMAable, but the system contains
 *			some non-DMAable device memory.
 *
 * 	RDMA_SMALL:	The VM system maintains three separate pools of pages:
 *			STD_PAGE, DMA_PAGE, and PAD_PAGE (see mem/page.h
 *			for a more complete description). KMA maintains 3
 *			types of pools: (i) for those that require DMAable
 *			memory, (ii) for those that prefer DMAable memory,
 *			and (iii) for those that prefer non-DMAable
 *			memory. VM paging policy is largely driven by the
 *			aggregate of the STD_PAGE and PAD_PAGE pools.
 *
 *			This model works best when the ratio of non-DMAable
 *			to DMAable memory is small (typically < 1.5:1) and
 *			when the main disks do not have the capability to DMA
 *			to all of memory.
 *
 * 	RDMA_LARGE:	The VM system maintains two separate pools of pages
 *			(STD_PAGE and DMA_PAGE). KMA maintains 2 types of
 *			pools: (i) for those that require DMAable memory,
 *			and (ii) for those that do not require DMAable
 *			memory. VM paging policy is entirely driven by the
 *			STD_PAGE pool.
 *
 *			This model works best when the ratio of non-DMAable
 *			to DMAable memory is large, or if the main disks have
 *			the ability to DMA to all of memory.
 */

typedef enum {
	RDMA_DISABLED = 0,
	RDMA_MICRO = 1,
	RDMA_SMALL = 2,
	RDMA_LARGE = 3
} rdma_mode_t;
extern rdma_mode_t rdma_mode;

#define	DMA_PP(pp)	((pp)->p_physdma)
#define	DMA_PFN(pfn)	(tune.t_dmalimit <= tune.t_dmabase || \
			 ((pfn) < tune.t_dmalimit && (pfn) >= tune.t_dmabase))
#define	DMA_BYTE(b)	DMA_PFN(btop(b))

#define RDMA_NOTREQUIRED	0
#define RDMA_REQUIRED		1
#define RDMA_IMPOSSIBLE		2

#define RDMA_REQUIREMENT(physreqp) \
		((rdma_mode != RDMA_DISABLED && physreqp->phys_dmasize != 0 && \
		  physreqp->phys_dmasize < NBBY * sizeof(paddr_t)) ? \
			RDMA_REQUIRED : RDMA_NOTREQUIRED)

struct buf;
struct physreq;
extern void rdma_page_init(void);
extern void rdma_pool_init(void);
extern void rdma_fix_swtbls(void);
extern void rdma_fix_bswtbl(int);
extern boolean_t rdma_must_copy(const struct buf *, const struct physreq *);
extern void rdma_substitute_pages(struct buf *, const struct physreq *);
extern void rdma_convert(void);
extern void rdma_convert_pages(boolean_t);

extern struct bcb rdma_dflt_bcb;

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _MEM_RDMA_H */
