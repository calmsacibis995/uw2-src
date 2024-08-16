#ident	"@(#)nwsetup:dialogs.C	1.12"
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
#include "mnem.h"

void create_push_buttons (Widget parent, Widget top, Widget ok, Widget cancel, Widget help, const int client_data)
{
	int ac, i;
	Arg al[20];

	i = client_data;
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_ok), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, top); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 33); ac++;
	ok = XmCreatePushButton (parent, "button", al, ac);
	XtManageChild (ok);
	XtVaSetValues(parent,XmNdefaultButton,ok,NULL);
	XtAddCallback (ok, XmNactivateCallback, (XtCallbackProc) buttonCB, (XtPointer) i);
	registerMnemInfo(ok,getStr(MNEM_ok),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);

	i++;
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_cancel), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, top); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNleftWidget, ok); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 65); ac++;
	cancel = XmCreatePushButton (parent, "button", al, ac);
	XtManageChild (cancel);
	XtVaSetValues(parent,XmNcancelButton,cancel,NULL);
	XtAddCallback (cancel, XmNactivateCallback, (XtCallbackProc) buttonCB, (XtPointer) i);
	registerMnemInfo(cancel,getStr(MNEM_cancel),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);

	i++;
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_help), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, top); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNleftWidget, cancel); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 97); ac++;
	help = XmCreatePushButton (parent, "button", al, ac);
	XtManageChild (help);
	XtAddCallback (help, XmNactivateCallback, (XtCallbackProc) buttonCB, (XtPointer) i);
	XtAddCallback(parent, XmNhelpCallback, (XtCallbackProc) buttonCB, (XtPointer) i);
	registerMnemInfo(help,getStr(MNEM_help),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(parent);
}

void create_help_text (Widget parent, Widget top, Widget separator1, Widget *text, Widget *separator2)
{
	int ac;
	Arg al[20];

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, top); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	separator1 = XmCreateSeparator (parent, "separator", al, ac);
	XtManageChild (separator1);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, separator1); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
	XtSetArg (al[ac], XmNwordWrap, True); ac++;
	XtSetArg (al[ac], XmNeditable, False); ac++;
	XtSetArg (al[ac], XmNscrollHorizontal, False); ac++;
	XtSetArg (al[ac], XmNrows, 2); ac++;
	*text = XmCreateScrolledText (parent, "scrolled_text", al, ac);
	XtManageChild (*text);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, *text); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	*separator2 = XmCreateSeparator (parent, "separator", al, ac);
	XtManageChild (*separator2);
}

