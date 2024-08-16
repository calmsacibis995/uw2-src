/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_gs.c	1.2"
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
export enum debug_level  p9000_graphics_state_debug = DEBUG_LEVEL_NONE;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

extern void p9000__gs_change__();

/***
 ***	Includes.
 ***/

#include "sidep.h"
#include "p9k_opt.h"
#include "p9k_state.h"
#include "p9k_tile.h"
#include "p9k_stpl.h"

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
 * p9000_graphics_state_download_state
 *
 * Downloading of a graphics state from SI.  After calling the generic
 * layers `download_state' method, we save any downloaded tile and
 * stipples and perform whatever transformations are needed for these.
 */

STATIC SIBool
p9000_graphics_state_download_state(SIint32 state_index,
   SIint32 state_flag,
   SIGStateP state_p)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_graphics_state *const graphics_state_p =
		(struct p9000_graphics_state*)
		(screen_state_p->generic_state.screen_current_graphics_state_p);

	struct p9000_graphics_state *new_graphics_state_p;

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_graphics_state, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
"(p9000_graphics_state_download_state)\n"
"{\n"
"\tstate_index = %ld\n"
"\tstate_flag = 0x%lx\n"
"\tstate_p = %p\n"
"}\n",
					   state_index,
					   state_flag,
					   (void *) state_p);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
	    (struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));

	ASSERT(state_index >= 0 && state_index <
		   screen_state_p->generic_state.screen_number_of_graphics_states);

	/*
	 * Call the generic libraries download state.
	 */

	if ((*graphics_state_p->generic_si_functions.si_download_state)
		(state_index, state_flag, state_p) != SI_SUCCEED)
	{
#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_graphics_state,SCAFFOLDING))
		{
			(void) fprintf(debug_stream_p,
"(p9000_download_state)\n"
"{\n"
"\tgeneric download state failed.\n"
"}\n"
						   );
		}
#endif

		return (SI_FAIL);
	}

	/*
	 * retrieve the new graphics state.
	 */

	new_graphics_state_p = (struct p9000_graphics_state *)
		screen_state_p->generic_state.screen_graphics_state_list_pp
			[state_index];

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		   (struct generic_graphics_state *) new_graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 new_graphics_state_p));

	if (state_flag & SetSGstipple)
	{

#if defined(__DEBUG__)
		SIbitmapP si_stipple_p = state_p->SGstipple;
		
		if (DEBUG_LEVEL_MATCH(p9000_graphics_state,INTERNAL))
		{
			(void)fprintf(debug_stream_p,
						  "(p9000_graphics_state_download_state){\n"  
						  "\t# New stipple\n"
						  "\twidth = %d\n"
						  "\theight = %d\n"
						  "\torg_x = %d\n"
						  "\torg_y = %d\n"
						  "}\n",
						  si_stipple_p->Bwidth,
						  si_stipple_p->Bheight,
						  si_stipple_p->BorgX,
						  si_stipple_p->BorgY);
		}
#endif

		/*
	 	 * Mark that a stipple download is pending
		 */

		new_graphics_state_p->is_stipple_downloaded = FALSE;

	}

	if (state_flag & SetSGtile)
	{

#if defined(__DEBUG__)
		SIbitmapP si_tile_p = state_p->SGtile;
	 
		if (DEBUG_LEVEL_MATCH(p9000_graphics_state,INTERNAL))
		{
			(void)fprintf(debug_stream_p,
						  "(p9000_graphics_state_download_state){\n"  
						  "\t# New stipple\n"
						  "\twidth = %d\n"
						  "\theight = %d\n"
						  "\torg_x = %d\n"
						  "\torg_y = %d\n"
						  "}\n",
						  si_tile_p->Bwidth,
						  si_tile_p->Bheight,
						  si_tile_p->BorgX,
						  si_tile_p->BorgY);
		}
#endif
		
		/*
		 * Mark that a tile download is pending.
		 */

		new_graphics_state_p->is_tile_downloaded = FALSE;

	}

	return (SI_SUCCEED);

}

/*
 * p9000_graphics_state_select_state
 *
 * Make the current graphics state that specified by `index'.  If the
 * new graphics state is not the same as the current graphics, or if
 * something in the graphics state has changed since the last time it
 * was selected, call the module synchronization function.
 */

STATIC SIBool
p9000_graphics_state_select_state(SIint32 state_index)
{
	GENERIC_CURRENT_SCREEN_STATE_DECLARE();

	struct p9000_graphics_state *graphics_state_p;
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_graphics_state, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_graphics_state_select_state)\n"
					   "{\n"
					   "\tstate_index = %ld\n"
					   "}\n",
					   state_index);
	}
