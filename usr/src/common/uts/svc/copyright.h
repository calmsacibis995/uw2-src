/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_COPYRIGHT_H	/* wrapper symbol for kernel use */
#define _SVC_COPYRIGHT_H	/* subject to change without notice */

#ident	"@(#)kern:svc/copyright.h	1.14"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Strings for title and copyright messages
 * printed during system initialization.
 */

#ifdef _KERNEL_HEADERS

#include <svc/copyright_f.h> /* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

#define COPYRIGHT_COMMON "Copyright 1984-1995 Novell, Inc.  "

#define MAXCOPYRIGHT	10	/* Maximum # copyright lines,
				 * including "All Rights Reserved"
				 */

#define MAXTITLE	10	/* Maximum # title lines */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_COPYRIGHT_H */
