/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:passwd.c	1.1.3.1"
#endif

#include "ftp.h"
#include "TextFGizmo.h"
#include <Gizmo/LabelGizmo.h>

extern Widget	Root;

/*
 * Bring down the password popup.
 */

/*
 * Turn on the sensitivity of the main menu buttons.
 */
static void
TurnSensitivityOn (CnxtRec *cr)
{
	Widget	menuWidget;
	int	i;
	Arg	arg[10];

	/* button bar menu, set sensitivity for Parent and Reset */
	menuWidget = (Widget)QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget, "barMenu"
	);
	XtSetArg (arg[0], XtNsensitive, True);
	OlFlatSetValues (menuWidget, 0, arg, 1);
	OlFlatSetValues (menuWidget, 8, arg, 1);

	/* Main menu */
	menuWidget = (Widget)QueryGizmo (
		BaseWindowGizmoClass, cr->base, GetGizmoWidget, "mainMenu"
	);
	XtSetArg (arg[0], XtNsensitive, True);
	for (i=0; i<5; i++) {
		OlFlatSetValues (menuWidget, i, arg, 1);
	}
}

void
StartCd (CnxtRec *cr)
{
	CdCmd (cr, cr->pwd, NextCmdGroup(), High);
}

void
LoginOk (CnxtRec *cr)
{
	TurnSensitivityOn (cr);
	if (cr->copyCnxt == False) {
		/* If the remote wouldn't let us set the idle before login */
		/* then set it now. */
		if (cr->prop->maxTimeout == -1) {
			IdleCmd (cr);	/* Get the remote systems idle time */
			SetIdleCmd (cr);/* Set the remote systems idle time */
		}
		PwdCmd (cr, NextCmdGroup(), High);
	}
	/* The slider popup may have been up, try to bring it down */
	if (cr->sliderPopup != NULL) {
		BringDownPopup (GetModalGizmoShell (cr->sliderPopup));
	}
}

static void
ResetCB (Widget wid, CnxtRec *cr, XtPointer call_data)
{
	Widget	w;

	w = QueryGizmo (
		PopupGizmoClass, cr->passwdPopup, GetGizmoWidget, "enter"
	);
	OlTextEditClearBuffer (w);
}

static void
ApplyCB (Widget wid, CnxtRec *cr, XtPointer call_data)
{
	Widget	w;
	char *	passwd;
	char *	username;
	int	len = 0;

	w = QueryGizmo (
		PopupGizmoClass, cr->passwdPopup, GetGizmoWidget, "enter"
	);
	passwd = (char *)OlTextFieldGetString (w, (Cardinal *)&len);
	if (passwd != NULL && passwd[0] != '\0') {
		cr->passwd = passwd;
		BringDownPopup (GetPopupGizmoShell (cr->passwdPopup));
		MapGizmo (BaseWindowGizmoClass, cr->base);
		User (cr);
	}
}

static void
PassCancelCB (Widget wid, CnxtRec *cr, XtPointer call_data)
{
	XtPopdown ((Widget) _OlGetShellOfWidget (wid));
	ExitDtftp (cr);
}

static MenuItems passwdItems[] = {
	{True, BUT_OK,		MNEM_OK,	NULL,	ApplyCB},
	{True, BUT_RESET,	MNEM_RESET,	NULL,	ResetCB},
	{True, BUT_CANCEL,	MNEM_CANCEL,	NULL,	PassCancelCB},
	{True, BUT_HELP,	MNEM_HELP,	NULL,	NULL},
	{NULL}
};

static MenuGizmo passwdMenu = {
	NULL,			/* help		*/
	"passwdMenu",		/* name		*/
	"na",			/* title	*/
	passwdItems,		/* menu		*/
	0,			/* function	*/
	NULL,			/* client_data	*/
	CMD,			/* buttonType	*/
	OL_FIXEDROWS,		/* layoutType	*/
	1,			/* measure	*/
	0			/* default Item */
};

