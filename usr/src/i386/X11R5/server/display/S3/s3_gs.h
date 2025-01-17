/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)S3:S3/s3_gs.h	1.3"

#if (! defined(__S3_GS_INCLUDED__))

#define __S3_GS_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/
#include <sidep.h>
#include "g_state.h"
#include "g_gs.h"
#include "g_omm.h"
#include "s3_state.h"
#include "s3_options.h"
#include "stdenv.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

#define S3_CURRENT_GRAPHICS_STATE_DECLARE()\
	struct s3_graphics_state *graphics_state_p =\
	(struct s3_graphics_state *) generic_current_screen_state_p->\
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
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean s3_graphics_state_debug ;
#endif

/*
 *	Current module state.
 */

extern void
s3_graphics_state_download_tile(struct s3_screen_state *screen_state_p,
	  struct s3_graphics_state *graphics_state_p, SIbitmapP si_tile_p)
;

extern void
s3_graphics_state_download_stipple(struct s3_screen_state *screen_state_p,
	 struct s3_graphics_state *graphics_state_p, SIbitmapP si_stipple_p)
;

extern void
s3_graphics_state__initialize__(SIScreenRec *si_screen_p,
								  struct s3_options_structure *options_p)
;


#endif
