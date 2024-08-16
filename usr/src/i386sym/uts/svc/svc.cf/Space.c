/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:svc/svc.cf/Space.c	1.4"
#ident	"$Header: $"

#include <config.h>
#include <sys/types.h>
#include <sys/slicreg.h>
#include <sys/clkarb.h>

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

/*
 * light_show decides which LED's are used to display system activity.
 * S16 and S27 can only display on the processor board LED's; S81's can
 * display on the front-panel and processor board LED's.  light_show == 1
 * displays in one place (front-panel if S81); light_show == 2 displays in
 * both places if possible; light_show == 0 doesn't flash at all (quite boring).
 */

int light_show = 2;		/* display on most useful place */

/*
 * The front panel has 48 programmable leds. These are arranged in 12 columns
 * with 4 leds in each row. The front panel led's are addressed from
 * left to right, top to bottom.
 */

/*
 * Currently assumes only processors will turn on lights.
 * Table is indexed by processor number. The first MAXNUMPROC entries
 * are reserved for processor use.
 */
#define	FP_LED(i)	(SL_C_FP_LIGHT + ((i) * 2))
uchar_t fp_lightmap[FP_NLIGHTS] = {
	FP_LED(0),  FP_LED(1),  FP_LED(2),  FP_LED(3),  FP_LED(4),
	FP_LED(5),  FP_LED(6),  FP_LED(7),  FP_LED(8),  FP_LED(9),
	FP_LED(10), FP_LED(11), FP_LED(12), FP_LED(13), FP_LED(14),
	FP_LED(15), FP_LED(16), FP_LED(17), FP_LED(18), FP_LED(19),
	FP_LED(20), FP_LED(21), FP_LED(22), FP_LED(23), FP_LED(24),
	FP_LED(25), FP_LED(26), FP_LED(27), FP_LED(28), FP_LED(29),
	FP_LED(30), FP_LED(31), FP_LED(32), FP_LED(33), FP_LED(34),
	FP_LED(35), FP_LED(36), FP_LED(37), FP_LED(38), FP_LED(39),
	FP_LED(40), FP_LED(41), FP_LED(42), FP_LED(43), FP_LED(44),
	FP_LED(45), FP_LED(46), FP_LED(47)
};