static TextFieldGizmo enter = {
	"enter",
	" ", 
	40
};

static GizmoRec enterArray [] = {
	{TextFieldGizmoClass,	&enter},
};

static LabelGizmo enterLabel = {
	NULL, "enterLabel", 
	TXT_ENTER_PASSWORD,
	enterArray, XtNumber (enterArray),
	OL_FIXEDROWS, 1,
	0, 0,
	False
};

static GizmoRec passwdArray [] = {
	{LabelGizmoClass,	&enterLabel},
};

static PopupGizmo passwordPopup = {
	NULL,
	"password",
	" ",
	&passwdMenu,
	passwdArray,
	XtNumber (passwdArray),
};

static void
CreatePasswdPopup (CnxtRec *cr)
{
	MenuGizmo *	g;
	LabelGizmo *	enter;
	char		buf[BUF_SIZE];
	Widget		wid;
	Pixel		back;
	Arg		arg[10];
	static XtCallbackRec	protocolCB[] = {
		{WindowManagerEventHandler, (XtPointer)CancelCB},
		{NULL}
	};

	if (cr->passwdPopup != NULL) {
		return;
	}
	cr->passwdPopup = CopyGizmo (
		PopupGizmoClass, &passwordPopup
	);
	g = (MenuGizmo *)QueryGizmo (
		PopupGizmoClass, cr->passwdPopup, GetGizmoGizmo, "passwdMenu"
	);
	enter = QueryGizmo (
		PopupGizmoClass, cr->passwdPopup, GetGizmoGizmo, "enterLabel"
	);
	sprintf (buf, GGT(TXT_ENTER_PASSWORD), cr->systemAddr);
	enter->caption = STRDUP (buf);

	g->client_data = (char *)cr;
	XtSetArg(arg[0], XtNwmProtocol, protocolCB);
	CreateGizmo (
		GetBaseWindowShell (cr->base),
		PopupGizmoClass, cr->passwdPopup, arg, 1
	);
	sprintf (buf, GGT(TXT_PASSWD_TITLE), cr->systemAddr, cr->userName);
	XtVaSetValues (
		GetPopupGizmoShell(cr->passwdPopup), XtNtitle, buf, (String)0
	);
	XtUnmanageChild (cr->passwdPopup->message);

	/* Set the foreground and background to be the same so the password
	 * can't be seen.
	 */
	wid = (Widget)QueryGizmo (
		PopupGizmoClass, cr->passwdPopup, GetGizmoWidget, "enter"
	);
	XtVaGetValues (wid, XtNbackground, &back, (String)0);
	XtVaSetValues (
		wid,
		XtNforeground, back,
		XtNfontColor,  back,
		(String)0
	);

	/* Set clientData to cr for Cancel button */
	wid = (Widget)QueryGizmo (
		PopupGizmoClass, cr->passwdPopup, GetGizmoWidget, "passwdMenu"
	);
	OlVaFlatSetValues (wid, 2, XtNclientData, (XtPointer)cr, (String)0);
}

void
GetUserPassword (CnxtRec *cr)
{
	TypeCmd (cr);
	/*
	 * Sometimes this routine is called as a result of reconnecting
	 * after a copy has been canceled.  In this case there already
	 * exists a passwd, no need to get it again.
	 */
	if (cr->passwd == NULL) {
		CreatePasswdPopup (cr);
		MapGizmo (PopupGizmoClass, cr->passwdPopup);
	}
	else {
		/* Otherwise, the user command must be issued.  This */
		/* is normally done from within the CreatePasswd callbacks. */
		User (cr);
	}
}

