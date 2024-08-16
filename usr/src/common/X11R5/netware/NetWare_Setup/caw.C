#ident	"@(#)nwsetup:caw.C	1.17"
// caw.c

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
#include <Xm/Protocols.h> 

XmStringCharSet char_set=XmSTRING_DEFAULT_CHARSET;

Widget make_menu_item (char *item_name, int client_data, Widget menu)
{
	int ac;
	char a[2];
	Arg al[20];
	Widget item;

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (item_name, char_set)); ac++;
	item = XmCreatePushButton (menu, item_name, al, ac);
	XtManageChild (item);
	XtAddCallback (item, XmNactivateCallback, (XtCallbackProc) menuCB, (XtPointer) client_data);
	if (strcmp (item_name, getStr (TXT_exit_item)) == 0){
		XtVaSetValues (form,XmNcancelButton,item,NULL);
		strncpy(a, getStr (TXT_exit_mnemonic),1);
	}else if (strcmp (item_name, getStr (TXT_scs_item)) == 0)
		strncpy(a, getStr (TXT_scs_mnemonic),1);
	else if (strcmp (item_name, getStr (TXT_rps_item)) == 0)
		strncpy(a, getStr (TXT_rps_mnemonic),1);
	else if (strcmp (item_name, getStr (TXT_rds_item)) == 0)
		strncpy(a, getStr (TXT_rds_mnemonic),1);
	else if (strcmp (item_name, getStr (TXT_show_item)) == 0) {
		strncpy(a, getStr (TXT_show_mnemonic),1);
		XtSetSensitive (item, False);
	} else if (strcmp (item_name, getStr (TXT_hide_item)) == 0)
		strncpy(a, getStr (TXT_hide_mnemonic),1);
	else if (strcmp (item_name, getStr (TXT_nwsetup_help_item)) == 0){
		XtAddCallback(form,XmNhelpCallback,(XtCallbackProc) menuCB, (XtPointer) client_data);
		strncpy(a, getStr (TXT_nwsetup_help_mnemonic),1);
	}else if (strcmp (item_name, getStr (TXT_toc_help_item)) == 0)
		strncpy(a, getStr (TXT_toc_help_mnemonic),1);
	else if (strcmp (item_name, getStr (TXT_hd_help_item)) == 0)
		strncpy(a, getStr (TXT_hd_help_mnemonic),1);
	XtVaSetValues (item, XmNmnemonic, a[0], NULL);
	return (item);
}

Widget make_option_menu_item (char *item_name, int client_data, Widget menu)
{
	int ac;
	Arg al[20];
	Widget item;

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (item_name, char_set)); ac++;
	item = XmCreatePushButton (menu, item_name, al, ac);
	XtManageChild (item);
	XtAddCallback (item, XmNactivateCallback, (XtCallbackProc) option_menuCB, (XtPointer) client_data);
	return (item);
}

Widget make_menu (char *menu_name, Widget menu_bar)
{
	int ac;
	char a[2];
	Arg al[20];
	Widget menu, cascade;

	menu = XmCreatePulldownMenu (menu_bar, menu_name, NULL, 0);
	ac = 0;
	XtSetArg (al[ac], XmNsubMenuId, menu); ac++;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (menu_name, char_set)); ac++;
	cascade = XmCreateCascadeButton (menu_bar, menu_name, al, ac);
	XtManageChild (cascade);
	if (strcmp (menu_name, getStr (TXT_file_menu)) == 0) {
		strncpy (a, getStr (TXT_file_mnemonic),1);
		XtVaSetValues(form,XmNdefaultButton,cascade,NULL);
		XtVaSetValues (cascade, XmNmnemonic, a[0], NULL);
	} else if (strcmp (menu_name, getStr (TXT_action_menu)) == 0) {
		strncpy (a, getStr (TXT_action_mnemonic),1);
		XtVaSetValues (cascade, XmNmnemonic, a[0], NULL);
		XtVaSetValues (menu, XmNtearOffModel, XmTEAR_OFF_ENABLED, NULL);
	} else if (strcmp (menu_name, getStr (TXT_view_menu)) == 0) {
		strncpy (a, getStr (TXT_view_mnemonic),1);
		XtVaSetValues (cascade, XmNmnemonic, a[0], NULL);
	} else {
		strncpy (a, getStr (TXT_help_mnemonic),1);
		XtVaSetValues (cascade, XmNmnemonic, a[0], NULL);
		XtVaSetValues (menu_bar, XmNmenuHelpWidget, cascade, NULL);
	}
	return (menu);
}

