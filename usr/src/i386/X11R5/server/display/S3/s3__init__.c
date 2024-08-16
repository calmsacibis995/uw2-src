/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3__init__.c	1.1"


#include <sidep.h>
#include "s3_options.h"

extern void s3_screen_state__initialize__(SIScreenRec *si_screen_p, struct s3_options_structure *options_p);
extern void s3_register__initialize__(SIScreenRec *si_screen_p, struct s3_options_structure *options_p);
extern void s3_scanline__initialize__(SIScreenRec *si_screen_p, struct s3_options_structure *options_p);
extern void s3_colormap__initialize__(SIScreenRec *si_screen_p, struct s3_options_structure *options_p);
extern void s3_cursor__initialize__(SIScreenRec *si_screen_p, struct s3_options_structure *options_p);
extern void s3_fill__initialize__(SIScreenRec *si_screen_p, struct s3_options_structure *options_p);
extern void s3_font__initialize__(SIScreenRec *si_screen_p, struct s3_options_structure *options_p);
extern void s3_graphics_state__initialize__(SIScreenRec *si_screen_p, struct s3_options_structure *options_p);
extern void s3_line__initialize__(SIScreenRec *si_screen_p, struct s3_options_structure *options_p);
extern void s3_polypoint__initialize__(SIScreenRec *si_screen_p, struct s3_options_structure *options_p);
extern void s3_arc__initialize__(SIScreenRec *si_screen_p, struct s3_options_structure *options_p);
extern void s3_fillspans__initialize__(SIScreenRec *si_screen_p, struct s3_options_structure *options_p);
extern void s3_bitblt__initialize__(SIScreenRec *si_screen_p, struct s3_options_structure *options_p);

void
s3__initialize__(SIScreenRec *si_screen_p, struct s3_options_structure *options_p)
{
	s3_screen_state__initialize__(si_screen_p, options_p);
	s3_register__initialize__(si_screen_p, options_p);
	s3_scanline__initialize__(si_screen_p, options_p);
	s3_colormap__initialize__(si_screen_p, options_p);
	s3_cursor__initialize__(si_screen_p, options_p);
	s3_fill__initialize__(si_screen_p, options_p);
	s3_font__initialize__(si_screen_p, options_p);
	s3_graphics_state__initialize__(si_screen_p, options_p);
	s3_line__initialize__(si_screen_p, options_p);
	s3_polypoint__initialize__(si_screen_p, options_p);
	s3_arc__initialize__(si_screen_p, options_p);
	s3_fillspans__initialize__(si_screen_p, options_p);
	s3_bitblt__initialize__(si_screen_p, options_p);
}
