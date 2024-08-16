/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)scrollwin:ScrolledWi.h	1.12"
#endif

/*
 * ScrolledWi.h
 *
 */

#ifndef _ScrolledWi_h
#define _ScrolledWi_h

typedef struct _ScrolledWindowClassRec	*ScrolledWindowWidgetClass;
typedef struct _ScrolledWindowRec	*ScrolledWindowWidget;

typedef struct _OlSWGeometries
   {
   Widget               sw;
   Widget               vsb;
   Widget               hsb;
   Dimension            bb_border_width;
   Dimension            vsb_width;
   Dimension            vsb_min_height;
   Dimension            hsb_height;
   Dimension            hsb_min_width;
   Dimension            sw_view_width;
   Dimension            sw_view_height;
   Dimension            bbc_width;
   Dimension            bbc_height;
   Dimension            bbc_real_width;
   Dimension            bbc_real_height;
   Boolean              force_hsb;
   Boolean              force_vsb;
   } OlSWGeometries;

extern OlSWGeometries
GetOlSWGeometries OL_ARGS((
	ScrolledWindowWidget
));

extern void
OlLayoutScrolledWindow OL_ARGS((
	ScrolledWindowWidget,
	int			/* being resized? */
));

extern WidgetClass scrolledWindowWidgetClass;

#endif /* _ScrolledWi_h */
