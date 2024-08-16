/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_bitblt.h	1.2"

#if (! defined(__M64_BITBLT_INCLUDED__))

#define __M64_BITBLT_INCLUDED__



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
extern boolean m64_bitblt_debug ;
extern boolean m64_ms_bitblt_debug ;
extern boolean m64_sm_bitblt_debug ;
extern boolean m64_ss_bitblt_debug ;
extern boolean m64_ms_stplblt_debug ;
#endif

/*
 *	Current module state.
 */


extern void
m64_bitblt__gs_change__(void)
;

extern void
m64_bitblt__initialize__(SIScreenRec *si_screen_p,
						  struct m64_options_structure * options_p)
;


#endif
