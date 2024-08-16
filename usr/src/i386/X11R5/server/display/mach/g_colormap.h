/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/g_colormap.h	1.1"

#if (! defined(__G_COLORMAP_INCLUDED__))

#define __G_COLORMAP_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/
#include <sidep.h>
#include "stdenv.h"

/***
 ***	Constants.
 ***/

#define GENERIC_COLORMAP_STAMP\
	(( 'G' << 0 ) + ( 'E' << 1 ) + ( 'N' << 2 ) + ( 'E' << 3 ) +\
	 ( 'R' << 4 ) + ( 'I' << 5 ) + ( 'C' << 6 ) + ( '_' << 7 ) +\
	 ( 'C' << 8 ) + ( 'O' << 9 ) + ( 'L' << 10 ) + ( 'O' << 11 ) +\
	 ( 'R' << 12 ) + ( 'M' << 13 ) + ( 'A' << 14 ) + ( 'P' << 15))

#define GENERIC_VISUAL_STAMP\
	(( 'G' << 0 ) + ( 'E' << 1 ) + ( 'N' << 2 ) + ( 'E' << 3 ) +\
	 ( 'R' << 4 ) + ( 'I' << 5 ) + ( 'C' << 6 ) + ( '_' << 7 ) +\
	 ( 'V' << 8 ) + ( 'I' << 9 ) + ( 'S' << 10 ) + ( 'U' << 11 ) +\
	 ( 'A' << 12 ) + ( 'L' << 13 ))

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

/*
 * The visual structure encapsulates SI's visual with the 
 * appropriate colormap programming methods.
 */
struct generic_visual
{
	/*
	 * SI's visual.
	 */
	SIVisual *si_visual_p;

	int dac_flags;				/* info to dac */
	
	/*
	 * Methods.
	 */
	void (*get_color_method_p)(const struct generic_visual *this_p, 
							   const int color_index, unsigned short
							  *rgb_values_p);
	void (*set_color_method_p)(const struct generic_visual *this_p,
							   const int color_index, unsigned short
							  *rgb_values_p);

#if (defined(__DEBUG__))
	int stamp;
#endif
};

/*
 * The colormap structure is an SICmap structure with
 * an additional stamp field.
 */
struct generic_colormap
{
	/*
	 * SI's idea of the colormap
	 */
	SICmap si_colormap;
	/*
	 * Visual associated with this colormap
	 */
	struct generic_visual *visual_p;
	/*
	 * Raw bits from the DAC.
	 */
	unsigned short *rgb_values_p;

#if (defined(__DEBUG__))
	int stamp;
#endif
};

/*
 *	Forward declarations.
 */


/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

/*
 *	Current module state.
 */


#endif
