/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)panes:Panes.h	1.7"
#endif

#ifndef _PANES_H
#define _PANES_H

#include "Xol/Manager.h"

extern WidgetClass			panesWidgetClass;

typedef struct _PanesClassRec *		PanesWidgetClass;
typedef struct _PanesRec *		PanesWidget;
typedef struct _PanesConstraintRec *	PanesConstraints;

typedef struct OlQueryGeometryCallData {
	Widget			w;
	XtWidgetGeometry *	suggested;
	XtWidgetGeometry *	preferred;
	XtGeometryResult	result;
}			OlQueryGeometryCallData;

#endif
