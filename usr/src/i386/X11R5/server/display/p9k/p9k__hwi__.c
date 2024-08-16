/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k__hwi__.c	1.1"

#include <sidep.h>
#include "p9k_opt.h"

extern void p9000_dac__hardware_initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);
extern void p9000_clock__hardware_initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);

void
p9000__hardware_initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p)
{
	p9000_dac__hardware_initialize__(si_screen_p, options_p);
	p9000_clock__hardware_initialize__(si_screen_p, options_p);
}
