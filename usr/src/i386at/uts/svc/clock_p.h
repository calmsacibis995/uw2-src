/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_SVC_CLOCK_P_H	/* wrapper symbol for kernel use */
#define	_SVC_CLOCK_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:svc/clock_p.h	1.16"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <proc/seg.h>	/* PORTABILITY */
#include <svc/reg.h>	/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * We allow a toid_t to be a 64-bit integer on certain platforms.
 * Such platforms will define toid_incr as a asm or function.
 * The value zero must be skipped, since it is used to indicate failure.
 * toid_incr() returns the new value of (id).
 */
#define	toid_incr(id)	(++(id) == 0 ? ++(id) : (id))

/*
 * Number of buckets in the callout hash list.  This is essentially
 * a platform specific tuneable, as each platform is designed to
 * handle a certain workload.  Each platform should tune this
 * bucket size according to their expected workload.
 */
#define	NCALLOUT_HASH	1024	/* some power of 2 */

/*
 * Platform specific macros which specify what the arguments to the
 * clock handler are for this particular architecture.
 *
 * Arguments for the global clock handler.
 */
#define	TODCLOCK_ARGS	const uint *r0ptr

/*
 * Determine if the interrupted priority was at base level.
 */
#define	TOD_WAS_BASE_PRIORITY	(((r0ptr[T_EDX - 1]) & 0xFF) == PL0)

/*
 * Per-processor clock handler arguments.
 */
#define	LCLCLOCK_ARGS		TODCLOCK_ARGS
/*
 * Per-processor program counter to be passed to kernel profiler
 */
#define	LCLCLOCK_PC		(r0ptr[T_EIP])

/*
 * Determine if the interrupted priority was base level.
 */
#define	LCL_WAS_BASE_PRIORITY	TOD_WAS_BASE_PRIORITY

/*
 * Determine if the interrupted mode was user-mode.
 */
#define	LCL_WAS_USER_MODE	((r0ptr[T_CS] & RPL_MASK) != KERNEL_RPL)

extern void todclock(TODCLOCK_ARGS);
extern void lclclock(LCLCLOCK_ARGS);

/*
 * Take a profiling sample from lclclock.  A no-op on this platform,
 *	since profiling is done from devint0 directly
 */
#define	PRFINTR(pc, usermode)	/* no-op */

/*
 * Largest, signed integer.
 */
#define	CALLOUT_MAXVAL	0x7fffffff

/*
 * Flag for ticks argument of itimeout, et al.
 */
#define TO_PERIODIC	0x80000000	/* periodic repeating timer */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_CLOCK_P_H */
