/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/g_regs.c	1.2"

/***
 ***	NAME
 ***
 ***		generic_regs.c : generic modules interface to the chipset
 ***					 registers.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "g_regs.h"
 ***
 ***	DESCRIPTION
 ***
 ***		This module defines the interface using which the generic
 ***	module manipulates the chipset registers.  Since the generic
 ***	module does not know about the specific chipsets registers,
 ***	the actual manipulation is done at the chipset-specific and
 ***	board-specific layers.  Calls the the relevant functions are
 ***	through pointers in the "generic_registers" structure.
 ***
 ***		The register invalidation flags are kept at this topmost
 ***	level as they are shared across all implementations of
 ***	chipsets.
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
#include "g_state.h"

/***
 ***	Constants.
 ***/
/*
 * The values for the flags field of the register_put_state.
 */
#define REGISTER_PUT_STATE_XSERVER_STATE			(0x01 << 0U)
#define REGISTER_PUT_STATE_SAVED_STATE				(0x01 << 1U)

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

struct generic_register_state 
{
	/*
	 * The flags bits which indicate when specific components of the
	 * chipset become inconsistent with the current register state.
	 */
	unsigned long register_invalid_flags;
	
	/*
	 * The methods in this object
	 */
	int (*register_get_state)(struct generic_screen_state *state_p,int flags);
	int (*register_put_state)(struct generic_screen_state *state_p,int flags);

#if (defined(__DEBUG__))
	/*
	 * Debug print-outs
	 */
	int (*register_dump_state)(struct generic_screen_state *state_p,int flags);
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
export boolean generic_registers_debug = 0;
#endif /* __DEBUG__ */

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
