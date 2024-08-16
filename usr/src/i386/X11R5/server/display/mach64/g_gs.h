/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/g_gs.h	1.2"

#if (! defined(__G_GS_INCLUDED__))

#define __G_GS_INCLUDED__



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

#if (defined(__DEBUG__))
#define GENERIC_GRAPHICS_STATE_STAMP\
	(('G' << 0) + ('E' << 1) + ('N' << 2) + ('E' << 3) +\
	 ('R' << 4) + ('I' << 5) + ('C' << 6) + ('G' << 7) +\
	 ('R' << 8) + ('A' << 9) + ('P' << 10) + ('H' << 11) +\
	 ('I' << 12) + ('C' << 13) + ('S' << 14) + ('S' << 15) +\
	 ('T' << 16) + ('A' << 17) + ('T' << 18) + ('E' << 19))
#endif

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

/***
 ***	Forward declarations.
 ***/

/***
 ***	Variables.
 ***/
/*
 *	Debugging variables.
 */
#if (defined(__DEBUG__))
extern boolean generic_graphics_state_debug ;
#endif



/*
 *	Current module state.
 */

extern void
generic_graphics_state__initialize__(SIScreenRec *si_screen_p)
;


#endif
