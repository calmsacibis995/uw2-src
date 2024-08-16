/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef	NOIDENT
#ident	"@(#)olexamples:tutorial/s_button.c	1.6"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/OblongButt.h>

void 
QuitCallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	exit(0);
}


int 
main(argc, argv)
int argc;
char **argv;
{

	Widget	toplevel, quitButton;
	Arg	args[10];
	int	n;

	toplevel = OlInitialize("top", "Top", NULL, 0, &argc, argv);

	n = 0;
	XtSetArg(args[n], XtNlabel, "Quit");		n++;

	quitButton = XtCreateManagedWidget(	"button", 
						oblongButtonWidgetClass, 
						toplevel, args, n);

	XtAddCallback(quitButton, XtNselect, (XtCallbackProc)QuitCallback, NULL);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
