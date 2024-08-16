/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364__init__.c	1.2"
#include <sidep.h>
#include "s364_opt.h"

extern void s364_register__initialize__(SIScreenRec *si_screen_p, struct s364_options_structure *options_p);
extern void s364_screen_state__initialize__(SIScreenRec *si_screen_p, struct s364_options_structure *options_p);
extern void s364_graphics_state__initialize__(SIScreenRec *si_screen_p, struct s364_options_structure *options_p);
extern void s364_colormap__initialize__(SIScreenRec *si_screen_p, struct s364_options_structure *options_p);
extern void s364_fill__initialize__(SIScreenRec *si_screen_p, struct s364_options_structure *options_p);
extern void s364_bitblt__initialize__(SIScreenRec *si_screen_p, struct s364_options_structure *options_p);
extern void s364_font__initialize__(SIScreenRec *si_screen_p, struct s364_options_structure *options_p);
extern void s364_line__initialize__(SIScreenRec *si_screen_p, struct s364_options_structure *options_p);
extern void s364_polypoint__initialize__(SIScreenRec *si_screen_p, struct s364_options_structure *options_p);
extern void s364_arc__initialize__(SIScreenRec *si_screen_p, struct s364_options_structure *options_p);
extern void s364_cursor__initialize__(SIScreenRec *si_screen_p, struct s364_options_structure *options_p);
extern void s364_scanline__initialize__(SIScreenRec *si_screen_p, struct s364_options_structure *options_p);
extern void s364_fillspans__initialize__(SIScreenRec *si_screen_p, struct s364_options_structure *options_p);

void
s364__initialize__(SIScreenRec *si_screen_p, struct s364_options_structure *options_p)
{
	s364_register__initialize__(si_screen_p, options_p);
	s364_screen_state__initialize__(si_screen_p, options_p);
	s364_graphics_state__initialize__(si_screen_p, options_p);
	s364_colormap__initialize__(si_screen_p, options_p);
	s364_fill__initialize__(si_screen_p, options_p);
	s364_bitblt__initialize__(si_screen_p, options_p);
	s364_font__initialize__(si_screen_p, options_p);
	s364_line__initialize__(si_screen_p, options_p);
	s364_polypoint__initialize__(si_screen_p, options_p);
	s364_arc__initialize__(si_screen_p, options_p);
	s364_cursor__initialize__(si_screen_p, options_p);
	s364_scanline__initialize__(si_screen_p, options_p);
	s364_fillspans__initialize__(si_screen_p, options_p);
}
