/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_SWAP_H	/* wrapper symbol for kernel use */
#define _MEM_SWAP_H	/* subject to change without notice */

#ident	"@(#)kern:mem/swap.h	1.18"
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

#ifdef _KERNEL_HEADERS

#include <svc/systm.h>		/* REQUIRED */
#include <util/param.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/param.h>		/* REQUIRED */
#include <sys/systm.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * SWAPINFO: locus of activity for adding, deleting, allocating and
 * freeing real backing storage for anonymous pages. Identity is lent
 * to these pages by the use of a virtual anode/offset pair. If and when
 * pages are pushed to swap, the bitmap in the swapinfo structure is
 * used to give real allocation for the page. VOP_PUTPAGELIST is called
 * using the real vp/offset.
 *
 * Swap devices are configured into the swap table (swaptab). This table
 * and all swapinfo structures are protected by the swap_lck.
 *
 * swapcursor points to the next slot in the swaptab from which space
 * will be allocated. swapcursor is advanced as space is allocated.
 *
 * The global nswapfiles is a count of configured swap devices and is also
 * protected by swap_lck. So long as (nswapfiles == 0) no anon pages will
 * be cleaned. Another global nswappgfree is a count of available swap pages.
 *
 * swap_lck, swapinfo, nextswapalloc, nswapfiles, and nswappgfree are declared 
 * in kern:mem/vm_swap.c
 */

typedef struct swapinfo {
	struct vnode	*si_ovp;	/* vnode returned by VOP_OPEN */
	struct vnode	*si_cvp;	/* vnode returned by common_specvp() */
	struct vnode	*si_stvp;	/* vnode returned by VOP_STABLESTORE */
	off_t		si_soff;	/* starting offset (bytes) of file */
	off_t		si_osoff;	/* original starting offset (bytes) */
					/* before page/block roundups */
	off_t		si_eoff;	/* ending offset (bytes) of file */
	ushort_t	si_flags;	/* flags defined below */
	ulong_t		si_npgs;	/* number of pages of swap space */
	ulong_t		si_nfpgs;	/* number of free pages of swap space */
	uint_t		*si_pgmap;	/* bitmap of pages in use */
	void		*si_obj;	/* object returned by VOP_STABLESTORE */
	char		*si_pname;	/* swap file name */
	uint_t		si_cursor;	/* allocation cursor (word index) */
} swapinfo_t;

#if PAGESHIFT >= NBBY + 1

/*
 * Packed backing store location:
 *
 * The swaploc_t data is used to achieve a packing of the swap file offset
 * (off_t) and swap table index (uchar_t) values. It takes advantage of the
 * fact the the lower log2(PAGESIZE) bits of a page aligned off_t are 0.
 * These bit positions can then be used store the swap table index or some
 * special flags..
 */
typedef off_t	swaploc_t;

#define SWL_IDXMASK	((1 << NBBY) - 1)
#define SWL_OFFMASK	PAGEMASK
#define SWL_SPECIAL	(SWL_IDXMASK + 1)
#define SWL_SPIDX	(SWL_IDXMASK|SWL_SPECIAL)
#define SWL_NOSWAP	(SWL_SPECIAL + 0)
#define SWL_IOERR	(SWL_SPECIAL + 1)

#define SWAPLOC_IS_EMPTY(swlp)			(*(swlp) == SWL_NOSWAP)
#define SWAPLOC_IS_IOERR(swlp)			(*(swlp) == SWL_IOERR)
#define SWAPLOC_TO_OFF(swlp)			(*(swlp) & SWL_OFFMASK)
#define SWAPLOC_TO_IDX(swlp)			(*(swlp) & SWL_IDXMASK)
#define SWAPLOC_MAKE(swlp, off, idx)		{ *(swlp) = (off)|(idx); }
#define SWAPLOC_MAKE_EMPTY(swlp)		{ *(swlp) = SWL_NOSWAP; }
#define SWAPLOC_MAKE_IOERR(swlp)		{ *(swlp) = SWL_IOERR; }
#define SWAPLOC_ADD_OFFSET(swlp, off)		{ *(swlp) += (off); }
#define SWAPLOC_CONTIG(swl1p, off, swl2p)		\
	(*(swl1p) + (off) == *(swl2p))
#define SWAPLOC_IDX_EQUAL(swlp, idx)			\
	((*(swlp) & SWL_SPIDX) == (idx))
#define SWAPLOC_HAS_BACKING(swlp)		(!(*(swlp) & SWL_SPECIAL))
#define SWAPLOC_EQUAL(swl1p, off, swl2p)			\
	(*(swl1p) == *(swl2p) || SWAPLOC_CONTIG(swl1p, off, swl2p))

#else /* PAGESHIFT >= NBBY + 1 */

/*
 * If the pagesize is too small, then the above packing method
 * does not work.
 */
typdef struct swaploc {
	off_t		swl_offset;
	uchar_t		swl_idx;
	uchar_t		swl_flags;
} swaploc_t;

#define SWL_NOFLAGS	0
#define SWL_NOSWAP	(1 << 1)
#define SWL_IOERR	(1 << 2)

