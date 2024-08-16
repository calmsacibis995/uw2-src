/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:svc/autotune.c	1.8"
#ident	"$Header: $"

#include <mem/tuneable.h>
#include <svc/autotune.h>
#include <util/types.h>
#include <svc/memory.h>
#include <util/debug.h>

/*
 * Memory size used for auto-tuning (in megabytes).
 */
int tunemem;

/*
 *	int
 *	tune_calc(struct tune_point *curve, int arrsz): 
 *		Calculates the appropriate value for a tunable based on  
 *		memory size and information about the tunable.
 *
 *	Calling/Exit State:
 *		No locks are held on entry or exit.
 *
 *	Description:
 *		Calculates the appropriate value for a tunable based on 
 *		memory size and an array of points of the form 
 *		(memory-size, tunable-value, flag) 
 *		where flag is an instruction on whether to use a step 
 *		function or linear interpolation between a given point and 
 *		the next.  (For the last point, this would be a step 
 *		function or extrapolation.)
 *
 */
int
tune_calc(struct tune_point *curve, int arrsz)
{
	int i, val, num, den;

	ASSERT(arrsz > 0);

	for (i=0; i< arrsz; i++) {
		if (curve[i].tv_mempt > tunemem)
			break;
	}

	if (i==0) {	
		/* smaller than first memory point */
		/* or only one value specified 	   */ 
		return(curve[0].tv_tuneval);
	}

	if ((curve[i-1].tv_typeflag == TV_STEP) ) {
		return(curve[i-1].tv_tuneval);
	} 

	/* curve[i-1].tv_typeflag == TV_LINEAR */

	if (i == arrsz) { 
		/* Beyond last point, extrapolate based on last two points */
		i--;	  
	}

	num = tunemem - curve[i-1].tv_mempt;
	den = curve[i].tv_mempt - curve[i-1].tv_mempt;
	if (den == 0) {
		/*
		 * Two values are specified for the same memory size.
		 */
		val = curve[i-1].tv_tuneval;
	} else {
		val = curve[i-1].tv_tuneval 
		    + (num * (curve[i].tv_tuneval - curve[i-1].tv_tuneval))/den;
	}

	return(val);
}
