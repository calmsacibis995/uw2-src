/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:copy.c	1.1.1.1"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "ftp.h"
#include <Gizmo/SpaceGizmo.h>
#include <Gizmo/STextGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include "SlideGizmo.h"
#include <libgen.h>

void
StopCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *		cr = client_data;
	SliderGizmo *		sg;

	sg = (SliderGizmo *) QueryGizmo (
		ModalGizmoClass, cr->sliderPopup, GetGizmoGizmo, "slider"
	);
	FPRINTF ((stderr, "STOPCB\n"));
	if (sg->canceled == False) {
		/* Don't allow a second cancel */
		kill (cr->pid, SIGINT);
		FPRINTF ((stderr, "KILL %d\n", cr->pid));
		sg->canceled = True;
		cr->top->canceled = True;
		FPRINTF ((stderr, "ftp->suspended = %d\n", ftp->suspended));
	}
}

static SliderGizmo slider = {
	"slider",
};

static StaticTextGizmo copying1 = {
	NULL, "copying1", "", NorthWestGravity, NULL
};
static StaticTextGizmo copying2 = {
	NULL, "copying2", "", NorthWestGravity, NULL
};
static StaticTextGizmo copying3 = {
	NULL, "copying3", "", NorthWestGravity, NULL
};
static StaticTextGizmo copying4 = {
	NULL, "copying4", "", NorthWestGravity, NULL
};

static GizmoRec array1[] = {
	{StaticTextGizmoClass,	&copying1}
};
static GizmoRec array2[] = {
	{StaticTextGizmoClass,	&copying2}
};
static GizmoRec array3[] = {
	{StaticTextGizmoClass,	&copying3}
};
static GizmoRec array4[] = {
	{StaticTextGizmoClass,	&copying4}
};

static LabelGizmo label1 = {
	NULL, "label1", TXT_COPYING_LABEL, array1, XtNumber (array1),
	OL_FIXEDROWS, 1, NULL, 0, True
};
static LabelGizmo label2 = {
	NULL, "label2", TXT_COPYING_FROM_LABEL, array2, XtNumber (array2),
	OL_FIXEDROWS, 1, NULL, 0, True
};
static LabelGizmo label3 = {
	NULL, "label3", TXT_COPYING_TO_LABEL, array3, XtNumber (array3),
	OL_FIXEDROWS, 1, NULL, 0, True
};
static LabelGizmo label4 = {
	NULL, "label4", TXT_COPYING_ON_LABEL, array4, XtNumber (array4),
	OL_FIXEDROWS, 1, NULL, 0, True
};
static SpaceGizmo space1 = {
	3, 3
};


static GizmoRec copyArray[] = {
	{LabelGizmoClass,	&label1},
	{LabelGizmoClass,	&label2},
	{SpaceGizmoClass,	&space1},
	{LabelGizmoClass,	&label3},
	{LabelGizmoClass,	&label4},
	{SliderGizmoClass,	&slider},
};

static MenuItems copyItems[] = {
	{True, BUT_STOP,	MNEM_STOP,	NULL,	StopCB},
	{True, BUT_HELP,	MNEM_HELP,	NULL,	NULL},
	{NULL}
};

static MenuGizmo copyMenu = {
	NULL,			/* Help		*/
	"copyMenu",		/* Name		*/
	"copyMenu",		/* Title	*/
	copyItems,		/* Items	*/
	NULL,			/* Function	*/
	NULL,			/* Client data	*/
	CMD,			/* Button type	*/
	OL_FIXEDROWS,		/* Layout type	*/
	1,			/* Measure	*/
	0			/* Default item */
};

static ModalGizmo sliderPopup = {
	NULL,
	"copyPopup",
	TXT_FILE_TRANS,
	&copyMenu,
	NULL,
	copyArray,
	XtNumber (copyArray)
};

