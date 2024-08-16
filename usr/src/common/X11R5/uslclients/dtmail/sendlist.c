/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:sendlist.c	1.45"
#endif

#define SENDLIST_C

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include "mail.h"
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/Gizmos.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/LabelGizmo.h>
#include "SendGizmo.h"

extern HelpInfo		SenderHelp;
extern MailRec *	LastMailRec;
extern char *		Signature;
extern Widget		Root;
extern char *		SendName;

static void		FileSelectCB();
static void		EditSelectCB();
static void		MailSelectCB();

#define T	(XtPointer)True
#define F	(XtPointer)False

static MenuItems barNewItems[] = {
	{True, "",	(XtArgVal)0,	NULL, SendBarNewCB},
	{NULL}
};
static MenuItems barSaveAsItems[] = {
	{True, "",	(XtArgVal)0,	NULL, SendBarSaveAsCB},
	{NULL}
};
static MenuItems barPrintItems[] = {
	{True, "",	(XtArgVal)0,	NULL, SendBarPrintCB},
	{NULL}
};
static MenuItems barSendItems[] = {
	{True, "",	(XtArgVal)0,	NULL, SendBarSendCB},
	{NULL}
};

static MenuGizmo barNewMenu = {
	NULL, "barSendNewMenu", NULL, barNewItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barSaveAsMenu = {
	NULL, "barSendSaveAsMenu", NULL, barSaveAsItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barPrintMenu = {
	NULL, "barSendPrintMenu", NULL, barPrintItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};
static MenuGizmo barSendMenu = {
	NULL, "barSendSendMenu", NULL, barSendItems,
	0, NULL, CMD, OL_FIXEDROWS, 1, OL_NO_ITEM
};

static GizmoRec buttonArray0[] = {
	{MenuBarGizmoClass,	&barNewMenu}
};

static GizmoRec buttonArray1[] = {
	{MenuBarGizmoClass,	&barSaveAsMenu}
};

static GizmoRec buttonArray2[] = {
	{MenuBarGizmoClass,	&barPrintMenu}
};

static GizmoRec buttonArray3[] = {
	{MenuBarGizmoClass,	&barSendMenu}
};


static Arg buttonArgs[] = {
	XtNhPad,	0
};

static LabelGizmo label0 = {
	NULL, "readerBarSend", "",
	buttonArray3, XtNumber (buttonArray3),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label1 = {
	NULL, "readerBarNew", "",
	buttonArray0, XtNumber (buttonArray0),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label2 = {
	NULL, "readerBarSaveAs", "",
	buttonArray1, XtNumber (buttonArray1),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static LabelGizmo label3 = {
	NULL, "readerBarPrint", "",
	buttonArray2, XtNumber (buttonArray2),
	OL_FIXEDROWS, 1, buttonArgs, 1, False
};

static GizmoRec sendBar[] = {
	{LabelGizmoClass,	&label0},
	{LabelGizmoClass,	&label1},
	{LabelGizmoClass,	&label2},
	{LabelGizmoClass,	&label3},
};

static Arg labelArgs[] = {
	XtNhSpace,	0,
	XtNhPad,	0
};

static LabelGizmo pixmapLabel = {
	NULL, "sendPixmaps", "", sendBar, XtNumber (sendBar),
	OL_FIXEDROWS, 1, labelArgs, 2, False
};


static SendGizmo partsIsParts = {
	SEND
};

static GizmoRec partsArray[] = {
	{LabelGizmoClass,	&pixmapLabel},
	{SendGizmoClass, &partsIsParts}
};

/* Send Base Window Menubar -------------------------------------- */

static MenuItems fileItems[] = {
    {True, BUT_OPEN_DDD,	MNEM_OPEN_DDD},
    {True, BUT_SAVE,		MNEM_SAVE},
    {True, BUT_SAVE_AS_DDD,	MNEM_SAVE_AS_DDD},
    {True, BUT_PRINT,		MNEM_PRINT},
    {True, BUT_SEND_PROP,	MNEM_SEND_PROP_t},
    {True, BUT_ALIASES_DDD,	MNEM_ALIASES_DDD},
    {True, BUT_EXIT,		MNEM_EXIT_E},
    {NULL}
};
static MenuGizmo file = {
	NULL, "sendFile", NULL, fileItems, SendFileCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems editItems[] = {
    {True, BUT_UNDO,		MNEM_UNDO},
    {True, BUT_CUT,		MNEM_CUT},
    {True, BUT_COPY,		MNEM_COPY},
    {True, BUT_PASTE,		MNEM_PASTE},
    {True, BUT_DELETE,		MNEM_DELETE},
    {True, BUT_SELECT_ALL,	MNEM_SELECT_ALL},
    {True, BUT_UNSELECT_ALL,	MNEM_UNSELECT_ALL},
    {NULL}
};
static MenuGizmo edit = {
	NULL, "sendEdit", NULL, editItems, SendEditCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems mailItems[] = {
    {True, BUT_SEND,		MNEM_SEND},
    {True, BUT_NEW,		MNEM_NEW},
    {True, BUT_INCLUDE,		MNEM_INCLUDE},
    {NULL}
};
static MenuGizmo mail = {
	NULL, "sendMail", NULL, mailItems, SendMailCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems helpItems[] = {
    {True, BUT_SENDER_DDD, MNEM_SENDER_DDD,NULL,NULL,(XtPointer)HelpSender},
    {True, BUT_TOC_DDD,	MNEM_TOC_DDD, NULL,NULL,(XtPointer)HelpTOC},
    {True, BUT_HELP_DESK_DDD, MNEM_HELP_DESK_DDD,NULL,NULL,(XtPointer)HelpDesk},
    {NULL}
};
static MenuGizmo help = {
	NULL, "sendHelp", NULL, helpItems, HelpCB,
	NULL, CMD, OL_FIXEDCOLS, 1, 0
};

static MenuItems sendItems[] = {
    {True, BUT_FILE,	MNEM_FILE,		(char *) &file,	FileSelectCB},
    {True, BUT_EDIT,	MNEM_EDIT,		(char *) &edit,	EditSelectCB},
    {True, BUT_MESS,	MNEM_MESS,		(char *) &mail,	MailSelectCB},
    {True, BUT_HELP,	MNEM_HELP,		(char *) &help,	NULL},
    {NULL}
};

static MenuGizmo send =  {
	NULL, "send", NULL, sendItems, NULL,
	NULL, CMD, OL_FIXEDROWS, 1, 0
};

static BaseWindowGizmo SendShell = {
	&SenderHelp,
	"SendWindow",
	" ",
	&send,
	partsArray,
	XtNumber (partsArray),
	TXT_SENDER_ICON_NAME,
	"sndmail.icon",
	" ",
	" ",
	100
};

SendRec *sendRec = (SendRec *)0;

void
InitOriginalText (sp)
SendRec *	sp;
{
	SendGizmo *	gizmo;

	gizmo = (SendGizmo *)QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo,
		GetGizmoGizmo, SEND
	);
	sp->origText = NULL;
	if (Signature != NULL) {
		sp->origText = STRDUP (Signature);
	}
	sp->origTo = NULL;
	sp->origSubject = NULL;
	sp->origCc = NULL;
	sp->origBcc = NULL;
	SetSendTextAndHeader (gizmo, NULL, NULL, NULL, NULL, Signature);
}

static void
ManageMiniHelp(w, client, event)
Widget  w;
XtPointer       client;
XEvent *event;
{
	SendRec *	sp = FindSendRec (w);

        DisplayInLeftFooter (sp->baseWGizmo,
                (event->type == EnterNotify) ? client : "",
                False);
}

SendRec *
AddToSendList (mng)
ManageRec *     mng;
{
	void			MarkUnused();
	SendRec *		sp;
	SendGizmo *		gizmo;
	Arg			arg[10];
	static XtCallbackRec	protocolCB[] = {
		{WindowManagerEventHandler, (XtPointer)SendExitCB},
		{NULL}
	};
	Widget			cap;

	/* Look for an unused base window in the manager's send list */

	if ((sp=mng->topsender) != (SendRec *)0)
	{
		do
		{
			
			if (TextChanged (sp) == False) {
				/*
				 * If the text in this sender hasn't changed
				 * then use it
				 */
				MarkUnused (sp);
			}
			if (sp->used == False) {
				/* Return this unused Send Basewindow */
				return sp;
			}
		} while (sp = sp->mngnext, sp != mng->topsender);
	}
	/*
	 * Can't find one in the list so create a new one.
	 */
	sp = (SendRec *)MALLOC (sizeof (SendRec));
	SendShell.title = GetGizmoText (TXT_OUT_GOING);
	sp->baseWGizmo = CopyGizmo (BaseWindowGizmoClass, &SendShell);
	XtSetArg(arg[0], XtNwmProtocol, protocolCB);
	CreateGizmo (
		Root, BaseWindowGizmoClass, sp->baseWGizmo, arg, 1
	);
	/* Add this new BaseWindow to the front of the list */

	/* Set up callback for drops of the send window */
	OlDnDRegisterDDI (
		sp->baseWGizmo->icon_shell, OlDnDSitePreviewNone,
		SendDropNotify, (OlDnDPMNotifyProc)NULL, True, (XtPointer)NULL
	);
	gizmo = (SendGizmo *)QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo,
		GetGizmoGizmo, SEND
	);
	OlDnDRegisterDDI (
		gizmo->displayArea, OlDnDSitePreviewNone, SendDropNotify,
		(OlDnDPMNotifyProc)NULL, True, (XtPointer)NULL
	);

	XtVaSetValues (
                QueryGizmo (
                        BaseWindowGizmoClass, sp->baseWGizmo,
                        GetGizmoWidget, "sendPixmaps"
                ),
                XtNweight,      0,
                (String)0
        );

	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, sp->baseWGizmo,
			GetGizmoWidget, "barSendNewMenu"
		)),
		"m.new24"
	);
        XtAddEventHandler(cap,
                EnterWindowMask | LeaveWindowMask,
                False,
                (XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_SEND_NEW)
        );
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, sp->baseWGizmo,
			GetGizmoWidget, "barSendSaveAsMenu"
		)),
		"m.saveas24"
	);
        XtAddEventHandler(cap,
                EnterWindowMask | LeaveWindowMask,
                False,
                (XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_SEND_SAVE)
        );
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, sp->baseWGizmo,
			GetGizmoWidget, "barSendPrintMenu"
		)),
		"dtm.print.24"
	);
        XtAddEventHandler(cap,
                EnterWindowMask | LeaveWindowMask,
                False,
                (XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_SEND_PRINT)
        );
	SetMenuPixmap (
		(cap = (Widget)QueryGizmo (
			BaseWindowGizmoClass, sp->baseWGizmo,
			GetGizmoWidget, "barSendSendMenu"
		)),
		"m.send24"
	);
        XtAddEventHandler(cap,
                EnterWindowMask | LeaveWindowMask,
                False,
                (XtEventHandler) ManageMiniHelp,
		GetGizmoText(MINI_HELP_SEND_SEND)
        );

	XtSetArg(arg[0], XtNposition, OL_RIGHT);
        XtSetValues(XtParent(XtParent(XtParent(XtParent(cap)))), arg, 1);

	sp->next = sendRec;
	sp->saveFilename = NULL;
	sendRec = sp;
	if (mng->topsender == (SendRec *)0)
	{
		mng->topsender = sp;
		sp->mngnext = sp->mngprev = sp;
	}
	else
	{
		/* put at end of list to be last scanned for reuse */
		sp->mngnext = mng->topsender;
		sp->mngprev = mng->topsender->mngprev;
		mng->topsender->mngprev->mngnext = sp;
		mng->topsender->mngprev = sp;
	}
	mng->sendercnt++;
	sp->parent = mng;
	mng->mp->numBaseWindows++;
	return sp;
}

