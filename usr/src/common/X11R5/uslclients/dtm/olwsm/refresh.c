/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/refresh.c	1.7"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>

#include <misc.h>
#include <list.h>
#include <wsm.h>

static Window		CreateCover( Screen * );

/**
 ** RefreshCB()
 **/

void
RefreshCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
	)
{
	static Window		cover = (Window)0;


	if (!cover)
	  {
		cover = CreateCover(SCREEN);
	  }

	XMapRaised (DISPLAY, cover);
	XUnmapWindow (DISPLAY, cover);

	return;
} /* RefreshCB */

/**
 ** CreateCover()
 **/

static Window
CreateCover (
	Screen *		scr
	)
{
	XSetWindowAttributes	xswa;

	xswa.background_pixmap = None;
	xswa.override_redirect = True;

	return (XCreateWindow(
		DisplayOfScreen(scr),
		RootWindowOfScreen(scr),
		0, 0,
		WidthOfScreen(scr), HeightOfScreen(scr),
		0,
		PlanesOfScreen(scr),
		InputOutput,
		CopyFromParent,
		CWBackPixmap | CWOverrideRedirect,
		&xswa
	));
} /* CreateCover */