#define SWAPLOC_HAS_BACKING(swlp)		\
	((swlp)->swl_flags == SWL_NOFLAGS)

#define SWAPLOC_IS_EMPTY(swlp)			\
	((swlp)->swl_flags == SWL_NOSWAP)

#define SWAPLOC_IS_IOERR(swlp)			\
	((swlp)->swl_flags == SWL_IOERR)

#define SWAPLOC_TO_OFF(swlp)			\
	((swlp)->swl_offset)

#define SWAPLOC_TO_IDX(swlp)			\
	((swlp)->swl_idx)

#define SWAPLOC_MAKE(swlp, off, idx) {		\
	(swlp)->swl_offset = (off);		\
	(swlp)->swl_idx = (idx);		\
	(swlp)->swl_flags = SWL_NOFLAGS;	\
}

#define SWAPLOC_MAKE_EMPTY(swlp) {		\
	(swlp)->swl_flags = SWL_NOSWAP;		\
}

#define SWAPLOC_MAKE_IOERR(swlp) {		\
	(swlp)->swl_flags = SWL_IOERR;		\
}

#define SWAPLOC_ADD_OFFSET(swlp, off) {		\
	(swlp)->swl_offset += (off);		\
}

#define SWAPLOC_CONTIG(swl1p, off, swl2p)			\
	((swl1p)->swl_flags == (swl2p)->swl_flags &&		\
	 (swl1p)->swl_idx == (swl2p)->swl_idx &&		\
	 (swl1p)->swl_offset + (off) == (swl2p)->swl_offset)

#define SWAPLOC_IDX_COMPARE(swlp, idx)		\
	((swlp)->swl_flags == SWL_NOFLAGS && (swlp)->swl_idx == (idx))

#define SWAPLOC_HAS_BACKING(swlp)		\
	((swlp)->swl_flags == SWL_NOFLAGS)

#define SWAPLOC_EQUAL(swl1p, off, swl2p)			\
	(SWAPLOC_IS_EMPTY(swl1p) ?  SWAPLOC_IS_EMPTY(swl2p) :	\
				    SWAPLOC_CONTIG(swl1p, off, swl2p))

#endif /* PAGESHIFT >= NBBY + 1 */

#endif /* defined(_KERNEL) || defined(_KMEMUSER) */

#ifdef _KERNEL

/*
 * Exported Interfaces
 */
struct swapcmda;
extern int swapctl(struct swapcmda *uap, rval_t *rvp);
extern boolean_t swap_alloc(int npages, swaploc_t *swaplocp);
extern void swap_free(int npages, swaploc_t *swaplocp,
		      boolean_t breakdouble);
extern void swap_add(void);
extern void swap_init(void);

/*
 * Exported Data
 */
extern swapinfo_t **swaptab;
extern ulong_t swapdoubles;
extern ulong_t nswappgfree;
extern ulong_t nswappg;
extern uint_t nswapfiles;
extern lock_t swap_lck;

#else

#ifdef __STDC__
extern int swapctl(int, void *);
#else
extern int swapctl();
#endif

#endif /* _KERNEL */


/*
 * Bits for si_flags and ste_flags (below)
 */

#define ST_INDEL	0x01		/* This file is in the process */
                                        /* of being deleted. Don't     */
                                        /* allocate from it. This can  */
                                        /* be turned off by swapadding */
                                        /* this device again.          */
					/* NOTE this is the only flag  */
					/* which users see when they   */ 
					/* do an SC_LIST.  	       */

#define ST_NOTREADY	0x02		/* This file is in the process */
					/* of being added and has been */
					/* placed in the swapinfo table*/
					/* to prevent duplicate adds.   */

/* The following are for the swapctl system call */

#define SC_ADD          1       /* add a specified resource for swapping */
#define SC_LIST         2       /* list all the swapping resources */
#define SC_REMOVE       3       /* remove the specified swapping resource */
#define SC_GETNSWP      4       /* get number of swapping resources configued */
#define SC_GETUSAGE	5	/* get allocation/reservation information */

typedef struct swapres {
        char    *sr_name;       /* pathname of the resource specified */
        off_t   sr_start;       /* starting offset of the swapping resource */
        off_t   sr_length;      /* length of the swap area */
} swapres_t;

typedef struct swapent {
        char    *ste_path;      /* get the name of the swap file */
        off_t   ste_start;      /* starting block for swapping */
        off_t   ste_length;     /* length of swap area */
        long    ste_pages;      /* numbers of pages for swapping */
        long    ste_free;       /* numbers of ste_pages free */
        long    ste_flags;      /* see above */
} swapent_t;

typedef struct  swaptable {
        int     swt_n;                  /*number of swapents following */
        struct  swapent swt_ent[1];     /* array of swt_n swapents */
} swaptbl_t;

typedef struct swapusage {
	ulong_t	stu_allocated;	/* swap pages allocated */
	ulong_t	stu_used;	/* swap pages reserved or allocated */
	ulong_t	stu_max;	/* maximum possible usage */
} swapusage_t;

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_SWAP_H */
