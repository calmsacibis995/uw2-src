/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_MEM_HIER_H	/* wrapper symbol for kernel use */
#define _MEM_MEM_HIER_H	/* subject to change without notice */

#ident	"@(#)kern:mem/mem_hier.h	1.12"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ghier.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#ifdef _KERNEL

/*
 * This header file has all the hierarchy and minipl information 
 * pertaining to the VM subsystem. Note that all lock hierarchies in 
 * this file will be expressed as an offset from a base hierarchy value that
 * will be associated with the VM subsystem. Clearly, locks that can be 
 * held across subsystem boundaries need to be dealt with separately.
 * The hierarchy values of such "global" locks will be defined in ghier.h 
 * file under the util directory.
 */


/*
 * The hierarchy values will be checked amongst locks that have identical
 * minipl and hence the hierarchy namespace can be shared among locks that
 * have different minipls.
 */

/*
 * Locks with minipl = PLMIN
 */

#define VM_UBLOCK_HIER		(VM_HIER_BASE + 0)	/* ublock locks */
#define VM_UBRESV_HIER		(VM_HIER_BASE + 5)	/* ublock_swresv_mutex */

#define	VM_SEG_HIER		(VM_HIER_BASE + 20)	/* all seg drivers */

#define VM_SEGMAP_HIER		VM_SEG_HIER		/* smd_list_lock */

#define VM_SEGDEV_HIER		VM_SEG_HIER		/* segdev mutex */

#define VM_SEGKVN_HIER		VM_SEG_HIER		/* segkvn mutex */
#define VM_SEGKVN_IPL		PLMIN

#define VM_SEGVN_HIER		VM_SEG_HIER		/* svd_seglock */
#define VM_SEGVN_IPL		PLMIN

#define VM_AMLOCK_HIER		VM_SEG_HIER		/* segvn's anon_map */
#define VM_AMLOCK_IPL		PLMIN			/* am_lock */

/*
 * interrupt level for which the following locks give protection.
 */
#define VM_INTR_IPL		PLHI
#define VM_INTR_IPL_IS_PLMIN	0

/* 
 * Locks with minipl = VM_INTR_IPL.
 */

#define VM_PVNMEMRESV_HIER	(VM_HIER_BASE + 25)	/* pvn_memresv_lock */
#define VM_PVNMEMRESV_IPL	VM_INTR_IPL

#define VM_SWAPREC_HIER		(VM_HIER_BASE + 30)     /* page_swapreclaim_lck */
#define VM_SWAPREC_IPL		VM_INTR_IPL

#define VM_PUSELOCK_HIER	(VM_HIER_BASE + 40)	/* p_uselock */

#define VM_PAGEID_HIER		(VM_HIER_BASE + 45)	/* vm_pageidlock */

/*
 * The following range is used by the hat layer for locks that have 
 * interactions with page layer functions.
 */
#define VM_HAT_PAGE_HIER_MIN	(VM_HIER_BASE + 50)
#define VM_HAT_PAGE_HIER_MAX	(VM_HIER_BASE + 54)

#define VM_PAGEFREE_HIER	(VM_HIER_BASE + 55)	/* vm_pagefreelock */

#define VM_PAGE_IPL		VM_INTR_IPL		/* for free & id locks */

#define VM_ZBM_HIER		(VM_HIER_BASE + 60)	/* zbm_lock */
#define VM_ZBM_IPL		VM_INTR_IPL

#define VM_SWAP_HIER		(VM_HIER_BASE + 65)	/* swap_lck */
#define VM_SWAP_IPL		VM_INTR_IPL

/*
 * The following range of hierarchy is reserved for the HAT layer for 
 * internal usage.
 */
#define VM_HAT_LOCAL_HIER_MIN		(VM_HIER_BASE + 70)
#define VM_HAT_LOCAL_HIER_MAX		(VM_HIER_BASE + 79)

/* 
 * Locks with minipl = PLHI.
 */
#define VM_KMA_GIVEBACK_HIER	(VM_HIER_BASE + 80)	/* kma_giveback_lock */
#define VM_KMA_GIVEBACK_IPL	PLHI

#define VM_KMA_HIER		(VM_HIER_BASE + 85)	/* km_lock */
#define VM_KMA_IPL		PLHI

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_MEM_HIER_H */