static void
SetSliderSystemNames (CnxtRec *cr, char *src, char *dest)
{
	char			buf[BUF_SIZE];
	StaticTextGizmo *	g;

	sprintf (buf, GGT(TXT_COPYING_FROM), src);
	g = (StaticTextGizmo *)QueryGizmo (
		ModalGizmoClass, cr->sliderPopup, GetGizmoGizmo, "copying2"
	);
	SetStaticTextGizmoText (g, src);

	sprintf (buf, GGT(TXT_COPYING_ON), dest);
	g = (StaticTextGizmo *)QueryGizmo (
		ModalGizmoClass, cr->sliderPopup, GetGizmoGizmo, "copying4"
	);
	SetStaticTextGizmoText (g, buf);
}

void
CreateSliderPopup (CmdPtr cp)
{
	StaticTextGizmo *	g;
	char			buf[BUF_SIZE];
	Widget			menuWidget;
	Widget			shell;
	SliderGizmo *		sg;
	CnxtRec *		cr = cp->cr;

	if (cr->sliderPopup == (ModalGizmo *)0) {
		cr->sliderPopup = CopyGizmo (
			ModalGizmoClass, &sliderPopup
		);
		CreateGizmo (
			GetBaseWindowShell (cr->base),
			ModalGizmoClass, cr->sliderPopup, NULL, 0
		);
	}
	sg = (SliderGizmo *) QueryGizmo (
		ModalGizmoClass, cr->sliderPopup, GetGizmoGizmo, "slider"
	);
	/* Indicate StopCB can be called */
	sg->canceled = False;

	SetSliderSystemNames (cr, cp->srcfile, cp->destfile);
	SetSliderFilenames (cr, " ", " ");
	CenterLabel (cr->sliderPopup, ModalGizmoClass, "label1");
	CenterLabel (cr->sliderPopup, ModalGizmoClass, "label2");
	CenterLabel (cr->sliderPopup, ModalGizmoClass, "label3");
	CenterLabel (cr->sliderPopup, ModalGizmoClass, "label4");

	/* Turn off box around gizmo array */
	XtVaSetValues (
		cr->sliderPopup->control, XtNshadowThickness, 0, (String)0
	);

	/* Popup the modal without grabbing the pointer so */
	/* other things in the application can be accessed. */
	shell = GetModalGizmoShell (cr->sliderPopup);
	XtPopup (shell, XtGrabNone);
	XRaiseWindow (XtDisplay(shell), XtWindow(shell));
	cr->sliderUp = True;

	/* Display the working icon */
	XtVaSetValues (shell, XtNnoticeType, OL_WORKING, (String)0);
}

/*
 * Return the folder name based on the destination file name
 * and the ftp->showFullPaths setting.
 */
static char *
FolderName (CnxtRec *cr, char *destfile)
{
	static char	buf[BUF_SIZE];
	char *		cp;

	strcpy (buf, destfile);
	cp = strrchr (buf, '/');
	if (cp == NULL) {
		return buf;
	}
	*cp = '\0';
	if (ftp->showFullPaths == True) {
		return buf;
	}
	cp = strrchr (buf, '/');
	if (cp == NULL) {
		return buf;
	}
	return cp+1;
}

void
SetSliderFilenames (CnxtRec *cr, char *srcfile, char *destfile)
{
	SliderGizmo *		sg;
	StaticTextGizmo *	g;
	char			buf[BUF_SIZE];
	Widget			menuWidget;

	menuWidget = (Widget)QueryGizmo (
		ModalGizmoClass, cr->sliderPopup, GetGizmoWidget, "copyMenu"
	);
	/* Remember connection for the stop command button */
	OlVaFlatSetValues (
		menuWidget, 0, XtNclientData, (XtPointer)cr, (String)0
	);

	/* Set text of first field */
	sprintf (
		buf,
		GGT (TXT_COPYING),
		(ftp->showFullPaths == True) ? srcfile : basename (srcfile)
	);
	g = (StaticTextGizmo *)QueryGizmo (
		ModalGizmoClass, cr->sliderPopup,
		GetGizmoGizmo, "copying1"
	);
	SetStaticTextGizmoText (g, buf);

	sprintf (buf, GGT(TXT_COPYING_TO), FolderName(cr, destfile));
	g = (StaticTextGizmo *)QueryGizmo (
		ModalGizmoClass, cr->sliderPopup, GetGizmoGizmo, "copying3"
	);
	SetStaticTextGizmoText (g, buf);

	sg = (SliderGizmo *) QueryGizmo (
		ModalGizmoClass, cr->sliderPopup, GetGizmoGizmo, "slider"
	);
	SetSliderValue (sg, 0);
}

