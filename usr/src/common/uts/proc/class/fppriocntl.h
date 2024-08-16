/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CLASS_FPPRIOCNTL_H	/* wrapper symbol for kernel use */
#define _PROC_CLASS_FPPRIOCNTL_H	/* subject to change without notice */

#ident	"@(#)kern:proc/class/fppriocntl.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Fixed-priority class specific structures for the priocntl system call.
 */

typedef struct fpparms {
	short	fp_pri;		/* FP priority */
	ulong_t	fp_tqsecs;	/* seconds in time quantum */
	long	fp_tqnsecs;	/* additional nanosecs in time quantum */
} fpparms_t;


typedef struct fpinfo {
	short	fp_maxpri;	/* maximum configured FP priority */
} fpinfo_t;


#define	FP_NOCHANGE	-5	


#define FP_TQINF	-2	/* infinite time quantum */
#define FP_TQDEF	-3	/* default time quantum for this priority */


/*
 * The following is used by the dispadmin(1M) command for
 * scheduler administration and is not for general use.
 */

typedef struct fpadmin {
	struct fpdpent	*fp_dpents;
	short		fp_ndpents;
	short		fp_cmd;
} fpadmin_t;

#define	FP_GETDPSIZE	1	/* get the size of FP dispatcher table */
#define	FP_GETDPTBL	2	/* get the FP dispatcher table values */
#define	FP_SETDPTBL	3	/* set the FP dispatcher table values */


#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_CLASS_FPPRIOCNTL_H */
