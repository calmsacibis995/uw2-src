/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:statictext/unit_test7.c	1.6"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/ControlAre.h>
#include <Xol/OpenLook.h>
#include <Xol/StaticText.h>

static char string1[] = "All the world's a stage and all the pepole merely players.";

void main (argc, argv)
int argc;
char **argv;
{
	Widget toplevel, box;
	Arg arg[20];
	unsigned int n;

	toplevel = OlInitialize("quitButton",
		"QuitButton",
		NULL,
		0,
		&argc,
		argv);

	n = 0;
	XtSetArg(arg[n], XtNlayoutType, OL_FIXEDCOLS);		n++;
	box = XtCreateManagedWidget("box",
		controlAreaWidgetClass,
		toplevel,
		arg,
		n);

/*
 *  OLXTK-31.9
 */

	n = 0;
	XtSetArg(arg[n], XtNstring, string1);			n++;
	XtSetArg(arg[n], XtNwidth, 120);			n++;
	XtSetArg(arg[n], XtNalignment, OL_LEFT);		n++;
	XtCreateManagedWidget("Static Text",
		staticTextWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string1);			n++;
	XtSetArg(arg[n], XtNwidth, 120);			n++;
	XtSetArg(arg[n], XtNalignment, OL_RIGHT);		n++;
	XtCreateManagedWidget("Static Text",
		staticTextWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string1);			n++;
	XtSetArg(arg[n], XtNwidth, 120);			n++;
	XtSetArg(arg[n], XtNalignment, OL_CENTER);	n++;
	XtCreateManagedWidget("Static Text",
		staticTextWidgetClass,
		box,
		arg,
		n);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
