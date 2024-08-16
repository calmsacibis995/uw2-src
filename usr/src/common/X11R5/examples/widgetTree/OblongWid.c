/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:OblongWid.c	1.2"
#endif

/*  Example of the Oblong Button widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/OblongButt.h>

#include "WidgetTree.h"

/*ARGSUSED*/
void
OblongButtonWidgetCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, lower;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "OblongButton",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup, XtNlowerControlArea, &lower, (String) 0);

        XtVaCreateManagedWidget("Oblong Button Widget", oblongButtonWidgetClass,
		lower,
		XtNaccelerator, "Ctrl<z>",
		(String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "OblongWid.c");

        XtPopup(popup, XtGrabNone);
}  /* end of OblongButtonWidgetCB() */
