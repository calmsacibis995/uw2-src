/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/g_state.h	1.1"
#if (! defined(__G_STATE_INCLUDED__))

#define __G_STATE_INCLUDED__



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

#define GENERIC_SCREEN_STATE_STAMP\
	(('G' << 0) + ('E' << 1) + ('N' << 2) + ('E' << 3) + ('R' << 4) +\
	 ('I' << 5) + ('C' << 6) + ('S' << 7) + ('C' << 8) + ('R' << 9) +\
	 ('E' << 10) + ('E' << 11) + ('N' << 12) + ('S' << 13) + ('T' << 14) +\
	 ('A' << 15) + ('T' << 16) + ('E' << 17))

/***
 ***	Macros.
 ***/

#define GENERIC_CURRENT_SCREEN_STATE_DECLARE()		\
	struct generic_screen_state *screen_state_p =	\
		generic_screen_current_state_p

/***
 ***	Types.
 ***/

/*
 * @doc:generic_clip_kind:
 * 
 * As the SDD services drawing requests, the state of the chipsets clipping
 * rectangle may deviate from that specified in the current screen state.
 * This enumeration is used to name the different states the clipping can
 * be in.
 *
 * @enddoc
 */

#define DEFINE_CLIP_TYPES()\
	DEFINE_CLIP(NULL),\
	DEFINE_CLIP(TO_GRAPHICS_STATE),\
	DEFINE_CLIP(TO_VIRTUAL_SCREEN),\
	DEFINE_CLIP(TO_VIDEO_MEMORY),\
	DEFINE_CLIP(COUNT)

enum generic_clip_kind
{
#define DEFINE_CLIP(TYPE)\
	GENERIC_CLIP_##TYPE
DEFINE_CLIP_TYPES()
#undef DEFINE_CLIP
};

/*
 * Screen state : information for the current screen
 */

struct generic_screen_state
{
	/*
	 * The stamp field is kept up front in order to allow subclassing of 
	 * this object by other modules.
	 */

#if (defined(__DEBUG__))
	int			stamp;
#endif

	/*
	 * The version number of this SDD : for handling backward
	 * compatibility modes.
	 */

	int			screen_sdd_version_number;
	int 		screen_server_version_number;
	
	/*
	 * The virtual terminal file descriptor, associated with 
	 * this screen.
	 */

	SIint32		screen_virtual_terminal_file_descriptor;
	
	/*
	 * Some commonly used parameters.
	 */

	int     	screen_virtual_width;	/* virtual screen width */
	int     	screen_virtual_height;	/* virtual screen depth */
	int		    screen_depth;			/* screen depth */
	int     	screen_depth_shift;		/* shift count for dividing by screen
									 	 * depth. */
	
	int 		screen_displayed_width;	/* displayed screen width */
	int			screen_displayed_height; /* displayed screen height */
	int     	screen_physical_width; 	/* physical memory pitch */
	int     	screen_physical_height; /* physical memory height */

	/*
	 * Current clip kind.
	 */

	enum generic_clip_kind 
		        screen_current_clip;

	/*
	 * current GS derived clip coordinates : inclusive coords.
	 */

	int         screen_clip_left;
	int         screen_clip_right;
	int         screen_clip_top;
	int         screen_clip_bottom;
	
	/*
	 * SI's information saved here.
	 */

	SIFlagsP	screen_flags_p;
	SIConfigP	screen_config_p;
	SIFunctionsP	screen_functions_p;

	/*
	 * List of visuals supported.
	 */

	struct generic_visual    *screen_visuals_list_p;
	int			screen_number_of_visuals;

	/*
	 * List of list of colormaps supported (indexed by visual index).
	 */

	struct generic_colormap		**screen_colormaps_pp;
	
	/*
	 * Graphics States.
	 */

	struct generic_graphics_state		*screen_current_graphics_state_p;	
	struct generic_graphics_state		**screen_graphics_state_list_pp;
	int			screen_number_of_graphics_states;

	/*
	 * Saved frame buffer for VT switch times.
	 */

	void        *screen_contents_p;
	
	/*
	 * Memory manager state.
	 */

	void 		*screen_memory_manager_state_p;

	/*
	 * Dac state.
	 */

	void 		*screen_dac_state_p;

	/*
	 * Register state associated with this screen.
	 */

	void 		*screen_register_state_p;
	
};


/*
 *	Forward declarations.
 */


/***
 ***	Variables.
 ***/

/*
 * @doc:generic_screen_current_state_p:pointer
 *
 * Pointer to the current screen state.  Its the responsibility of the
 * lower layers to maintain this correctly pointing to the current screen.
 * Ideally this should be the only global in the whole screen.
 *
 * @enddoc
 */

extern void *generic_screen_current_state_p;

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean generic_state_debug ;
#endif

/*
 *	Current module state.
 */

extern void
generic_screen_state__initialize__(SIScreenRec *si_screen_p)
;


#endif
