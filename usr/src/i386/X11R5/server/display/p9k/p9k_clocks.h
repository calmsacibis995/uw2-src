/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_clocks.h	1.2"

#if (! defined(__P9K_CLOCKS_INCLUDED__))

#define __P9K_CLOCKS_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

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

struct p9000_clock_functions 
{
	boolean (*clock_initialize_function_p)();
	boolean (*clock_uninitialize_function_p)();
	boolean (*can_support_frequency_function_p)(unsigned long);
};

enum p9000_clock_kind
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, INIT_FUNC, UNINIT_FUNC,\
	 CAN_SUPPORT_FREQ_FUNC, NUM_FREQ,\
	f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15)\
		NAME
#include "p9k_clocks.def"
#undef DEFINE_CLOCK_CHIP						
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
extern boolean p9000_clocks_debug ;
#endif

/*
 *	Current module state.
 */

extern boolean
p9000_clock__hardware_initialize__(SIScreenRec *si_screen_p,
	struct p9000_options_structure *options_p)
;


#endif
