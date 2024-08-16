/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)slider:Slider.h	1.4"
#endif

#ifndef _Slider_h
#define _Slider_h

#include <Xol/Primitive.h>		/* include superclasses' header */

/***********************************************************************
 *
 * Slider Widget (subclass of CompositeClass)
 *
 ***********************************************************************/


/* Class record constants */

extern WidgetClass sliderWidgetClass;


typedef struct _SliderClassRec *SliderWidgetClass;
typedef struct _SliderRec      *SliderWidget;

typedef struct OlSliderVerify {
	int	new_location;
	Boolean	more_cb_pending;
} OlSliderVerify;

#endif /* _Slider_h */