static void
RetryCB (Widget wid, CnxtRec *cr, XtPointer call_data)
{
	Widget		w;

	w = QueryGizmo (
		PopupGizmoClass, cr->passwdPopup, GetGizmoWidget, "enter"
	);
	if (OlCanAcceptFocus (w, CurrentTime) != False) {
		OlSetInputFocus (w, RevertToParent, CurrentTime);
	}
	MapGizmo (PopupGizmoClass, cr->passwdPopup);
	ResetCB (NULL, cr, NULL);
	XtPopdown (GetModalGizmoShell (ftp->invalidPasswdModal));
}

static void
CancelPasswdCB (Widget wid, CnxtRec *cr, XtPointer call_data)
{
	XtPopdown ((Widget) _OlGetShellOfWidget (wid));
	ExitDtftp (cr);
}

static MenuItems Items400[] = {
	{True, BUT_OK,		MNEM_OK,	NULL, CancelPasswdCB},
	{True, BUT_HELP,	MNEM_HELP,	NULL, NULL },
	{NULL}
};

static MenuItems invPasswdItems[] = {
	{True, BUT_RETRY,	MNEM_RETRY,	NULL, RetryCB},
	{True, BUT_CANCEL,	MNEM_CANCEL,	NULL, CancelPasswdCB},
	{True, BUT_HELP,	MNEM_HELP,	NULL, NULL },
	{NULL}
};

static MenuGizmo invPasswdMenu = {
	NULL, "invalidPasswordMenu", NULL, invPasswdItems,
	NULL, NULL, CMD, OL_FIXEDROWS, 1, 0
};

static ModalGizmo invalidPassword = {
	NULL,
	"no name",
	TXT_JUST_FTP,
	&invPasswdMenu
};

void
Ret400 (CnxtRec *cr)
{
	char			buf[BUF_SIZE];
	Widget			menuWidget;
	static ModalGizmo *	modal400 = (ModalGizmo *)0;

	if (modal400 == (ModalGizmo *)0) {
		invPasswdMenu.items = Items400;
		modal400 = CopyGizmo (ModalGizmoClass, &invalidPassword);
		CreateGizmo (
			Root, ModalGizmoClass, modal400, 0, 0
		);
	}
	menuWidget = (Widget)QueryGizmo (
		ModalGizmoClass, modal400, GetGizmoWidget,
		"invalidPasswordMenu"
	);
	/* Remember connection for the RetryCB command button */
	OlVaFlatSetValues (	/* Cancel */
		menuWidget, 0, XtNclientData, (XtPointer)cr, (String)0
	);
	sprintf (buf, GGT(TXT_CONNECT_FAILED), cr->systemAddr, cr->buffer+4);
	SetModalGizmoMessage (modal400, buf);

	MapGizmo (ModalGizmoClass, modal400);
}

void
Ret500 (CnxtRec *cr)
{
	char	buf[BUF_SIZE];
	Widget	menuWidget;

	if (ftp->invalidPasswdModal == (ModalGizmo *)0) {
		invPasswdMenu.items = invPasswdItems;
		ftp->invalidPasswdModal = CopyGizmo (
			ModalGizmoClass, &invalidPassword
		);
		CreateGizmo (
			Root, ModalGizmoClass, ftp->invalidPasswdModal, 0, 0
		);
	}
	menuWidget = (Widget)QueryGizmo (
		ModalGizmoClass, ftp->invalidPasswdModal, GetGizmoWidget,
		"invalidPasswordMenu"
	);
	/* Remember connection for the RetryCB command button */
	OlVaFlatSetValues (	/* Retry */
		menuWidget, 0, XtNclientData, (XtPointer)cr, (String)0
	);
	OlVaFlatSetValues (	/* Cancel */
		menuWidget, 1, XtNclientData, (XtPointer)cr, (String)0
	);
	sprintf (buf, GGT(TXT_LOGIN_FAILED), cr->systemAddr, cr->buffer+4);
	SetModalGizmoMessage (ftp->invalidPasswdModal, buf);

	MapGizmo (ModalGizmoClass, ftp->invalidPasswdModal);
}
