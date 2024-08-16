/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _SVC_AUTOTUNE_H	/* wrapper symbol for kernel use */
#define _SVC_AUTOTUNE_H	/* subject to change without notice */

#ident	"@(#)kern:svc/autotune.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

struct tune_point {
	int tv_mempt;	/* an amount of physical memory, in megabytes   */
	int tv_tuneval;	/* the value of the tunable at that much memory */
	int tv_typeflag;/* how to calculate the value for memory sizes
			   between this one and the next; see below 	*/
};
#define TV_STEP		1
#define TV_LINEAR	2

#define MEGABYTE	(1024 * 1024)
#define MEGSHIFT 	20

/*
 * Convert a memory size to megabytes (rounding up)
 */
#define TUNE_ROUNDUP(size)	(((size) + (MEGABYTE - 1)) >> MEGSHIFT)

#define TV_AUTOTUNE 	-1

/*
 * Memory sizes use for auto-tuning (in megabytes).
 */
extern int tunemem;

extern int tune_calc(struct tune_point *, int);

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_AUTOTUNE_H */