#endif

	ASSERT(state_index >= 0 && state_index <
		   screen_state_p->screen_number_of_graphics_states);

	/*
	 * check the index passed down for validity.
	 */

	if (state_index < 0 || state_index >=
		screen_state_p->screen_number_of_graphics_states)
	{
		return (SI_FAIL);
	}

	/*
	 * retrieve the graphics state requested.
	 */

	graphics_state_p = (struct p9000_graphics_state *)
		screen_state_p->
		screen_graphics_state_list_pp[state_index];

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *)
		graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
		graphics_state_p));

	/*
	 * Check if anything was downloaded in the this state after the
	 * last call to select state for this graphics state.
	 */

	if (graphics_state_p->generic_state.si_state_flags == 0 &&
		(graphics_state_p ==
		 (struct p9000_graphics_state *) 
		 screen_state_p->screen_current_graphics_state_p))
	{
		return (SI_SUCCEED);
	}
	
	/*
	 * make the requested graphics state `current'.
	 */

	screen_state_p->screen_current_graphics_state_p =
		(struct generic_graphics_state *) graphics_state_p;

	/*
	 * Now let each module handle its graphics state changes.
	 */

	p9000__gs_change__();

	/*
	 * Mark this module as having been downloaded.
	 */

	graphics_state_p->generic_state.si_state_flags = 0;

	return (SI_SUCCEED);
}

/*
 * p9000_graphics_state_get_state
 *
 * Retrieve a graphics state from the SDD's internal data structure.
 */

STATIC SIBool
p9000_graphics_state_get_state(SIint32 state_index, 
							   SIint32 state_flag,
							   SIGStateP state_p)
{
	GENERIC_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_graphics_state *const graphics_state_p =
		(struct p9000_graphics_state*)
		(screen_state_p->screen_current_graphics_state_p);
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_graphics_state, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_graphics_state_get_state) {\n"
					   "\tstate_index = %ld\n"
					   "\tstate_flag = 0x%lx\n"
					   "\tstate_p = %p\n"
					   "}\n",
					   state_index,
					   state_flag,
					   (void *) state_p);
	}
#endif	

	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));
	
	if (state_index < 0 || state_index >=
		screen_state_p->screen_number_of_graphics_states)
	{
		return (SI_FAIL);
	}
	
	/*
	 * Call the generic libraries get state.
	 */

	if ((*graphics_state_p->generic_si_functions.si_get_state)
		(state_index, state_flag, state_p) != SI_SUCCEED)
	{
		return (SI_FAIL);
	}	
	
	/*
	 * We don't do anything specific for get_state in this module.
	 */

	return (SI_SUCCEED);
}

/*
 * Initialize this module.
 */

function void
p9000_graphics_state__initialize__(SIScreenRec *si_screen_p,
								  struct p9000_options_structure *options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	GENERIC_CURRENT_SCREEN_STATE_DECLARE();


	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 screen_state_p));

	/*
	 * Confirm that the offscreen allocation requests are a power of
	 * two.  Use the default other wise.
	 */

	/*
	 * Check if we need to fill in the graphics state list.
	 */

	if (screen_state_p->screen_graphics_state_list_pp ==
		NULL)
	{
		int count;

#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_graphics_state, INTERNAL))
		{
			(void) fprintf(debug_stream_p,
						   "(p9000_graphics_state__initialize__) "
						   "allocating space for graphics states.\n");
		}
#endif

		si_screen_p->flagsPtr->SIstatecnt =
			screen_state_p->screen_number_of_graphics_states =
			(options_p->number_of_graphics_states != 
			 P9000_OPTIONS_NUMBER_OF_GRAPHICS_STATES_DEFAULT) ?
				 options_p->number_of_graphics_states :
				 P9000_DEFAULT_NUMBER_OF_GRAPHICS_STATES;

		screen_state_p->screen_graphics_state_list_pp =
			allocate_and_clear_memory(si_screen_p->flagsPtr->SIstatecnt *
									  sizeof(struct p9000_graphics_state *));

		for (count = 0; count < si_screen_p->flagsPtr->SIstatecnt;
			 count ++)
		{
			struct p9000_graphics_state *graphics_state_p;

			graphics_state_p =
					allocate_and_clear_memory(
					sizeof(struct p9000_graphics_state));
			screen_state_p->
				screen_graphics_state_list_pp[count] =
				(struct generic_graphics_state *) graphics_state_p;

#if (defined(__DEBUG__))
			STAMP_OBJECT(GENERIC_GRAPHICS_STATE,
						 (struct generic_graphics_state *)
						 graphics_state_p);
			STAMP_OBJECT(P9000_GRAPHICS_STATE,
						 graphics_state_p);
#endif

#ifdef DELETE

			/*
			 * Force the first use of a GS's to reprogram the chipset.
			 */

			graphics_state_p->generic_state.si_state_flags = ~0U;
#endif

		}

		/*
		 * Initialize Graphics state index `0' to a known value.
		 */

		screen_state_p->screen_current_graphics_state_p =
			screen_state_p->screen_graphics_state_list_pp[0];

	}

	/*
	 * Fill in the graphics state manipulation functions.
	 */

	functions_p->si_download_state = p9000_graphics_state_download_state;
	functions_p->si_select_state = p9000_graphics_state_select_state;
	functions_p->si_get_state = p9000_graphics_state_get_state;

}
