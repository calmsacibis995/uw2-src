/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:open.c	1.1.3.1"
#endif

#include "ftp.h"
#include <Gizmo/LabelGizmo.h>
#include "TextFGizmo.h"

void
Execute (char *name, char *value)
{
	DtPropList	plist;
	char *		buf;

	plist.ptr = 0;
	plist.count = 0;
	DtAddProperty (&plist, "F", name, NULL);
	DtAddProperty (&plist, "f", name, NULL);
	DtAddProperty (
		&plist, "_DEFAULT_PRINTER", ftp->defaultPrinter, NULL
	);
	buf = DtExpandProperty (value, &plist);
	FPRINTF ((stderr, "DtExecuteShellCmd (%s)\n", buf));
	if (DtExecuteShellCmd (buf) != 1) {
		FPRINTF ((stderr, "Exec failed\n"));
	}
	MYFREE (name);
}

static void
GetDefaultOpenProp (CnxtRec *cr, DtReply *reply, char *name)
{
	char *	openCmd;

	openCmd = DtGetProperty (
		&reply->query_fclass.plist, "_Open", NULL
	);
	FPRINTF ((stderr, "DEFAULT OPEN = %s\n", openCmd));
	Execute (name, openCmd);
	DtFreeReply (reply);
}


static void
GetDefaultOpen (CnxtRec *cr, char *file)
{
	GetDataProperties (cr, GetDefaultOpenProp, file);
}

/*
 * Retrieve the open property for the given file.
 * If there is no such property then attempt to get the 
 * open property for the file class DATA.
 */
static void
GetOpenProp (CnxtRec *cr, DtReply *reply, XtPointer clientData)
{
	DtPropList	plist;
	char *		value = 0;
	char *		openProp;

	plist.ptr = 0;
	plist.count = 0;
	DtAddProperty (&plist, "F", reply->get_fclass.file_name, NULL);
	DtAddProperty (
		&plist, "f",
		(char *)basename(reply->get_fclass.file_name), NULL
	);
	openProp = DtGetProperty (&reply->get_fclass.plist, "_Open", NULL);
	if (openProp != NULL) {
		value = (char *)DtExpandProperty (openProp, &plist);
	}
	if (value == NULL) {
		/* This file has no open property so use the default */
		/* DATA _Open property. */
		GetDefaultOpen (cr, STRDUP (reply->get_fclass.file_name));
	}
	else if (DtExecuteShellCmd (value) != 1) {
		FPRINTF ((stderr, "Exec failed\n"));
	}
	DtFreeReply (reply);
}

static void
_EditCmd (CmdPtr cp)
{
	DropObject *	dobj = (DropObject *)cp->clientData;

	/* dobj->destfile can be NULL because of a get error. */
	if (dobj->destfiles[dobj->fileindex-1] != NULL) {
		/* Queue a request to retrieve the open */
		/* property for this file */
		QueueGetFileClassRequest (
			GetBaseWindowShell (cp->cr->base), GetOpenProp,
			cp->srcfile, DT_NO_FILETYPE, (XtPointer)0
		);
	}
	FreeDobj (dobj);
}

void
EditCmd (CnxtRec *cr, char *name, DropObject *dobj)
{
	/* The name can be NULL because the last few files copies */
	/* could have been canceled (No Overwrite). */
	/* It is neccessary to call dobj->cmd for closure. */
	if (name != NULL) {
		QueueCmd (
			"edit", cr, _EditCmd, 0, cr->top->group, name, 0,
			(XtPointer)dobj, 0, Low
		);
	}
}

/* Only one modal can be display at once so it's ok top make ItemIndex */
/* global to the file. */

static int	ItemIndex;

static void
OpenFileContinuation (CnxtRec *cr)
{
	DropObject *	dobj;

	dobj = (DropObject *)CALLOC (1, sizeof (DropObject));
	if (dobj == NULL) {
		return;
	}
	dobj->cmd = EditCmd;
	CopyOutside (cr, dobj, ItemIndex, cr->tmpdir);
}

