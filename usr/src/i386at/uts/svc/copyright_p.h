/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_COPYRIGHT_P_H	/* wrapper symbol for kernel use */
#define _SVC_COPYRIGHT_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:svc/copyright_p.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Strings for copyright messages printed during system initialization.
 */

#ifdef _KERNEL

#define COPYRIGHT_PLATFORM	/* none */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_COPYRIGHT_P_H */
