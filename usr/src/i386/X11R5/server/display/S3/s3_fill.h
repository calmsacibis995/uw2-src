/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)S3:S3/s3_fill.h	1.3"

#if (! defined(__S3_FILL_INCLUDED__))

#define __S3_FILL_INCLUDED__



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
extern boolean s3_fill_debug ;
extern boolean s3_fill_tile_debug ;
extern boolean s3_fill_stipple_debug ;
#endif

/*
 *	Current module state.
 */

extern void
s3_fill__gs_change__(void)
;

extern void
s3_fill__initialize__(SIScreenRec *si_screen_p,
						struct s3_options_structure *options_p)
;


#endif
