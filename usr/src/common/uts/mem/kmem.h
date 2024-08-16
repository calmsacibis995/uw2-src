/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_KMEM_H	/* wrapper symbol for kernel use */
#define _MEM_KMEM_H	/* subject to change without notice */

#ident	"@(#)kern:mem/kmem.h	1.27"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	public definitions for kernel memory allocator
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Physical Alignment Requirements structure.
 *
 * Used by routines which need to indicate alignment constraints on
 * physical addresses.
 *
 * Always allocate these structures by calling physreq_alloc(D3).  Never
 * create them statically.
 *
 * Before a physreq_t can be passed to any other routine, besides
 * physreq_free(D3), it must be passed to physreq_prep(D3).  After
 * calling physreq_prep, none of the fields should be changed.
 */
typedef struct physreq {
	/*
	 * Fields exported to the client.
	 */
	paddr_t		phys_align;	/* physical address must be a
					 * multiple of phys_align */
	paddr_t		phys_boundary;	/* if non-zero, physical addresses
					 * may not cross a multiple of
					 * phys_boundary */
	uchar_t		phys_dmasize;	/* DMA-ability requirement; 0 if none,
					 * else number of address bits used;
					 * ignored if NO_RDMA */
	uchar_t		phys_max_scgth;	/* maximum length of scatter/gather
					 * list; used only by buf_breakup
					 * in the BA_SCGTH case */

	/*
	 * This field has flags that are used only internally as well
	 * as flags exported to the client.
	 */
	uchar_t		phys_flags;	/* misc flags */

	/*
	 * The remaining fields are used only internally, and should not
	 * be accessed by physreq_alloc callers.
	 */
	void		*phys_brkup_poolp;	/* used by buf_breakup */
} physreq_t;

/*
 * phys_flags values:
 */

/* internal only flags */
#define PREQ_PREPPED	(1 << 0)	/* physreq_prep has been called;
					 * used under DEBUG only */

/* exported flags */
#define PREQ_PHYSCONTIG	(1 << 1)	/* contiguous memory required */

#endif /* _KERNEL || _KMEMUSER */

/*
 * Flag argument values for routines in the kmem_alloc() family:
 *
 * Notes:
 *	1) KM_SLEEP and KM_NOSLEEP must match SLEEP and NOSLEEP as defined
 *	   in util/param.h.
 *
 *	2) It is erroneous for a caller to specify both KM_DMA and
 *	   KM_REQ_DMA.
 *
 *	3) The KM_DMA, KM_REQ_DMA, and KM_PHYSCONTIG flags are not
 *	   defined in the DDI, and are therefore not for use in
 *	   drivers.
 */
#define KM_SLEEP	0	/* can sleep to get memory */
#define KM_NOSLEEP	1	/* cannot sleep to get memory */
#define KM_DMA		2	/* caller prefers DMAable memory */
#define KM_REQ_DMA	4	/* caller requires non-DMAable memory */
#define KM_PHYSCONTIG	8	/* caller requires physically contiguous */
				/* memory */

/*
 * Valid kmem_alloc() family flags.
 */
#define KM_VALID_FLAGS		(KM_NOSLEEP|KM_DMA|KM_REQ_DMA|KM_PHYSCONTIG)

/*
 * _KMEM_C is defined only when compiling kma.c
 */
#if defined(_KERNEL) && !defined(_KMEM_C)

#if defined(_KMEM_STATS) || defined(_KMEM_HIST)

/*
 * instrumented kmem functions for statistics gathering
 */

#define kmem_alloc(size, flag)	\
	kmem_instr_alloc(size, (flag), __LINE__, __FILE__)
#define kmem_zalloc(size, flag) \
	kmem_instr_zalloc(size, (flag), __LINE__, __FILE__)
#define kmem_alloc_physcontig(size, physreq, flags) \
	kmem_i_alloc_physcont(size, physreq, flags, __LINE__, __FILE__)
#define kmem_alloc_physreq(size, physreq, flags) \
	kmem_i_alloc_physreq(size, physreq, flags, __LINE__, __FILE__)
#define kmem_zalloc_physreq(size, physreq, flags) \
	kmem_i_zalloc_physreq(size, physreq, flags, __LINE__, __FILE__)
#define kmem_free(addr, size) \
	kmem_instr_free(addr, size, __LINE__, __FILE__)
#define kmem_free_physcontig(addr, size) \
	kmem_instr_free(addr, size, __LINE__, __FILE__)

#ifdef __STDC__
extern void *kmem_instr_alloc(size_t, int, int, char *);
extern void *kmem_instr_zalloc(size_t, int, int line, char *);
extern void *kmem_i_alloc_physcont(size_t, const physreq_t *, int, int, char *);
extern void *kmem_i_alloc_physreq(size_t, const physreq_t *, int, int, char *);
extern void *kmem_i_zalloc_physreq(size_t, const physreq_t *, int, int, char *);
extern void kmem_instr_free(void *, size_t, int, char *);
#else
extern void *kmem_instr_alloc();
extern void *kmem_instr_zalloc();
extern void *kmem_i_alloc_physcont();
extern void *kmem_i_alloc_physreq();
extern void *kmem_i_zalloc_physreq();
extern void kmem_instr_free();
#endif

#else /* !KMEM_STATS && !_KMEM_HIST */

#ifdef __STDC__
extern void *kmem_alloc(size_t, int);
extern void *kmem_zalloc(size_t, int);
extern void *kmem_alloc_physcontig(size_t, const physreq_t *, int);
extern void *kmem_alloc_physreq(size_t, const physreq_t *, int);
extern void *kmem_zalloc_physreq(size_t, const physreq_t *, int);
extern void kmem_free(void *, size_t);
#else
extern void *kmem_alloc();
extern void *kmem_zalloc();
extern void *kmem_alloc_physcontig();
extern void *kmem_alloc_physreq();
extern void *kmem_zalloc_physreq();
extern void kmem_free();
#endif

#endif /* !KMEM_STATS && !_KMEM_HIST */

#ifdef __STDC__

extern void kmem_advise(size_t);
extern int kmem_avail(size_t, int);
extern physreq_t *physreq_alloc(int);
extern void physreq_free(physreq_t *);
extern boolean_t physreq_prep(physreq_t *, int);
extern boolean_t physreq_met(const void *, size_t, const physreq_t *);

#else /* !__STDC__ */

extern void kmem_advise();
extern int kmem_avail();
extern physreq_t *physreq_alloc();
extern void physreq_free();
extern void physreq_prep();
extern boolean_t physreq_met();

#endif /* __STDC__ */

#endif /* defined(_KERNEL) && !defined(_KMEM_C) */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_KMEM_H */
