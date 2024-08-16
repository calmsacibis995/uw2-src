/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_TIMES_H	/* wrapper symbol for kernel use */
#define _PROC_TIMES_H	/* subject to change without notice */

#ident	"@(#)kern:proc/times.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#else

#include <sys/types.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * Structure returned by times()
 */
struct tms {
	clock_t	tms_utime;		/* user time */
	clock_t	tms_stime;		/* system time */
	clock_t	tms_cutime;		/* user time, children */
	clock_t	tms_cstime;		/* system time, children */
};

#if !defined(_KERNEL)
#if defined(__STDC__)
clock_t times(struct tms *);
#else
clock_t times();
#endif
#endif

#if defined(__cplusplus)
	}
#endif

#endif /*_PROC_TIMES_H */
