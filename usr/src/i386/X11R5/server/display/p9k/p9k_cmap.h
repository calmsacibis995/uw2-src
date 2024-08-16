/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_cmap.h	1.2"

#if (! defined(__P9K_CMAP_INCLUDED__))

#define __P9K_CMAP_INCLUDED__



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

enum p9000_visual_kind 
{
#define	DEFINE_VISUAL(NAME, DESCRIPTION, TYPE, DEPTH,\
			N_COLORMAPS, SIZE, N_VALID_BITS,\
			R_MASK, G_MASK, B_MASK,\
			R_OFFSET, G_OFFSET, B_OFFSET)\
	P9000_VISUAL_##NAME
#include "p9k_vis.def"
#undef DEFINE_VISUAL
};

/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */
#if (defined(__DEBUG__))
extern enum debug_level p9000_colormap_debug ;
#endif

extern void
p9000_colormap__vt_switch_in__(void)
;

extern void
p9000_colormap__initialize__(SIScreenRec *si_screen_p,
	struct p9000_options_structure *options_p)
;


#endif
