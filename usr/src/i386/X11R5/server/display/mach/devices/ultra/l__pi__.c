/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/devices/ultra/l__pi__.c	1.2"


#include <sidep.h>
#include "l_opt.h"

extern void lfb_graphics_state__pre_initialize__(SIScreenRec *si_screen_p, struct lfb_options_structure *options_p);

void
lfb__pre_initialize__(SIScreenRec *si_screen_p, struct lfb_options_structure *options_p)
{
	lfb_graphics_state__pre_initialize__(si_screen_p, options_p);
}
