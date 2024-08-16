/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_gbls.h	1.1"
#if (! defined(__P9K_GBLS_INCLUDED__))

#define __P9K_GBLS_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include "stdenv.h"
#include "sidep.h"


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
extern enum debug_level p9000_globals_debug ;
#endif

/*
 *	Current module state.
 */

extern SIBool
p9000_global_no_operation_fail(void)
;

extern SIBool
p9000_global_no_operation_succeed(void)
;

extern SIvoid
p9000_global_no_operation_void(void)
;

extern void
p9000_global_graphics_engine_timeout_handler(
	char *file_name_p, 
	int line_number)
;

extern void
p9000_global_reissue_quad_command(int xl, int yt, int xr, int yb)
;

extern SIBool
p9000_global_apply_SI_1_1_rect_fill_function(
	void *module_private_p,
	SIint32 count,
	SIRectP rect_p,
	SIBool (*function_p)(void *module_private_p,
						 int count, 
						 SIRectOutlineP rect_p))
;


#endif
