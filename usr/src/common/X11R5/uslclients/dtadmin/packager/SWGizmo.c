/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:packager/SWGizmo.c	1.1"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/ScrolledWi.h>
#include <Gizmo/Gizmos.h>
#include "SWGizmo.h"

extern Widget		CreateScrolledWindowGizmo();
extern void		FreeScrolledWindowGizmo();
extern Gizmo		CopyScrolledWindowGizmo();
static XtPointer	QueryScrolledWindowGizmo();

GizmoClassRec ScrolledWindowGizmoClass[] = {
	"ScrolledWindowGizmo",
	CreateScrolledWindowGizmo,	/* Create	*/
	CopyScrolledWindowGizmo,	/* Copy		*/
	FreeScrolledWindowGizmo,	/* Free		*/
	NULL,				/* Map		*/
	NULL,				/* Get		*/
	NULL,				/* Get Menu	*/
	NULL,				/* Build	*/
	NULL,				/* Manipulate	*/
	QueryScrolledWindowGizmo	/* Query	*/
};

static Gizmo
CopyScrolledWindowGizmo (gizmo)
ScrolledWindowGizmo *	gizmo;
{
	ScrolledWindowGizmo * new = (ScrolledWindowGizmo *) MALLOC (
		sizeof (ScrolledWindowGizmo)
	);

	new->name = gizmo->name;
	new->width = gizmo->width;
	new->height = gizmo->height;

	return (Gizmo)new;
}

static void
FreeScrolledWindowGizmo (gizmo)
ScrolledWindowGizmo *	gizmo;
{
	FREE (gizmo);
}

static Widget
CreateScrolledWindowGizmo (parent, g)
Widget			parent;
ScrolledWindowGizmo *	g;
{
	Arg	args[10];

	XtSetArg (args[0], XtNwidth, g->width);
	XtSetArg (args[1], XtNheight, g->height);
	g->sw = XtCreateManagedWidget (
		"scrolled window gizmo",
		scrolledWindowWidgetClass,
		parent,
		args,
		2
	);
	return g->sw;
}

static XtPointer
QueryScrolledWindowGizmo (ScrolledWindowGizmo * gizmo, int option, char * name)
{
	if (!name || strcmp(name, gizmo->name) == 0) {
		switch (option) {
			case GetGizmoWidget: {
				return (XtPointer)(gizmo->sw);
				break;
			}
			case GetGizmoGizmo: {
				return (XtPointer)(gizmo);
				break;
			}
		}
	}
	else {
		return (XtPointer)NULL;
	}
}
