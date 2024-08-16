#ident	"@(#)systuner:caw.C	1.11"
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
#include <Xm/Protocols.h>

mnemInfoPtr	mPtr;

XmStringCharSet char_set=XmSTRING_DEFAULT_CHARSET;

Widget make_menu_item (char *item_name, char *client_data, Widget menu)
{
	int ac;
	Arg al[20];
	Widget item;

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (item_name, char_set)); ac++;
	item = XmCreatePushButton (menu, item_name, al, ac);
	XtManageChild (item);
	XtAddCallback (item, XmNactivateCallback, (XtCallbackProc) menuCB, client_data);
	return (item);
}

void create_all_widgets (int *argc, char **argv)
{
	Widget menu, option_menu, separator1, separator2, separator3, scroll_win1, ok_button, reset_button, reset_factory_button, cancel_button, help_button;
	int ac, screen;
	Arg al[15];
	_category *temp;
	_parameter *t;
	Dimension height, width;
	Pixmap icon;
	Window root;
	Atom atom;

	// Set up Mnemonics
	mPtr = createMnemInfoRec ();

	// create the toplevel widget
	ac = 0;
	XtSetArg (al[ac], XmNtitle, getStr (TXT_title)); ac++;
	XtSetArg (al[ac], XtNmappedWhenManaged, False); ac++;
	XtSetArg (al[ac], XmNdeleteResponse, XmDO_NOTHING); ac++;                       //1-These four lines connect the
	toplevel = XtAppInitialize (&context, "System_Tuner", NULL, 0, argc, argv, NULL, al, ac);
        atom = XmInternAtom (XtDisplay(toplevel), "WM_DELETE_WINDOW", False);           //2-window manager "Close" to the
        XmAddWMProtocols (toplevel, &atom,  1);                                         //3-"Exit" code of the program.
        XmAddWMProtocolCallback (toplevel, atom, (XtCallbackProc) buttonCB,(XtPointer) CANCEL);//4-


	// set up display variables
	dpy = XtDisplay (toplevel);
	screen = DefaultScreen (dpy);
	root = RootWindow (dpy, screen);

	// create the form to hold all widgets
	form = XmCreateForm (toplevel, "form", NULL, 0);
	XtManageChild (form);

	// create and attach the option menu
	ac = 0;
	XtSetArg (al[ac], XmNtearOffModel, XmTEAR_OFF_ENABLED); ac++;
	menu = XmCreatePulldownMenu (form, "menu", al, ac);
	ac = 0;
	XtSetArg (al[ac], XmNsubMenuId, menu); ac++;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 1); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 20); ac++;
	option_menu = XmCreateOptionMenu (form, "menu", al, ac);
	XtManageChild (option_menu);
	temp = c_list;
	while (temp != NULL) {
		temp->menu_item = make_menu_item (temp->name, temp->name, menu);
		temp = temp->next;
	}
	XtSetSensitive (c_list->menu_item, False);

	// create separator
	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, option_menu); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	separator1 = XmCreateSeparator (form, "separator", al, ac);
	XtManageChild (separator1);

	// create left scrolled window and rowcolumn manager widget
	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, separator1); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 55); ac++;
	XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNbottomPosition, 70); ac++;
	XtSetArg (al[ac], XmNscrollingPolicy, XmAUTOMATIC); ac++;
	scroll_win1 = XmCreateScrolledWindow (form, "scrolled_window", al, ac);
	XtManageChild (scroll_win1);

	ac = 0;
	XtSetArg (al[ac], XmNorientation, XmVERTICAL); ac++;
	rowcolumn = XmCreateRowColumn (scroll_win1, "rowcolumn", al, ac);
	XtManageChild (rowcolumn);

	// create scrolled text widget
	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, separator1); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNleftWidget, scroll_win1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNbottomPosition, 70); ac++;
	XtSetArg (al[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
	XtSetArg (al[ac], XmNwordWrap, True); ac++;
	XtSetArg (al[ac], XmNeditable, False); ac++;
	XtSetArg (al[ac], XmNscrollHorizontal, False); ac++;
	text = XmCreateScrolledText (form, "scrolled_text", al, ac);
	XtManageChild (text);

	// create separator
	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, scroll_win1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	separator2 = XmCreateSeparator (form, "separator", al, ac);
	XtManageChild (separator2);

	// create scale widget
	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNtopPosition, 70); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 10); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 90); ac++;
	XtSetArg (al[ac], XmNshowValue, True); ac++;
	XtSetArg (al[ac], XmNorientation, XmHORIZONTAL); ac++;
	scale = XmCreateScale (form, "scale", al, ac);
	XtManageChild (scale);
	XtAddCallback (scale, XmNdragCallback, (XtCallbackProc) scaleCB, NULL);
	XtAddCallback (scale, XmNvalueChangedCallback, (XtCallbackProc) scaleCB, NULL);
	XtSetSensitive (scale, False);

	// create separator
	ac = 0;
	XtSetArg (al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNtopWidget, scale); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	separator3 = XmCreateSeparator (form, "separator", al, ac);
	XtManageChild (separator3);

	// create push buttons
	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_ok_button), char_set)); ac++;
	XtSetArg (al[ac], XmNnavigationType, XmTAB_GROUP); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNleftPosition, 1); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 16); ac++;
	XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNbottomPosition, 98); ac++;
	ok_button = XmCreatePushButton (form, "button", al, ac);
	XtVaSetValues(form,XmNdefaultButton,ok_button,NULL);
	XtManageChild (ok_button);
	XtAddCallback (ok_button, XmNactivateCallback, (XtCallbackProc) buttonCB, (XtPointer) OK);
	addMnemInfo (mPtr, ok_button, getStr(MNEM_ok), MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	nextMnemInfoSlot(mPtr);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_reset_button), char_set)); ac++;
	XtSetArg (al[ac], XmNnavigationType, XmTAB_GROUP); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNleftWidget, ok_button); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 32); ac++;
	XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNbottomPosition, 98); ac++;
	reset_button = XmCreatePushButton (form, "button", al, ac);
	XtManageChild (reset_button);
	XtAddCallback (reset_button, XmNactivateCallback, (XtCallbackProc) buttonCB, (XtPointer) RESET);
	addMnemInfo (mPtr, reset_button, getStr(MNEM_reset), MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	nextMnemInfoSlot(mPtr);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_reset_factory_button), char_set)); ac++;
	XtSetArg (al[ac], XmNnavigationType, XmTAB_GROUP); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNleftWidget, reset_button); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 68); ac++;
	XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNbottomPosition, 98); ac++;
	reset_factory_button = XmCreatePushButton (form, "button", al, ac);
	XtManageChild (reset_factory_button);
	XtAddCallback (reset_factory_button, XmNactivateCallback, (XtCallbackProc) buttonCB, (XtPointer) RESET_FACTORY);
	addMnemInfo (mPtr, reset_factory_button, getStr(MNEM_reset_factory), MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	nextMnemInfoSlot(mPtr);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_cancel_button), char_set)); ac++;
	XtSetArg (al[ac], XmNnavigationType, XmTAB_GROUP); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNleftWidget, reset_factory_button); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 84); ac++;
	XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNbottomPosition, 98); ac++;
	cancel_button = XmCreatePushButton (form, "button", al, ac);
	XtVaSetValues(form,XmNcancelButton,cancel_button,NULL);
	XtManageChild (cancel_button);
	XtAddCallback (cancel_button, XmNactivateCallback, (XtCallbackProc) buttonCB, (XtPointer) CANCEL);
	addMnemInfo (mPtr, cancel_button, getStr(MNEM_cancel), MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	nextMnemInfoSlot(mPtr);

	ac = 0;
	XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_help_button), char_set)); ac++;
	XtSetArg (al[ac], XmNnavigationType, XmTAB_GROUP); ac++;
	XtSetArg (al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg (al[ac], XmNleftWidget, cancel_button); ac++;
	XtSetArg (al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNrightPosition, 99); ac++;
	XtSetArg (al[ac], XmNbottomAttachment, XmATTACH_POSITION); ac++;
	XtSetArg (al[ac], XmNbottomPosition, 98); ac++;
	help_button = XmCreatePushButton (form, "button", al, ac);
	XtManageChild (help_button);
	XtAddCallback (form,XmNhelpCallback, (XtCallbackProc) buttonCB, (XtPointer) HELP);
	XtAddCallback (help_button, XmNactivateCallback, (XtCallbackProc) buttonCB, (XtPointer) HELP);
	addMnemInfo (mPtr, help_button, getStr(MNEM_help), MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	nextMnemInfoSlot(mPtr);

	applyMnemInfoOnShell (mPtr, form);


	// setting first default menu option
	get_list (c_list);
	t = c_list->head;
	while (t != NULL) {
		get_para_value (t);
		t = t->next;
	}
	item_selected (c_list);
	last_item = c_list->menu_item;
	last_list = c_list;
	ac = 0;
	XtSetArg (al[ac], XmNminimum, c_list->head->min); ac++;
	XtSetArg (al[ac], XmNmaximum, c_list->head->max); ac++;
	XtSetArg (al[ac], XmNvalue, c_list->head->current); ac++;
	XtSetValues (scale, al, ac);

	// check for permission to run this program
	if (!DtamIsOwner (dpy))
		XtSetSensitive (ok_button, False);

	// displaying icon
	getIconPixmap (XtDisplay (toplevel), XPM_FILE, &icon);
	setIconPixmap (toplevel, &icon, (unsigned char *) getStr (TXT_icon_label));

	XtRealizeWidget (toplevel);

	// ensure that only one instance of this application is running on desktop
	root = DtSetAppId (dpy, XtWindow (toplevel), "System_Tuner");
	if (root != None) {
		XMapWindow (dpy, root);
		XRaiseWindow (dpy, root);
		XSetInputFocus (dpy, root, RevertToNone, CurrentTime);
		XFlush (dpy);
		exit (0);
	}
	XtVaSetValues (toplevel, XtNmappedWhenManaged, (XtArgVal)True, NULL);
	XtMapWidget (toplevel);

	// set main window minimum sizes
	ac = 0;
	XtSetArg (al[ac], XmNheight, &height); ac++;
	XtSetArg (al[ac], XmNwidth, &width); ac++;
	XtGetValues (toplevel, al, ac);
	width = width + ((width / 100) * 30);
	height += 50;
	ac = 0;
	XtSetArg (al[ac], XmNheight, height); ac++;
	XtSetArg (al[ac], XmNminHeight, height); ac++;
	XtSetArg (al[ac], XmNwidth, width); ac++;
	XtSetArg (al[ac], XmNminWidth, width); ac++;
	XtSetValues (toplevel, al, ac);

	XtAppMainLoop (context);
}
