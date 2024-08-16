/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:Exclusives.c	1.1"
#endif

/*  Example of the Exclusives widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/Caption.h>
#include <Xol/OblongButt.h>
#include <Xol/RectButton.h>
#include <Xol/Exclusives.h>

#include "WidgetTree.h"

/*ARGSUSED*/
void
ExclusivesCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, caption, exclusives;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "Exclusives",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup,
		XtNupperControlArea, &upper,
		XtNlowerControlArea, &lower,
		(String) 0);

        caption = XtVaCreateManagedWidget("caption", captionWidgetClass, upper, 
			XtNlabel, "Exclusives:",
			XtNfont, (XFontStruct *) _OlGetDefaultFont(upper, OlDefaultBoldFont),
			(String) 0);

        exclusives = XtCreateManagedWidget("exclusives",
		exclusivesWidgetClass, caption, NULL, 0);

        XtCreateManagedWidget("On", rectButtonWidgetClass,
		exclusives, NULL, 0);

        XtCreateManagedWidget("Off", rectButtonWidgetClass,
		exclusives, NULL, 0);

        XtCreateManagedWidget("OK", oblongButtonWidgetClass,
		lower, NULL, 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "Exclusives.c");

        XtPopup(popup, XtGrabNone);
}  /* end of ExclusivesCB() */
