/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/g__init__.c	1.1"


#include <sidep.h>

extern void generic_graphics_state__initialize__(SIScreenRec *si_screen_p);
extern void generic_screen_state__initialize__(SIScreenRec *si_screen_p);

void
generic__initialize__(SIScreenRec *si_screen_p)
{
	generic_graphics_state__initialize__(si_screen_p);
	generic_screen_state__initialize__(si_screen_p);
}
