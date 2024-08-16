/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:RubberGizmo.c	1.1"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/RubberTile.h>
#include <Gizmo/Gizmos.h>
#include "RubberGizmo.h"

#include "ftp.h"

extern Widget		CreateRubberGizmo();
extern void		FreeRubberGizmo();
extern Gizmo		CopyRubberGizmo();
static XtPointer	QueryRubberGizmo();
static void		ManipulateRubberGizmo();

GizmoClassRec RubberGizmoClass[] = {
	"RubberGizmo",
	CreateRubberGizmo,	/* Create	*/
	CopyRubberGizmo,	/* Copy		*/
	FreeRubberGizmo,	/* Free		*/
	NULL,			/* Map		*/
	NULL,			/* Get		*/
	NULL,			/* Get Menu	*/
	NULL,			/* Build	*/
	ManipulateRubberGizmo,	/* Manipulate	*/
	QueryRubberGizmo	/* Query	*/
};

static Gizmo
CopyRubberGizmo (gizmo)
RubberGizmo *	gizmo;
{
	RubberGizmo *	new = (RubberGizmo *) MALLOC (sizeof (RubberGizmo));
	RubberGizmo *	old = (RubberGizmo*)gizmo;

	new->name = gizmo->name;
	CopyGizmoArray (
		&new->gizmos, &new->numGizmos, old->gizmos, old->numGizmos
	);
	return (Gizmo)new;
}

static void
FreeRubberGizmo (gizmo)
RubberGizmo *	gizmo;
{
	RubberGizmo *	old = (RubberGizmo*)gizmo;

	FREE (gizmo);
	FreeGizmoArray (old->gizmos, old->numGizmos);
}

static Widget
CreateRubberGizmo (parent, g)
Widget		parent;
RubberGizmo *	g;
{
	Widget	rubber;
	Widget	footer;

	rubber = XtVaCreateManagedWidget (
		"rubber",
		rubberTileWidgetClass,
		parent,
		XtNorientation,		OL_VERTICAL,
		XtNshadowThickness,	0,
		XtNpaneType,		OL_HANDLES,
		/*
		XtNpaneGravity,		EastWestGravity,
		*/
		(String)0
	);
	if (g->numGizmos > 0) {
		CreateGizmoArray (rubber, g->gizmos, g->numGizmos);
	}

	return g->rubber;
}
static void
ManipulateRubberGizmo (RubberGizmo *g, ManipulateOption option)
{
	GizmoArray	gp = g->gizmos;
	int		i;

	for (i=0; i<g->numGizmos; i++) {
		ManipulateGizmo (gp[i].gizmo_class, gp[i].gizmo, option);
	}

}

static XtPointer
QueryRubberGizmo (RubberGizmo * gizmo, int option, char * name)
{
	if (!name || strcmp(name, gizmo->name) == 0) {
		switch (option) {
			case GetGizmoWidget: {
				return (XtPointer)(gizmo->rubber);
				break;
			}
			case GetGizmoGizmo: {
				return (XtPointer)(gizmo);
				break;
			}
		}
	}
	else {
		return QueryGizmoArray (
			gizmo->gizmos, gizmo->numGizmos, option, name
		);

	}
}
