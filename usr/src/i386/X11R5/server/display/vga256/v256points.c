/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256points.c	1.7"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/************
 * Copyrighted as an unpublished work.
 * (c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 * All rights reserved.
 ***********/

#include "X.h"
#include "Xmd.h"
#include "sidep.h"
#include "miscstruct.h"
#include "sys/types.h"
#include "sys/dl.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"
#include "sys/inline.h"


/*
 *	v256_plot_points(cnt, ptsIn)	-- draw a series of pixels based on
 *					points passed in.  Points are drawn
 *					in the current foreground color using
 *					the current ROP.
 *
 *	Input:
 *		int		cnt	-- count of number of points
 *		DDXPointRec	*ptsIn	-- Points
 */
SIBool
v256_plot_points(cnt, ptsIn)
  register SIint32	cnt;
  register SIPointP	ptsIn;
{
	BYTE 	*paddr, fg, mask;
	register int	i, local_page;

	DBENTRY("v256_poly_points()");

	if (v256_gs->mode == GXnoop)
		return(SI_SUCCEED);

        mask = v256_gs->pmask;
	fg = v256_src & mask;
	i = ptsIn->x + v256_slbytes * ptsIn->y;
	selectpage(i);
	local_page = i & ~VIDEO_PAGE_MASK;

	switch (v256_function) {
	case V256_COPY:
		if ((mask = ~mask) == 0) {
			for (;cnt--; ptsIn++) {
				i = ptsIn->x + v256_slbytes * ptsIn->y;
				if ((i & ~VIDEO_PAGE_MASK) != local_page) {
					selectpage(i);
					local_page = i & ~VIDEO_PAGE_MASK;
				}
				i &= VIDEO_PAGE_MASK;
				*(v256_fb + i) = fg;
			}
		}
		else {
			for (;cnt--; ptsIn++) {
				i = ptsIn->x + v256_slbytes * ptsIn->y;
				selectpage(i);
				i &= VIDEO_PAGE_MASK;
				paddr = v256_fb + i;
				*paddr = fg | (*paddr & mask);
			}
		}
		return(SI_SUCCEED);

	case V256_XOR:
		for (;cnt--; ptsIn++) {
			i = ptsIn->x + v256_slbytes * ptsIn->y;
			selectpage(i);
			i &= VIDEO_PAGE_MASK;
			paddr = v256_fb + i;
			*paddr ^= fg;
		}
		return(SI_SUCCEED);

	case V256_OR:
		for (;cnt--; ptsIn++) {
			i = ptsIn->x + v256_slbytes * ptsIn->y;
			selectpage(i);
			i &= VIDEO_PAGE_MASK;
			paddr = v256_fb + i;
			*paddr |= fg;
		}
		return(SI_SUCCEED);

	case V256_OR_INVERT:
		for (;cnt--; ptsIn++) {
			i = ptsIn->x + v256_slbytes * ptsIn->y;
			selectpage(i);
			i &= VIDEO_PAGE_MASK;
			paddr = v256_fb + i;
			*paddr = (*paddr | fg) ^ mask;
		}
		return(SI_SUCCEED);

	case V256_AND:
		fg |= ~mask;
		for (;cnt--; ptsIn++) {
			i = ptsIn->x + v256_slbytes * ptsIn->y;
			selectpage(i);
			i &= VIDEO_PAGE_MASK;
			paddr = v256_fb + i;
			*paddr &= fg;
		}
		return(SI_SUCCEED);

	case V256_AND_INVERT:
		fg |= ~mask;
		for (;cnt--; ptsIn++) {
			i = ptsIn->x + v256_slbytes * ptsIn->y;
			selectpage(i);
			i &= VIDEO_PAGE_MASK;
			paddr = v256_fb + i;
			*paddr = (*paddr & fg) ^ mask;
		}
		return(SI_SUCCEED);
	}
	return(SI_FAIL);
}

/*
 * Hopefully faster version of the point plotting routine
 */
SIBool
v256PlotPoints(SIint32 count, SIPointP ptsIn)
{

     register int i, offset;
     register SIPointP ppt;
     register unsigned char *fb_base_p = (unsigned char *)v256_fb;
     const unsigned char foreground = v256_src;

     if (v256_gs->mode != GXcopy || v256_gs->pmask != 0xFF)
     {
	  /*
	   * call the generic plot routine
	   */
	  v256_plot_points(count, ptsIn);
     }

     for(i = 0, ppt = ptsIn; i < count; i++, ppt++)
     {
	  offset = ppt->x + (v256_slbytes *ppt->y);
	  selectwritepage(offset);
	  *(fb_base_p + (offset & VIDEO_PAGE_MASK)) = foreground;
     }

     return SI_SUCCEED;
}
