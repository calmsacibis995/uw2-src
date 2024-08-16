/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_tile.h	1.2"

#if (! defined(__P9K_TILE_INCLUDED__))

#define __P9K_TILE_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include "stdenv.h"
#include "sidep.h"
#include "p9k_opt.h"

/***
 ***	Constants.
 ***/

#define	P9000_TILE_STATE_STAMP												\
	(('P' << 0) + ('9' << 1) + ('0' << 2) + ('0' << 3) + ('0' << 4) +		\
	 ('_' << 5) + ('T' << 6) + ('I' << 7) + ('L' << 8) + ('E' << 9) +		\
	 ('_' << 10) + ('S' << 11) + ('T' << 12) + ('A' << 13) + ('T' << 14) +	\
	 ('E' << 15) + 0 )

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
extern enum debug_level p9000_tile_debug ;
#endif

/*
 *	Current module state.
 */


extern void
p9000_tile__gs_change__(void)
;

extern void
p9000_tile__initialize__(
	SIScreenRec *si_screen_p,
	struct p9000_options_structure *options_p)
;


#endif
