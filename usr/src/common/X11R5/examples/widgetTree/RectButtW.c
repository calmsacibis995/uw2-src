/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:RectButtW.c	1.1"
#endif

/*  Example of the RectButton widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/OblongButt.h>
#include <Xol/RectButton.h>

#include "WidgetTree.h"

/*ARGSUSED*/
void
RectButtonWidgetCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "RectButton",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup,
		XtNupperControlArea, &upper,
		XtNlowerControlArea, &lower,
		(String) 0);

        XtCreateManagedWidget("Rect Button Widget", rectButtonWidgetClass,
		upper, NULL, 0);

        XtCreateManagedWidget("OK", oblongButtonWidgetClass,
		lower, NULL, 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "RectButtW.c");

        XtPopup(popup, XtGrabNone);
}  /* end of RectButtWidgetCB() */
