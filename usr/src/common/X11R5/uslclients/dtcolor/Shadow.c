/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:Shadow.c	1.1"
/* static char rcsid[] = 
	"$Header: Shadow.c,v 1.1 93/08/18 11:14:01 xbuild Exp $";
*/
/**---------------------------------------------------------------------
***	
***	file:		Shadow.c
***
***	project:	Motif Widgets
***
***	description:	Source code for MotifPlus shadow drawing extensions.
***	
***
***			(c) Copyright 1990 by Hewlett-Packard Company.
***
***
***-------------------------------------------------------------------*/


/*-------------------------------------------------------------
**	Include Files
*/

#include <Xm/XmP.h>
#include <Xm/ManagerP.h>
#include <MacrosP.h>


/*-------------------------------------------------------------
**	Public Interface
**-------------------------------------------------------------
*/

void _DtDrawEtchedShadow ();
void _DtManagerClearShadow ();


/*-------------------------------------------------------------
**	Forward Declarations
*/

static void get_rects();



/*-------------------------------------------------------------
**	_DtManagerDrawShadow
**		Draw shadow to drawable with gcs from manager.
*/
void _DtManagerDrawShadow (mgr, drawable, x, y, w, h, h_t, s_t, s_type)
XmManagerWidget	mgr;
Drawable	drawable;
Position	x, y;
Dimension	w, h, h_t, s_t;
unsigned char	s_type;
{
	Display *	d = 		XtDisplay (mgr);
	GC		top_gc = 	NULL,
			bottom_gc = 	NULL;
	Boolean		s_out = 	NULL,
			s_etched = 	NULL;			

/*	Get shadow attributes.
*/
	s_out = (s_type == XmSHADOW_OUT || s_type == XmSHADOW_ETCHED_OUT)
		? True : False;
	s_etched = (s_type == XmSHADOW_ETCHED_IN ||
		    s_type == XmSHADOW_ETCHED_OUT)
		? True : False;
	top_gc = (s_out)
		? M_TopShadowGC (mgr) : M_BottomShadowGC (mgr);
	bottom_gc = (s_out)
		? M_BottomShadowGC (mgr) : M_TopShadowGC (mgr);

/*	Draw shadow.
*/
	if (s_etched)
		_DtDrawEtchedShadow (d, drawable, top_gc, bottom_gc,
			s_t, x + h_t, y + h_t, w - 2*h_t, h - 2*h_t);
	else
		_XmDrawShadow (d, drawable, top_gc, bottom_gc,
			s_t, x + h_t, y + h_t, w - 2*h_t, h - 2*h_t);
}



/************************************************************************
 *
 *  _DtDrawEtchedShadow
 *	Draw an n segment wide etched shadow on the drawable
 *	d, using the provided GC's and rectangle.
 *
 *  If _XmDrawEtchedShadow weren't static in Manager.c, we wouldn't need this.
 *
 ************************************************************************/

static XRectangle *rects = NULL;
static int rect_count = 0;

void _DtDrawEtchedShadow (display, d, top_GC, bottom_GC, 
                         size, x, y, width, height)
Display * display;
Drawable d;
GC top_GC;
GC bottom_GC;
register int size;
register int x;
register int y;
register int width;
register int height;

{
   int half_size;
   int size2;
   int size3;
   int pos_top, pos_left, pos_bottom, pos_right;

   if (size <= 0) return;
   if (size > width / 2) size = width / 2;
   if (size > height / 2) size = height / 2;
   if (size <= 0) return;

   size = (size % 2) ? (size-1) : (size);

   half_size = size / 2;
   size2 = size + size;
   size3 = size2 + size;
   
   if (rect_count == 0)
   {
      rects = (XRectangle *) XtMalloc (sizeof (XRectangle) * size * 4);
      rect_count = size;
   }

   if (rect_count < size)
   {
      rects = (XRectangle *)
         XtRealloc((char*)rects, sizeof (XRectangle) * size * 4);
      rect_count = size;
   }

   pos_top = 0;
   pos_left = half_size;
   pos_bottom = size2;
   pos_right = size2 + half_size;

   get_rects(half_size, 0, x, y, width, height, 
             pos_top, pos_left, pos_bottom, pos_right);

   pos_top = size3;
   pos_left = size3 + half_size;
   pos_bottom = size;
   pos_right = size + half_size;

   get_rects(half_size, half_size, x, y, width, height, 
                pos_top, pos_left, pos_bottom, pos_right);

   XFillRectangles (display, d, bottom_GC, &rects[size2], size2);
   XFillRectangles (display, d, top_GC, &rects[0], size2);
}



static void get_rects(max_i, offset, x, y, width, height, 
               pos_top, pos_left, pos_bottom, pos_right)

int max_i;
register int offset;
register int x;
register int y;
register int width;
register int height;
register int pos_top, pos_left, pos_bottom, pos_right;

{
   register int i;
   register int offsetX2;
   
   for (i = 0; i < max_i; i++, offset++)
   {
      offsetX2 = offset + offset;

      /*  Top segments  */

      rects[pos_top + i].x = x + offset;
      rects[pos_top + i].y = y + offset;
      rects[pos_top + i].width = width - offsetX2 -1;
      rects[pos_top + i].height = 1;


      /*  Left segments  */

      rects[pos_left + i].x = x + offset;
      rects[pos_left + i].y = y + offset;
      rects[pos_left + i].width = 1;
      rects[pos_left + i].height = height - offsetX2 - 1;


      /*  Bottom segments  */

      rects[pos_bottom + i].x = x + offset;
      rects[pos_bottom + i].y = y + height - offset - 1;
      rects[pos_bottom + i].width = width - offsetX2;
      rects[pos_bottom + i].height = 1;


      /*  Right segments  */

      rects[pos_right + i].x = x + width - offset - 1;
      rects[pos_right + i].y = y + offset;
      rects[pos_right + i].width = 1;
      rects[pos_right + i].height = height - offsetX2;
   }
}


