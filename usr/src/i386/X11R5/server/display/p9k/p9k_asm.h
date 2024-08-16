/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_asm.h	1.2"

#if (! defined(__P9K_ASM_INCLUDED__))

#define __P9K_ASM_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

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



extern void p9000_asm_memory_to_screen_transfer_helper
	(unsigned char *data_p,
	int count,
	volatile unsigned int * const base_address_p,
	int source_step,
	int height);

/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern enum debug_level p9000_asm_debug ;
#endif

/*
 *	Current module state.
 */


#endif
