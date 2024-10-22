/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:AbbrevMenu.c	1.1"
#endif

/*  Example of the AbbreviatedMenuButton widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/Caption.h>
#include <Xol/OblongButt.h>
#include <Xol/AbbrevMenu.h>

#include "WidgetTree.h"

/*ARGSUSED*/
void
AbbreviatedMenuCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, caption, abbrevmenu, pane;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "AbbreviatedMenuButton",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup,
		XtNupperControlArea, &upper,
		XtNlowerControlArea, &lower,
		(String) 0);

	caption = XtVaCreateManagedWidget("caption", captionWidgetClass, upper,
		XtNlabel, "AbbreviatedMenuButton:",
		XtNfont, (XFontStruct *) _OlGetDefaultFont(upper, OlDefaultBoldFont),
		(String) 0);

        abbrevmenu = XtVaCreateManagedWidget("abbrevmenu",
		abbrevMenuButtonWidgetClass, caption,
		(String)0);

        XtVaGetValues(abbrevmenu, XtNmenuPane, &pane, (String) 0);

        XtCreateManagedWidget("Open...", oblongButtonWidgetClass,
		pane, NULL, 0);

        XtCreateManagedWidget("Save...", oblongButtonWidgetClass,
		pane, NULL, 0);

        XtCreateManagedWidget("Exit", oblongButtonWidgetClass,
		pane, NULL, 0);

        XtCreateManagedWidget("OK", oblongButtonWidgetClass,
		lower, NULL, 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "AbbrevMenu.c");

        XtPopup(popup, XtGrabNone);
}  /* end of AbbreviatedMenuCB() */