static void
_SetSliderNames (CmdPtr cp)
{
	SetSliderFilenames (cp->cr, cp->srcfile, cp->destfile);
	XSync (XtDisplay (Root), False);
}

static void
SetSliderNames (CnxtRec *cr, int g, char *src, char *dst)
{
	if (cr->prop->displaySlider == True) {
		QueueCmd (
			"set silder names", cr,
			_SetSliderNames, 0, g, src, dst, 0, 0, Low
		);
	}
}

void
RestartQueues (CnxtRec *cr, Suspend s)
{
	ftp->timeout = XtAddTimeOut (
		50, (XtTimerCallbackProc)ReadTimeOutCB, NULL
	);
	ftp->suspended = s;
}

static void
OverWriteCB (Widget w, CnxtRec *cr, XtPointer callData)
{
	RestartQueues (cr, OverWrite);
	CancelCB (w, cr, callData);
}

static void
DontOverWriteCB (Widget w, CnxtRec *cr, XtPointer callData)
{
	RestartQueues (cr, DontOverWrite);
	CancelCB (w, cr, callData);
}

static void
OverWriteCancelCB (Widget w, CnxtRec *cr, XtPointer callData)
{
	CancelCB (w, cr, callData);
	RestartQueues (cr, NotSuspended);
	cr->top->canceled = True;
}

static MenuItems fileExistsItems[] = {
	{True,	BUT_OVERWRITE,	MNEM_OVERWRITE,	NULL, OverWriteCB},
	{True,	BUT_DONTWRITE,	MNEM_DONTWRITE,	NULL, DontOverWriteCB},
	{True,	BUT_CANCEL,	MNEM_CANCEL,	NULL, OverWriteCancelCB},
	{True,	BUT_HELP,	MNEM_HELP,	NULL, NULL},
	{NULL}
};

