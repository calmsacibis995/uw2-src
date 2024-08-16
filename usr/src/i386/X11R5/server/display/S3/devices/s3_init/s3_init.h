/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/devices/s3_init/s3_init.h	1.1"

#if (! defined(__S3_INIT_INCLUDED__))

#define __S3_INIT_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include "s3_init_gl.h"

#include "s3.h"

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
extern boolean s3_init_debug ;
#endif

/*
 *	Current module state.
 */
#include "stdenv.h"

extern SIBool
DM_InitFunction(SIint32 virtual_terminal_file_descriptor,
				SIScreenRec *si_screen_p)
;


#endif
