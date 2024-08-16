/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_SEG_DEV_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_DEV_H	/* subject to change without notice */

#ident	"@(#)kern:mem/seg_dev.h	1.9"
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

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h> /* REQUIRED */
#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h> /* REQUIRED */
#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef struct segdev_page segdev_page_t;

/*
 * Structure whose pointer is passed to the segdev_create routine.
 */
struct segdev_crargs {
	int (*mapfunc)(dev_t, off_t, uint_t);	/* mmap function to call */
	off_t offset;		/* starting offset */
	dev_t dev;		/* device number */
	uchar_t prot;		/* initial protection */
	uchar_t maxprot;	/* maximum protection */
};

/*
 * (Semi) private data maintained by the seg_dev driver per segment mapping.
 */
struct segdev_data {
	int (*mapfunc)(dev_t, off_t, uint_t);	/* logically-speaking,
						 * returns ppid_t, not int */
	off_t offset;		/* device offset for start of mapping */
	struct vnode *vp;	/* vnode associated with device */
	segdev_page_t *vpage;	/* per-page information, if needed */
	uchar_t prot;		/* current segment prot if vpage == NULL */
	uchar_t maxprot;	/* maximum segment protections */
	lock_t mutex;		/* lock to protect vpage[] */
	sv_t softsv;		/* block here if waiting on dvp_softcnt */
};

/*
 * Per-page data.
 */
struct segdev_page {
	uchar_t dvp_prot;	/* per-page protections */
	uchar_t dvp_softcnt;	/* count of pending SOFTLOCKs */
};

#define DVP_SOFTMAX	(1 << (sizeof(uchar_t) * NBBY))

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern int segdev_create(struct seg *, void *);

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _MEM_SEG_DEV_H */