static MenuGizmo fileExistsMenu = {
	NULL, "fileExistsMenu", NULL, fileExistsItems,
	NULL, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static ModalGizmo fileExistsModal = {
	NULL, "fileExistsModal", TXT_FILE_EXISTS_TITLE, &fileExistsMenu
};

void
FileExistsPopup (CnxtRec *cr, char *name, char *system)
{
	static ModalGizmo *	existsModal = (ModalGizmo *)0;
	char			buf[BUF_SIZE];
	Widget			menuWidget;

	if (existsModal == (ModalGizmo *)0) {
		existsModal = &fileExistsModal;
		CreateGizmo (Root, ModalGizmoClass, existsModal, 0, 0);
	}
	sprintf (buf, GGT(TXT_FILE_EXISTS), name, system);
	SetModalGizmoMessage (existsModal, buf);
	menuWidget = (Widget)QueryGizmo (
		ModalGizmoClass, existsModal, GetGizmoWidget,
		"fileExistsMenu"
	);
	OlVaFlatSetValues (menuWidget, 0, XtNclientData, cr, (String)0);
	OlVaFlatSetValues (menuWidget, 1, XtNclientData, cr, (String)0);
	OlVaFlatSetValues (menuWidget, 2, XtNclientData, cr, (String)0);
	MapGizmo (ModalGizmoClass, existsModal);
}

static void
_RemoteLocalCopy (CmdPtr cp)
{
	/* This is the copy connection record */
	CnxtRec *	cr = cp->cr;
	char *		src;
	char *		dest;
	DropObject *	dobj = (DropObject *)cp->clientData;
	int		grp = cp->group;
	mode_t		m;

	if (dobj->numfiles == 0) {
		return;
	}
	dest = dobj->destfiles[dobj->fileindex];
	src = dobj->srcfiles[dobj->fileindex];
	SetSliderFilenames (cr, src, dest);
	if (ftp->suspended == OverWrite) {
		ftp->suspended = NotSuspended;
	}
	else {
		src = NULL;
		dest = NULL;
		if (dobj->type[dobj->fileindex] == 'd') {
			mkdir (dest, 0777);
			/* Go to the next file */
		}
		else if (ftp->suspended == DontOverWrite) {
			ftp->suspended = NotSuspended;
			/* If this file can't be overwritten */
			/* the skip to next file */
		}
		else {
			dest = dobj->destfiles[dobj->fileindex];
			src = dobj->srcfiles[dobj->fileindex];
			if (StatFile (dest, &m) != -1) {
				/* File already exists */
				ftp->suspended = StartSuspend;
				SetSliderFilenames (cr, src, dest);
				FileExistsPopup (cr, dest, ftp->systemName);
				return;
			}
		}
	}
	cr->numHash = dobj->numHash[dobj->fileindex++];
	/* Need to check to see if dest is a directory. */
	/* If it is then it needs to be removed. */
	/* Note: this could be in our own temp directory. */
	GetCmd (cr, grp, dobj, src, dest);
	if (dobj->fileindex < dobj->numfiles) {
		RemoteLocalCopy (cr, dobj, grp, Low);
	}
}

void
RemoteLocalCopy (CnxtRec *cr, DropObject *dobj, int grp, Priority p)
{
	/* The first time this command is queued, it is queued at the */
	/* lowest priority (scum).  This is done to allow other copies */
	/* queued at (Low) to finish before this new copy starts.  The */
	/* second and following times this command is queued it is queued */
	/* at Low priority to allow it to finish before any new copies. */
	QueueCmd (
		"remote->local copy", cr,
		_RemoteLocalCopy, 0, grp, 0, 0, (XtPointer)dobj, 0, p
	);
}

/*
 * The directory already exists so find out from the
 * user if the directory should be overwritten.
 */
void
ChkDir (CnxtRec *cr)
{
}

static void
_LocalRemoteCopy (CmdPtr cp)
{
	int		grp = NextCmdGroup();
	CnxtRec *	putcr = cp->cr;
	DropObject *	dobj = (DropObject *)cp->clientData;
	char *		src;
	char *		dest;
	int		i;

	i = dobj->fileindex;
	putcr->numHash = dobj->numHash[i];
	src = dobj->srcfiles[i];
	dest = dobj->destfiles[i];
	if (dobj->type[i] == 'd') {
		MkDirCmd (putcr, grp, dest, Low);
	}
	else {
		SetSliderNames (putcr, grp, src, dest);
		PutCmd (putcr, grp, src, dest, dobj);
	}
	if (++dobj->fileindex < dobj->numfiles) {
		LocalRemoteCopy (putcr, dobj, cp->group);
	}
	else {
		FreeDobj (dobj);
	}
}

void
LocalRemoteCopy (CnxtRec *cr, DropObject *dobj, int grp)
{
	QueueCmd (
		"local->remote copy", cr,
		_LocalRemoteCopy, 0, grp, 0, 0, (XtPointer)dobj, 0, Low
	);
}

static void
CopyCancelCB (Widget wid, XtPointer clientData, XtPointer callData)
{
	CnxtRec *	cr = FindCnxtRec (wid);

	XtPopdown ((Widget) _OlGetShellOfWidget (wid));
	cr->copyMapped = False;
}

static void
CopyFolderOpenCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *	cr = FindCnxtRec (wid);
	DmContainerPtr	cntrp = cr->dir.container;
	DmItemPtr	item = cr->dir.itp;
	DropObject *	dobj;
	int		i;
	char *		dir;
	char		buf[BUF_SIZE];

	if (cr->selectedFiles == 0) {
		sprintf (buf, GGT(TXT_SELECT_FILE));
		SetFileGizmoMessage (cr->copyPopup, buf);
		return;
	}
	/* Find the first selected item */
	for (i=0; i<cntrp->num_objs; i++) {
		if (ITEM_MANAGED(item) && ITEM_SELECT(item)) {
			break;
		}
		item += 1;
	}

	/* Go and copy the files */
	XtVaGetValues (
		cr->copyPopup->textFieldWidget, XtNstring, &dir, (String)0
	);
	if (dir[0] != '/') {
		dir = GetFilePath (cr->copyPopup);
	}
	dobj = (DropObject *)CALLOC (1, sizeof (DropObject));
	if (dobj == NULL) {
		return;
	}
	dobj->cmd = DropOnFolder;
	/* Indicate all selected files should be copied by passing -1 */
	CopyOutside (cr, dobj, -1, dir);
	XtPopdown ((Widget) _OlGetShellOfWidget (wid));
	cr->copyMapped = False;
}

