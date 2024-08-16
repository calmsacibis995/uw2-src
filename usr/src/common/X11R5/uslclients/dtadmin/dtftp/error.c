/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:error.c	1.1.3.1"
#endif

#include "ftp.h"
#include <locale.h>
#include <Gizmo/STextGizmo.h>
#include "SlideGizmo.h"

void
CancelCB (Widget wid, XtPointer client_data, XtPointer call_data)
{
	XtPopdown ((Widget) _OlGetShellOfWidget (wid));
}

static MenuItems errorItems[] = {
	{True,	BUT_OK,		MNEM_OK,	NULL, CancelCB},
	{NULL}
};

static MenuGizmo errorMenu = {
	NULL, "errorMenu", NULL, errorItems,
	NULL, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static ModalGizmo errorModal = {
	NULL, "ErrorModal", TXT_ERROR, &errorMenu
};

typedef struct Errors {
	char *		errorNum;
	char *		errorText;
	int		index;
} Errors;

Errors errorTable[] = {
	{"500",	TXT_SYNTAX_ERROR,	500},
	{"501",	TXT_SYNTAX_PARAM_ERROR,	501},
	{"502",	TXT_NO_CMD,		502},
	{"503",	TXT_BAD_SEQUENCE,	503},
	{"504",	"Error 504",		504},
	{"530",	"Error 530",		530},
	{"532",	"Error 532",		532},
	{"550",	"Error 550",		550},
	{"551",	"Error 551",		551},
	{"552",	TXT_NO_SPACE,		552},
	{"553",	"Error 553",		553},
	{NULL,	NULL,			0}
};

/*
 * Given the errno return a printable message in some language.
 */

static char *
GetTextGivenErrno (errno)
int	errno;
{
	char	buf[10];

	/*
	 * Return error message errno from catalog UX.
	 */
	sprintf (buf, "UX:%d", errno+1);
	return (char *)gettxt (buf, strerror (errno));
}

typedef struct _FtpErrors {
	char *	text;
	char *	international;
} FtpErrors;

static FtpErrors ftpErrors[] = {
	{"not a plain file.", TXT_NOT_PLAIN_FILE},
	{NULL, NULL}
};
/*
 * Given an error message produced by perror return a printable message in
 * some language.
 */

static char *
GetTextGivenText (perror)
char *	perror;
{
	char *		cp;
	int		i;

	/*
	 * Look though all of the system errors for this error message
	 */
	for (i=0; (cp=strerror(i))!=NULL; i++) {
		if (strncmp (perror, cp, strlen(cp)) == 0) {
			/*
			 * String found
			 */
			return GetTextGivenErrno (i);
		}
	}
	/*
	 * Look thru some ftp error messages
	 */
	for (i=0; (cp=ftpErrors[i].text)!=NULL; i++) {
		if (strncmp (perror, cp, strlen(cp)) == 0) {
			return GGT(ftpErrors[i].international);
		}
	}

	return NULL;
}

static char *
CreateErrorMessage (char *msg, char *dflt)
{
	char *		cp;
	static char	buf[BUF_SIZE];

	cp = strrchr (msg, ':');
	if (strncmp ("550", msg, 3) == 0) {
		*cp = '\0';
		*++cp = '\0';
		sprintf (buf, "%s", GetTextGivenText (cp+1));
		return buf;
	}
	if (msg == NULL) {
		return dflt;
	}
	cp += 2;
	cp = GetTextGivenText (cp);
	if (cp == NULL) {
		return dflt;
	}
	return cp;
}

/*
 * Errors can be of the form:
 *	553 Button.c: Permission denied.
 *	UX:ls: ERROR: Cannot access directory .: Permission denied
 *	Cannot access directory .: Permission denied
 */
static void
ErrorModal (CnxtRec *cr, char *string, char *errorString, ModalGizmo *m)
{
	char			buf[BUF_SIZE];
	char			buf1[BUF_SIZE];
	char			buf2[BUF_SIZE];
	int			i;

	FPRINTF ((stderr, "ERROR- %s\n", cr->buffer));
	buf1[0] = '\0';
	if (string != NULL) {
		sprintf (buf1, "%s\n\n", GGT(string));
	}
	if (strncmp ("UX", cr->buffer, 2) == 0 || cr->buffer[0] != '5') {
		sprintf (
			buf2,
			"%s: %s",
			errorString,
			CreateErrorMessage (cr->buffer, "")
		);
		strcpy (buf, buf1);
		strcat (buf, buf2);
		SetModalGizmoMessage (m, buf);
	}
	else for (i=0; errorTable[i].index!=0; i++) {
		if (strncmp (errorTable[i].errorNum, cr->buffer, 3) == 0) {
			sprintf (
				buf2,
				"%s\n%s",
				errorString+4,
				CreateErrorMessage (cr->buffer, errorTable[i].errorText)
			);
			strcpy (buf, buf1);
			strcat (buf, buf2);
			SetModalGizmoMessage (m, buf);
			break;
		}
	}
	MapGizmo (ModalGizmoClass, m);
}

void
DisplayNormalError (CnxtRec *cr, char *string, int type)
{
	static ModalGizmo *	modalError = (ModalGizmo *)0;
	char *			title;
	Widget			shell;

	if (modalError == (ModalGizmo *)0) {
		modalError = &errorModal;
		CreateGizmo (Root, ModalGizmoClass, modalError, 0, 0);
	}
	title = TXT_WARNING;
	if (type == OL_ERROR) {
		title = TXT_ERROR;
	}
	shell = GetModalGizmoShell (modalError);
	SetModalGizmoMessage (modalError, string);
	XtVaSetValues (shell, XtNtitle, GGT (title), (String)0);
	MapGizmo (ModalGizmoClass, modalError);
	XtVaSetValues (shell, XtNnoticeType, type, (String)0);
}

void
DisplayError (CnxtRec *cr, char *string, PFV cancel)
{
	static ModalGizmo *	modalError = (ModalGizmo *)0;

	if (modalError == (ModalGizmo *)0) {
		modalError = &errorModal;
		if (cancel == (PFV)NULL) {
			errorItems[0].function = CancelCB;
		}
		else {
			errorItems[0].function = cancel;
		}
		errorItems[0].client_data = (char *)cr;
		CreateGizmo (Root, ModalGizmoClass, modalError, 0, 0);
	}
	ErrorModal (cr, NULL, string, modalError);
}

static void
CopyCancelCB (Widget wid, CnxtRec *cr, XtPointer call_data)
{
	cr->top->canceled = True;
	/*
	StopCB (wid, cr, call_data);
	*/
	RestartQueues (cr, NotSuspended);
	XtPopdown ((Widget) _OlGetShellOfWidget (wid));
}

static void
ContinueCB (Widget wid, CnxtRec *cr, XtPointer call_data)
{
	RestartQueues (cr, NotSuspended);
	XtPopdown ((Widget) _OlGetShellOfWidget (wid));
}

static MenuItems copyErrorItems1[] = {
	{True,	BUT_CONTINUE,	MNEM_CONTINUE,	NULL, ContinueCB},
	{True,	BUT_CANCEL,	MNEM_CANCEL,	NULL, CopyCancelCB},
	{True,	BUT_HELP,	MNEM_HELP,	NULL, NULL},
	{NULL}
};
static MenuItems copyErrorItems2[] = {
	{True,	BUT_CANCEL,	MNEM_CANCEL,	NULL, CopyCancelCB},
	{True,	BUT_HELP,	MNEM_HELP,	NULL, NULL},
	{NULL}
};

static MenuGizmo copyErrorMenu = {
	NULL, "copyErrorMenu", NULL, copyErrorItems1,
	NULL, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static ModalGizmo copyErrorModal = {
	NULL, "copyErrorModal", TXT_COPY_ERROR, &copyErrorMenu
};

static void
GetPutError (CnxtRec *cr, char *text, char *errorText)
{
	static ModalGizmo *	copyModalError = (ModalGizmo *)0;
	SliderGizmo *		sg;
	Widget			menu;
	DropObject *		dobj = (DropObject *)cr->top->clientData;

	sg = (SliderGizmo *) QueryGizmo (
		ModalGizmoClass, cr->sliderPopup, GetGizmoGizmo, "slider"
	);
	sg->canceled = True;

	/* If there are more files to copy then display menu */
	/* with "Continue" in it. */
	if (dobj->numfiles == dobj->fileindex) {
		copyErrorMenu.items = copyErrorItems2;
	}
	else {
		copyErrorMenu.items = copyErrorItems1;
	}
	if (copyModalError != (ModalGizmo *)0) {
		copyErrorMenu.parent = (Widget)0;
		XtDestroyWidget (copyModalError->shell);
	}
	copyModalError = &copyErrorModal;
	CreateGizmo (Root, ModalGizmoClass, copyModalError, 0, 0);
	ErrorModal (cr, text, errorText, copyModalError);

	menu = copyErrorMenu.child;
	OlVaFlatSetValues (menu, 0, XtNclientData, (XtPointer)cr, (String)0);
	if (dobj->numfiles != dobj->fileindex) {
		OlVaFlatSetValues (
			menu, 1, XtNclientData, (XtPointer)cr, (String)0
		);
	}
	ftp->suspended = StartSuspend;
}

/* 
 * Display an error returned from get command.
 */
void
GetError (CnxtRec *cr)
{
	DropObject *	dobj = (DropObject *)cr->top->clientData;

	MYFREE (dobj->destfiles[dobj->fileindex-1]);
	dobj->destfiles[dobj->fileindex-1] = NULL;

	GetPutError (cr, TXT_CANT_COPY, cr->buffer);
}

void
PutError (CnxtRec *cr)
{
	GetPutError (cr, TXT_CANT_COPY, cr->buffer);
}

/* In the PutCmd the cd was successful.  This means if the src is
 * a file then the copy can't proceed and an error should be posted. */
void
DirError (CnxtRec *cr)
{
	GetPutError (cr, TXT_CANT_COPY, GGT(TXT_DEST_DIR));
}

void
DeleteError (CnxtRec *cr)
{
	char	buf[BUF_SIZE];

	sprintf (buf, "550 %s %s", GGT(TXT_CANT_DELETE), cr->top->srcfile);
	DisplayError (cr, buf, NULL);
}

void
PwdError (CnxtRec *cr)
{
	DisplayError (cr, GGT(TXT_CANT_LIST), NULL);
}

void
NotEmpty (CnxtRec *cr)
{
	char	buf[BUF_SIZE];
	char *	cp;

	cp = strrchr (cr->buffer, ':');
	if (cp != NULL) {
		*cp = '\0';
	}
	sprintf (buf, GGT(TXT_NOT_EMPTY), cr->buffer+4);
	*cp = ':';
	DisplayNormalError (cr, buf, OL_ERROR);
}

static MenuItems abbendItems[] = {
	{True,	BUT_EXIT,		MNEM_EXIT,	NULL, ReallyExitCB},
	{NULL}
};

static MenuGizmo abbendMenu = {
	NULL, "abbendMenu", NULL, abbendItems,
	NULL, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static ModalGizmo abbendModal = {
	NULL, "AbbendModal", TXT_ERROR, &abbendMenu
};

/*
 * Display modal telling the user that we got into trouble and don't
 * know how to get out of it.
 */
void
Abbend (CnxtRec *cr)
{
	Widget			w;
	static ModalGizmo *	modalAbbend = (ModalGizmo *)0;
	char			string [BUF_SIZE];
	FILE *			fp;
	int			i;
	char *			logfile = GGT(TXT_ERROR_LOG);

	if (modalAbbend == (ModalGizmo *)0) {
		modalAbbend = &abbendModal;
		CreateGizmo (Root, ModalGizmoClass, modalAbbend, 0, 0);
	}
	sprintf (string, GGT(TXT_ABBEND), logfile);
	SetModalGizmoMessage (modalAbbend, string);
	MapGizmo (ModalGizmoClass, modalAbbend);
	if ((fp=fopen (logfile, "w")) == NULL) {
		sprintf (string, GGT(TXT_ABBEND), "stdout");
		SetModalGizmoMessage (modalAbbend, string);
		fp = stdout;
	}
	fprintf (fp, GGT(TXT_ERROR_HEADER));
	fprintf (fp, GGT(TXT_ERROR_OUTPUT));
	fprintf (fp, "%s", cr->lineHistory[0]);
	for (i=1; i<cr->lineIndex; i++) {
		fprintf (fp, GGT(TXT_ERROR_READ));
		fprintf (fp, "%s", cr->lineHistory[i]);
	}
	fprintf (fp, "%s", GGT(TXT_ERROR_TRAILER));
	fflush (fp);
	if (fp != stdout) {
		fclose (fp);
	}

	w = (Widget)QueryGizmo (
		ModalGizmoClass, modalAbbend, GetGizmoWidget, "abbendMenu"
	);
	OlVaFlatSetValues (w, 0, XtNclientData, cr, (String)0);
}

typedef struct _ReadOnlyData {
	PFV		func;
	CnxtRec *	cr;
	Widget		w;
} ReadOnlyData;

static void
ReadOnlyCB (Widget w, ReadOnlyData *d, XtPointer callData)
{
	Widget	wid;
	Boolean	showReadOnly = d->cr->prop->showReadOnly;

	OlVaFlatGetValues (d->w, 0, XtNset, &d->cr->prop->showReadOnly);

	/* Need to set the value of the read only menu */
	/* because prop->showReadOnly could have changed since this */
	/* popup was created. */
	if (showReadOnly != d->cr->prop->showReadOnly) {
		if (d->cr->connectPropPopup != NULL) {
			wid = (Widget)QueryGizmo (
				PopupGizmoClass, d->cr->connectPropPopup,
				GetGizmoWidget, "warning"
			);
			OlVaFlatSetValues (
				wid, 0,
				XtNset, d->cr->prop->showReadOnly,
				(String)0
			);
		}
		UpdateSystemsInfo ();
	}

	CancelCB (w, d, callData);
	if (d->func != NULL) {
		(d->func) (d->cr);
	}
}

static StaticTextGizmo readOnlyText = {
	NULL, "readOnlyText", TXT_READ_ONLY, CenterGravity
};

static MenuItems displayItems[] = {
	{True, BUT_ALWAYS},
	{NULL}
};
static MenuGizmo displayMenu = {
	NULL, "displayMenu", NULL, displayItems,
	NULL, NULL, CHK, OL_FIXEDROWS, 1, 0
};

static GizmoRec readOnlyArray [] = {
	{StaticTextGizmoClass,	&readOnlyText},
	{MenuBarGizmoClass,	&displayMenu}
};

static MenuItems readOnlyItems[] = {
	{True,	BUT_CONTINUE,	MNEM_CONTINUE,	NULL, ReadOnlyCB},
	{True,	BUT_CANCEL,	MNEM_CANCEL,	NULL, CancelCB},
	{True,	BUT_HELP,	MNEM_HELP,	NULL, NULL},
	{NULL}
};
static MenuGizmo readOnlyMenu = {
	NULL, "readOnlyMenu", NULL, readOnlyItems,
	NULL, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static ModalGizmo readOnlyModal = {
	NULL, "readOnlyModal", TXT_READ_ONLY_TITLE, &readOnlyMenu,
	"", readOnlyArray, XtNumber (readOnlyArray)
};

/* Display a read only warning popup */
void
ReadOnlyWarning (CnxtRec *cr, PFV func)
{
	static ModalGizmo *	modal = (ModalGizmo *)0;
	static ReadOnlyData	data;
	Widget			menuWidget;
	Widget			text;

	if (cr->prop->showReadOnly == False) {
		(func) (cr);
		return;
	}
	if (modal == (ModalGizmo *)0) {
		modal = &readOnlyModal;
		CreateGizmo (Root, ModalGizmoClass, modal, 0, 0);
		text = (Widget)QueryGizmo (
			ModalGizmoClass, modal, GetGizmoWidget, "readOnlyText"
		);
		XtVaSetValues (
			text,
			XtNgravity,	CenterGravity,
			XtNalignment,	OL_CENTER,
			XtNwrap,	True,
			(String)0
		);
		XtVaSetValues (
			modal->control, XtNshadowThickness, 0, (String)0
		);
	}
	menuWidget = (Widget)QueryGizmo (
		ModalGizmoClass, modal, GetGizmoWidget, "displayMenu"
	);
	OlVaFlatSetValues (menuWidget, 0, XtNset, True, (String)0);

	data.func = func;
	data.cr = cr;
	data.w = menuWidget;

	menuWidget = (Widget)QueryGizmo (
		ModalGizmoClass, modal, GetGizmoWidget, "readOnlyMenu"
	);
	OlVaFlatSetValues (menuWidget, 0, XtNclientData, &data, (String)0);
	MapGizmo (ModalGizmoClass, modal);
}
