/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/generic.h	1.2"
#if (! defined(__GENERIC_INCLUDED__))

#define __GENERIC_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "g_state.h"

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
extern boolean generic_debug ;
#endif

/*
 *	Current module state.
 */

#include "stdenv.h"

extern SIBool
generic_no_operation_fail(void)
;

extern SIBool
generic_no_operation_succeed(void)
;

extern SIvoid
generic_no_operation_void(void)
;

extern SIBool
generic_initialize_display_library(SIint32 virtual_terminal_file_descriptor,
    SIScreenRec *si_screen_p,
    struct generic_screen_state *screen_state_p)
;


#endif
