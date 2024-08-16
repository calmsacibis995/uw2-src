/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64__init__.c	1.2"

#include <sidep.h>
#include "m64_opt.h"

extern void m64_register__initialize__(SIScreenRec *si_screen_p, struct m64_options_structure *options_p);
extern void m64_screen_state__initialize__(SIScreenRec *si_screen_p, struct m64_options_structure *options_p);
extern void m64_graphics_state__initialize__(SIScreenRec *si_screen_p, struct m64_options_structure *options_p);
extern void m64_colormap__initialize__(SIScreenRec *si_screen_p, struct m64_options_structure *options_p);
extern void m64_fill__initialize__(SIScreenRec *si_screen_p, struct m64_options_structure *options_p);
extern void m64_bitblt__initialize__(SIScreenRec *si_screen_p, struct m64_options_structure *options_p);
extern void m64_font__initialize__(SIScreenRec *si_screen_p, struct m64_options_structure *options_p);
extern void m64_line__initialize__(SIScreenRec *si_screen_p, struct m64_options_structure *options_p);
extern void m64_polypoint__initialize__(SIScreenRec *si_screen_p, struct m64_options_structure *options_p);
extern void m64_arc__initialize__(SIScreenRec *si_screen_p, struct m64_options_structure *options_p);
extern void m64_cursor__initialize__(SIScreenRec *si_screen_p, struct m64_options_structure *options_p);
extern void m64_scanline__initialize__(SIScreenRec *si_screen_p, struct m64_options_structure *options_p);
extern void m64_fillspans__initialize__(SIScreenRec *si_screen_p, struct m64_options_structure *options_p);

void
m64__initialize__(SIScreenRec *si_screen_p, struct m64_options_structure *options_p)
{
	m64_register__initialize__(si_screen_p, options_p);
	m64_screen_state__initialize__(si_screen_p, options_p);
	m64_graphics_state__initialize__(si_screen_p, options_p);
	m64_colormap__initialize__(si_screen_p, options_p);
	m64_fill__initialize__(si_screen_p, options_p);
	m64_bitblt__initialize__(si_screen_p, options_p);
	m64_font__initialize__(si_screen_p, options_p);
	m64_line__initialize__(si_screen_p, options_p);
	m64_polypoint__initialize__(si_screen_p, options_p);
	m64_arc__initialize__(si_screen_p, options_p);
	m64_cursor__initialize__(si_screen_p, options_p);
	m64_scanline__initialize__(si_screen_p, options_p);
	m64_fillspans__initialize__(si_screen_p, options_p);
}