void create_menus (Widget menu_bar)
{
	Widget file_menu, exit_item, action_menu, rps_item, rds_item, view_menu, help_menu, nc_help_item, toc_help_item, hd_help_item;

	// create the file menu
	file_menu = make_menu (getStr (TXT_file_menu), menu_bar);
	exit_item = make_menu_item (getStr (TXT_exit_item), 1, file_menu);

	// create the action menu
	action_menu = make_menu (getStr (TXT_action_menu), menu_bar);
	scs_item = make_menu_item (getStr (TXT_scs_item), 2, action_menu);
	rps_item = make_menu_item (getStr (TXT_rps_item), 3, action_menu);
	rds_item = make_menu_item (getStr (TXT_rds_item), 4, action_menu);

	// create the view menu
	view_menu = make_menu (getStr (TXT_view_menu), menu_bar);
	show_item = make_menu_item (getStr (TXT_show_item), 5, view_menu);
	hide_item = make_menu_item (getStr (TXT_hide_item), 6, view_menu);

	// create the help menu
	help_menu = make_menu (getStr (TXT_help_menu), menu_bar);
	nc_help_item = make_menu_item (getStr (TXT_nwsetup_help_item), 7, help_menu);
	toc_help_item = make_menu_item (getStr (TXT_toc_help_item), 8, help_menu);
	hd_help_item = make_menu_item (getStr (TXT_hd_help_item), 9, help_menu);
}

