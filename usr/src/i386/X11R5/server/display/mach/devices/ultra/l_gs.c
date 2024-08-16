/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/devices/ultra/l_gs.c	1.2"

/***
 ***	NAME
 ***
 ***		l_gs.c : graphics state handling for the ultra+lfb
 ***	devices layer.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "l_gs.h"
 ***
 ***	DESCRIPTION
 ***
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
export boolean lfb_gs_debug = FALSE;
#endif

PRIVATE

/***
 ***	Private declarations.
 ***/

extern void lfb__gs_change__();

/***
 ***	Includes.
 ***/

#include "l_globals.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

/*
 * lfb_graphics_state_select_state
 * 
 * Entry point for the select graphics state SI function.
 */

STATIC SIBool
lfb_graphics_state_select_state(SIint32 state_index)
{
	LFB_CURRENT_SCREEN_STATE_DECLARE();
	
	struct lfb_graphics_state *lfb_graphics_state_p =
		(struct lfb_graphics_state *) generic_current_screen_state_p->
		screen_graphics_state_list_pp[state_index]; 

	ASSERT(IS_OBJECT_STAMPED(LFB_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(LFB_GRAPHICS_STATE, lfb_graphics_state_p));

	screen_state_p->lfb_graphics_state_p = lfb_graphics_state_p;

	ASSERT(lfb_graphics_state_p->mach_graphics_state_select_state_function_p);

#if defined(__DEBUG__)
	if (lfb_gs_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_gs_select_state)\n"
"{\n"
"\tstate_index = %ld\n"
"}\n",
					  state_index);
	}
#endif

	if ((*lfb_graphics_state_p->mach_graphics_state_select_state_function_p)
		(state_index) == SI_FAIL)
	{
		return SI_FAIL;
	}

	/*
	 * Inform interested modules of the graphics state change.
	 */

	lfb__gs_change__();

	return (SI_SUCCEED);

}

/*
 * lfb_graphics_state__pre_initialize__
 *
 * This is called before the chipset layer initialize functions are called
 * therefore no assumption should be made about the chipset level data
 * structures. 
 */

function void
lfb_graphics_state__pre_initialize__(SIScreenRec *si_screen_p,
									 struct lfb_options_structure *options_p)
{
	int count;
	
#if (__DEBUG__)
	if(lfb_gs_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_gs__pre_initialize__)\n"
"{\n"
"\tsi_screen_p = %p\n"
"\toptions_p  %p\n"
"}\n",
				(void*) si_screen_p,
				(void*) options_p);
	}
#endif

	si_screen_p->flagsPtr->SIstatecnt =
		generic_current_screen_state_p->screen_number_of_graphics_states =
		((options_p->number_of_graphics_states > 0) ?
		 (options_p->number_of_graphics_states) :
		 DEFAULT_LFB_NUMBER_OF_GRAPHICS_STATES);

	generic_current_screen_state_p->screen_graphics_state_list_pp =
		allocate_and_clear_memory(si_screen_p->flagsPtr->SIstatecnt *
								  sizeof(struct lfb_graphics_state *));

	for (count = 0; 
		 count < si_screen_p->flagsPtr->SIstatecnt;
		 count ++)
	{
		struct lfb_graphics_state *graphics_state_p;

		graphics_state_p =
			allocate_and_clear_memory(sizeof(struct lfb_graphics_state));

		generic_current_screen_state_p->
			screen_graphics_state_list_pp[count] =
				(struct generic_graphics_state *) graphics_state_p;

#if (defined(__DEBUG__))
		STAMP_OBJECT(GENERIC_GRAPHICS_STATE,
					 (struct generic_graphics_state *)
					 graphics_state_p);
		STAMP_OBJECT(MACH_GRAPHICS_STATE,
					 (struct mach_graphics_state*)graphics_state_p);
		STAMP_OBJECT(LFB_GRAPHICS_STATE,
					 graphics_state_p);
#endif
		/*
		 * Force initialize GS's to reprogram the chipset.
		 */
		((struct mach_graphics_state *)
		 graphics_state_p)->generic_state.si_state_flags = ~0U;

	}

	/*
	 * Initialize Graphics state index `0' to a known value.
	 */
	generic_current_screen_state_p->screen_current_graphics_state_p =
		generic_current_screen_state_p->screen_graphics_state_list_pp[0];

}

/*
 * lfb_graphics_state__post_initialize__
 * 
 * initialization code that is run *after* the chipset layer
 * initializes itself.
 */

function void
lfb_graphics__post_initialize__(SIScreenRec *si_screen_p,
						  struct lfb_options_structure * options_p)
{
	LFB_CURRENT_SCREEN_STATE_DECLARE();
	
	int count;

#if (defined(__DEBUG__))
	if (lfb_gs_debug)
	{
		(void) fprintf(debug_stream_p,
					   "(lfb_graphics__post_initialize__) {}\n");
	}
#endif

	for (count = 0; count < si_screen_p->flagsPtr->SIstatecnt;
		 count ++)
	{
		struct lfb_graphics_state *graphics_state_p;

		/*
		 * We are storing the chipset level select_gs function
		 * pointer in all the graphics states, this is not
		 * necessary  because there is going to be only one
		 * common gs_select function for all states. But this
		 * is a clean  way of doing it.
		 */
		graphics_state_p = (struct  lfb_graphics_state *)
			generic_current_screen_state_p->
				screen_graphics_state_list_pp[count]; 

		graphics_state_p->mach_graphics_state_select_state_function_p = 
			si_screen_p->funcsPtr->si_select_state;

	}

	/*
	 * Hook the pointer to our graphics state handler into the SI
	 * structure.
	 */

	si_screen_p->funcsPtr->si_select_state =
		lfb_graphics_state_select_state;

	/*
	 * place the lfb_graphics_state pointer in lfb_state structure
	 */

	screen_state_p->lfb_graphics_state_p = 
		(struct lfb_graphics_state *) generic_current_screen_state_p->
		screen_graphics_state_list_pp[0]; 
	 
}
