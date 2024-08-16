/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_misc.h	1.3"

#if (! defined(__P9K_MISC_INCLUDED__))

#define __P9K_MISC_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include "stdenv.h"

/***
 ***	Constants.
 ***/

/*
 * Bresenham axes.
 */

#define P9000_MISC_CFB_X_AXIS 	1
#define P9000_MISC_CFB_Y_AXIS	2


/***
 ***	Macros.
 ***/

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/*
 *	Forward declarations.
 */


/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean p9000_misc_debug ;
#endif

/*
 *	Current module state.
 */

extern void
p9000_cfbBresD(
	int *pdashIndex,			/* current dash */
	long*pDash,					/* dash list */
	int numInDashList,			/* total length of dash list */
	int *pdashOffset,			/* offset into current dash */
	int isDoubleDash,
	int *addrl,					/* pointer to base of bitmap */
	int nlwidth,				/* width in longwords of bitmap */
	int signdx, 
	int signdy,					/* signs of directions */
	int axis,					/* major axis (Y_AXIS or X_AXIS) */
	int x1, 
	int y1,						/* initial point */
	register int e,				/* error accumulator */
	register int e1,			/* bresenham increments */
	int e2,
	int len,					/* length of line */
	int fg,						/* foreground color */
	int bg)						/* background color */
;

extern void
p9000_miStepDash (
    int dist,					/* distance to step */
    int *pDashIndex,			/* current dash */
    long *pDash,					/* dash list */
    int numInDashList,			/* total length of dash list */
    int *pDashOffset)			/* offset into current dash */
;


#endif
