/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_sline.c	1.1"
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

#include <sidep.h>
#include "stdenv.h"
#include "p9k_opt.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

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
export enum debug_level p9000_scanline_debug = DEBUG_LEVEL_NONE;
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

#include "p9k_state.h"
#include "p9k_gbls.h"
#include "p9k_regs.h"


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
 *p0000_get_scanline
 */
STATIC SILine
p9000_get_scanline(const SIint32 scanline_y)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,screen_state_p));

	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

	return  (SILine)(screen_state_p->framebuffer_p +
		scanline_y*screen_state_p->generic_state.screen_physical_width);

}


STATIC SIvoid
p9000_set_scanline(const SIint32 scanline_y, const SILine scanline_p)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,screen_state_p));
	
	/*
	 *Since get_scanline returns the actual pointer to framebuffer
	 *itself we don't have to do anything here, just return
	 */
}

/*
 * p9000_free_scanline
 * We don't have anything to free
 */

STATIC SIvoid
p9000_free_scanline(void)
{
	return;
}

function void
p9000_scanline__initialize__(SIScreenRec *si_screen_p,
	struct p9000_options_structure * options_p)
{
	GENERIC_CURRENT_SCREEN_STATE_DECLARE();
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIConfigP config_p = si_screen_p->cfgPtr;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 screen_state_p));
	
#if (defined(__DEBUG__))
	if(DEBUG_LEVEL_MATCH(p9000_scanline,INTERNAL))
	{
		(void) fprintf(debug_stream_p, 
		"(p9000_scanline__initialize__) {\n"
		"\tfunctions_p = %p\n"
		"\tflags_p = %p\n"
		"\tconfig_p = %p\n"
		"\toptions_p = %p\n"
		"}\n",
		(void *) functions_p, (void *) flags_p, 
		(void *) config_p, (void *) options_p);
	}
#endif

	functions_p->si_getsl = p9000_get_scanline;
	functions_p->si_setsl = p9000_set_scanline;
	functions_p->si_freesl = p9000_free_scanline;
}