void create_can_dialogs ()
{
	int ac;
	Arg al[20];

	// create decision dialog
	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNnoResize, True); ac++;
	XtSetArg (al[ac], XmNokLabelString, XmStringCreateLtoR (getStr (TXT_yes), char_set)); ac++;
	XtSetArg (al[ac], XmNcancelLabelString, XmStringCreateLtoR (getStr (TXT_no), char_set)); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	decision_dialog = XmCreateQuestionDialog (toplevel, "dialog", al, ac);
	XtAddCallback (decision_dialog, XmNokCallback, (XtCallbackProc) dialogCB, (XtPointer) 2);
	XtAddCallback (decision_dialog, XmNcancelCallback, (XtCallbackProc) dialogCB, (XtPointer) 3);
	XtUnmanageChild (XmMessageBoxGetChild (decision_dialog, XmDIALOG_HELP_BUTTON));
	registerMnemInfo(XmMessageBoxGetChild(decision_dialog,XmDIALOG_OK_BUTTON), getStr(MNEM_yes),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfo(XmMessageBoxGetChild(decision_dialog,XmDIALOG_CANCEL_BUTTON), getStr(MNEM_no),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(decision_dialog);

	// create still exit  decision dialog
	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNnoResize, True); ac++;
	XtSetArg (al[ac], XmNokLabelString, XmStringCreateLtoR (getStr (TXT_yes), char_set)); ac++;
	XtSetArg (al[ac], XmNcancelLabelString, XmStringCreateLtoR (getStr (TXT_cancel), char_set)); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	still_exit_dialog = XmCreateQuestionDialog (toplevel, "dialog", al, ac);
	XtAddCallback (still_exit_dialog, XmNokCallback, (XtCallbackProc) dialogCB, (XtPointer) 5);
	XtAddCallback (still_exit_dialog, XmNcancelCallback, (XtCallbackProc) dialogCB, (XtPointer) 6);
	XtUnmanageChild (XmMessageBoxGetChild (still_exit_dialog, XmDIALOG_HELP_BUTTON));
	ac=0;
        XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_no), char_set));ac++;
        XtSetArg (al[ac], XmNshadowThickness, 2);ac++;
        no_button=XmCreatePushButton(still_exit_dialog,"button",al, ac);
        XtManageChild (no_button);
        XtAddCallback (no_button, XmNactivateCallback, (XtCallbackProc) dialogCB, (XtPointer) 7);
        registerMnemInfo(no_button,getStr(MNEM_no),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfo(XmMessageBoxGetChild(still_exit_dialog,XmDIALOG_OK_BUTTON), getStr(MNEM_yes),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfo(XmMessageBoxGetChild(still_exit_dialog,XmDIALOG_CANCEL_BUTTON), getStr(MNEM_cancel),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(still_exit_dialog);

	// create info dialog
	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNnoResize, True); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	info_dialog = XmCreateInformationDialog (toplevel, "dialog", al, ac);
	XtAddCallback (info_dialog, XmNokCallback, (XtCallbackProc) dialogCB, (XtPointer) 4);
	XtUnmanageChild (XmMessageBoxGetChild (info_dialog, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild (XmMessageBoxGetChild (info_dialog, XmDIALOG_CANCEL_BUTTON));
	registerMnemInfo(XmMessageBoxGetChild(info_dialog,XmDIALOG_OK_BUTTON), getStr(MNEM_ok),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(info_dialog);
	can_dialogs_created = True;
}

void create_plist ()
{
	int i, ac;
	Arg al[20];
	Widget plist_form, plist_separator, plist_ok, plist_help;
	Pixmap pixmap;

	// create PList for Logical LAN Configuration
	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNnoResize, True); ac++;
	XtSetArg (al[ac], XmNautoUnmanage, False); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	XtSetArg (al[ac], XmNwidth, 150); ac++;
	plist_dialog = XmCreateFormDialog (toplevel, "dialog", al, ac);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNbottomPosition, 80); ac++;
	plist_form = XmCreateForm (plist_dialog, "form", al, ac);
	XtManageChild (plist_form);
	plist = new PList (plist_form, "plist");

	for (i = 0; i < 2; i++)
		plist->UW_AddPixmap (pixmap_files[i], &pixmap);
	plist->UW_RegisterSingleCallback (plistCB, NULL);
	for (i = 0; i < 8; i++) {
		table[i] = XmStringCreateLtoR (plist_strings[i], char_set);
		add_item (0, table[i], 0);
	}

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, plist_form); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	plist_separator = XmCreateSeparator (plist_dialog, "separator", al, ac);
	XtManageChild (plist_separator);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_dismiss), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, plist_separator); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 10); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 49); ac++;
	plist_ok = XmCreatePushButton (plist_dialog, "button", al, ac);
	XtManageChild (plist_ok);
	XtVaSetValues(plist_dialog,XmNcancelButton,plist_ok,NULL);
	XtVaSetValues(plist_dialog,XmNdefaultButton,plist_ok,NULL);
	XtAddCallback (plist_ok, XmNactivateCallback, (XtCallbackProc) buttonCB, (XtPointer) 1);
	registerMnemInfo(plist_ok,getStr(MNEM_cancel),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_help), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, plist_separator); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 51); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 90); ac++;
	plist_help = XmCreatePushButton (plist_dialog, "button", al, ac);
	XtManageChild (plist_help);
	XtAddCallback (plist_dialog,XmNhelpCallback,(XtCallbackProc) buttonCB, (XtPointer) 2);
	XtAddCallback (plist_help, XmNactivateCallback, (XtCallbackProc) buttonCB, (XtPointer) 2);
	registerMnemInfo(plist_help,getStr(MNEM_help),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(plist_dialog);

	plist_created = True;
}