static MenuItems copyFolderItems[] = {
	{True, BUT_COPY,	MNEM_COPY,	 NULL, CopyFolderOpenCB},
	{True, BUT_CANCEL,	MNEM_COPY_CANCEL,NULL, CopyCancelCB},
	{True, BUT_HELP,	MNEM_HELP,	 NULL, NULL},
	{NULL}
};

MenuGizmo copyFolderMenu = {
	NULL, "copyFolderMenu", NULL, copyFolderItems, 0,
	NULL, CMD, OL_FIXEDROWS, 1, 0
};

static FileGizmo copyFolder = {
	0,
	"folder",
	" ",
	&copyFolderMenu,
	"", "", "", FOLDERS_ONLY,
	NULL, 0, TXT_COPY_FILES
};

static void
CopyPopup (CnxtRec *cr)
{
	DmContainerPtr	cntrp = cr->dir.container;
	DmObjectPtr	op;
	DmItemPtr	item = cr->dir.itp;
	char		names[BUF_SIZE];
	int		i;
	int		j;
	char		buf[BUF_SIZE];

	/* Look to see if any items are selected */
	j = 0;
	names[0] = '\0';
	for (i=0; i<cntrp->num_objs; item++, i++) {
		if (ITEM_MANAGED(item) && ITEM_SELECT(item)) {
			op = (DmObjectPtr)item->object_ptr;
			/* Create string: "<file1>, <file2>, ..." */
			if (++j > 1) {
				strcat (names, GGT(TXT_COMMA));
			}
			if (j == 3) {
				break;
			}
			strcat (names, op->name);
		}
	}
	if (j != 0) {
		/* Blank out previous message */
		if (cr->copyPopup != (FileGizmo *)0) {
			SetFileGizmoMessage (cr->copyPopup, "");
		}
	}
	cr->selectedFiles = j;
	if (j == 3) {
		strcat (names, GGT(TXT_ELIPSIS));
	}
	if (cr->copyPopup == (FileGizmo *)0) {
		cr->copyPopup = CopyGizmo (FileGizmoClass, &copyFolder);
		CreateGizmo (
			GetBaseWindowShell (cr->base), FileGizmoClass,
			cr->copyPopup, 0, 0
		);
		SetFileGizmoNameLabel(cr->copyPopup, TXT_TO);
		/* Set the title */
		sprintf (
			buf, GGT(TXT_COPY_TITLE),
			cr->systemAddr, ftp->systemName
		);
		XtVaSetValues (
			GetFileGizmoShell(cr->copyPopup), XtNtitle, buf,
			(String)0
		);
	}
	SetFileGizmoSrcList (cr->copyPopup, names);
}

void
UpdateCopyPopup (CnxtRec *cr)
{
	if (cr->copyPopup != (FileGizmo *)0) {
		if (XtIsRealized (GetFileGizmoShell (cr->copyPopup))) {
			CopyPopup (cr);
		}
	}
}

void
CopyCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	CnxtRec *	cr = FindCnxtRec (wid);

	CopyPopup (cr);	/* -> CopyFolderOpenCB() */
	MapGizmo (FileGizmoClass, cr->copyPopup);
	cr->copyMapped == True;
}

void
CopyFileName (
	CnxtRec *cr, DmFileType ftype, long size, char *name,
	DropObject *dobj, int *num, char* src, char *dest
)
{
	char	buf[BUF_SIZE];
	char	dir[BUF_SIZE];

	sprintf (buf, "%s/%s", src, name);
	sprintf (dir, "%s/%s", dest, name);
	if (ftype == DM_FTYPE_DIR) {
		/* Go gather up the directories to be copied. */
		GetDirCmd (
			cr, cr->top->group,
			buf, dir, dobj, Low
		);
	}
	else {
		if (
			(dobj->cmd == EditCmd || dobj->cmd == PrintCmd) &&
			ftype != DM_FTYPE_DATA
		) {
			/* If this is a copy for edit or print and this */
			/* isn't a data file then don't copy it. */
			FPRINTF ((stderr, "WARNING: not copying %s\n", buf));
			dobj->numfiles -= 1;
			return;
		}
	}
	dobj->numHash[*num] = (size/1024) + 1;
	/* Copy source file name */
	dobj->srcfiles[*num] = STRDUP (buf);

	/* Copy destination file name */
	if (dest != NULL) {
		dobj->destfiles[*num] = STRDUP (dir);
	}
	else {
		dobj->destfiles[*num] = NULL;
	}
	*num += 1;
	dobj->destfiles[*num] = NULL;
}

