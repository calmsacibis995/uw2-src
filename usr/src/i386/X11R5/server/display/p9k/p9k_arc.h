/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_arc.h	1.1"
#if (! defined(__P9K_ARC_INCLUDED__))

#define __P9K_ARC_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"
#include "p9k_opt.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

#define P9000_CURRENT_ARC_STATE_DECLARE()\
	struct p9000_arc_state *arc_state_p = screen_state_p->arc_state_p

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
extern enum debug_level p9000_arc_debug ;
#endif

/*
 *	Current module state.
 */

extern void
p9000_arc__gs_change__(void)
;

extern void
p9000_arc__initialize__(SIScreenRec *si_screen_p,
					   struct p9000_options_structure *options_p)
;


#endif
