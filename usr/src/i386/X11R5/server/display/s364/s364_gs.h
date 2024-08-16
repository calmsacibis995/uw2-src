/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_gs.h	1.2"
#if (! defined(__S364_GS_INCLUDED__))

#define __S364_GS_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/
#include <sidep.h>
#include "g_state.h"
#include "g_gs.h"
#include "stdenv.h"
#include "s364_state.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/
#define S364_CURRENT_GRAPHICS_STATE_DECLARE()\
	struct s364_graphics_state *graphics_state_p =\
	(struct s364_graphics_state *) generic_current_screen_state_p->\
	screen_current_graphics_state_p


/***
 ***	Types.
 ***/

/*
 *	Forward declarations.
 */


/***
 ***	Variables.
 ***/
/*
 * C and N represent Current and New. 
 * Refer s364 928 programmers guide pg 10-14.
 */
extern const int 
s364_graphics_state_rop_to_alu_function[16] ;

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean s364_graphics_state_debug ;
#endif

/*
 *	Current module state.
 */

extern boolean
s364_invert_stipple_bits( SIbitmapP si_stipple_p, 
	SIbitmapP new_bitmap_p)
;

extern void
s364_graphics_state_download_stipple(struct s364_screen_state *screen_state_p,
	struct s364_graphics_state *graphics_state_p, SIbitmapP si_stipple_p)
;

extern void
s364_graphics_state_download_tile(struct s364_screen_state *screen_state_p,
	  struct s364_graphics_state *graphics_state_p, SIbitmapP si_tile_p)
;

extern void
s364_graphics_state__initialize__(SIScreenRec *si_screen_p,
								  struct s364_options_structure *options_p)
;


#endif