void create_all_widgets (int *argc, char **argv)
{
	XtAppContext context;
	Widget menu_bar, win_form;
	Widget form1, form2, form2a, form3, form4, form5, form6, form7, form8, form9, form10, form11;
	Widget string1, string2, string2a, string3, string4, string5, string6, string7, string8, string9, string10, string11;
	Widget frame1, frame2, frame2a, frame3, frame4, frame5, frame6, frame7, frame8, frame9, frame10, frame11;
	Widget rb1_form, radio_box1, rb2a_form, radio_box2a, rb6_form, radio_box6, rb7_form, radio_box7, rb9_form, radio_box9, rb10_form, radio_box10, rb11_form, radio_box11;
	Widget toggle5_form, toggle8_form, toggle8;
	Widget separator1, separator2;
	int ac, screen;
	Arg al[15];
	Dimension width, height;
	Pixmap icon;
	Atom atom;

	XtSetLanguageProc(NULL, NULL, NULL);


	// create the toplevel widget
	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XtNmappedWhenManaged, False); ac++;
	XtSetArg (al[ac], XmNdeleteResponse, XmDO_NOTHING); ac++;			//1-These four lines connect the 
	toplevel = XtAppInitialize (&context, "NetWare_Setup", NULL, 0, argc, argv, NULL, al, ac);
	atom = XmInternAtom (XtDisplay(toplevel), "WM_DELETE_WINDOW", False);		//2-window manager "Close" to the 
	XmAddWMProtocols (toplevel, &atom,  1);						//3-"Exit" code of the program. 
	XmAddWMProtocolCallback (toplevel, atom, (XtCallbackProc) menuCB,(XtPointer) 1);//4- 

	// set up for PList pixmaps
	dpy = XtDisplay (toplevel);
	screen = DefaultScreen (dpy);
	root = RootWindow (dpy, screen);

	// create error dialog
	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); ac++;
	error_dialog = XmCreateErrorDialog (toplevel, "dialog", al, ac);
	XtAddCallback (error_dialog, XmNokCallback, (XtCallbackProc) dialogCB, (XtPointer) 1);
	registerMnemInfo(XmMessageBoxGetChild(error_dialog,XmDIALOG_OK_BUTTON), getStr(MNEM_ok),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(error_dialog);
	XtUnmanageChild (XmMessageBoxGetChild (error_dialog, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild (XmMessageBoxGetChild (error_dialog, XmDIALOG_CANCEL_BUTTON));

	// create the form to hold all widgets
	form = XmCreateForm (toplevel, "form", NULL, 0);
	XtManageChild (form);

	// create and attach the menu bar
	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	menu_bar = XmCreateMenuBar (form, "menu_bar", al, ac);
	XtManageChild (menu_bar);
	create_menus (menu_bar);

	// create a separator
	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, menu_bar); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	separator1 = XmCreateSeparator (form, "separator", al, ac);
	XtManageChild (separator1);

	// create scrolled window and form
	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, separator1); ac++;
	XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNbottomPosition, 80); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNscrollingPolicy, XmAUTOMATIC); ac++;
	XtSetArg (al[ac], XmNscrollBarDisplayPolicy, XmAS_NEEDED); ac++;
	scrolled_win = XmCreateScrolledWindow (form, "scrolled window", al, ac);
	XtManageChild (scrolled_win);

	win_form = XmCreateForm (scrolled_win, "form", NULL, 0);
	XtManageChild (win_form);

	// create all forms to hold labels and frames
	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, separator1); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	form1 = XmCreateForm (win_form, "form", al, ac);
	XtManageChild (form1);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, form1); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	form2 = XmCreateForm (win_form, "form", al, ac);
	XtManageChild (form2);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, form2); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	form2a = XmCreateForm (win_form, "form", al, ac);
	XtManageChild (form2a);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, form2a); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	form3 = XmCreateForm (win_form, "form", al, ac);
	XtManageChild (form3);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, form3); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	form4 = XmCreateForm (win_form, "form", al, ac);
	XtManageChild (form4);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, form4); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	form5 = XmCreateForm (win_form, "form", al, ac);
	XtManageChild (form5);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, form5); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	form6 = XmCreateForm (win_form, "form", al, ac);
	XtManageChild (form6);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, form6); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	form7 = XmCreateForm (win_form, "form", al, ac);
	XtManageChild (form7);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, form7); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	form8 = XmCreateForm (win_form, "form", al, ac);
	XtManageChild (form8);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, form8); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	form9 = XmCreateForm (win_form, "form", al, ac);
	XtManageChild (form9);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, form9); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	form10 = XmCreateForm (win_form, "form", al, ac);
	XtManageChild (form10);

	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, form10); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	form11 = XmCreateForm (win_form, "form", al, ac);
	XtManageChild (form11);

	// create all frames
	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	frame1 = XmCreateFrame (form1, "frame", al, ac);
	XtManageChild (frame1);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	frame2 = XmCreateFrame (form2, "frame", al, ac);
	XtManageChild (frame2);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	frame2a = XmCreateFrame (form2a, "frame", al, ac);
	XtManageChild (frame2a);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	frame3 = XmCreateFrame (form3, "frame", al, ac);
	XtManageChild (frame3);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	frame4 = XmCreateFrame (form4, "frame", al, ac);
	XtManageChild (frame4);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	frame5 = XmCreateFrame (form5, "frame", al, ac);
	XtManageChild (frame5);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	frame6 = XmCreateFrame (form6, "frame", al, ac);
	XtManageChild (frame6);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	frame7 = XmCreateFrame (form7, "frame", al, ac);
	XtManageChild (frame7);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	frame8 = XmCreateFrame (form8, "frame", al, ac);
	XtManageChild (frame8);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	frame9 = XmCreateFrame (form9, "frame", al, ac);
	XtManageChild (frame9);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	frame10 = XmCreateFrame (form10, "frame", al, ac);
	XtManageChild (frame10);

	ac = 0;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 55); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	frame11 = XmCreateFrame (form11, "frame", al, ac);
	XtManageChild (frame11);

	// create all labels
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_string2), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, frame1); ac++;
	string1 = XmCreateLabel (form1, "label", al, ac);
	XtManageChild (string1);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_string1), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, frame2); ac++;
	string2 = XmCreateLabel (form2, "label", al, ac);
	XtManageChild (string2);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_string2a), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, frame2a); ac++;
	string2a = XmCreateLabel (form2a, "label", al, ac);
	XtManageChild (string2a);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_string3), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, frame3); ac++;
	string3 = XmCreateLabel (form3, "label", al, ac);
	XtManageChild (string3);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_string4), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, frame4); ac++;
	string4 = XmCreateLabel (form4, "label", al, ac);
	XtManageChild (string4);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_string5), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, frame5); ac++;
	string5 = XmCreateLabel (form5, "label", al, ac);
	XtManageChild (string5);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_string6), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, frame6); ac++;
	string6 = XmCreateLabel (form6, "label", al, ac);
	XtManageChild (string6);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_string7a), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, frame7); ac++;
	string7 = XmCreateLabel (form7, "label", al, ac);
	XtManageChild (string7);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_string8), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, frame8); ac++;
	string8 = XmCreateLabel (form8, "label", al, ac);
	XtManageChild (string8);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_string9), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, frame9); ac++;
	string9 = XmCreateLabel (form9, "label", al, ac);
	XtManageChild (string9);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_string10), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, frame10); ac++;
	string10 = XmCreateLabel (form10, "label", al, ac);
	XtManageChild (string10);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (getStr (TXT_string11), char_set)); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 20); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNrightWidget, frame11); ac++;
	string11 = XmCreateLabel (form11, "label", al, ac);
	XtManageChild (string11);
	
	// create the input field for server name
	ac = 0;
	XtSetArg (al[ac], XmNmaxLength, 256); ac++;
	XtSetArg (al[ac], XmNeditable, False); ac++;
	XtSetArg (al[ac], XmNcursorPositionVisible, False); ac++;
	text2 = XmCreateTextField (frame1, "text", al, ac);
	XtManageChild (text2);
	XtAddCallback (text2, XmNfocusCallback, (XtCallbackProc) help_textCB, (XtPointer) 2);

	// create input field for NetWare UNIX Client label
	rb1_form = XmCreateForm (frame2, "form", NULL, 0);
	XtManageChild (rb1_form);
	XtAddCallback (rb1_form, XmNfocusCallback, (XtCallbackProc) help_textCB, (XtPointer) 1);
	ac = 0;
	XtSetArg (al[ac], XmNorientation, XmHORIZONTAL); ac++;
	radio_box1 = XmCreateRadioBox (rb1_form, "radio_box", al, ac);
	XtManageChild (radio_box1);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_on_string), char_set)); ac++;
	rb1_on = XmCreateToggleButton (radio_box1, "toggle", al, ac);
	XtManageChild (rb1_on);
	XtAddCallback (rb1_on, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 1);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_off_string), char_set)); ac++;
	rb1_off = XmCreateToggleButton (radio_box1, "toggle", al, ac);
	XtManageChild (rb1_off);
	XtAddCallback (rb1_off, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 2);
	if (nuc_pkg_loaded)
 		XtSetSensitive (rb1_form, True);
	else
 		XtSetSensitive (rb1_form, False);

	// create input field for Enable IPX Auto Discovery label
	rb2a_form = XmCreateForm (frame2a, "form", NULL, 0);
	XtManageChild (rb2a_form);
	XtAddCallback (rb2a_form, XmNfocusCallback, (XtCallbackProc) help_textCB, (XtPointer) 3);
	ac = 0;
	XtSetArg (al[ac], XmNorientation, XmHORIZONTAL); ac++;
	radio_box2a = XmCreateRadioBox (rb2a_form, "radio_box", al, ac);
	XtManageChild (radio_box2a);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_on_string), char_set)); ac++;
	rb2a_on = XmCreateToggleButton (radio_box2a, "toggle", al, ac);
	XtManageChild (rb2a_on);
	XtAddCallback (rb2a_on, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 19);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_off_string), char_set)); ac++;
	rb2a_off = XmCreateToggleButton (radio_box2a, "toggle", al, ac);
	XtManageChild (rb2a_off);
	XtAddCallback (rb2a_off, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 20);

	// create the input field for IPX Internal LAN Address
	ac = 0;
	XtSetArg (al[ac], XmNmaxLength, 15); ac++;
	text3 = XmCreateTextField (frame3, "text", al, ac);
	XtManageChild (text3);
	XtAddCallback (text3, XmNlosingFocusCallback, (XtCallbackProc) textCB, (XtPointer) 2);
	// This taken out so that I could change the value of internal lan when losing focus
	//XtAddCallback (text3, XmNvalueChangedCallback, (XtCallbackProc) textCB, (XtPointer) 2);
	XtAddCallback (text3, XmNfocusCallback, (XtCallbackProc) help_textCB, (XtPointer) 4);

	// create the input field for IPX Maximum Hops
	ac = 0;
	XtSetArg (al[ac], XmNmaxLength, 15); ac++;
	text4 = XmCreateTextField (frame4, "text", al, ac);
	XtManageChild (text4);
	XtAddCallback (text4, XmNlosingFocusCallback, (XtCallbackProc) textCB, (XtPointer) 3);
	XtAddCallback (text4, XmNfocusCallback, (XtCallbackProc) help_textCB, (XtPointer) 5);

	// create the input field for Logical LAN Configuration
	toggle5_form = XmCreateForm (frame5, "form", NULL, 0);
	XtManageChild (toggle5_form);
	XtAddCallback (toggle5_form, XmNfocusCallback, (XtCallbackProc) help_textCB, (XtPointer) 11);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_string5a), char_set)); ac++;
	toggle5 = XmCreateToggleButton (toggle5_form, "toggle", al, ac);
	XtManageChild (toggle5);
	XtAddCallback (toggle5, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 3);

	// create the input field for Sequence Packet Exchange
	rb6_form = XmCreateForm (frame6, "form", NULL, 0);
	XtManageChild (rb6_form);
	XtAddCallback (rb6_form, XmNfocusCallback, (XtCallbackProc) help_textCB, (XtPointer) 6);
	ac = 0;
	XtSetArg (al[ac], XmNorientation, XmHORIZONTAL); ac++;
	radio_box6 = XmCreateRadioBox (rb6_form, "radio_box", al, ac);
	XtManageChild (radio_box6);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_on_string), char_set)); ac++;
	rb6_on = XmCreateToggleButton (radio_box6, "toggle", al, ac);
	XtManageChild (rb6_on);
	XtAddCallback (rb6_on, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 4);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_off_string), char_set)); ac++;
	rb6_off = XmCreateToggleButton (radio_box6, "toggle", al, ac);
	XtManageChild (rb6_off);
	XtAddCallback (rb6_off, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 5);

	// create the input field for Service Advertising Protocol
	rb7_form = XmCreateForm (frame7, "form", NULL, 0);
	XtManageChild (rb7_form);
	XtAddCallback (rb7_form, XmNfocusCallback, (XtCallbackProc) help_textCB, (XtPointer) 7);
	ac = 0;
	XtSetArg (al[ac], XmNorientation, XmHORIZONTAL); ac++;
	radio_box7 = XmCreateRadioBox (rb7_form, "radio_box", al, ac);
	XtManageChild (radio_box7);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_on_string), char_set)); ac++;
	rb7_on = XmCreateToggleButton (radio_box7, "toggle", al, ac);
	XtManageChild (rb7_on);
	XtAddCallback (rb7_on, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 6);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_off_string), char_set)); ac++;
	rb7_off = XmCreateToggleButton (radio_box7, "toggle", al, ac);
	XtManageChild (rb7_off);
	XtAddCallback (rb7_off, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 7);
	if (nuc_pkg_loaded)
 		XtSetSensitive (rb7_form, True);
	else
 		XtSetSensitive (rb7_form, False);

	// create the input field for Network Management
	toggle8_form = XmCreateForm (frame8, "form", NULL, 0);
	XtManageChild (toggle8_form);
	XtAddCallback (toggle8_form, XmNfocusCallback, (XtCallbackProc) help_textCB, (XtPointer) 11);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_string8a), char_set)); ac++;
	toggle8 = XmCreateToggleButton (toggle8_form, "toggle", al, ac);
	XtManageChild (toggle8);
	XtAddCallback (toggle8, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 8);
	if (netmgt_pkg_loaded)
 		XtSetSensitive (toggle8_form, True);
	else
 		XtSetSensitive (toggle8_form, False);

	// create the input field for Diagnostics Daemon
	rb9_form = XmCreateForm (frame9, "form", NULL, 0);
	XtManageChild (rb9_form);
	XtAddCallback (rb9_form, XmNfocusCallback, (XtCallbackProc) help_textCB, (XtPointer) 8);
	ac = 0;
	XtSetArg (al[ac], XmNorientation, XmHORIZONTAL); ac++;
	radio_box9 = XmCreateRadioBox (rb9_form, "radio_box", al, ac);
	XtManageChild (radio_box9);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_on_string), char_set)); ac++;
	rb9_on = XmCreateToggleButton (radio_box9, "toggle", al, ac);
	XtManageChild (rb9_on);
	XtAddCallback (rb9_on, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 9);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_off_string), char_set)); ac++;
	rb9_off = XmCreateToggleButton (radio_box9, "toggle", al, ac);
	XtManageChild (rb9_off);
	XtAddCallback (rb9_off, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 10);

	// create the input field for NUC Auto Authentication Panel
	rb10_form = XmCreateForm (frame10, "form", NULL, 0);
	XtManageChild (rb10_form);
	XtAddCallback (rb10_form, XmNfocusCallback, (XtCallbackProc) help_textCB, (XtPointer) 9);
	ac = 0;
	XtSetArg (al[ac], XmNorientation, XmHORIZONTAL); ac++;
	radio_box10 = XmCreateRadioBox (rb10_form, "radio_box", al, ac);
	XtManageChild (radio_box10);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_on_string), char_set)); ac++;
	rb10_on = XmCreateToggleButton (radio_box10, "toggle", al, ac);
	XtManageChild (rb10_on);
	XtAddCallback (rb10_on, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 21);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_off_string), char_set)); ac++;
	rb10_off = XmCreateToggleButton (radio_box10, "toggle", al, ac);
	XtManageChild (rb10_off);
	XtAddCallback (rb10_off, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 22);
	if (nuc_pkg_loaded)
 		XtSetSensitive (rb10_form, True);
	else
 		XtSetSensitive (rb10_form, False);

	// create the input field for Enable NetWare Single Login
	rb11_form = XmCreateForm (frame11, "form", NULL, 0);
	XtManageChild (rb11_form);
	XtAddCallback (rb11_form, XmNfocusCallback, (XtCallbackProc) help_textCB, (XtPointer) 10);
	ac = 0;
	XtSetArg (al[ac], XmNorientation, XmHORIZONTAL); ac++;
	radio_box11 = XmCreateRadioBox (rb11_form, "radio_box", al, ac);
	XtManageChild (radio_box11);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_on_string), char_set)); ac++;
	rb11_on = XmCreateToggleButton (radio_box11, "toggle", al, ac);
	XtManageChild (rb11_on);
	XtAddCallback (rb11_on, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 23);
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_off_string), char_set)); ac++;
	rb11_off = XmCreateToggleButton (radio_box11, "toggle", al, ac);
	XtManageChild (rb11_off);
	XtAddCallback (rb11_off, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, (XtPointer) 24);
	if (nuc_pkg_loaded)
 		XtSetSensitive (rb11_form, True);
	else
 		XtSetSensitive (rb11_form, False);

	// create second separator
	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, scrolled_win); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	separator2 = XmCreateSeparator (form, "separator", al, ac);
	XtManageChild (separator2);

	// create scrolled text for on-line help text
	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, separator2); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
	XtSetArg (al[ac], XmNwordWrap, True); ac++;
	XtSetArg (al[ac], XmNeditable, False); ac++;
	XtSetArg (al[ac], XmNscrollHorizontal, False); ac++;
	help_text = XmCreateScrolledText (form, "scrolled_text", al, ac);
	XtManageChild (help_text);

	// set initial values for main window
	set_values (True);
	collect_data ();

	// check for permission to run this program
	if (!DtamIsOwner (dpy))
		XtSetSensitive (scs_item, False);

	// displaying icon
	getIconPixmap (XtDisplay (toplevel), XPM_FILE, &icon);
	setIconPixmap (toplevel, &icon, (unsigned char *) getStr (TXT_icon_label));

	XtRealizeWidget (toplevel);

	// ensure that only one instance of this application is running on desktop
	root = DtSetAppId (dpy, XtWindow (toplevel), "NetWare_Setup");
	if (root != None) {
		XMapWindow (dpy, root);
		XRaiseWindow (dpy, root);
		XSetInputFocus (dpy, root, RevertToNone, CurrentTime);
		XFlush (dpy);
		exit (0);
	}
	XtVaSetValues (toplevel, XtNmappedWhenManaged, (XtArgVal)True, NULL);
	XtMapWidget (toplevel);

	// set main window size
	ac = 0;
	XtSetArg (al[ac], XmNwidth, &width); ac++;
	XtSetArg (al[ac], XmNheight, &height); ac++;
	XtGetValues (toplevel, al, ac);
	width += 225;
	height+= 100;
	ac = 0;
	XtSetArg (al[ac], XmNheight, height); ac++;
	XtSetArg (al[ac], XmNminHeight, height); ac++;
	XtSetArg (al[ac], XmNwidth, width); ac++;
	XtSetArg (al[ac], XmNminWidth, width); ac++;
	XtSetValues (toplevel, al, ac);

	XtAppMainLoop (context);
}
