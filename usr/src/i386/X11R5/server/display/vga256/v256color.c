/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256color.c	1.3"

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

#include "Xmd.h"
#include "sidep.h"
#include "miscstruct.h"
#include "sys/types.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"

#include <sys/inline.h>
#include "vgaregs.h"


/*
 *	v256_set_cmap(visual, cmap, colors, count)	-- fill in a colormap.
 *
 *	Input:
 *		int	visual		-- index of the visual being updated
 *		int	cmap		-- index of the colormap being updated
 *		SIcolor	*colors		-- colors to be updated
 *		int	count		-- number of colors being updated
 */
v256_set_cmap(visual, cmap, colors, count)
int visual;
int cmap;
register SIColor *colors;
register int count;
{
#if 0
	while (count--)
		v256_set_color(*colors++);
#else
	register SIColor *cp;
	register v256_rgb *vp;
	int ii;
	
	/* wait for vblank... */
	while (!(inb(vendorInfo.ad_addr+STATUS_REG) & S_VSYNC))
	  ;

	while (count--) {
	    cp = colors++;
	    ii = cp->SCpindex;
	    vp = &v256_pallette[ii];

	    vp->red   = (cp->SCred   >> 10) & 0x3f;
	    vp->green = (cp->SCgreen >> 10) & 0x3f;
	    vp->blue  = (cp->SCblue  >> 10) & 0x3f;

	    outb(PEL_WRITE, ii);
	    outb(PEL_DATA, vp->red);
	    outb(PEL_DATA, vp->green);
	    outb(PEL_DATA, vp->blue);
	}
#endif
}


/*
 *	v256_set_color(color)	-- determine the V256 pallette value
 *				for an attribute register based on the 
 *				color structure passed in.
 *
 *	Input:
 *		SIColor	color	-- the index and RGB values for the color
 */
v256_set_color(color)
SIColor color;
{
	register int	i;

	i = color.SCpindex;

	v256_pallette[i].red   = (color.SCred   >> 10) & 0x3f;
	v256_pallette[i].green = (color.SCgreen >> 10) & 0x3f;
	v256_pallette[i].blue  = (color.SCblue  >> 10) & 0x3f;

	v256_color(i, v256_pallette[i].red,
		     v256_pallette[i].green,
		     v256_pallette[i].blue);
}

v256_get_color(color)
SIColor *color;
{
	register i;

	i = color->SCpindex;
	color->SCred   = v256_pallette[i].red << 10;
	color->SCgreen = v256_pallette[i].green << 10;
	color->SCblue  = v256_pallette[i].blue << 10;
}



/*
 *	v256_set_attrs()	-- set up all the attribute registers based on 
 *			the current color map.
 */
v256_set_attrs()
{
	int i;

	for (i = 0; i < V256_MAXCOLOR; i++)  {
		v256_color(i, v256_pallette[i].red,
			     v256_pallette[i].green,
			     v256_pallette[i].blue);
	}
}
