/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_gs.h	1.2"
#if (! defined(__P9K_GS_INCLUDED__))

#define __P9K_GS_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include "stdenv.h"
#include "g_gs.h"

/***
 ***	Constants.
 ***/

#define	P9000_GRAPHICS_STATE_STAMP\
	(('P' << 0) + ('9' << 1) + ('0' << 2) + ('0' << 3) + ('0' << 4) +\
	 ('_' << 5) + ('G' << 6) + ('R' << 7) + ('A' << 8) + ('P' << 9) +\
	 ('H' << 10) + ('I' << 11) + ('C' << 12) + ('S' << 13) + ('_' << 14) +\
	 ('S' << 15) + ('T' << 16) + ('A' << 17) + ('T' << 18) + ('E' << 19) + 0 )


/***
 ***	Macros.
 ***/

/*
 * Define the current graphics state.
 */

#define P9000_CURRENT_GRAPHICS_STATE_DECLARE()								\
	struct p9000_graphics_state *const graphics_state_p =					\
		(struct p9000_graphics_state*)										\
		(screen_state_p->generic_state.screen_current_graphics_state_p)

/***
 ***	Types.
 ***/

struct p9000_graphics_state
{
	/*
	 * Inherit from the generic graphics state.
	 */

	struct generic_graphics_state generic_state;

	SIFunctions generic_si_functions;

	/*
	 * Flags to indicate if a tile/stipple download is pending
	 * This is required because we avoid the actual downloading
	 * of the bitmap till drawing time.
	 * (Downloading of tile/stipple carries  lot of overheads).
	 */

	boolean is_tile_downloaded;
	boolean is_stipple_downloaded;
	boolean is_line_dash_downloaded;

	/*
	 * Pointers to tile and stipple states
	 */


	void *tile_state_p;
	
	void *stipple_state_p;

	void *line_state_p;
	
	void *solid_state_p;
	
#if defined(__DEBUG__)
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

#if (defined(__DEBUG__))
extern enum debug_level  p9000_graphics_state_debug ;
#endif

/*
 *	Current module state.
 */

extern void
p9000_graphics_state__initialize__(SIScreenRec *si_screen_p,
								  struct p9000_options_structure *options_p)
;


#endif