void create_dialog5 ()
{
	int ac, i;
	Arg al[20];
	Widget dialog5_label1a, dialog5_label2, dialog5_label3, dialog5_label4, dialog5_label5;
	Widget dialog5_separator1, dialog5_separator2, dialog5_separator3;
	Widget dialog5_form1, dialog5_form2, dialog5_form3, dialog5_form4;
	Widget dialog5_frame1, dialog5_frame2, dialog5_frame3, dialog5_frame4;
	Widget dialog5_menu1_form, dialog5_menu1, dialog5_menu2_form, dialog5_menu2;
	Widget dialog5_ok, dialog5_cancel, dialog5_help;

	// create dialog for Logical LAN Configuration
	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNnoResize, True); ac++;
	XtSetArg (al[ac], XmNautoUnmanage, False); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	if(config.internal_lan_addr > 0)
		dialog5 = XmCreateFormDialog (plist_dialog, "dialog", al, ac);
	else
		dialog5 = XmCreateFormDialog (toplevel, "dialog", al, ac);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 2); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	dialog5_label1b = XmCreateLabel (dialog5, "label", al, ac);
	XtManageChild (dialog5_label1b);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_dialog5_string1), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 2); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, dialog5_label1b); ac++;
	dialog5_label1a = XmCreateLabel (dialog5, "label", al, ac);
	XtManageChild (dialog5_label1a);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, dialog5_label1a); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	dialog5_separator1 = XmCreateSeparator (dialog5, "separator", al, ac);
	XtManageChild (dialog5_separator1);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, dialog5_separator1); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	dialog5_form1 = XmCreateForm (dialog5, "form", al, ac);
	XtManageChild (dialog5_form1);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, dialog5_form1); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	dialog5_form2 = XmCreateForm (dialog5, "form", al, ac);
	XtManageChild (dialog5_form2);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, dialog5_form2); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	dialog5_form3 = XmCreateForm (dialog5, "form", al, ac);
	XtManageChild (dialog5_form3);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, dialog5_form3); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	dialog5_form4 = XmCreateForm (dialog5, "form", al, ac);
	XtManageChild (dialog5_form4);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	dialog5_frame1 = XmCreateFrame (dialog5_form1, "frame", al, ac);
	XtManageChild (dialog5_frame1);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	dialog5_frame2 = XmCreateFrame (dialog5_form2, "frame", al, ac);
	XtManageChild (dialog5_frame2);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	dialog5_frame3 = XmCreateFrame (dialog5_form3, "frame", al, ac);
	XtManageChild (dialog5_frame3);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	dialog5_frame4 = XmCreateFrame (dialog5_form4, "frame", al, ac);
	XtManageChild (dialog5_frame4);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_dialog5_string2), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, dialog5_frame1); ac++;
	dialog5_label2 = XmCreateLabel (dialog5_form1, "label", al, ac);
	XtManageChild (dialog5_label2);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_dialog5_string3), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, dialog5_frame2); ac++;
	dialog5_label3 = XmCreateLabel (dialog5_form2, "label", al, ac);
	XtManageChild (dialog5_label3);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_dialog5_string4), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, dialog5_frame3); ac++;
	dialog5_label4 = XmCreateLabel (dialog5_form3, "label", al, ac);
	XtManageChild (dialog5_label4);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_dialog5_string5), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, dialog5_frame4); ac++;
	dialog5_label5 = XmCreateLabel (dialog5_form4, "label", al, ac);
	XtManageChild (dialog5_label5);

	nic_count = get_nic_count ();		// get total number of NICs in system

	dialog5_menu1_form = XmCreateForm (dialog5_frame1, "form", NULL, 0);
	XtManageChild (dialog5_menu1_form);
	XtAddCallback (dialog5_menu1_form, XmNfocusCallback, (XtCallbackProc) dialog5_help_textCB, (XtPointer) 1);
	dialog5_menu1 = XmCreatePulldownMenu (dialog5_menu1_form, "option_menu", NULL, 0);
	ac = 0;
	XtSetArg (al[ac], XmNsubMenuId, dialog5_menu1); ac++;
	dialog5_option_menu1 = XmCreateOptionMenu (dialog5_menu1_form, "option_menu", al, ac);
	XtManageChild (dialog5_option_menu1);
	nic_menu[0].menu_item = make_option_menu_item (getStr (TXT_none), 0, dialog5_menu1);
	for (i = 1; i <= nic_count; i++) 
		nic_menu[i].menu_item = make_option_menu_item (nic_menu[i].name, i, dialog5_menu1);

	dialog5_menu2_form = XmCreateForm (dialog5_frame2, "form", NULL, 0);
	XtManageChild (dialog5_menu2_form);
	XtAddCallback (dialog5_menu2_form, XmNfocusCallback, (XtCallbackProc) dialog5_help_textCB, (XtPointer) 2);
	dialog5_menu2 = XmCreatePulldownMenu (dialog5_menu2_form, "option_menu", NULL, 0);
	ac = 0;
	XtSetArg (al[ac], XmNsubMenuId, dialog5_menu2); ac++;
	dialog5_option_menu2 = XmCreateOptionMenu (dialog5_menu2_form, "option_menu", al, ac);
	XtManageChild (dialog5_option_menu2);
	e_II = make_option_menu_item (getStr (TXT_frametype_string1), 9, dialog5_menu2);
	e_8022 = make_option_menu_item (getStr (TXT_frametype_string2), 10, dialog5_menu2);
	e_8023 = make_option_menu_item (getStr (TXT_frametype_string3), 11, dialog5_menu2);
	e_SNAP = make_option_menu_item (getStr (TXT_frametype_string4), 12, dialog5_menu2);
	t_8022 = make_option_menu_item (getStr (TXT_frametype_string5), 13, dialog5_menu2);
	t_SNAP = make_option_menu_item (getStr (TXT_frametype_string6), 14, dialog5_menu2);

	ac = 0;
	XtSetArg (al[ac], XmNmaxLength, 15); ac++;
	dialog5_text2 = XmCreateTextField (dialog5_frame3, "text", al, ac);
	XtManageChild (dialog5_text2);
	XtAddCallback (dialog5_text2, XmNlosingFocusCallback, (XtCallbackProc) textCB, (XtPointer) 4);
	XtAddCallback (dialog5_text2, XmNfocusCallback, (XtCallbackProc) dialog5_help_textCB, (XtPointer) 3);

	ac = 0;
	XtSetArg (al[ac], XmNmaxLength, 15); ac++;
	dialog5_text3 = XmCreateTextField (dialog5_frame4, "text", al, ac);
	XtManageChild (dialog5_text3);
	XtAddCallback (dialog5_text3, XmNlosingFocusCallback, (XtCallbackProc) textCB, (XtPointer) 5);
	XtAddCallback (dialog5_text3, XmNfocusCallback, (XtCallbackProc) dialog5_help_textCB, (XtPointer) 4);

	create_help_text (dialog5, dialog5_form4, dialog5_separator2, &dialog5_help_text, &dialog5_separator3);
	create_push_buttons (dialog5, dialog5_separator3, dialog5_ok, dialog5_cancel, dialog5_help, 4);

	dialog5_created = True;
}

