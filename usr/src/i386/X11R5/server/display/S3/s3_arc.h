/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_arc.h	1.2"

#if (! defined(__S3_ARC_INCLUDED__))

#define __S3_ARC_INCLUDED__



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


#define S3_ARC_IS_ENTRY_MATCHING(cache_p, arc_p)	\
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
extern boolean s3_arc_debug ;
#endif

/*
 *	Current module state.
 */

extern void
s3_arc__gs_change__(void)
;

extern void
s3_arc__initialize__(SIScreenRec *si_screen_p,
					   struct s3_options_structure *options_p)
;


#endif
