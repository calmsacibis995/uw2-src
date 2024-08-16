/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:svc/clock_p.c	1.12"
#ident	"$Header: $"

/*
 * Machine-dependent clock routines.  This file defines routines that
 * are called from machine-independent files (usually) and call routines
 * whose names may not be mentioned in machine-independent files.
 */

#include <io/ssm/ssm_misc.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <svc/clock.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#define	MICROCOUNT 10

extern void _tenmicrosec(void);

long microdata = MICROCOUNT;		/* MUST be initialized for those who
					 * inisit on calling _tenmicrosec or
					 * drv_usecwait before clock has been
					 * initialized
					 */
time_t c_correct;

/*
 * void wtodc(timestruc_t *hrt)
 *	Set the hardware time-of-day clock, if any, to the given time.
 *
 * Calling/Exit State:
 *	hrt is the time to set the clock to.  Returns: none.
 */
void
wtodc(timestruc_t *hrt)
{
	/*
	 * On the Symmetry, we call ssm_set_tod with the number of
	 * seconds since the epoch to set the clock.
	 */
	ssm_set_tod(hrt->tv_sec);
}


/*
 * void
 * microfind(void)
 *	set microdata (used for drv_usecwait().
 *	called from sysinit for now ... eventually should be
 *	from clock init.
 *
 * Calling/Exit State:
 *	none
 */

void
microfind(void)
{
	unsigned long	t1, t2;
	long		diff;

	/*
	 * check actual elapsed time when call _tenmicrosec()
	 * adjust microdata accordingly
	 */
	diff = 0;
	t1 = *(unsigned long *)KVETC;
	_tenmicrosec();
	t2 = *(unsigned long *)KVETC;
	diff += t2 - t1 - 10;

	t1 = *(unsigned long *)KVETC;
	_tenmicrosec();
	t2 = *(unsigned long *)KVETC;
	diff += t2 - t1 - 10;

	t1 = *(unsigned long *)KVETC;
	_tenmicrosec();
	t2 = *(unsigned long *)KVETC;
	diff += t2 - t1 - 10;

	t1 = *(unsigned long *)KVETC;
	_tenmicrosec();
	t2 = *(unsigned long *)KVETC;
	diff += t2 - t1 - 10;

	diff /= 4;
	if ( diff < 10 )
		microdata += -diff;
	else if ( diff > 10 )
		microdata -= diff;
	return;
}
