/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:svc/svc.cf/Space.c	1.2"
#ident	"$Header: $"

#include <config.h>

/* Time variables for XENIX-style ftime() system call. */
int Timezone = TIMEZONE;	/* tunable time zone for XENIX ftime() */
int Dstflag = DSTFLAG;		/* tunable daylight time flag for XENIX */

/*
 * Switch to turn on/off Intel386(tm) B1 stepping workarounds.
 * The variables do386b1 and do386b1_x87 are all controlled by the DO386B1
 * tunable parameter, which takes 3 values:
 *	0: disable workarounds,
 *	1: always enable workarounds,
 *	2: auto-detect B1 stepping; enable workarounds if needed.
 * The do386b1_x87 variable will be turned off if neither an 80287 nor an
 * 80387 is being used, in order to disable workarounds which are only needed
 * when a math coprocessor is present.
 */
int do386b1 = DO386B1;
int do386b1_x87;	/* copied from do386b1 if 80287 or 80387 used */

/*
 * DO387CR3 controls the workaround for the Intel386 B1 stepping errata #21.
 * Like do386b1_x87, it will be turned off if do386b1 is off or if no math
 * chip is being used.  It needs to be separate since some hardware can't
 * support this workaround (which requires H/W address line A31 be ignored
 * for main memory accesses).
 */
int do387cr3 = DO387CR3;

/*
 * FPKIND: type of floating-point support: 0 = auto-detect,
 *	  1 = software emulator, 2 = Intel287 coproc, 3 = Intel387 coproc
 */
int fp_kind = FPKIND;
