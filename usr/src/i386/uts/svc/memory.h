/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_MEMORY_H	/* wrapper symbol for kernel use */
#define _SVC_MEMORY_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/memory.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Memory related interfaces and such.
 */

extern	paddr_t	topmem;
extern	size_t	totalmem;

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_MEMORY_H */
