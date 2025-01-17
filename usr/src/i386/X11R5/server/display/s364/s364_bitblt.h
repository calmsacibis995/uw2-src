/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_bitblt.h	1.2"
#if (! defined(__S364_BITBLT_INCLUDED__))

#define __S364_BITBLT_INCLUDED__



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
extern boolean s364_bitblt_debug ;
extern boolean s364_ms_bitblt_debug ;
extern boolean s364_sm_bitblt_debug ;
extern boolean s364_ss_bitblt_debug ;
extern boolean s364_ms_stplblt_debug ;
#endif

/*
 *	Current module state.
 */


extern void
s364_bitblt__initialize__(SIScreenRec *si_screen_p,
						  struct s364_options_structure * options_p)
;


#endif
