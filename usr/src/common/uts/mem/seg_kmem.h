/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_SEG_KMEM_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_KMEM_H	/* subject to change without notice */

#ident	"@(#)kern:mem/seg_kmem.h	1.18"
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
 * VM - Kernel Segment Driver
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

struct seg;
struct page;

extern struct seg *kpgseg;	/* segment for mapping dynamic kernel memory */
extern int kpg_cellsize;	/* size in pages of a kpg_cell */

extern void segkmem_create(struct seg *);
extern void segkmem_ppid_mapin(struct seg *, vaddr_t, ulong_t, ppid_t, uint_t);
extern void segkmem_pl_mapin(struct seg *, vaddr_t, ulong_t, struct page *,
			     uint_t);
extern void segkmem_mapout(struct seg *, vaddr_t, ulong_t);

extern void kpg_calloc(void);
extern void kpg_init(void);
extern void kpg_vaddr_limits(vaddr_t *, ulong_t *);

extern void *kpg_alloc(ulong_t, uint_t, uint_t);
extern void kpg_free(void *, ulong_t);
extern void *kpg_ppid_mapin(ulong_t, ppid_t, uint_t, uint_t);
extern void *kpg_pl_mapin(ulong_t, struct page *, uint_t, uint_t);
extern void kpg_mapout(void *, ulong_t);
extern vaddr_t kpg_vm_alloc(ulong_t, uint_t);
extern void kpg_vm_free(vaddr_t, ulong_t);

/*
 * kpg virtual allocation is done via ZBM
 */
#define _KPG_VM_ALLOC(npages, flag)	zbm_alloc(&kpg_zbm, npages, flag)
#define _KPG_VM_FREE(vaddr, npages)	zbm_free(&kpg_zbm, vaddr, npages)

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_SEG_KMEM_H */
