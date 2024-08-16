/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/devices/ultra/l_arc.h	1.1"

#if (! defined(__L_ARC_INCLUDED__))

#define __L_ARC_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"
#include "l_globals.h"
#include "l_opt.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

#define LFB_CURRENT_ARC_STATE_DECLARE()\
	struct lfb_arc_state *arc_state_p = screen_state_p->arc_state_p

#define LFB_ARC_IS_ENTRY_MATCHING(cache_p, arc_p)	\
	(((cache_p)->width == (arc_p)->width) &&		\
	 ((cache_p)->height == (arc_p)->height) &&		\
	 ((cache_p)->angle1 == (arc_p)->angle1) &&      \
	 ((cache_p)->angle2 == (arc_p)->angle2))

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
extern boolean lfb_arc_debug ;
#endif

/*
 *	Current module state.
 */

extern void
lfb_arc__gs_change__(void)
;

extern void
lfb_arc__post_initialize__(SIScreenRec *si_screen_p,
					   struct lfb_options_structure *options_p)
;


#endif
