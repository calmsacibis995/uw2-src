/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:manageprop.c	1.2"
#endif

#define SENDPROP_C

#include <IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/PopupWindo.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <CoreP.h>
#include "mail.h"
#include "TextGizmo.h"
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/LabelGizmo.h>
#include <Gizmo/SpaceGizmo.h>

DblCMessage		dblCMessage;

extern char *		DoubleClick;
extern MailRec *        mailRec;
extern Widget           Root;

#define INPLACE		"InPlace"

typedef enum {
	PropSet, PropReset, PropCancel, PropHelp
} PropMenuItemIndex;

typedef struct _ManageSettings {
	Setting inPlace;
} ManageSettings;

static ManageSettings inputSettings = {
	{"inplace",	NULL, NULL, (XtPointer)0	},
};

static MenuItems inPlaceItems[] = {
	{True, BUT_OPEN_READER,	MNEM_OPEN_READER,   "openreader"},
	{True, BUT_NEW_READER,	MNEM_NEW_READER,  "newreader"},
	{NULL}
};

static MenuGizmo inPlaceMenu = {
	NULL, "inplace", NULL, inPlaceItems, 0, NULL, EXC,
	OL_FIXEDROWS, 1, OL_NO_ITEM
};

static ChoiceGizmo inPlace = {
	NULL,
	INPLACE,
	TXT_DBLC_MESSAGE_IN,
	&inPlaceMenu,
	&inputSettings.inPlace,
};

static GizmoRec PropManageGiz[] = {
	{ChoiceGizmoClass,	&inPlace},
};

/* Define the Apply menu */

static MenuItems compApplyItems[] = {
	{True,	BUT_APPLY,	MNEM_APPLY},
	{True,	BUT_RESET,	MNEM_RESET},
	{True,	BUT_CANCEL,	MNEM_CANCEL},
	{NULL}
};

static void PropertyCB();

static MenuGizmo compApply = {
	NULL, "", NULL, compApplyItems, PropertyCB,
	NULL, CMD, OL_FIXEDROWS, 1, 0
};

/* Define the send property popup */

static PopupGizmo *	propManagePopup = (PopupGizmo *)0;

static PopupGizmo	PropManagePopup = {
	NULL,
	"mailProp",
	TXT_MAIL_PROPERTIES,
	&compApply,
	PropManageGiz,
	XtNumber (PropManageGiz),
};

/* Create the manage window property sheet popup */

void
ManagePropPopupCB(wid, clientData, callData)
Widget wid;
XtPointer clientData;
XtPointer callData;
{
	ManageRec *mp = FindManageRec (wid);
	Widget	shell;
	Widget	control;

	/* Set the appropriate state of the Record menu */
	if (dblCMessage == InPlace) {
		inputSettings.inPlace.previous_value = (XtPointer)0;
	}
	else {
		inputSettings.inPlace.previous_value = (XtPointer)1;
	}
	if (propManagePopup == (PopupGizmo *)0) {
		propManagePopup = &PropManagePopup;
		shell = CreateGizmo (
			Root,
			PopupGizmoClass,
			propManagePopup,
			NULL, 0
		);
		XtVaGetValues (
			shell, XtNupperControlArea, &control, (String)0
		);
		XtVaSetValues (
			control, XtNalignCaptions, False, (String)0
		);
	}
	else
	{
		ManipulateGizmo (
			(GizmoClass)PopupGizmoClass,
			propManagePopup,
			ResetGizmoValue
		);
	}

	MapGizmo (PopupGizmoClass, propManagePopup);
}

Setting *		dblCSetting;

static void
PropertyCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	Widget			shell;

	shell = GetPopupGizmoShell(propManagePopup);

	switch (p->item_index) {
		case PropSet:
			ManipulateGizmo (
				(GizmoClass)PopupGizmoClass,
				propManagePopup,
				GetGizmoValue
			);

			ManipulateGizmo (
				(GizmoClass)PopupGizmoClass,
				propManagePopup,
				SetGizmoValue
			);
			dblCSetting = (Setting *)QueryGizmo (
				PopupGizmoClass, propManagePopup,
				GetGizmoSetting, INPLACE
			);
			if (dblCSetting->current_value == 0)
				dblCMessage = InPlace;
			else
				dblCMessage = New;

			BringDownPopup(shell);
			Mail_Properties.Open_Or_New = dblCMessage;
			SaveMailProperties();
			break;
		case PropReset:
			ManipulateGizmo (
				(GizmoClass)PopupGizmoClass,
				propManagePopup,
				ResetGizmoValue
			);
			break;
		case PropCancel:
			SetWMPushpinState (
				XtDisplay(shell),
				XtWindow(shell),
				WMPushpinIsOut
			);
			XtPopdown(shell);
			break;
		case PropHelp:
			break;
		default:
			break;
	}

}
