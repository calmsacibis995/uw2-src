/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_misc.c	1.3"
/***
 ***	NAME
 ***
 ***	SYNOPSIS
 ***
 ***	DESCRIPTION
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***
 ***	SEE ALSO
 ***
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	AUTHORS
 ***
 ***	HISTORY
 ***
 ***/


PUBLIC

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
export boolean p9000_misc_debug = FALSE;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

/***
 ***	Includes.
 ***/

#include "sidep.h"

/***
 ***	Constants.
 ***/



/***
 ***	Macros.
 ***/

#define BresStep(minor, major) 					\
	{											\
		if ((e += e1) >= 0) 					\
		{ 										\
			e += e3; 							\
			minor; 								\
		} 										\
		major;									\
	}

#define NextDash								\
	{											\
		dashIndex++;							\
		if (dashIndex == numInDashList)			\
			dashIndex = 0;						\
		dashRemaining = pDash[dashIndex];		\
		if ((thisDash = dashRemaining) >= len)	\
		{										\
			dashRemaining -= len;				\
			thisDash = len;						\
		}										\
	}

#define Loop(store) while (thisDash--)				\
	{												\
		store;										\
		BresStep(addrb += signdy, addrb += signdx)	\
	}

/***
 ***	Functions.
 ***/

/*
 * @doc:cfbBresD:
 * 
 * Zerowidth dashed bresenham line draw.  This code has been taken
 * from the MIT R5 ddx/cfb directory.
 *
 * @enddoc
 */


/* $XConsortium: cfbbresd.c,v 1.11 91/12/26 14:32:45 rws Exp $ */

function void
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
{
    register unsigned char *addrb;

    register int e3 = e2-e1;

    int dashIndex;
    int dashOffset;
    int dashRemaining;
    int thisDash;

    dashOffset = *pdashOffset;
    dashIndex = *pdashIndex;
    dashRemaining = pDash[dashIndex] - dashOffset;

    if ((thisDash = dashRemaining) >= len)
    {
		thisDash = len;
		dashRemaining -= len;
    }

    e = e-e1;					/* to make looping easier */

	/* point to first point */

	nlwidth <<= 2;
	addrb = (unsigned char *)(addrl) + (y1 * nlwidth) + x1;
	signdy *= nlwidth;

	if (axis == P9000_MISC_CFB_Y_AXIS)
	{
		int t;

		t = signdx;
		signdx = signdy;
		signdy = t;
	}

	for (;;)
	{ 
		len -= thisDash;
		if (dashIndex & 1) 
		{
			if (isDoubleDash) 
			{
				Loop(*addrb = (unsigned char) bg)
							
			} 
			else 
			{
				Loop(;)
			}
		} 
		else 
		{
			Loop(*addrb = (unsigned char) fg)
		}

		if (!len)
		{
			break;
		}
		
		NextDash
	}

	*pdashIndex = dashIndex;
	*pdashOffset = pDash[dashIndex] - dashRemaining;

}

/*
 * @doc:miStepDash:
 *
 * Walk a dash list around.
 *
 * @enddoc
 */

/* $XConsortium: midash.c,v 5.3 89/09/14 19:14:48 rws Exp $ */

function void
p9000_miStepDash (
    int dist,					/* distance to step */
    int *pDashIndex,			/* current dash */
    long *pDash,					/* dash list */
    int numInDashList,			/* total length of dash list */
    int *pDashOffset)			/* offset into current dash */
{
    int	dashIndex, dashOffset;
    int totallen;
    int	i;
    
    dashIndex = *pDashIndex;
    dashOffset = *pDashOffset;

    if (dist < pDash[dashIndex] - dashOffset)
    {
		*pDashOffset = dashOffset + dist;
		return;
    }

    dist -= pDash[dashIndex] - dashOffset;

    if (++dashIndex == numInDashList)
	{
		dashIndex = 0;
	}
	
    totallen = 0;

    for (i = 0; i < numInDashList; i++)
	{
		totallen += pDash[i];
	}
	
    if (totallen <= dist)
	{
		dist = dist % totallen;
	}
	
    while (dist >= pDash[dashIndex])
    {
		dist -= pDash[dashIndex];

		if (++dashIndex == numInDashList)
		{
			dashIndex = 0;
		}
    }

    *pDashIndex = dashIndex;
    *pDashOffset = dist;

}

