/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:StatGizmo.c	1.4"
#endif

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Footer.h>
#include <Xol/RubberTile.h>
#include <MGizmo/Gizmo.h>
#include "StatGizmo.h"

static Widget		CreateStatusGizmo();
static void		FreeStatusGizmo();
static Gizmo		CopyStatusGizmo();
static XtPointer	QueryStatusGizmo();

GizmoClassRec StatusGizmoClass[] = {
	"StatusGizmo",
	CreateStatusGizmo,	/* Create	*/
	CopyStatusGizmo,	/* Copy		*/
	FreeStatusGizmo,	/* Free		*/
	NULL,			/* Map		*/
	NULL,			/* Get		*/
	NULL,			/* Get Menu	*/
	NULL,			/* Build	*/
	NULL,			/* Manipulate	*/
	QueryStatusGizmo	/* Query	*/
};

static Gizmo
CopyStatusGizmo(gizmo)
StatusGizmo *gizmo;
{
	StatusGizmo *new = (StatusGizmo *)MALLOC(sizeof(StatusGizmo));

	new->name         = STRDUP(gizmo->name);
	new->left_percent = gizmo->left_percent;
	new->widget = NULL;
	return (Gizmo)new;
}

static void
FreeStatusGizmo(gizmo)
StatusGizmo *gizmo;
{
	FREE(gizmo->name);
	FREE((void *)gizmo);
}

static Widget
CreateStatusGizmo(parent, gizmo)
Widget		parent;
StatusGizmo	*gizmo;
{
	Arg arg[5];
	int i;

	XtSetArg(arg[0], XtNleftFoot,    "left");
	XtSetArg(arg[1], XtNrightFoot,   "right ");
	XtSetArg(arg[2], XtNleftWeight,  gizmo->left_percent);
	XtSetArg(arg[3], XtNrightWeight, 100 - gizmo->left_percent);
	i = 4;
	if (XtIsSubclass(parent, rubberTileWidgetClass) != False) {
		XtSetArg(arg[i], XtNweight, 0);
		i++;
	}

	return(gizmo->widget = XtCreateManagedWidget("statusGizmo",
			 footerWidgetClass, parent, arg, i));
}

static XtPointer
QueryStatusGizmo(StatusGizmo *gizmo, int option, char * name)
{
   if (!name || !strcmp(name, gizmo->name)) {
      switch(option) {
         case GetGizmoSetting:
            return (XtPointer)(NULL);
            break;
         case GetGizmoWidget:
            return (XtPointer)(gizmo->widget);
            break;
         case GetGizmoGizmo:
            return (XtPointer)(gizmo);
            break;
         default:
            return (XtPointer)(NULL);
            break;
      }
   }
   else
      return (XtPointer)(NULL);

} /* end of QueryStatusGizmo */

