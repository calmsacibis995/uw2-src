/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)slider:Gauge.h	1.1"
#endif

#ifndef _Gauge_h
#define _Gauge_h

#include <Xol/Primitive.h>		/* include superclasses' header */

/***********************************************************************
 *
 * Gauge Widget (subclass of PrimitiveClass)
 *
 ***********************************************************************/


/* Class record constants */

extern WidgetClass gaugeWidgetClass;


typedef struct _SliderClassRec *GaugeWidgetClass;
typedef struct _SliderRec      *GaugeWidget;

/* extern functions */
extern void OlSetGaugeValue();
/*
 * Widget sw;
 * int    val;
 */

#endif /* _Gauge_h */