DropObject *
CopyFilenames (CnxtRec *cr, DropObject *dobj, int item_index, char *destdir)
{
	DmContainerPtr	cntrp;
	DmObjectPtr	op;
	DmItemPtr	item;
	DirEntry *	dp;
	int		i;
	int		j;

	/* Queue command to copy each selected file */
	cntrp = cr->dir.container;
	item = cr->dir.itp + item_index;
	/* An item_index == -1 means all selected files should be copied. */
	/* Otherwise, only the item_index item should be copied. */
	if (item_index != -1) {
		j = 1;
	}
	else {
		/* Count the number of selected files */
		item = cr->dir.itp;
		j = 0;
		for (i=0; i<cntrp->num_objs; item++, i++) {
			if (ITEM_MANAGED(item) && ITEM_SELECT(item)) {
				j += 1;
			}
		}
	}

	/* Make a list of the files */
	dobj->fileindex = 0;
	dobj->srcfiles = (char **)MALLOC (sizeof (char *)*j);
	dobj->destfiles = (char **)MALLOC (sizeof (char *)*(j+1));
	dobj->numHash = (int *)MALLOC (sizeof (int)*j);
	dobj->type = (char *)MALLOC (sizeof (char)*j);
	j = 0;
	item = cr->dir.itp + item_index;
	if (item_index != -1) {
		/* Position to name of unselected file */
		op = (DmObjectPtr)item->object_ptr;
		dp = (DirEntry *)op->objectdata;
		dobj->type[j] = dp->permission[0];
		CopyFileName (
			cr->copycr, op->ftype, dp->size, op->name, dobj,
			&j, cr->pwd, destdir
		);
	}
	else {
		item = cr->dir.itp;
		for (i=0; i<cntrp->num_objs; i++) {
			if (ITEM_MANAGED(item) && ITEM_SELECT(item)) {
				op = (DmObjectPtr)item->object_ptr;
				dp = (DirEntry *)op->objectdata;
				dobj->type[j] = dp->permission[0];
				CopyFileName (
					cr->copycr, op->ftype, dp->size,
					op->name, dobj, &j, cr->pwd, destdir
				);
			}
			item += 1;
		}
	}
	dobj->numfiles = j;

	return dobj;
}

void
CopyOutside (CnxtRec *cr, DropObject *dobj, int item_index, char *dir)
{
	CnxtRec *	copycr;
	int		i;
	mode_t		m;

	/* Establish copy connection */
	if (cr->copycr == NULL) {
		cr->copycr = CreateCopyConnection (cr);
	}

	/* Makefile sure the temporary directory exists */
	i = StatFile (dir, &m);
	if (i == -1 || (m & S_IFMT) != S_IFDIR) {
		TmpDirError (cr, dobj, item_index, dir);
		return;
	}

	copycr = cr->copycr;
	dobj->cr = cr;

	CopyFilenames (cr, dobj, item_index, dir);

	/* Create the gauge for copying */
	if (cr->prop->displaySlider == True) {
		QueueCmd (
			"create slider popup",
			copycr, CreateSliderPopup, 0, NextCmdGroup(),
			copycr->systemAddr, ftp->systemName, 0, 0, Scum
		);
	}

	/* Go an physically copy the files */
	RemoteLocalCopy (cr->copycr, dobj, NextCmdGroup(), Scum);
	if (cr->prop->displaySlider == True) {
		QueueCmd (
			"pop down popup", copycr, PopDownPopup,
			0, NextCmdGroup(), 0, 0, 0, 0, Scum
		);
	}
}
