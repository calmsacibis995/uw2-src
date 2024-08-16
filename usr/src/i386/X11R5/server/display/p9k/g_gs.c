/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/g_gs.c	1.1"
/***
 ***	NAME
 ***
 ***		g_gs.c : generic graphics state for the MACH/S3 display
 *** 	libraries.
 *** 	
 ***	SYNOPSIS
 ***
 *** 		#include "g_gs.h"
 *** 	
 ***	DESCRIPTION
 ***
 *** 	This module implements a minimal handler for graphics states
 *** 	downloaded by SI.  The handlers in this module save and
 *** 	restore the relevant fields passed down by SI in the download
 *** 	state request.
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
 ***		All structures must be initially allocated zero-filled.
 ***
 ***	BUGS
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
#include "stdenv.h"

/***
 ***	Constants.
 ***/

#define GENERIC_GRAPHICS_STATE_STAMP\
	(('G' << 0) + ('E' << 1) + ('N' << 2) + ('E' << 3) +\
	 ('R' << 4) + ('I' << 5) + ('C' << 6) + ('G' << 7) +\
	 ('R' << 8) + ('A' << 9) + ('P' << 10) + ('H' << 11) +\
	 ('I' << 12) + ('C' << 13) + ('S' << 14) + ('S' << 15) +\
	 ('T' << 16) + ('A' << 17) + ('T' << 18) + ('E' << 19))

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

/*
 * @doc:generic_graphics_state:struct
 * This structure keeps a copy of all the relevant
 * GS information.
 * @enddoc
 */

struct generic_graphics_state
{

	/*
	 * The stamp field is kept up front to allow sub-classing 
	 * this structure.
	 */

#if (defined(__DEBUG__))
	int stamp;
#endif

	/*
	 * flag used to keep track of changes to the graphics state.
	 */

	SIint32  si_state_flags;

	/*
	 * A copy of the graphics state downloaded from SI.
	 */

	SIGState si_graphics_state;

};

/***
 ***	Forward declarations.
 ***/

/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export enum debug_level generic_graphics_state_debug = FALSE;
#endif

/*
 *	Current module state.
 */

PRIVATE
extern SIBool generic_no_operation_succeed(void);

/***
 ***	Private declarations.
 ***/

/***
 ***	Includes.
 ***/
#include "generic.h"
#include "g_state.h"

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
 * @doc:generic_graphics_state_download_state:
 *
 * Copy out the SI servers graphics state requested into the SDD's
 * local storage.  Since successive calls to download state are
 * possible before a call to `select_state', we keep track of the
 * changes to the structure using the `flags' member.
 *
 * @enddoc
 */

STATIC SIBool
generic_graphics_state_download_state(
	SIint32 state_index, 	/* The index of the state being downloaded */
	SIint32 state_flags,	/* List of state members being modified */
	SIGStateP state_p)		/* pointer from SI */
{
	GENERIC_CURRENT_SCREEN_STATE_DECLARE();

	struct generic_graphics_state *graphics_state_p;

#if (defined(__DEBUG__))

	DEBUG_FUNCTION_ENTRY(generic_graphics_state, download_state)

	if (DEBUG_LEVEL_MATCH(generic_graphics_state, INTERNAL))
	{
		(void) fprintf(debug_stream_p,
"\tstate_index = %ld\n"
"\tstate_flags = 0x%lx\n"
"\tstate_p = %p\n",
					   state_index,
					   state_flags,
					   (void *)state_p);
		
	}
#endif
	
	/*
	 * Check the pointers passed in.
	 */

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 screen_state_p));

	ASSERT(state_index >= 0 && state_index <
		   screen_state_p->screen_number_of_graphics_states);

	/*
	 * Check the index for validity.
	 */

	if (state_index < 0 || state_index >=
		screen_state_p->screen_number_of_graphics_states)
	{
		return (SI_FAIL);
	}
	
	graphics_state_p =
		screen_state_p->
		screen_graphics_state_list_pp[state_index];
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 graphics_state_p));
	/*
	 * Save the information from SI into the local structure.
	 */

