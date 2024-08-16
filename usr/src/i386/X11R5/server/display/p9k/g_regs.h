/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/g_regs.h	1.1"

#if (! defined(__G_REGS_INCLUDED__))

#define __G_REGS_INCLUDED__



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

	int (*register_get_state)(struct generic_screen_state *state_p);
	int (*register_put_state)(struct generic_screen_state *state_p);

#if (defined(__DEBUG__))

	/*
	 * Debug print-outs
	 */

	int (*register_dump_state)(struct generic_screen_state *state_p);
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
extern boolean generic_registers_debug ;
#endif /* __DEBUG__ */

/*
 *	Current module state.
 */


#endif
