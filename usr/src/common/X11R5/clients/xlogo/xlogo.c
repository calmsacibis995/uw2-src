/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5xlogo:xlogo.c	1.1"
/*
 * $XConsortium: xlogo.c,v 1.15 91/01/10 12:27:35 converse Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 */

#include <X11/Intrinsic.h>
#include <X11/Xaw/Logo.h>
#include <X11/Xaw/Cardinals.h>

extern void exit();
static void quit();

static XrmOptionDescRec options[] = {
{ "-shape", "*shapeWindow", XrmoptionNoArg, (caddr_t) "on" },
};

static XtActionsRec actions[] = {
    {"quit",	quit}
};

static Atom wm_delete_window;

String fallback_resources[] = {
    "*iconPixmap:    xlogo32",
    "*iconMask:      xlogo32",
    NULL,
};

/*
 * Report the syntax for calling xlogo.
 */

static void
Syntax(call)
    char *call;
{
    (void) printf ("Usage: %s [-fg <color>] [-bg <color>] [-rv] %s\n", call, 
		   "[-bw <pixels>] [-bd <color>]");
    (void) printf ("             [-d [<host>]:[<vs>]]\n");
    (void) printf ("             [-g [<width>][x<height>]%s", 
		   "[<+-><xoff>[<+-><yoff>]]]\n");
    (void) printf ("             [-shape]\n");
    (void) printf ("\n");
    exit(1);
}

void 
main(argc, argv)
int argc;
char **argv;
{
    Widget toplevel;
    XtAppContext app_con;

    toplevel = XtAppInitialize(&app_con, "XLogo", options, XtNumber(options), 
			       &argc, argv, fallback_resources, NULL, ZERO);
    if (argc != 1) 
	Syntax(argv[0]);

    XtAppAddActions
	(XtWidgetToApplicationContext(toplevel), actions, XtNumber(actions));
    XtOverrideTranslations
	(toplevel, XtParseTranslationTable ("<Message>WM_PROTOCOLS: quit()"));
    XtCreateManagedWidget("xlogo", logoWidgetClass, toplevel, NULL, ZERO);
    XtRealizeWidget(toplevel);
    wm_delete_window = XInternAtom(XtDisplay(toplevel), "WM_DELETE_WINDOW",
				   False);
    (void) XSetWMProtocols (XtDisplay(toplevel), XtWindow(toplevel),
                            &wm_delete_window, 1);
    XtAppMainLoop(app_con);
}

static void quit(w, event, params, num_params)
    Widget w;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    if (event->type == ClientMessage && 
	event->xclient.data.l[0] != wm_delete_window) {
	XBell(XtDisplay(w), 0);
    } else {
	XCloseDisplay(XtDisplay(w));
	exit(0);
    }
}