/*
 * Mark this send window as reusable.
 */

void
MarkUnused (sp)
SendRec *	sp;
{
	sp->used = False;

	FREENULL (sp->origText);
	FREENULL (sp->origTo);
	FREENULL (sp->origSubject);
	FREENULL (sp->origCc);
	FREENULL (sp->origBcc);
}

static void
FileSelectCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	SendRec *	sp = FindSendRec (wid);
	Boolean		flag = True;

	/*
	 * If there is a save file named then the Save button
	 * can be sensitive.
	 */
	if (sp->saveFilename == NULL) {
		flag = False;
	}
	SetSensitivity (sp->baseWGizmo, "sendFile", 3, 3, flag);

	/*
	 * Set the sensitivity of the Include button.
	 */
	flag = False;
	if (LastMailRec != (MailRec *)0) {
		if (LastMailRec->summary->clientData == (XtArgVal)1) {
			flag = True;
		}
	}
	SetSensitivity (sp->baseWGizmo, "sendFile", 2, 2, flag);

	SetSensitivity (sp->baseWGizmo, "sendFile", 1, 2, ReadyToSend(sp));
}

static void
EditSelectCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
}

Boolean
ReadyToSend (sp)
SendRec *	sp;
{
	char *		text;
	char *		subject;
	char *		to;
	char *		cc;
	char *		bcc;
	SendGizmo *	gizmo;
	Boolean		flag = True;
	int		i;

	gizmo = (SendGizmo *) QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo, GetGizmoGizmo,
		SendName
	);

	GetSendText (gizmo, &subject, &to, &cc, &bcc, &text);

	/* Make sure there is a mail message */

	if ((i = strlen (to)) != 0) {
		if (strspn (to, " ") == i) {
			flag = False;
		}
	}
	else {
		flag = False;
	}
	if (strlen (subject) == 0) {
		if (strlen (text) == 0) { 
			flag = False;
		}
	}
	else {
		if (strlen (text) == 0 && strcspn (subject, " ") == 0) {
			flag = False;
		}
	}
	return flag;
}

static void
MailSelectCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	SendRec *	sp = FindSendRec (wid);

	SetSensitivity (sp->baseWGizmo, "sendMail", 0, 0, ReadyToSend(sp));
	SetSensitivity (sp->baseWGizmo, "sendMail", 2, 2, (sp->parent->mp->summary->clientData == 1) ? 1 : 0);
}
