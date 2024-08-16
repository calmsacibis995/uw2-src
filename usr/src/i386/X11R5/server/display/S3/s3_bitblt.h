/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_bitblt.h	1.2"
#if (! defined(__S3_BITBLT_INCLUDED__))

#define __S3_BITBLT_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"
#include "s3_globals.h"
#include "s3_options.h"

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
extern boolean s3_bitblt_debug ;
extern boolean s3_ms_bitblt_debug ;
extern boolean s3_sm_bitblt_debug ;
extern boolean s3_ss_bitblt_debug ;
extern boolean s3_ms_stplblt_debug ;
extern boolean s3_sm_stplblt_debug ;
extern boolean s3_ss_stplblt_debug ;
#endif

/*
 *	Current module state.
 */


extern void
s3_bitblt__gs_change__(void)
;

extern void
s3_bitblt__initialize__(SIScreenRec *si_screen_p,
						  struct s3_options_structure * options_p)
;


#endif
