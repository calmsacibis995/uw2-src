/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_points.h	1.1"

#if (! defined(__M_POINTS_INCLUDED__))

#define __M_POINTS_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"
#include "m_globals.h"
#include "m_opt.h"

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
extern boolean mach_polypoint_debug ;
#endif

/*
 *	Current module state.
 */

extern void
mach_polypoint__gs_change__()
;

extern void
mach_polypoint__initialize__(SIScreenRec *si_screen_p,
							 struct mach_options_structure *
							 options_p)
;


#endif
