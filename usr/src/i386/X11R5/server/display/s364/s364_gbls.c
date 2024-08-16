/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_gbls.c	1.2"

/***
 ***	NAME
 ***
 ***		s364_gbls.c : global definitions in the S364 display library.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "s364_gbls.h"
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
export int s364_graphics_engine_micro_delay_count = 0;
export int s364_graphics_engine_loop_timeout_count = 0;
export int s364_crtc_sync_loop_timeout_count = 0;
export unsigned short 
	s364_graphics_engine_number_of_fifo_entries_free_mask = 0xFFF8;

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean s364_globals_debug = 0;
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
#include "s364_opt.h"
#include "s364_regs.h"
#include "s364_gs.h"
#include "s364_state.h"

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
 * PURPOSE
 * 
 * Null routine.
 * 
 * RETURN VALUE
 * 
 * 	Always returns SI_FAIL.
 */
function SIBool
s364_no_operation_fail()
{

	S3_WAIT_FOR_GE_IDLE();
#if (defined(__DEBUG__))
	if (s364_globals_debug)
	{
		(void) fprintf(debug_stream_p, "(s364_globals)\tNo Operation Fail "
				"called.\n");
	}
#endif /* __DEBUG__ */	
	return (SI_FAIL);
}

/*
 * PURPOSE
 * 
 * Null routine.
 * 
 * RETURN VALUE
 * 
 * 	Always returns SI_SUCCEED.
 */
function SIBool
s364_no_operation_succeed()
{
#if (defined(__DEBUG__))
	if (s364_globals_debug)
	{
		(void) fprintf(debug_stream_p, "(s364_globals)\tNo Operation Succeed "
				"called.\n");
	}
#endif /* __DEBUG__ */

	return (SI_SUCCEED);

}

/*
 * PURPOSE
 * 
 * Null routine.
 * 
 * RETURN VALUE
 * 
 * 	None.
 */
function SIvoid
s364_no_operation_void()
{

#if (defined(__DEBUG__))
	if (s364_globals_debug)
	{
		(void) fprintf(debug_stream_p, "(s364_globals)\tNo Operation Void"
				"called.\n");
	}
#endif /* __DEBUG__ */

	return;

}

