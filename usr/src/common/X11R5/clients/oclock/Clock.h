/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5oclock:Clock.h	1.1"
/*
 * $XConsortium: Clock.h,v 1.7 90/12/01 13:04:08 rws Exp $
 */

#ifndef _XtClock_h
#define _XtClock_h

/***********************************************************************
 *
 * Clock Widget
 *
 ***********************************************************************/

/* Parameters:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 background	     Background		pixel		White
 border		     BorderColor	pixel		Black
 borderWidth	     BorderWidth	int		1
 minute		     Foreground		Pixel		Black
 hour		     Foreground		Pixel		Black
 height		     Height		int		120
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 width		     Width		int		120
 x		     Position		int		0
 y		     Position		int		0

*/

#define XtNminute	"minute"
#define XtNhour		"hour"
#define XtNjewel	"jewel"

#define XtNshapeWindow	"shapeWindow"
#define XtCShapeWindow	"ShapeWindow"
#define XtNtransparent	"transparent"
#define XtCTransparent	"Transparent"

#define XtNjewelSize	"jewelSize"
#define XtNborderSize	"borderSize"
#define XtCBorderSize	"BorderSize"

typedef struct _ClockRec *ClockWidget;  /* completely defined in ClockPrivate.h */
typedef struct _ClockClassRec *ClockWidgetClass;    /* completely defined in ClockPrivate.h */

extern WidgetClass clockWidgetClass;

#endif /* _XtClock_h */
/* DON'T ADD STUFF AFTER THIS #endif */
