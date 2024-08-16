/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_METDISK_P_H	/* wrapper symbol for kernel use */
#define _IO_METDISK_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/metdisk_p.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <mem/vmparam.h>	/* PORTABILITY */
#include <mem/vm_mdep.h>	/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * GET_MET_TIME() is used by met_ds_queued() and met_ds_iodone() to
 * get the current time in the best way for the machine.
 * If a usec time is easy to access, that's the best choice.
 * Anything that fits in a ulong is fine though, it doesn't have
 * to be usecs.  It is DIFF_TIME()'s job to take a difference of
 * two of these times and return usecs.
 */
#define GET_MET_TIME(time)	((time) = *(unsigned long *)KVETC)

/*
 * DIFF_MET_TIME() is used by met_ds_queued() and met_ds_iodone() to
 * take the difference between time and lasttime and to return the
 * usec equivalent in since_lasttime.
 */
#define DIFF_MET_TIME(time, lasttime, since_lasttime)	\
	((since_lasttime) = (time) - (lasttime))

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_METDISK_P_H */
