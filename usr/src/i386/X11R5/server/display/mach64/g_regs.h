/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/g_regs.h	1.2"

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
	
#ifdef DELETE
	int (*dac_init)(struct generic_screen_state *state_p);	
	int (*dac_uninit)(struct generic_screen_state *state_p);
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
