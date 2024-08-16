/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)handles:Handles.h	1.3"
#endif

#ifndef _HANDLES_H
#define _HANDLES_H

#include "Xol/EventObj.h"

extern WidgetClass			handlesWidgetClass;

typedef struct _HandlesClassRec *	HandlesWidgetClass;
typedef struct _HandlesRec *		HandlesWidget;

typedef enum OlHandlesPaneResizeReason {
	OlHandlesPaneResizing,
	OlHandlesPaneResized
}			OlHandlesPaneResizeReason;

typedef struct OlHandlesPaneResizeData {
	OlHandlesPaneResizeReason	reason;
	Widget				pane;
	XtWidgetGeometry *		geometry;
}			OlHandlesPaneResizeData;

#endif