void
OpenFile (CnxtRec *cr, DmObjectPtr op, int itemIndex)
{
	char		buf[BUF_SIZE];

	/* If only one item is selected and it's a directory *
	/* then just change directories. */
	if (op->ftype == DM_FTYPE_DIR) {
		sprintf (buf, "%s/%s", cr->pwd, op->name);
		CdCmd (cr, buf, NextCmdGroup(), Medium);
	}
	else {
		ItemIndex = itemIndex;
		ReadOnlyWarning (cr, OpenFileContinuation);
	}
}

void
OpenPrint (CnxtRec *cr, PFV func)
{
	DmContainerPtr	cntrp = cr->dir.container;
	DmObjectPtr	op;
	int		i;
	Boolean		set;
	DmItemPtr	item;
	char		tmp[BUF_SIZE];

	item = cr->dir.itp;
	/* Look for at least one selected file */
	for (i=0; i<cntrp->num_objs; item++, i++) {
		if (ITEM_MANAGED(item) && ITEM_SELECT(item)) {
			op = (DmObjectPtr)item->object_ptr;
			/* Indicate all selected files with "-1" */
			(func) (cr, op, -1);
			break;
		}
	}
}

void
OpenCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *	cr = FindCnxtRec (wid);

	OpenPrint (cr, OpenFile);
}

#ifdef DEBUG
static void
OtherFolderOpenCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *		cr = FindCnxtRec (wid);
	char *			filename;

	/* Get the name of the folder */
	XtVaGetValues (
		cr->otherPopup->textFieldWidget,
		XtNstring, &filename, (String)0
	);
	if (filename[0] != '/') {
		filename = GetFilePath (cr->otherPopup);
	}


	BringDownPopup (GetFileGizmoShell (cr->otherPopup));
	CdCmd (cr, filename, NextCmdGroup(), Medium);
}

static MenuItems otherFolderItems[] = {
	{True, BUT_OPEN,	MNEM_OPEN,	 NULL, OtherFolderOpenCB},
	{True, BUT_CANCEL,	MNEM_CANCEL,	 NULL, CancelCB},
	{True, BUT_HELP,	MNEM_HELP,	 NULL, NULL},
	{NULL}
};

MenuGizmo otherFolderMenu = {
	NULL, "otherFolderMenu", NULL, otherFolderItems, 0,
	NULL, CMD, OL_FIXEDROWS, 1, 0
};

static FileGizmo otherFolder = {
	0,
	"folder",
	TXT_OPEN_FOLDER,
	&otherFolderMenu,
	"", "", "", FOLDERS_ONLY
};

void
OtherFolder (CnxtRec *cr)
{
	if (cr->otherPopup == (FileGizmo *)0) {
		cr->otherPopup = CopyGizmo (FileGizmoClass, &otherFolder);
		CreateGizmo (
			GetBaseWindowShell (cr->base), FileGizmoClass,
			cr->otherPopup, 0, 0
		);
	}
	MapGizmo (FileGizmoClass, cr->otherPopup);
}
#endif /* DEBUG */

static void
ParentDir (CnxtRec *cr)
{
	CdCmd (cr, "..", NextCmdGroup(), Medium);
}

void
ParentDirCB (Widget widget, XtPointer clientData, XtPointer callData)
{
	CnxtRec *		cr = FindCnxtRec(widget);

	ParentDir (cr);
}

void
FoldersCB (Widget widget, XtPointer clientData, XtPointer callData)
{
	CnxtRec *		cr = FindCnxtRec(widget);
	OlFlatCallData *	d = (OlFlatCallData *)callData;
	static MenuGizmo *	g = (MenuGizmo *)0;
	int			grp = NextCmdGroup();
	char *			path;

	if (g == (MenuGizmo *)0) {
		g = QueryGizmo (
			BaseWindowGizmoClass, cr->base, GetGizmoGizmo,
			"foldersMenu"
		);
	}
	path = g->items[d->item_index].label;
	if (d->item_index == 0) {
		ParentDir (cr);
	}
#ifdef DEBUG
	else if (d->item_index == 1) {
		OtherFolder (cr);
	}
#endif
	else {
		CdCmd (cr, path, grp, Medium);
	}
}

