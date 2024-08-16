/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_UBLOCK_H	/* wrapper symbol for kernel use */
#define _MEM_UBLOCK_H	/* subject to change without notice */

#ident	"@(#)kern:mem/ublock.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef enum ub_locktype {
	UB_SWAPPABLE, UB_NOSWAP, UB_NOSWAP_USER
} ub_locktype_t;

#define UB_TOTAL_LOCKS	3
#define UB_NLOCKTYPE	4


typedef struct ubmem_chunk {
	void			*ubmem_nextmapcookie;
	struct ubmem_chunk	*ubmem_next;
} ubmem_chunk_t;

typedef struct ubmem_free {
	struct ubmem_free *ubmf_next;
} ubmem_free_t;

/*
 * struct proc_ubinfo: per process ublock information
 *
 *	One of these structures is allocated for each process.
 */

typedef struct proc_ubinfo {
	sv_t		ub_sv;
	lock_t		ub_mutex;
	uchar_t		ub_flags;
	uchar_t		ub_locks[UB_NLOCKTYPE];
	ushort_t	ub_refcnt;
	uint_t		ub_npages;
	ubmem_chunk_t	ubmem_head;
	ubmem_free_t	*ubmem_freelist;

	/*
	 * Information pertaining to the backing store.
	 */
	struct vnode	*ub_mvp;
	size_t		ub_len;		/* file length */
	uchar_t		ub_level;
	union {
		uint_t	uub_bitmap;	/* initial bitmap */
		uint_t	*uub_ibitmapp;	/* indirect bitmap (overflow) */
	} ub_ubitmap;
} proc_ubinfo_t;

#define UB_NWORDS(level)	(1 << ((level) * 2))
#define UB_NBYTES(level)	(UB_NWORDS(level) * sizeof(uint_t))
#define UB_NBITS(level)		(UB_NWORDS(level) * NBITPW)
#define UB_BITMAP(pubp)		(((pubp)->ub_level == 0) ?		 \
					  &(pubp)->ub_ubitmap.uub_bitmap \
					: (pubp)->ub_ubitmap.uub_ibitmapp)

/*
 * ub_flags values
 */
#define UB_INTRANS		(1 << 0)
#define UB_EXTENDING		(1 << 1)

/*
 * struct lwp_ubinfo: per lwp ublock information
 *
 *	A copy of this structure is embedded in each lwp_t.
 */
typedef struct lwp_ubinfo {
	void		*ub_mapcookie;
	off_t		ub_off;
	proc_ubinfo_t	*ub_proc_ubp;
	uchar_t		ub_private_resv;
	uchar_t		ub_detached;

	/*
	 * extension stack data
	 */
	void		*ub_stack_mem;
	void		*ub_stack;
#ifdef DEBUG
	size_t		ub_size;
#endif
} lwp_ubinfo_t;

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

#define ublock_npages(procp)	((procp)->p_ubinfop->ub_npages)

struct proc;
struct lwp;

extern void ublock_init(void);
extern int ublock_proc_init(struct proc *procp);
extern void ublock_proc_deinit(struct proc *procp);
extern vaddr_t ublock_lwp_alloc(struct lwp *lwpp);
extern void ublock_lwp_detach(struct lwp *lwpp);
extern void ublock_lwp_free(struct lwp *lwpp);
extern int ublock_lock(struct proc *procp, ub_locktype_t locktype);
extern void ublock_unlock(struct proc *procp, ub_locktype_t locktype);
extern void ublock_save_extension(void *stack, size_t size);
extern void ublock_restore_extension(void *stack, size_t size);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_UBLOCK_H */
