/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_IOBITMAP_H	/* wrapper symbol for kernel use */
#define _PROC_IOBITMAP_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/iobitmap.h	1.8"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/* Request types for iobitmapctl(): */
typedef enum iob_request {
	IOB_ENABLE,	/* Enable access to the indicated I/O ports */
	IOB_DISABLE,	/* Disable access to the indicated I/O ports */
	IOB_CHECK	/* Check access to the indicated I/O ports */
} iob_request_t;

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL
#ifdef __STDC__
extern int iobitmapctl(iob_request_t iob_rqt, ushort_t ports[]);
extern boolean_t iobitmap_sync(void);
extern void iobitmap_reset(void);
#else
extern int iobitmapctl();
#endif
#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_IOBITMAP_H */
