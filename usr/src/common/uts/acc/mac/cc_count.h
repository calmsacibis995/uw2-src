/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ACC_MAC_CC_COUNT_H	/* wrapper symbol for kernel use */
#define _ACC_MAC_CC_COUNT_H	/* subject to change without notice */

#ident	"@(#)kern:acc/mac/cc_count.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL
#ifdef CC_PARTIAL

#include <acc/mac/covert.h>
#include <proc/user.h>

/*
 * This header file defines the CC_COUNT macro used to count cc events.
 * It is defined here in a separate header file, rather than in covert.h,
 * to avoid a circular header file dependency.
 */

/*
 * CC_COUNT() records the fact that a covert channel event was encountered.
 * The event is only recorded here; it is not actually treated until just
 * before retruning to user level.  This is because some covert channel
 * events are encountered while spin locks are held, and treatment can involve
 * sleeping.
 *
 * Events need not be counted when called from interrupt
 * level since the information is not meaningful to the user
 * (unless interrupts can be controlled).
 *
 * The alternate definition for lint is because it complains about the
 * argument to ASSERT being a constant condition.  The *right* way to
 * shut it up would be with a CONSTCOND lint directive, but alas, this
 * doesn't work since lint doesn't see comments inside macro definitions.
 * There's talk of fixing this lint shortcoming in a later release, but
 * meanwhile...  :-(
 */

#ifndef lint

#define CC_COUNT(event, bits) {				\
	/*CONSTCOND*/					\
	ASSERT((event) < CC_MAXEVENTS);			\
	if (!servicing_interrupt()) {			\
		u.u_covert.c_bitmap |= 1 << (event);	\
		u.u_covert.c_cnt[event] += bits;	\
	}						\
}

#else /* lint */

#define CC_COUNT(event, bits) {				\
	if (!servicing_interrupt()) {			\
		u.u_covert.c_bitmap |= 1 << (event);	\
		u.u_covert.c_cnt[event] += bits;	\
	}						\
}

#endif /* lint */

#else /* CC_PARTIAL */

#define CC_COUNT(event, bits)

#endif /* CC_PARTIAL */
#endif /* _KERNEL */
#if defined(__cplusplus)
	}
#endif

#endif /* _ACC_MAC_CC_COUNT_H */
