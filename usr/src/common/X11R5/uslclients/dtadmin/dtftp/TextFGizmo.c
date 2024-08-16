/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:TextFGizmo.c	1.1"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/StaticText.h>
#include <Xol/TextField.h>
#include <Xol/TextEdit.h>
#include <Xol/ScrolledWi.h>
#include <Gizmo/Gizmos.h>
#include "TextFGizmo.h"

#ifdef DEBUG
#include "ftp.h"
#endif

extern Widget		CreateTextFieldGizmo();
extern void		FreeTextFieldGizmo();
extern Gizmo		CopyTextFieldGizmo();
static XtPointer	QueryTextFieldGizmo();

GizmoClassRec TextFieldGizmoClass[] = {
	"TextFieldGizmo",
	CreateTextFieldGizmo,	/* Create	*/
	CopyTextFieldGizmo,	/* Copy		*/
	FreeTextFieldGizmo,	/* Free		*/
	NULL,			/* Map		*/
	NULL,			/* Get		*/
	NULL,			/* Get Menu	*/
	NULL,			/* Build	*/
	NULL,			/* Manipulate	*/
	QueryTextFieldGizmo	/* Query	*/
};

static Gizmo
CopyTextFieldGizmo (gizmo)
TextFieldGizmo *	gizmo;
{
	TextFieldGizmo * new = (TextFieldGizmo *) MALLOC (sizeof (TextFieldGizmo));

	new->name = gizmo->name;
	new->source = STRDUP (gizmo->source);
	new->width = gizmo->width;
	new->textField = (Widget)0;

	return (Gizmo)new;
}

static void
FreeTextFieldGizmo (gizmo)
TextFieldGizmo *	gizmo;
{
	FREE (gizmo->source);
	FREE (gizmo);
}

static Widget
CreateTextFieldGizmo (parent, g)
Widget		parent;
TextFieldGizmo *	g;
{
	g->textField = XtVaCreateManagedWidget (
		"text edit",
		textFieldWidgetClass,
		parent,
		XtNcharsVisible,	(XtArgVal)g->width,
		XtNsource,		(XtArgVal)g->source,
		(String)0
	);
	return g->textField;
}

void
SetTextFieldValue (g, val)
TextFieldGizmo *	g;
char *		val;
{
	XtVaSetValues (g->textField, XtNsource, val, (String)0);
}

char *
GetTextFieldValue (g)
TextFieldGizmo *	g;
{
	char * text;

	OlTextEditCopyBuffer (g->textField, &text);
	return text;
}

static XtPointer
QueryTextFieldGizmo (TextFieldGizmo * gizmo, int option, char * name)
{
	if (!name || strcmp(name, gizmo->name) == 0) {
		switch (option) {
			case GetGizmoWidget: {
				return (XtPointer)(gizmo->textField);
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
