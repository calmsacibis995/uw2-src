/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CLASS_RTPRIOCNTL_H	/* wrapper symbol for kernel use */
#define _PROC_CLASS_RTPRIOCNTL_H	/* subject to change without notice */

#ident	"@(#)kern:proc/class/rtpriocntl.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#endif /* _KERNEL */

/*
 * Real-time class specific structures for the priocntl system call.
 */

typedef struct rtparms {
	short	rt_pri;		/* real-time priority */
	ulong	rt_tqsecs;	/* seconds in time quantum */
	long	rt_tqnsecs;	/* additional nanosecs in time quantum */
} rtparms_t;


typedef struct rtinfo {
	short	rt_maxpri;	/* maximum configured real-time priority */
} rtinfo_t;


#define	RT_NOCHANGE	-1
#define RT_TQINF	-2
#define RT_TQDEF	-3


/*
 * The following is used by the dispadmin(1M) command for
 * scheduler administration and is not for general use.
 */

typedef struct rtadmin {
	struct rtdpent	*rt_dpents;
	short		rt_ndpents;
	short		rt_cmd;
} rtadmin_t;

#define	RT_GETDPSIZE	1
#define	RT_GETDPTBL	2
#define	RT_SETDPTBL	3


#if defined(__cplusplus)
	}
#endif

#endif	/* _PROC_CLASS_RTPRIOCNTL_H */
