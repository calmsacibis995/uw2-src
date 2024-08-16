/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:TEditGizmo.c	1.1"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/TextEdit.h>
#include <Gizmo/Gizmos.h>
#include "TEditGizmo.h"

#include "ftp.h"

extern Widget		CreateTextEditGizmo();
extern void		FreeTextEditGizmo();
extern Gizmo		CopyTextEditGizmo();
static XtPointer	QueryTextEditGizmo();

GizmoClassRec TextEditGizmoClass[] = {
	"TextEditGizmo",
	CreateTextEditGizmo,	/* Create	*/
	CopyTextEditGizmo,	/* Copy		*/
	FreeTextEditGizmo,	/* Free		*/
	NULL,			/* Map		*/
	NULL,			/* Get		*/
	NULL,			/* Get Menu	*/
	NULL,			/* Build	*/
	NULL,			/* Manipulate	*/
	QueryTextEditGizmo	/* Query	*/
};

static Gizmo
CopyTextEditGizmo (gizmo)
TextEditGizmo *	gizmo;
{
	TextEditGizmo * new = (TextEditGizmo *) MALLOC (sizeof (TextEditGizmo));

	new->name = gizmo->name;
	new->lines = gizmo->lines;

	return (Gizmo)new;
}

static void
FreeTextEditGizmo (gizmo)
TextEditGizmo *	gizmo;
{
	FREE (gizmo);
}

static Widget
CreateTextEditGizmo (parent, g)
Widget		parent;
TextEditGizmo *	g;
{
	g->textedit = XtVaCreateManagedWidget (
		"text edit gizmo",
		textEditWidgetClass,
		parent,
		XtNeditType,		OL_TEXT_READ,
		XtNsourceType,		OL_STRING_SOURCE,
		XtNlinesVisible,	g->lines,
		(String)0
	);
	return g->textedit;
}

Arg          arg[5];

static XtPointer
QueryTextEditGizmo (TextEditGizmo * gizmo, int option, char * name)
{
	if (!name || strcmp(name, gizmo->name) == 0) {
		switch (option) {
			case GetGizmoWidget: {
				return (XtPointer)(gizmo->textedit);
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
