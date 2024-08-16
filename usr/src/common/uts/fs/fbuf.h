/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_FBUF_H	/* wrapper symbol for kernel use */
#define _FS_FBUF_H	/* subject to change without notice */

#ident	"@(#)kern:fs/fbuf.h	1.10"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <mem/seg.h> /* REQUIRED */
#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <vm/seg.h> /* REQUIRED */
#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * A struct fbuf is used to get a mapping to part of a file using the
 * segkmap facilities.  After you get a mapping, you can fbrelse() it
 * (giving a seg code to pass back to segmap_release), you can fbwrite()
 * it (causes a synchronous write back using the file mapping information),
 * or you can fbiwrite it (causing indirect synchronous write back to
 * the block number given without using the file mapping information).
 */

typedef struct fbuf {
	char	*fb_addr;
	size_t	 fb_count;
} fbuf_t;

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

struct vnode;

extern int fbread(struct vnode *vp, off_t off, size_t len, enum seg_rw rw,
		  fbuf_t **fbpp);
extern int fbrelse(fbuf_t *fbp, uint_t sm_flags);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_FBUF_H */