#define GENERIC_GS_SAVE_STATE(ID)\
	if (state_flags & SetSG##ID)\
	{\
		graphics_state_p->si_graphics_state.SG##ID =\
			state_p->SG##ID;\
	}

	GENERIC_GS_SAVE_STATE(pmask);
	GENERIC_GS_SAVE_STATE(mode);
	GENERIC_GS_SAVE_STATE(fillmode);
	GENERIC_GS_SAVE_STATE(stplmode);
	GENERIC_GS_SAVE_STATE(fillrule);
	GENERIC_GS_SAVE_STATE(arcmode);
	GENERIC_GS_SAVE_STATE(linestyle);
	GENERIC_GS_SAVE_STATE(fg);
	GENERIC_GS_SAVE_STATE(bg);
	GENERIC_GS_SAVE_STATE(cmapidx);
	GENERIC_GS_SAVE_STATE(visualidx);
	GENERIC_GS_SAVE_STATE(tile);
	GENERIC_GS_SAVE_STATE(stipple);
#undef GENERIC_GS_SAVE_STATE

	/*
	 * In the case of dashed lines, we need to save the dash pattern.
	 */

	if (state_flags & SetSGline)
	{
		int		i;

		graphics_state_p->si_graphics_state.SGlineCNT =
			state_p->SGlineCNT;

		/*
		 * Free if memory already allocated.
		 * @doc:generic_graphics_state_save_state:CAVEAT
		 * <<<Each graphics state needs to be allocated initially
		 * null filled.>>>
		 * @enddoc
		 */


		if( graphics_state_p->si_graphics_state.SGline)
		{
			free_memory(graphics_state_p->si_graphics_state.SGline);
		}

		/*
		 * Allocate space for downloading the on-off pairs.
		 */

		graphics_state_p->si_graphics_state.SGline = 
			allocate_memory(state_p->SGlineCNT * sizeof(SIint32));

		/*
		 * Copy the on-off pairs.
		 */

		for (i = 0; i < state_p->SGlineCNT; i++)
		{
			graphics_state_p->si_graphics_state.SGline[i] = 
				state_p->SGline[i];
		}

	}

	/*
	 * Save the clip list if asked to do so.
	 */

	if (state_flags & SetSGcliplist)
	{
		graphics_state_p->si_graphics_state.SGclipCNT = state_p->SGclipCNT;
		graphics_state_p->si_graphics_state.SGclipextent =
			state_p->SGclipextent;
		graphics_state_p->si_graphics_state.SGcliplist = state_p->SGcliplist;
	}
	
	/*
	 * Keep track of what all has changed.
	 */

	graphics_state_p->si_state_flags |= state_flags;
	
#if (defined(__DEBUG__))
	DEBUG_FUNCTION_EXIT(generic_graphics_state, download_state)
#endif

	return (SI_SUCCEED);
	
}

/*
 * @doc:generic_graphics_state_get_state:
 *
 * Retrieve the current graphics state information from the SDD's local
 * storage area.
 *
 * @enddoc
 */

STATIC SIBool
generic_graphics_state_get_state(
	SIint32 state_index, 	/* index of the state to retrieve */
	SIint32 state_flags,	/* list of members to retrieve */
	SIGStateP state_p)		/* SI data structure to fill */
{
	GENERIC_CURRENT_SCREEN_STATE_DECLARE();
	
	struct generic_graphics_state *graphics_state_p;

#if (defined(__DEBUG__))
	DEBUG_FUNCTION_ENTRY(generic_graphics_state, get_state)

	if (DEBUG_LEVEL_MATCH(generic_graphics_state, INTERNAL))
	{
		(void) fprintf(debug_stream_p,
"\tstate_index = %ld\n"
"\tstate_flags = 0x%lx\n"
"\tstate_p = %p\n",
					   state_index,
					   state_flags,
					   (void *)state_p);
		
	}
#endif
	
	/*
	 * check the pointers.
	 */

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 screen_state_p));

	ASSERT(state_index >= 0 && state_index <
		   screen_state_p->screen_number_of_graphics_states);

	/*
	 * check the index for sanity.
	 */

	if (state_index < 0 || state_index >=
		screen_state_p->screen_number_of_graphics_states)
	{
		return (SI_FAIL);
	}
	
	/*
	 * Retrieve the current graphics state.
	 */

	graphics_state_p =
		screen_state_p->
		screen_graphics_state_list_pp[state_index];
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 graphics_state_p));

	/*
	 * Save the information from SI.
	 */

#define GENERIC_GS_GET_STATE(ID)\
	if (state_flags & GetSG##ID)\
	{\
		 state_p->SG##ID = graphics_state_p->si_graphics_state.SG##ID;\
	}

	GENERIC_GS_GET_STATE(pmask);
	GENERIC_GS_GET_STATE(mode);
	GENERIC_GS_GET_STATE(fillmode);
	GENERIC_GS_GET_STATE(stplmode);
	GENERIC_GS_GET_STATE(arcmode);
	GENERIC_GS_GET_STATE(linestyle);
	GENERIC_GS_GET_STATE(fg);
	GENERIC_GS_GET_STATE(bg);
	GENERIC_GS_GET_STATE(cmapidx);
	GENERIC_GS_GET_STATE(visualidx);

#undef GENERIC_GS_GET_STATE

#if (defined(__DEBUG__))
	DEBUG_FUNCTION_EXIT(generic_graphics_state, get_state)
#endif

	return (SI_SUCCEED);
	
}

/*
 * Graphics state initialization.
 */

function void
generic_graphics_state__initialize__(SIScreenRec *si_screen_p)
{
	GENERIC_CURRENT_SCREEN_STATE_DECLARE();
	

	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	
	/*
	 * Check if a more specific layer has taken over the graphics
	 * state handling.  If not, set the state count to zero.
	 */

	if (screen_state_p->screen_graphics_state_list_pp ==
		NULL)
	{
		flags_p->SIstatecnt = 0; 
	}
	
	flags_p->SItilewidth  = DEFAULT_GENERIC_BEST_TILE_WIDTH;
	flags_p->SItileheight  = DEFAULT_GENERIC_BEST_TILE_HEIGHT;

	flags_p->SIstipplewidth  = DEFAULT_GENERIC_BEST_STIPPLE_WIDTH;
	flags_p->SIstippleheight  = DEFAULT_GENERIC_BEST_STIPPLE_HEIGHT;

	functions_p->si_download_state = generic_graphics_state_download_state;
	functions_p->si_get_state = generic_graphics_state_get_state;
	functions_p->si_select_state = 
		(SIBool (*)(SIint32)) generic_no_operation_succeed;

}
