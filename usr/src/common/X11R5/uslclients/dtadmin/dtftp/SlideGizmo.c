/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:SlideGizmo.c	1.1.1.1"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/Slider.h>
#include <Xol/RubberTile.h>
#include <Xol/Footer.h>
#include <Gizmo/Gizmos.h>
#include "SlideGizmo.h"

#include "ftp.h"

extern Widget		CreateSliderGizmo();
extern void		FreeSliderGizmo();
extern Gizmo		CopySliderGizmo();
static XtPointer	QuerySliderGizmo();

GizmoClassRec SliderGizmoClass[] = {
	"SliderGizmo",
	CreateSliderGizmo,	/* Create	*/
	CopySliderGizmo,	/* Copy		*/
	FreeSliderGizmo,	/* Free		*/
	NULL,			/* Map		*/
	NULL,			/* Get		*/
	NULL,			/* Get Menu	*/
	NULL,			/* Build	*/
	NULL,			/* Manipulate	*/
	QuerySliderGizmo	/* Query	*/
};

static Gizmo
CopySliderGizmo (gizmo)
SliderGizmo *	gizmo;
{
	SliderGizmo * new = (SliderGizmo *) MALLOC (sizeof (SliderGizmo));

	new->name = gizmo->name;

	return (Gizmo)new;
}

static void
FreeSliderGizmo (gizmo)
SliderGizmo *	gizmo;
{
	FREE (gizmo);
}

static Widget
CreateSliderGizmo (parent, g)
Widget		parent;
SliderGizmo *	g;
{
	Widget	rubber;
	Widget	footer;

	rubber = XtVaCreateManagedWidget (
		"slider rubber",
		rubberTileWidgetClass,
		parent,
		XtNorientation,		OL_VERTICAL,
		XtNshadowThickness,	0,
		XtNpaneGravity,		EastWestGravity,
		(String)0
	);
	g->slider = XtVaCreateManagedWidget (
		"slider gizmo",
		sliderWidgetClass,
		rubber,
		XtNsliderMin,		(XtArgVal)0,
		XtNsliderMax,		(XtArgVal)100,
		XtNsensitive,		False,
		XtNrecomputeSize,	True,
		XtNorientation,		(XtArgVal)OL_HORIZONTAL,
		(String)0
	);
	footer = XtVaCreateManagedWidget (
		"slider footer",
		footerWidgetClass,
		rubber,
		XtNweight,		0,
		XtNleftFoot,		(XtArgVal)GGT(TXT_SLIDER_MIN),
		XtNrightFoot,		(XtArgVal)GGT(TXT_SLIDER_MAX),
		XtNleftWeight,		5,
		XtNrightWeight,		1,
		(String)0
	);
	return g->slider;
}

Arg          arg[5];

void
SetSliderValue (g, val)
SliderGizmo *	g;
int	val;
{
	XtVaSetValues (g->slider, XtNsliderValue, val, (String)0);
}

static XtPointer
QuerySliderGizmo (SliderGizmo * gizmo, int option, char * name)
{
	if (!name || strcmp(name, gizmo->name) == 0) {
		switch (option) {
			case GetGizmoWidget: {
				return (XtPointer)(gizmo->slider);
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
