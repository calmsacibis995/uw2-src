/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/***
 ***	NAME
 ***
 ***		machglobals.c : global definitions in the Mach 8 / 32
 ***						display library.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "m_globals.h"
 ***
 ***	DESCRIPTION
 ***
 ***	This module supplies null functions used to return SI_FAIL or
 *** 	SI_SUCCEED.
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***	SEE ALSO
 ***
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	HISTORY
 ***
 ***/

#ident	"@(#)mach:mach/m_globals.c	1.2"
 
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
export boolean mach_globals_debug = FALSE;
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
/*
 * No Operation.
 */
function SIBool
mach_no_operation_fail()
{
#if (defined(__DEBUG__))
	if (mach_globals_debug)
	{
		(void) fprintf(debug_stream_p, "(mach_globals)\tNo Operation Fail "
				"called.\n");
	}
#endif /* __DEBUG__ */	
	return (SI_FAIL);
}

#if (defined(UNUSED_FUNCTIONALITY))
unused_function SIBool
mach_no_operation_succeed()
{
#if (defined(__DEBUG__))
	if (mach_globals_debug)
	{
		(void) fprintf(debug_stream_p, "(mach_globals)\tNo Operation Succeed "
				"called.\n");
	}
#endif /* __DEBUG__ */

	return (SI_SUCCEED);

}
#endif /* UNUSED_FUNCTIONALITY */

function SIvoid
mach_no_operation_void()
{

#if (defined(__DEBUG__))
	if (mach_globals_debug)
	{
		(void) fprintf(debug_stream_p, "(mach_globals)\tNo Operation Void"
				"called.\n");
	}
#endif /* __DEBUG__ */

	return;

}

