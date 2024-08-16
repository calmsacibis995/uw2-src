/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_cmap.h	1.2"
#if (! defined(__S364_CMAP_INCLUDED__))

#define __S364_CMAP_INCLUDED__



/***
 ***	Includes.
 ***/
#include <sidep.h>
#include "stdenv.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 *** Types.
 ***/

enum s364_visual_kind 
{
#define	DEFINE_VISUAL(NAME, DESCRIPTION, TYPE, DEPTH,\
			N_COLORMAPS, SIZE, N_VALID_BITS,\
			R_MASK, G_MASK, B_MASK,\
			R_OFFSET, G_OFFSET, B_OFFSET)\
	S364_VISUAL_##NAME
#include "s364_vis.def"
#undef DEFINE_VISUAL
};

/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */
#if (defined(__DEBUG__))
extern boolean s364_colormap_debug ;
extern boolean s364_video_blank_debug ;
#endif

extern void
s364_cmap__vt_switch_in__(void)
;

extern void
s364_colormap__initialize__(SIScreenRec *si_screen_p,
	struct s364_options_structure *options_p)
;


#endif
