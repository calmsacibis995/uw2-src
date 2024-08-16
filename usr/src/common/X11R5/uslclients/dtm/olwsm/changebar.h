/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/changebar.h	1.1"
#endif

#include "WSMcomm.h"

#ifndef _CHANGEBAR_H
#define _CHANGEBAR_H

typedef struct changeBar {
	Widget parent;
	int state;
} ChangeBar;

extern void CreateChangeBar( Widget , ChangeBar* );
extern void SetChangeBarState ( 
	ChangeBar *	bar, 
	int		element,
	int		state,
	int		propagate, 
	void 		(*change)()
);
extern void RedrawChangeBar (
	Widget			w,
	XtPointer		client_data,
	XEvent *		pe,
	Boolean *		continue_to_dispatch
);
extern void DrawChangeBar(Widget, XExposeEvent *, String *, Cardinal *);
#endif