static void
RenameCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *	cr = FindCnxtRec (widget);
	char *		oldname = (char *)client_data;
	char *		newname;
	Widget		w;
	int		len;

	CancelCB (widget, client_data, call_data);
	w = QueryGizmo (
		PopupGizmoClass, cr->renamePopup, GetGizmoWidget, "rename"
	);
	newname = (char *)OlTextFieldGetString (w, (Cardinal *)&len);

	RenameCmd (cr, oldname, newname);
	MYFREE (newname);
}

static MenuItems  renameMenuItems[] = {
	{ True, BUT_RENAME,	MNEM_RENAME,	NULL, RenameCB },
	{ True, BUT_CANCEL,	MNEM_CANCEL,	NULL, CancelCB },
	{ True, BUT_HELP,	MNEM_HELP,	NULL, NULL },
	{ NULL }
};

static MenuGizmo renameMenu = {
	NULL, "renameMenu", NULL, renameMenuItems, NULL, NULL, CMD,
	OL_FIXEDROWS, 1, 0
};

static TextFieldGizmo renameText = {
	"rename", "", 40
};

static GizmoRec renameTextArray [] = {
	{TextFieldGizmoClass,	&renameText}
};

static LabelGizmo renameCaption = {
	NULL, "renameLabel", TXT_RENAME_CAPTION,
	renameTextArray, XtNumber(renameTextArray), OL_FIXEDCOLS, 1
};

static GizmoRec renameArray[] = {
	{LabelGizmoClass, &renameCaption}
};

static PopupGizmo renamePopup = {
	NULL, "renamePopup", " ", &renameMenu,
	renameArray, XtNumber(renameArray)
};

void
EditRenameCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *	cr = FindCnxtRec (widget);
	Widget		w;
	char *		name = NULL;
	DmItemPtr	item;
	DmContainerPtr	cntrp = cr->dir.container;
	DmObjectPtr	op;
	int		i;
	char		buf[BUF_SIZE];

	/* Get the name of the file */
	item = cr->dir.itp;
	for (i=0; i<cntrp->num_objs; item++, i++) {
		if (ITEM_MANAGED(item) && ITEM_SELECT(item)) {
			op = (DmObjectPtr)item->object_ptr;
			name = op->name;
			break;
		}
	}
	if (name == NULL) {
		return;
	}

	/* Create the popup */
	if (cr->renamePopup == (PopupGizmo *)0) {
		cr->renamePopup = CopyGizmo (
			PopupGizmoClass, &renamePopup
		);
		CreateGizmo (
			GetBaseWindowShell (cr->base),
			PopupGizmoClass, cr->renamePopup, 0, 0
		);
		sprintf (
			buf,
			GGT(TXT_RENAME_TITLE),
			cr->systemAddr, cr->userName
		);
		XtVaSetValues (
			GetPopupGizmoShell(cr->renamePopup), XtNtitle, buf,
			(String)0
		);
		XtUnmanageChild (cr->renamePopup->message);
	}

	/* Attach this name to the rename menu */
	w = QueryGizmo (
		PopupGizmoClass, cr->renamePopup, GetGizmoWidget, "renameMenu"
	);
	OlVaFlatSetValues (w, 0, XtNclientData, name, (String)0);

	/* Set focus on the rename text field */
	w = QueryGizmo (
		PopupGizmoClass, cr->renamePopup, GetGizmoWidget, "renameMenu"
	);
	if (OlCanAcceptFocus (w, CurrentTime) != False) {
		OlSetInputFocus (w, RevertToParent, CurrentTime);
	}

	/* Display the popup */
	MapGizmo (PopupGizmoClass, cr->renamePopup);
}
