/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m__init__.c	1.2"

#include <sidep.h>
#include "m_opt.h"

extern void mach_bitblt__initialize__(SIScreenRec *si_screen_p, struct mach_options_structure *options_p);
extern void mach_colormap__initialize__(SIScreenRec *si_screen_p, struct mach_options_structure *options_p);
extern void mach_cursor__initialize__(SIScreenRec *si_screen_p, struct mach_options_structure *options_p);
extern void mach_fill__initialize__(SIScreenRec *si_screen_p, struct mach_options_structure *options_p);
extern void mach_font__initialize__(SIScreenRec *si_screen_p, struct mach_options_structure *options_p);
extern void mach_graphics_state__initialize__(SIScreenRec *si_screen_p, struct mach_options_structure *options_p);
extern void mach_line__initialize__(SIScreenRec *si_screen_p, struct mach_options_structure *options_p);
extern void mach_polypoint__initialize__(SIScreenRec *si_screen_p, struct mach_options_structure *options_p);
extern void mach_scanline__initialize__(SIScreenRec *si_screen_p, struct mach_options_structure *options_p);
extern void mach_fillspans__initialize__(SIScreenRec *si_screen_p, struct mach_options_structure *options_p);
extern void mach_screen_state__initialize__(SIScreenRec *si_screen_p, struct mach_options_structure *options_p);

void
mach__initialize__(SIScreenRec *si_screen_p, struct mach_options_structure *options_p)
{
	mach_bitblt__initialize__(si_screen_p, options_p);
	mach_colormap__initialize__(si_screen_p, options_p);
	mach_cursor__initialize__(si_screen_p, options_p);
	mach_fill__initialize__(si_screen_p, options_p);
	mach_font__initialize__(si_screen_p, options_p);
	mach_graphics_state__initialize__(si_screen_p, options_p);
	mach_line__initialize__(si_screen_p, options_p);
	mach_polypoint__initialize__(si_screen_p, options_p);
	mach_scanline__initialize__(si_screen_p, options_p);
	mach_fillspans__initialize__(si_screen_p, options_p);
	mach_screen_state__initialize__(si_screen_p, options_p);
}
