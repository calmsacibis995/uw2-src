/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_PVN_H	/* wrapper symbol for kernel use */
#define _MEM_PVN_H	/* subject to change without notice */

#ident	"@(#)kern:mem/pvn.h	1.24"
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
/*
 * VM - paged vnode.
 *
 * The VM system manages memory as a cache of paged vnodes.
 * This file describes the interfaces to common subroutines
 * used to help implement the VM/file system routines.
 */

#ifdef _KERNEL_HEADERS

#include <mem/page.h> /* REQUIRED */
#include <mem/mem_hier.h> /* REQUIRED */
#include <mem/seg.h> /* REQUIRED */
#include <mem/memresv.h> /* REQUIRED */
#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <vm/page.h> /* REQUIRED */
#include <vm/seg.h> /* REQUIRED */
#include <vm/mem_hier.h> /* REQUIRED */
#include <vm/memresv.h> /* REQUIRED */
#include <sys/types.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */


#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * pvn_mem_resv() maintains a hash list (in pvn_memresv_hash[]) of <vp, offset>
 * pages which have outstanding mem_resv() real memory reservations.
 */

typedef struct pvn_memresv_entry {
	struct pvn_memresv_entry *pml_next;	/* hash chain */
	struct vnode		 *pml_vp;	/* vnode of logical page */
	off_t			  pml_offset;	/* offset of logical page */
	ushort_t		  pml_rcount;	/* M_REAL resvs */
	uchar_t			  pml_scount;	/* M_SWAP resvs */
	uchar_t			  pml_realuser; /* M_REAL_USER done */
} pvn_memresv_entry;

#endif /* _KERNEL || _KMEMUSER */

#if defined(_KERNEL)

extern pvn_memresv_entry **pvn_memresv_hash;

struct buf;
struct cred;
struct vnode;

void	pvn_init(void);
void	pvn_fail(page_t *plist, int flags);
void	pvn_done(struct buf *bp);
void	pvn_unload(vnode_t *vp);
void	pvn_abort_range(struct vnode *vp, off_t off, uint_t len);
int	pvn_trunczero(struct vnode *vp, uint_t off, uint_t zbytes);
int     pvn_getdirty_range(int (*func)(), vnode_t *vp, off_t roff,
	    uint_t rlen, off_t doff, uint_t dlen, off_t filesize, 
	    int flags, cred_t *cr);
page_t *pvn_kluster(struct vnode *vp, off_t off, struct seg *seg, vaddr_t addr,
	   off_t *offp, uint_t *lenp, off_t vp_off, uint_t vp_len, page_t *pp);
int	pvn_getpages(int (*getapage)(), struct vnode *vp, off_t off,
		    uint_t len, uint_t *protp, page_t *pl[], uint_t plsz,
		    struct seg *seg, vaddr_t addr, enum seg_rw rw,
		    struct cred *cred);
int	pvn_memresv(struct vnode *vp, off_t offset, mresvtyp_t mtype, 
	    uint_t flag);
void	pvn_memunresv(struct vnode *vp, off_t offset, mresvtyp_t mtype);
void	pvn_cache_memresv(struct vnode *vp, off_t offset, mresvtyp_t mtype);
boolean_t pvn_memresv_query(struct vnode *vp, off_t offset);
boolean_t pvn_memresv_swap(uint_t swresv, void (*scan_func)(), void *arg);
boolean_t pvn_abort_range_nosleep(struct vnode *vp, off_t off, uint_t len);
void	pvn_syncsdirty(struct vnode *vp);

#define PVN_NICE_VAL 35

/*
 * When requesting pages from the VOP_GETPAGE routines, the segment drivers
 * allocate space to return PVN_KLUSTER_NUM pages, which map PVN_KLUSTER_SZ
 * worth of bytes. These numbers are chosen to be the minimum of
 * the max's given in terms of bytes and pages. In addition, PVN_KLUSTER_SZ
 * cannot exceed MAXBIOSIZE.
 */
#define PVN_MAX_KLUSTER_SZ	0x10000		/* kluster size limit */
#define PVN_MAX_KLUSTER_NUM	0x10		/* kluster page limit */

#if PVN_MAX_KLUSTER_SZ > PVN_MAX_KLUSTER_NUM * PAGESIZE
#define PVN_KN	PVN_MAX_KLUSTER_NUM
#else
#define PVN_KN	((PVN_MAX_KLUSTER_SZ + PAGEOFFSET) >> PAGESHIFT)
#endif

#if PVN_KN < (MAXBIOSIZE >> PAGESHIFT)
#define PVN_KLUSTER_NUM		PVN_KN
#else
#define PVN_KLUSTER_NUM		btop(MAXBIOSIZE)
#endif

#define PVN_KLUSTER_SZ		ptob(PVN_KLUSTER_NUM)

#if (!VM_INTR_IPL_IS_PLMIN)

#define PVN_MEMRESV_LOCK()	LOCK(&pvn_memresv_lock, VM_PVNMEMRESV_IPL)
#define PVN_MEMRESV_UNLOCK(ipl)	UNLOCK(&pvn_memresv_lock, (ipl))

#else  /* (VM_INTR_IPL_IS_PLMIN) */

#define PVN_MEMRESV_LOCK()      LOCK_PLMIN(&pvn_memresv_lock)
#define PVN_MEMRESV_UNLOCK(ipl) UNLOCK_PLMIN(&pvn_memresv_lock, (ipl))

#endif /* VM_INTR_IPL_IS_PLMIN */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_PVN_H */
