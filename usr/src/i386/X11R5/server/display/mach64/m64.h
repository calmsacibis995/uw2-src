/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64.h	1.2"

#if (! defined(__M64_INCLUDED__))

#define __M64_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

/*
 * System specific
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/kd.h>
#include <sys/errno.h>

#include <sys/inline.h>
#include "sidep.h"
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

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean m64_debug ;
#endif

extern void
m64_save_registers(struct m64_screen_state *screen_state_p, 
	unsigned long *registers_p)
;

extern int
m64_initialize_display_library(SIint32 virtual_terminal_file_descriptor,
	SIScreenRec *si_screen_p, struct m64_screen_state *screen_state_p)
;

extern void
m64_print_initialization_failure_message( const int status,
	const SIScreenRec *si_screen_p)
;


#endif