void create_dialog6 ()
{
	int ac;
	Arg al[20];
	Widget dialog6_form1, dialog6_form2, dialog6_form3;
	Widget dialog6_frame1, dialog6_frame2, dialog6_frame3;
	Widget dialog6_label1, dialog6_label2, dialog6_label3;
	Widget dialog6_separator1, dialog6_separator2;
	Widget dialog6_rb_form, dialog6_radio_box;
	Widget dialog6_ok, dialog6_cancel, dialog6_help;

	// create dialog for SPX field
	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNnoResize, True); ac++;
	XtSetArg (al[ac], XmNautoUnmanage, False); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	dialog6 = XmCreateFormDialog (toplevel, "dialog", al, ac);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	dialog6_form1 = XmCreateForm (dialog6, "form", al, ac);
	XtManageChild (dialog6_form1);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, dialog6_form1); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	dialog6_form2 = XmCreateForm (dialog6, "form", al, ac);
	XtManageChild (dialog6_form2);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, dialog6_form2); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	dialog6_form3 = XmCreateForm (dialog6, "form", al, ac);
	XtManageChild (dialog6_form3);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	dialog6_frame1 = XmCreateFrame (dialog6_form1, "frame", al, ac);
	XtManageChild (dialog6_frame1);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	dialog6_frame2 = XmCreateFrame (dialog6_form2, "frame", al, ac);
	XtManageChild (dialog6_frame2);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	dialog6_frame3 = XmCreateFrame (dialog6_form3, "frame", al, ac);
	XtManageChild (dialog6_frame3);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_dialog6_string1), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, dialog6_frame1); ac++;
	dialog6_label1 = XmCreateLabel (dialog6_form1, "label", al, ac);
	XtManageChild (dialog6_label1);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_dialog6_string2), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, dialog6_frame2); ac++;
	dialog6_label2 = XmCreateLabel (dialog6_form2, "label", al, ac);
	XtManageChild (dialog6_label2);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_dialog6_string3), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, dialog6_frame3); ac++;
	dialog6_label3 = XmCreateLabel (dialog6_form3, "label", al, ac);
	XtManageChild (dialog6_label3);

	dialog6_rb_form = XmCreateForm (dialog6_frame1, "form", NULL, 0);
	XtManageChild (dialog6_rb_form);
	XtAddCallback (dialog6_rb_form, XmNfocusCallback, (XtCallbackProc) dialog6_help_textCB, (XtPointer) 1);
	ac = 0;
	XtSetArg (al[ac], XmNorientation, XmHORIZONTAL); ac++;
	dialog6_radio_box = XmCreateRadioBox (dialog6_rb_form, "radio_box", al, ac);
	XtManageChild (dialog6_radio_box);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_on_string), char_set)); ac++;
	dialog6_rb_on = XmCreateToggleButton (dialog6_radio_box, "toggle", al, ac);
	XtManageChild (dialog6_rb_on);
	XtAddCallback (dialog6_rb_on, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 11);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_off_string), char_set)); ac++;
	dialog6_rb_off = XmCreateToggleButton (dialog6_radio_box, "toggle", al, ac);
	XtManageChild (dialog6_rb_off);
	XtAddCallback (dialog6_rb_off, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 12);

	ac = 0;
	XtSetArg (al[ac], XmNmaxLength, 15); ac++;
	dialog6_text2 = XmCreateTextField (dialog6_frame2, "text", al, ac);
	XtManageChild (dialog6_text2);
	XtAddCallback (dialog6_text2, XmNactivateCallback, (XtCallbackProc) textCB, (XtPointer) 6);
	XtAddCallback (dialog6_text2, XmNlosingFocusCallback, (XtCallbackProc) textCB, (XtPointer) 6);
	XtAddCallback (dialog6_text2, XmNfocusCallback, (XtCallbackProc) dialog6_help_textCB, (XtPointer) 2);

	ac = 0;
	XtSetArg (al[ac], XmNmaxLength, 15); ac++;
	dialog6_text3 = XmCreateTextField (dialog6_frame3, "text", al, ac);
	XtManageChild (dialog6_text3);
	XtAddCallback (dialog6_text3, XmNactivateCallback, (XtCallbackProc) textCB, (XtPointer) 7);
	XtAddCallback (dialog6_text3, XmNlosingFocusCallback, (XtCallbackProc) textCB, (XtPointer) 7);
	XtAddCallback (dialog6_text3, XmNfocusCallback, (XtCallbackProc) dialog6_help_textCB, (XtPointer) 3);

	create_help_text (dialog6, dialog6_form3, dialog6_separator1, &dialog6_help_text, &dialog6_separator2);
	create_push_buttons (dialog6, dialog6_separator2, dialog6_ok, dialog6_cancel, dialog6_help, 7);

	dialog6_created = True;
}

