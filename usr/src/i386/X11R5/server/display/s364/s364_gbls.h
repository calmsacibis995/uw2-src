/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_gbls.h	1.2"
#if (! defined(__S364_GBLS_INCLUDED__))

#define __S364_GBLS_INCLUDED__



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
extern int s364_graphics_engine_micro_delay_count ;
extern int s364_graphics_engine_loop_timeout_count ;
extern int s364_crtc_sync_loop_timeout_count ;
extern unsigned short 
	s364_graphics_engine_number_of_fifo_entries_free_mask ;

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean s364_globals_debug ;
#endif

/*
 *	Current module state.
 */

extern SIBool
s364_no_operation_fail()
;

extern SIBool
s364_no_operation_succeed()
;

extern SIvoid
s364_no_operation_void()
;


#endif
