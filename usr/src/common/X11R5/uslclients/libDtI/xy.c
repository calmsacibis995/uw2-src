/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)libDtI:xy.c	1.8" */

#include <X11/Intrinsic.h>
#include "DtI.h"

Boolean
DmIntersectItems(DmItemPtr items, Cardinal num_items, int x, int y, int w, int h)
{
/* coord's for existing icon */
#define IX	( (int)item->x )
#define IY	( (int)item->y )
#define IW	iw
#define IH	ih

/* coord's for icon to be positioned */
#define NX	x
#define NY	y
#define NW	xw
#define NH	yh

#define INRANGE(L,m,H)	((L) <= (m) && (m) <= (H))

    register int	xw = x + w - 1;		/* right-most 'x' */
    register int	yh = y + h - 1;		/* bottom-most 'y' */
    register int	iw, ih;
    register DmItemPtr	item;

    for (item = items; item < items + num_items; item++)
    {
	iw = (int)(ITEM_X(item) + ITEM_WIDTH(item) - 1);
	ih = (int)(ITEM_Y(item) + ITEM_HEIGHT(item) - 1);
	if (ITEM_MANAGED(item) && (IX != 0 || IY != 0) &&
	    ((INRANGE(IX, NX, IW) || INRANGE(IX, NW, IW) ||
	      INRANGE(NX, IX, NW) || INRANGE(NX, IW, NW)) &&
	     (INRANGE(IY, NY, IH) || INRANGE(IY, NH, IH) ||
	      INRANGE(NY, IY, NH) || INRANGE(NY, IY, NH))))
	    return(True);
    }

    return(False);
}

void
DmGetAvailIconPos(DmItemPtr items, Cardinal num_items,
		  Dimension item_width, Dimension item_height,
		  Dimension wrap_width,
		  Dimension grid_width, Dimension grid_height,
		  Position * ret_x, Position * ret_y)
{
    Dimension	margin = Ol_PointToPixel(OL_HORIZONTAL, ICON_MARGIN);
    int		x, y;
    Position	init_x;

    init_x = (item_width > grid_width) ? margin :
	margin + (Position)(grid_width - item_width) / 2;
    x = init_x;
    y = margin;

    while (DmIntersectItems(items, num_items, x, y, item_width, item_height))
    {
	/* overlapped with each others, so readjust the target... */
	x += grid_width;
	if ((Dimension)(x + item_width) > wrap_width)
	{
	    x = init_x;
	    y += grid_height;
	}

    }

    *ret_x = x;
    *ret_y = VertJustifyInGrid(y, item_height, grid_height);

}				/* end of DmGetAvailIconPos */