void create_dialog7 ()
{
	int ac;
	Arg al[20];
	Widget dialog7_form1;
	Widget dialog7_frame1;
	Widget dialog7_label1;
	Widget dialog7_separator1, dialog7_separator2;
	Widget dialog7_ok, dialog7_cancel, dialog7_help;

	// create Service Advertising Protocol dialog
	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNnoResize, True); ac++;
	XtSetArg (al[ac], XmNautoUnmanage, False); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	dialog7 = XmCreateFormDialog (toplevel, "dialog", al, ac);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 2); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	dialog7_form1 = XmCreateForm (dialog7, "form", al, ac);
	XtManageChild (dialog7_form1);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	dialog7_frame1 = XmCreateFrame (dialog7_form1, "frame", al, ac);
	XtManageChild (dialog7_frame1);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_dialog7_string1), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, dialog7_frame1); ac++;
	dialog7_label1 = XmCreateLabel (dialog7_form1, "label", al, ac);
	XtManageChild (dialog7_label1);

	ac = 0;
	XtSetArg (al[ac], XmNmaxLength, 15); ac++;
	dialog7_text1 = XmCreateTextField (dialog7_frame1, "text", al, ac);
	XtManageChild (dialog7_text1);
	XtAddCallback (dialog7_text1, XmNactivateCallback, (XtCallbackProc) textCB, (XtPointer) 8);
	XtAddCallback (dialog7_text1, XmNlosingFocusCallback, (XtCallbackProc) textCB, (XtPointer) 8);
	XtAddCallback (dialog7_text1, XmNfocusCallback, (XtCallbackProc) dialog7_help_textCB, NULL);

	create_help_text (dialog7, dialog7_form1, dialog7_separator1, &dialog7_help_text, &dialog7_separator2);
	create_push_buttons (dialog7, dialog7_separator2, dialog7_ok, dialog7_cancel, dialog7_help, 10);

	dialog7_created = True;
}

