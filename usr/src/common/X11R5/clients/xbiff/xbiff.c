/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5xbiff:xbiff.c	1.1"
/*
 * $XConsortium: xbiff.c,v 1.17 91/01/10 14:25:32 gildea Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Mailbox.h>
#include <X11/Xaw/Cardinals.h>

extern void exit();
static void quit();

char *ProgramName;

static XrmOptionDescRec options[] = {
{ "-update", "*mailbox.update", XrmoptionSepArg, (caddr_t) NULL },
{ "-file",   "*mailbox.file", XrmoptionSepArg, (caddr_t) NULL },
{ "-volume", "*mailbox.volume", XrmoptionSepArg, (caddr_t) NULL },
{ "-shape",  "*mailbox.shapeWindow", XrmoptionNoArg, (caddr_t) "on" },
};

static XtActionsRec xbiff_actions[] = {
    { "quit", quit },
};
static Atom wm_delete_window;

static void Usage ()
{
    static char *help_message[] = {
"where options include:",
"    -display host:dpy              X server to contact",
"    -geometry geom                 size of mailbox",
"    -file file                     file to watch",
"    -update seconds                how often to check for mail",
"    -volume percentage             how loud to ring the bell",
"    -bg color                      background color",
"    -fg color                      foreground color",
"    -rv                            reverse video",
"    -shape                         shape the window",
NULL};
    char **cpp;

    fprintf (stderr, "usage:  %s [-options ...]\n", ProgramName);
    for (cpp = help_message; *cpp; cpp++) {
	fprintf (stderr, "%s\n", *cpp);
    }
    fprintf (stderr, "\n");
    exit (1);
}


void main (argc, argv)
    int argc;
    char **argv;
{
    XtAppContext xtcontext;
    Widget toplevel, w;

    ProgramName = argv[0];

    toplevel = XtAppInitialize(&xtcontext, "XBiff", options, XtNumber (options),
			       &argc, argv, NULL, NULL, 0);
    if (argc != 1) Usage ();

    /*
     * This is a hack so that f.delete will do something useful in this
     * single-window application.
     */
    XtAppAddActions (xtcontext, xbiff_actions, XtNumber(xbiff_actions));
    XtOverrideTranslations(toplevel,
		   XtParseTranslationTable ("<Message>WM_PROTOCOLS: quit()"));

    w = XtCreateManagedWidget ("mailbox", mailboxWidgetClass, toplevel,
			       NULL, 0);
    XtRealizeWidget (toplevel);
    wm_delete_window = XInternAtom (XtDisplay(toplevel), "WM_DELETE_WINDOW",
                                    False);
    (void) XSetWMProtocols (XtDisplay(toplevel), XtWindow(toplevel),
                            &wm_delete_window, 1);
    XtAppMainLoop (xtcontext);
}

static void quit (w, event, params, num_params)
    Widget w;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    if (event->type == ClientMessage &&
        event->xclient.data.l[0] != wm_delete_window) {
        XBell (XtDisplay(w), 0);
        return;
    }
    XCloseDisplay (XtDisplay(w));
    exit (0);
}
