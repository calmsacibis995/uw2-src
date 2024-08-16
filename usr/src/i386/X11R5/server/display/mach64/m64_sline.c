/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_sline.c	1.2"

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
export boolean m64_scanline_debug = 0;
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
#include "g_state.h"
#include "m64_gbls.h"
#include "m64_opt.h"
#include "m64_regs.h"
#include "m64_gs.h"
#include "m64_state.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

STATIC SILine
m64_get_scanline(const SIint32 scanline_y)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	M64_CURRENT_FRAMEBUFFER_BASE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE,screen_state_p));

	M64_WAIT_FOR_GUI_ENGINE_IDLE();

	return  (SILine)(framebuffer_p + ( scanline_y * 
		((screen_state_p->generic_state.screen_physical_width * 
		 screen_state_p->generic_state.screen_depth) / 8)));
}

STATIC SIvoid
m64_set_scanline(const SIint32 scanline_y, const SILine scanline_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

	M64_WAIT_FOR_GUI_ENGINE_IDLE();
	return;
}

/*
 * m64_free_scanline
 *
 * we don't actually free the scanline at any point as this was
 * allocated 
 */
STATIC SIvoid
m64_free_scanline(void)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

#if (defined(__DEBUG__))
	if (m64_scanline_debug)
	{
		(void) fprintf(debug_stream_p, "(m64_free_scanline) {}\n");
	}
#endif
	M64_WAIT_FOR_GUI_ENGINE_IDLE();

	return;
}

function void
m64_scanline__initialize__(SIScreenRec *si_screen_p,
							struct m64_options_structure * options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIConfigP config_p = si_screen_p->cfgPtr;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	
#if (defined(__DEBUG__))
	if (m64_scanline_debug)
	{
		(void) fprintf(debug_stream_p, 
		"(m64_scanline__initialize__) {\n"
		"\tfunctions_p = %p\n"
		"\tflags_p = %p\n"
		"\tconfig_p = %p\n"
		"\toptions_p = %p\n"
		"}\n",
		(void *) functions_p, (void *) flags_p, 
		(void *) config_p, (void *) options_p);
	}
#endif

	functions_p->si_getsl = m64_get_scanline;
	functions_p->si_setsl = m64_set_scanline;
	functions_p->si_freesl = m64_free_scanline;
}