void create_dialog8 ()
{
	int ac;
	Arg al[20];
	Widget dialog8_form3, dialog8_form4;
	Widget dialog8_frame3, dialog8_frame4;
	Widget dialog8_label3, dialog8_label4;
	Widget dialog8_separator1, dialog8_separator2;
	Widget dialog8_rb3_form, dialog8_radio_box3;
	Widget dialog8_ok, dialog8_cancel, dialog8_help;
//	Widget dialog8_form1, dialog8_form2;
// 	Widget dialog8_frame1, dialog8_frame2;
//	Widget dialog8_label1, dialog8_label2;
//	Widget dialog8_rb1_form, dialog8_radio_box1, dialog8_rb2_form, dialog8_radio_box2; 

	// create NetWare Management dialog
	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNnoResize, True); ac++;
	XtSetArg (al[ac], XmNautoUnmanage, False); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	dialog8 = XmCreateFormDialog (toplevel, "dialog", al, ac);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 2); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	dialog8_form3 = XmCreateForm (dialog8, "form", al, ac);
	XtManageChild (dialog8_form3);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, dialog8_form3); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	dialog8_form4 = XmCreateForm (dialog8, "form", al, ac);
	XtManageChild (dialog8_form4);

	//ac = 0;
	//XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	//XtSetArg (al[ac], XmNtopWidget, dialog8_form4); ac++;
	//XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	//XtSetArg (al[ac], XmNleftPosition, 1); ac++;                      ***********************************************
	//XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;   * nucm and hostmib taken from APIs            *
	//XtSetArg (al[ac], XmNrightPosition, 99); ac++;                    * form1 and form2 replaced by form3 and form4 *
	//dialog8_form3 = XmCreateForm (dialog8, "form", al, ac);           ***********************************************
	//XtManageChild (dialog8_form3);

	//ac = 0;
	//XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	//XtSetArg (al[ac], XmNtopWidget, dialog8_form3); ac++;
	//XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	//XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	//XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	//XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	//dialog8_form4 = XmCreateForm (dialog8, "form", al, ac);
	//XtManageChild (dialog8_form4);

	//ac = 0;
	//XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	//XtSetArg (al[ac], XmNleftPosition, 60); ac++;
	//XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	//dialog8_frame1 = XmCreateFrame (dialog8_form1, "frame", al, ac);
	//XtManageChild (dialog8_frame1);

	//ac = 0;
	//XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	//XtSetArg (al[ac], XmNleftPosition, 60); ac++;
	//XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	//dialog8_frame2 = XmCreateFrame (dialog8_form2, "frame", al, ac);
	//XtManageChild (dialog8_frame2);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 60); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	dialog8_frame3 = XmCreateFrame (dialog8_form3, "frame", al, ac);
	XtManageChild (dialog8_frame3);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 60); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	dialog8_frame4 = XmCreateFrame (dialog8_form4, "frame", al, ac);
	XtManageChild (dialog8_frame4);

	//ac = 0;
	//XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_dialog8_string1), char_set)); ac++;
	//XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	//XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	//XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	//XtSetArg (al[ac], XmNrightWidget, dialog8_frame1); ac++;
	//dialog8_label1 = XmCreateLabel (dialog8_form1, "label", al, ac);
	//XtManageChild (dialog8_label1);

	//ac = 0;
	//XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_dialog8_string2), char_set)); ac++;
	//XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	//XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	//XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	//XtSetArg (al[ac], XmNrightWidget, dialog8_frame2); ac++;
	//dialog8_label2 = XmCreateLabel (dialog8_form2, "label", al, ac);
	//XtManageChild (dialog8_label2);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_dialog8_string3), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, dialog8_frame3); ac++;
	dialog8_label3 = XmCreateLabel (dialog8_form3, "label", al, ac);
	XtManageChild (dialog8_label3);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_dialog8_string4), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, dialog8_frame4); ac++;
	dialog8_label4 = XmCreateLabel (dialog8_form4, "label", al, ac);
	XtManageChild (dialog8_label4);

	//dialog8_rb1_form = XmCreateForm (dialog8_frame1, "form", NULL, 0);
	//XtManageChild (dialog8_rb1_form);
	//XtAddCallback (dialog8_rb1_form, XmNfocusCallback, (XtCallbackProc) dialog8_help_textCB, (XtPointer) 1);
	//ac = 0;
	//XtSetArg (al[ac], XmNorientation, XmHORIZONTAL); ac++;
	//dialog8_radio_box1 = XmCreateRadioBox (dialog8_rb1_form, "radio_box", al, ac);
	//XtManageChild (dialog8_radio_box1);
	//ac = 0;
	//XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_on_string), char_set)); ac++;
	//dialog8_rb1_on = XmCreateToggleButton (dialog8_radio_box1, "toggle", al, ac);
	//XtManageChild (dialog8_rb1_on);
	//XtAddCallback (dialog8_rb1_on, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 13);
	//ac = 0;
	//XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_off_string), char_set)); ac++;
	//dialog8_rb1_off = XmCreateToggleButton (dialog8_radio_box1, "toggle", al, ac);
	//XtManageChild (dialog8_rb1_off);
	//XtAddCallback (dialog8_rb1_off, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 14);

	//dialog8_rb2_form = XmCreateForm (dialog8_frame2, "form", NULL, 0);
	//XtManageChild (dialog8_rb2_form);
	//XtAddCallback (dialog8_rb2_form, XmNfocusCallback, (XtCallbackProc) dialog8_help_textCB, (XtPointer) 2);
	//ac = 0;
	//XtSetArg (al[ac], XmNorientation, XmHORIZONTAL); ac++;
	//dialog8_radio_box2 = XmCreateRadioBox (dialog8_rb2_form, "radio_box", al, ac);
	//XtManageChild (dialog8_radio_box2);
	//ac = 0;
	//XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_on_string), char_set)); ac++;
	//dialog8_rb2_on = XmCreateToggleButton (dialog8_radio_box2, "toggle", al, ac);
	//XtManageChild (dialog8_rb2_on);
	//XtAddCallback (dialog8_rb2_on, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 15);
	//ac = 0;
	//XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_off_string), char_set)); ac++;
	//dialog8_rb2_off = XmCreateToggleButton (dialog8_radio_box2, "toggle", al, ac);
	//XtManageChild (dialog8_rb2_off);
	//XtAddCallback (dialog8_rb2_off, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 16);

	dialog8_rb3_form = XmCreateForm (dialog8_frame3, "form", NULL, 0);
	XtManageChild (dialog8_rb3_form);
	XtAddCallback (dialog8_rb3_form, XmNfocusCallback, (XtCallbackProc) dialog8_help_textCB, (XtPointer) 3);
	ac = 0;
	XtSetArg (al[ac], XmNorientation, XmHORIZONTAL); ac++;
	dialog8_radio_box3 = XmCreateRadioBox (dialog8_rb3_form, "radio_box", al, ac);
	XtManageChild (dialog8_radio_box3);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_on_string), char_set)); ac++;
	dialog8_rb3_on = XmCreateToggleButton (dialog8_radio_box3, "toggle", al, ac);
	XtManageChild (dialog8_rb3_on);
	XtAddCallback (dialog8_rb3_on, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 17);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_off_string), char_set)); ac++;
	dialog8_rb3_off = XmCreateToggleButton (dialog8_radio_box3, "toggle", al, ac);
	XtManageChild (dialog8_rb3_off);
	XtAddCallback (dialog8_rb3_off, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 18);

	ac = 0;
	XtSetArg (al[ac], XmNmaxLength, 15); ac++;
	dialog8_text = XmCreateTextField (dialog8_frame4, "text", al, ac);
	XtManageChild (dialog8_text);
	XtAddCallback (dialog8_text, XmNactivateCallback, (XtCallbackProc) textCB, (XtPointer) 9);
	XtAddCallback (dialog8_text, XmNlosingFocusCallback, (XtCallbackProc) textCB, (XtPointer) 9);
	XtAddCallback (dialog8_text, XmNfocusCallback, (XtCallbackProc) dialog8_help_textCB, (XtPointer) 4);

	create_help_text (dialog8, dialog8_form4, dialog8_separator1, &dialog8_help_text, &dialog8_separator2);
	create_push_buttons (dialog8, dialog8_separator2, dialog8_ok, dialog8_cancel, dialog8_help, 13);

	dialog8_created = True;
}
