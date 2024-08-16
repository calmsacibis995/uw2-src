/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k__init__.c	1.2"

#include <sidep.h>
#include "p9k_opt.h"

extern void p9000_arc__initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);
extern void p9000_bitblt__initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);
extern void p9000_colormap__initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);
extern void p9000_cursor__initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);
extern void p9000_font__initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);
extern void p9000_graphics_state__initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);
extern void p9000_line__initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);
extern void p9000_polypoint__initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);
extern void p9000_screen_state__initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);
extern void p9000_solid__initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);
extern void p9000_tile__initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);
extern void p9000_stipple__initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);
extern void p9000_scanline__initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p);

void
p9000__initialize__(SIScreenRec *si_screen_p, struct p9000_options_structure *options_p)
{
	p9000_arc__initialize__(si_screen_p, options_p);
	p9000_bitblt__initialize__(si_screen_p, options_p);
	p9000_colormap__initialize__(si_screen_p, options_p);
	p9000_cursor__initialize__(si_screen_p, options_p);
	p9000_font__initialize__(si_screen_p, options_p);
	p9000_graphics_state__initialize__(si_screen_p, options_p);
	p9000_line__initialize__(si_screen_p, options_p);
	p9000_polypoint__initialize__(si_screen_p, options_p);
	p9000_screen_state__initialize__(si_screen_p, options_p);
	p9000_solid__initialize__(si_screen_p, options_p);
	p9000_tile__initialize__(si_screen_p, options_p);
	p9000_stipple__initialize__(si_screen_p, options_p);
	p9000_scanline__initialize__(si_screen_p, options_p);
}
