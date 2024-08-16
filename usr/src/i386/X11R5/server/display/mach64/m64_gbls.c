/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_gbls.c	1.2"

/***
 ***	NAME
 ***
 ***		m64_gbls.c : global definitions in the Mach64 display library.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "m64_gbls.h"
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


/***
 ***	Variables.
 ***/
export int m64_graphics_engine_micro_delay_count = 0;
export int m64_graphics_engine_loop_timeout_count = 0;
export int m64_graphics_engine_number_of_fifo_entries_free = 0;

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean m64_globals_debug = 0;
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

/*
 * Null routines.
 */
function SIBool
m64_no_operation_fail()
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

	M64_WAIT_FOR_GUI_ENGINE_IDLE();
#if (defined(__DEBUG__))
	if (m64_globals_debug)
	{
		(void) fprintf(debug_stream_p, "(m64_globals)\tNo Operation Fail "
				"called.\n");
	}
#endif /* __DEBUG__ */	
	return (SI_FAIL);
}

function SIBool
m64_no_operation_succeed()
{
#if (defined(__DEBUG__))
	if (m64_globals_debug)
	{
		(void) fprintf(debug_stream_p, "(m64_globals)\tNo Operation Succeed "
				"called.\n");
	}
#endif /* __DEBUG__ */

	return (SI_SUCCEED);

}

function SIvoid
m64_no_operation_void()
{

#if (defined(__DEBUG__))
	if (m64_globals_debug)
	{
		(void) fprintf(debug_stream_p, "(m64_globals)\tNo Operation Void"
				"called.\n");
	}
#endif /* __DEBUG__ */

	return;

}

