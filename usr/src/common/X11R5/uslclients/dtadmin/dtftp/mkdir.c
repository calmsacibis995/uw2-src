/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtftp:mkdir.c	1.1.1.1"
#endif

#include "ftp.h"
#include "TextFGizmo.h"
#include <Gizmo/LabelGizmo.h>

static void
_MkDirCmd (CmdPtr cp)
{
	char	buf[BUF_SIZE];

	sprintf (buf, "mkdir %s\n", cp->srcfile);
	Output (cp->cr, buf);
}

extern StateTable MkDirTable[];

void
MkDirCmd (CnxtRec *cr, int grp, char *dir, Priority p)
{
	QueueCmd ("mkdir", cr, _MkDirCmd, MkDirTable, grp, dir, 0, 0, 0, p);
}

static void
CreateCB (Widget wid, CnxtRec *cr, XtPointer callData)
{
	OlFlatCallData *	p = (OlFlatCallData *)callData;
	Widget			w;
	char *			destdir;
	int			len = 0;

	if (p->item_index < 2) {
		w = QueryGizmo (
			PopupGizmoClass, cr->newdirPopup, GetGizmoWidget, "new"
		);
		destdir = (char *)OlTextFieldGetString (w, (Cardinal *)&len);
		if (destdir != NULL && destdir[0] != '\0') {
			/* Create the directory */
			MkDirCmd (cr, NextCmdGroup(), destdir, Medium);
			if (p->item_index == 0) {
				/* Cd to that directory */
				CdCmd (cr, destdir, NextCmdGroup(), Medium);
			}
			else {
				/* Update the current directory */
				DirCmd (cr, NextCmdGroup(), Medium);
			}
			MYFREE (destdir);
		}
	}
	CancelCB (wid, cr, callData);
}

static MenuItems newItems [] = {
	{True, BUT_CREATE_OPEN,	MNEM_CREATE_OPEN},
	{True, BUT_CREATE,	MNEM_CREATE},
	{True, BUT_CANCEL,	MNEM_CANCEL},
	{True, BUT_HELP,	MNEM_HELP},
	{NULL}
};

static MenuGizmo newMenu = {
	NULL, "newMenu", "na", newItems, CreateCB,
	NULL, CMD, OL_FIXEDROWS, 1, 1
};

static TextFieldGizmo new = {
	"new", " ", 40
};

static GizmoRec newArray[] = {
	{TextFieldGizmoClass,	&new}
};

static LabelGizmo newLabel = {
	NULL, "newLabel", TXT_ENTER_DIRNAME,
	newArray, XtNumber (newArray),
	OL_FIXEDROWS, 1,
	0, 0,
	False
};

static GizmoRec newdirArray [] = {
	{LabelGizmoClass,	&newLabel},
};

static PopupGizmo newdirPopup = {
	NULL,
	"new dir",
	TXT_NEW_DIR,
	&newMenu,
	newdirArray,
	XtNumber (newdirArray)
};

void
NewFolderCB (Widget wid, XtPointer clientData, XtPointer callData)
{
	Widget		w;
	CnxtRec *	cr = FindCnxtRec (wid);

	if (cr->newdirPopup == (PopupGizmo *)0) {
		cr->newdirPopup = CopyGizmo (
			PopupGizmoClass, &newdirPopup
		);
		CreateGizmo (
			GetBaseWindowShell (cr->base),
			PopupGizmoClass, cr->newdirPopup, 0, 0
		);
		XtUnmanageChild (cr->newdirPopup->message);
	}
	w = (Widget)QueryGizmo (
		PopupGizmoClass, cr->newdirPopup, GetGizmoWidget, "newMenu"
	);
	OlVaFlatSetValues (w, 0, XtNclientData, cr, (String)0);
	OlVaFlatSetValues (w, 1, XtNclientData, cr, (String)0);
	OlVaFlatSetValues (w, 2, XtNclientData, cr, (String)0);
	MapGizmo (PopupGizmoClass, cr->newdirPopup);
}
