/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_globals.c	1.1"

/***
 ***	NAME
 ***
 ***		s3globals.c : global definitions in the S3 
 ***						display library.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "s3globals.h"
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
export boolean s3_globals_debug = FALSE;
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
s3_no_operation_fail()
{
#if (defined(__DEBUG__))
	if (s3_globals_debug)
	{
		(void) fprintf(debug_stream_p, "(s3_globals)\tNo Operation Fail "
				"called.\n");
	}
#endif /* __DEBUG__ */	
	return (SI_FAIL);
}

function SIBool
s3_no_operation_succeed()
{
#if (defined(__DEBUG__))
	if (s3_globals_debug)
	{
		(void) fprintf(debug_stream_p, "(s3_globals)\tNo Operation Succeed "
				"called.\n");
	}
#endif /* __DEBUG__ */

	return (SI_SUCCEED);

}

function SIvoid
s3_no_operation_void()
{

#if (defined(__DEBUG__))
	if (s3_globals_debug)
	{
		(void) fprintf(debug_stream_p, "(s3_globals)\tNo Operation Void"
				"called.\n");
	}
#endif /* __DEBUG__ */

	return;

}

