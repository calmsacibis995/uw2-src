/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_SEG_PSE_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_PSE_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/seg_pse.h	1.1"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

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

typedef struct segpse_page segpse_page_t;

/*
 * Structure whose pointer is passed to the segpse_create routine.
 */
struct segpse_crargs {
	int (*mapfunc)(dev_t, off_t, uint_t);	/* mmap function to call */
	off_t offset;		/* starting offset */
	dev_t dev;		/* device number */
	uchar_t prot;		/* initial protection */
	uchar_t maxprot;	/* maximum protection */
};

/*
 * (Semi) private data maintained by the seg_pse driver per segment mapping.
 */
struct segpse_data {
	int (*mapfunc)(dev_t, off_t, uint_t);	/* logically-speaking,
						 * returns ppid_t, not int */
	off_t offset;		/* device offset for start of mapping */
	struct vnode *vp;	/* vnode associated with device */
	segpse_page_t *vpage;	/* per-page information, if needed */
	uchar_t prot;		/* current segment prot if vpage == NULL */
	uchar_t maxprot;	/* maximum segment protections */
	lock_t mutex;		/* lock to protect vpage[] */
};

/*
 * Per-page data.
 */
struct segpse_page {
	uchar_t dvp_prot;	/* per-page protections */
};

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern int segpse_create(struct seg *, void *);

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _MEM_SEG_PSE_H */
