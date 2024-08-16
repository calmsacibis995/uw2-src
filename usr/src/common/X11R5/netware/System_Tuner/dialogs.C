#ident	"@(#)systuner:dialogs.C	1.4"
// dialogs.c

//////////////////////////////////////////////////////////////////
// Copyright (c) 1993 Novell
// All Rights Reserved
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NOVELL
//
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
///////////////////////////////////////////////////////////////////

#include "caw.h"

void create_dialogs ()
{
	int ac;
	Arg al[20];

	// create the question dialogs
	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_question1), char_set)); ac++;
	XtSetArg (al[ac], XmNokLabelString, XmStringCreateLtoR (getStr (TXT_yes_button), char_set)); ac++;
	XtSetArg (al[ac], XmNcancelLabelString, XmStringCreateLtoR (getStr (TXT_cancel_button), char_set)); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	q_dialog1 = XmCreateQuestionDialog (toplevel, "dialog", al, ac);
	XtUnmanageChild (XmMessageBoxGetChild (q_dialog1, XmDIALOG_HELP_BUTTON));
	XtAddCallback (q_dialog1, XmNokCallback, (XtCallbackProc) dialog_CB, (XtPointer) 1);
	XtAddCallback (q_dialog1, XmNcancelCallback, (XtCallbackProc) dialog_CB, (XtPointer) 3);
	ac=0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_no_button), char_set));ac++;
	XtSetArg (al[ac], XmNshadowThickness, 2);ac++;
	no_button=XmCreatePushButton(q_dialog1,"button",al, ac);
	XtManageChild (no_button);
	XtAddCallback (no_button, XmNactivateCallback, (XtCallbackProc) dialog_CB, (XtPointer) 2);
	registerMnemInfo(XmMessageBoxGetChild(q_dialog1,XmDIALOG_OK_BUTTON),getStr(MNEM_yes),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfo(XmMessageBoxGetChild(q_dialog1,XmDIALOG_CANCEL_BUTTON),getStr(MNEM_cancel),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfo(no_button, getStr(MNEM_no),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(q_dialog1);
	if (!DtamIsOwner (dpy))
		XtSetSensitive (XmMessageBoxGetChild (q_dialog1, XmDIALOG_CANCEL_BUTTON), False);

	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_question2), char_set)); ac++;
	XtSetArg (al[ac], XmNokLabelString, XmStringCreateLtoR (getStr (TXT_yes_button), char_set)); ac++;
	XtSetArg (al[ac], XmNcancelLabelString, XmStringCreateLtoR (getStr (TXT_cancel_button), char_set)); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	q_dialog2 = XmCreateQuestionDialog (toplevel, "dialog", al, ac);
	XtUnmanageChild (XmMessageBoxGetChild (q_dialog2, XmDIALOG_HELP_BUTTON));
	XtAddCallback (q_dialog2, XmNokCallback, (XtCallbackProc) dialog_CB, (XtPointer) 4);
	XtAddCallback (q_dialog2, XmNcancelCallback, (XtCallbackProc) dialog_CB, (XtPointer) 3);
	ac=0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_no_button), char_set));ac++;
	XtSetArg (al[ac], XmNshadowThickness, 2);ac++;
	no_button2=XmCreatePushButton(q_dialog2,"button",al, ac);
	XtManageChild (no_button2);
	XtAddCallback (no_button2, XmNactivateCallback, (XtCallbackProc) dialog_CB, (XtPointer) 5);
	registerMnemInfo(XmMessageBoxGetChild(q_dialog2,XmDIALOG_OK_BUTTON),getStr(MNEM_yes),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfo(XmMessageBoxGetChild(q_dialog2,XmDIALOG_CANCEL_BUTTON),getStr(MNEM_cancel),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfo(no_button2,getStr(MNEM_no),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(q_dialog2);

	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_question3), char_set)); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	q_dialog3 = XmCreateQuestionDialog (toplevel, "dialog", al, ac);
	XtAddCallback (q_dialog3, XmNokCallback, (XtCallbackProc) dialog_CB, (XtPointer) 6);
	XtAddCallback (q_dialog3, XmNcancelCallback, (XtCallbackProc) dialog_CB, (XtPointer) 7);
	XtUnmanageChild (XmMessageBoxGetChild (q_dialog3, XmDIALOG_HELP_BUTTON));
	registerMnemInfo(XmMessageBoxGetChild(q_dialog3,XmDIALOG_OK_BUTTON),getStr(MNEM_ok),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfo(XmMessageBoxGetChild(q_dialog3,XmDIALOG_CANCEL_BUTTON),getStr(MNEM_cancel),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(q_dialog3);

	// create the message dialogs
	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_message1), char_set)); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	m_dialog1 = XmCreateInformationDialog (toplevel, "dialog", al, ac);
	XtAddCallback (m_dialog1, XmNokCallback, (XtCallbackProc) dialog_CB, (XtPointer) 8);
	XtUnmanageChild (XmMessageBoxGetChild (m_dialog1, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild (XmMessageBoxGetChild (m_dialog1, XmDIALOG_CANCEL_BUTTON));
	registerMnemInfo(XmMessageBoxGetChild(m_dialog1,XmDIALOG_OK_BUTTON),getStr(MNEM_ok),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(m_dialog1);

	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_message2), char_set)); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	m_dialog2 = XmCreateInformationDialog (toplevel, "dialog", al, ac);
	XtAddCallback (m_dialog2, XmNokCallback, (XtCallbackProc) dialog_CB, (XtPointer) 9);
	XtUnmanageChild (XmMessageBoxGetChild (m_dialog2, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild (XmMessageBoxGetChild (m_dialog2, XmDIALOG_CANCEL_BUTTON));
	registerMnemInfo(XmMessageBoxGetChild(m_dialog2,XmDIALOG_OK_BUTTON),getStr(MNEM_ok),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(m_dialog2);

	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_message3), char_set)); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	m_dialog3 = XmCreateInformationDialog (toplevel, "dialog", al, ac);
	XtUnmanageChild (XmMessageBoxGetChild (m_dialog3, XmDIALOG_OK_BUTTON));
	XtUnmanageChild (XmMessageBoxGetChild (m_dialog3, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild (XmMessageBoxGetChild (m_dialog3, XmDIALOG_CANCEL_BUTTON));

	// create error dialog
	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNmessageString, XmStringCreateLtoR (getStr (TXT_error1), char_set)); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	e_dialog1 = XmCreateErrorDialog (toplevel, "dialog", al, ac);
	XtAddCallback (e_dialog1, XmNokCallback, (XtCallbackProc) dialog_CB, (XtPointer) 10);
	XtUnmanageChild (XmMessageBoxGetChild (e_dialog1, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild (XmMessageBoxGetChild (e_dialog1, XmDIALOG_CANCEL_BUTTON));
	registerMnemInfo(XmMessageBoxGetChild(e_dialog1,XmDIALOG_OK_BUTTON),getStr(MNEM_ok),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(e_dialog1);

	dialogs_created = True;
}
