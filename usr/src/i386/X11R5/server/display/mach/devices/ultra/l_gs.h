/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/devices/ultra/l_gs.h	1.1"

#if (! defined(__L_GS_INCLUDED__))

#define __L_GS_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "l_opt.h"
#include "stdenv.h"
#include "ultra.h"

/***
 ***	Constants.
 ***/

#if (defined(__DEBUG__))
#define	LFB_GRAPHICS_STATE_STAMP\
	(('L' << 0) + ('F' << 1) + ('B' << 2) + ('_' << 3) + ('G' << 4) +\
	 ('S' << 5) + ('_' << 6) + ('S' << 7) + ('T' << 8) + ('A' << 9) +\
	 ('T' << 10) + ('E' << 11))
#endif /* __DEBUG__ */

/***
 ***	Macros.
 ***/

#define LFB_CURRENT_GRAPHICS_STATE_DECLARE()\
	struct lfb_graphics_state *graphics_state_p =\
		screen_state_p->lfb_graphics_state_p

/***
 ***	Types.
 ***/

/*
 * The graphics state.
 */

struct	lfb_graphics_state
{
	/*
	 * The chipset layers graphics state.
	 */

	struct mach_graphics_state mach_state;

	/*
	 * stipple array for 8 bit mode operations
	 */

	unsigned long	*stipple_array_p_8;

	/*
	 * pointer to chipset level gs_select function
	 */

	SIBool (*mach_graphics_state_select_state_function_p)
		(SIint32 state_index); 

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
extern boolean lfb_gs_debug ;
#endif

extern void
lfb_graphics_state__pre_initialize__(SIScreenRec *si_screen_p,
									 struct lfb_options_structure *options_p)
;

extern void
lfb_graphics__post_initialize__(SIScreenRec *si_screen_p,
						  struct lfb_options_structure * options_p)
;


#endif
