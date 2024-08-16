/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_KSYNCH_P_H	/* wrapper symbol for kernel use */
#define _UTIL_KSYNCH_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:util/ksynch_p.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * kernel synchronization primitives
 */
#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/dl.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/dl.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#ifdef _KERNEL

extern ulong_t kvetc;	/* Free-running clock */

/*
 * TIME_INIT -- put the current time in a dl_t.  Symmetry-dependent.
 */
#define TIME_INIT(dlp) \
	((*(dlp)).dl_lop = kvetc, (*(dlp)).dl_hop = 0)

/*
 * TIME_UPDATE -- update the first parameter with the difference between
 *	the time stored in the second parameter and the current time.
 *	Symmetry-dependent.
 */
#define TIME_UPDATE(dlp, dl) {				\
	dl_t dltmp;					\
	dltmp.dl_lop = kvetc;		\
	dltmp.dl_hop = 0;				\
	*(dlp) = ladd(*(dlp), lsub(dltmp, (dl)));	\
}

/*
 * GET_TIME(timep) -- initialize the first argument with the current time.
 *	The first argument is a pointer to an unsigned long. This is a 
 * 	platform specific macro.
 */
#define GET_TIME(timep) \
	((*(timep)) = kvetc)

#define LK_THRESHOLD  500000	/* 500ms */
/*
 * GET_DELTA(deltap, stime) -- Initialize the first argument with the 
 *	difference between the current time and the second argument. The first 
 *	argument is a pointer to an unsigned long and the second argument is 
 *	an unsigned long. Further, if the difference is above a threshold,
 *	the first srgument is set to zero. This is a platform specific macro.
 */

#define GET_DELTA(deltap, stime) \
{ \
	ulong_t deltmp; \
	deltmp = kvetc - (stime); \
	if (deltmp > LK_THRESHOLD) \
		deltmp = 0; \
	(*(deltap)) = deltmp; \
}

/*
 * CONVERT_IPL -- Stores the ipl passed in the second argument in the
 * first argument. If the platform uses negative logic to manipulate ipls, 
 * the ones complement of the passed in ipl is stored in the first argument. 
 */
#define CONVERT_IPL(var, ipl)		((var) = ((~(ipl)) & (0xff)))

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_KSYNCH_P_H */
