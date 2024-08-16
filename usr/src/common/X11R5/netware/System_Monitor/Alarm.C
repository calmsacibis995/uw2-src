#ident	"@(#)systemmon:Alarm.C	1.10"
////////////////////////////////////////////////////////////////////
// Alarm.C:  Set up the alarm window
/////////////////////////////////////////////////////////////////////
#include "Application.h"
#include "main.h"
#include "Alarm.h"
#include "WorkArea.h"
#include "Help.h"
#include "i18n.h"
#include "Buttons.h"
#include "ErrDialog.h"
#include <Xm/DialogS.h>
#include <Xm/PanedW.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>
#include <Xm/List.h>
#include <Xm/ToggleB.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "mnem.h"

Alarm *theAlarmManager = new Alarm ();

/****************************************************************
		 GLOBAL VARIABLES 
******************************************************************/
#define 	alarmHelpSect		"80"

/****************************************************************
		 HELP VARIABLES 
******************************************************************/
static HelpText AlarmHelp = { 
//	HELP_FILE, TXT_appHelp, TXT_alarmHelpSect, TXT_helptitle,
	HELP_FILE, TXT_appHelp, alarmHelpSect, TXT_helptitle,
};

static ButtonItem ActionItems[] = {
	{(XtArgVal)TXT_apply, (XtArgVal)MNEM_apply, 0, },
	{(XtArgVal)TXT_cancel, (XtArgVal)MNEM_cancel, 0, },
	{(XtArgVal)TXT_help, (XtArgVal)MNEM_help, (XtArgVal) 0,},
};
static char *mnemonics[3];

Alarm::Alarm () 
{
	_workarea = NULL;
	_list_position = 0;
	_selected = False;
	_pane = _w = NULL;

}

void Alarm::postDialog (Widget parent, char *name, WorkArea *w) 
{
	Arg		al[5];
	XmString	*xmstring, xmstr;
	int		ac, i, j;
	Dimension	h;
	Widget		_sep1;
	static int	first = 0;
	Widget	_shellForm;

	if (!first) {
		first = 1;
		for (i = 0; i < 3; i++ )
			mnemonics[i] = (char *)ActionItems[i]._mnem;
		Buttons::SetLabels (ActionItems, XtNumber (ActionItems));
		theHelpManager->SetHelpLabels ( &AlarmHelp);
	}

	/* if window exists then return
	 */
	if (_pane && XtIsManaged(XtParent(_pane)) )
		return;
	
	_workarea = w;
	_list_position = 0;
	_selected = True;
	xmstring = NULL;

	/* create the form dialog to hold all the control area
	 * and the buttons	
	 */
	_w = XtVaCreatePopupShell (name,  xmDialogShellWidgetClass, parent, 
					XmNtitle,I18n::GetStr (TXT_alarmsetup),
					XmNdeleteResponse, XmDESTROY,
					0);
	XtAddCallback (_w, XmNdestroyCallback, &Alarm::destroyCB, this);
	
	//
	// Create an under-laying form widget, all others placed on top.
 	//  We get cancelButton and defaultButton behavior this way.
	//
	_shellForm = XtVaCreateWidget ("shellForm", xmFormWidgetClass, _w,
				NULL);

	// The PanedWindow widget was not being sized correctly.
//	_pane = XtVaCreateWidget ("Alarmpane",xmPanedWindowWidgetClass,
	_pane = XtVaCreateManagedWidget ("Alarmpane",xmRowColumnWidgetClass,
				_shellForm, 
				XmNsashWidth, 1,
				XmNsashHeight, 1,
				0);

	/* create the control area form to hold the list and options
	 */
	_form = XtVaCreateWidget ("form",xmFormWidgetClass, _pane, 0);

	/* create the label for the sar list on top 
	 */
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_alarmlist),charset);
	_listlabel = XtVaCreateManagedWidget ("listlabel",
				xmLabelWidgetClass, _form,
				XmNlabelString, 	xmstr,	
				0);
	XmStringFree (xmstr);

	// Added the separator which went away when the PanedWindow changed.
	_sep1 = XmCreateSeparator (_pane, "labelSeparator", 0, 0);
	XtManageChild (_sep1);

	/* create a visual separator between the upper control 
	 * area and lower control area
	 */
	_sep = XtVaCreateManagedWidget ("separator",
				xmSeparatorWidgetClass, _form,
				XmNshadowThickness,  2,
				XmNtopAttachment,  	XmATTACH_WIDGET,
				XmNtopWidget,  		_listlabel,
				XmNrightAttachment,  	XmATTACH_FORM,
				XmNleftAttachment,  	XmATTACH_FORM,
				NULL);

	/* create the scrolled list for selected sar items
	 */
	ac = 0;
	XtSetArg (al[ac],XmNlistSizePolicy, XmCONSTANT); ac++;
	_alarmlist = XmCreateScrolledList(_form, "alarmlist", al, ac);
	XtManageChild (_alarmlist);

	/*******************************************************
	 * The compound Strings  containing the list of all items
	 * is stored here. Count the selected members and create
	 * space for them. Store compound strings and display.
	 *******************************************************/
	j = 0;
	for (i = 0; i < SAR_TOTAL; i++) 
		if (_workarea->_sar[i].selected) 
			j++;
	if (j != 0) 
		xmstring = (XmString *) XtMalloc  (sizeof (XmString) * j);
	j = 0;
	for (i = 0; i < SAR_TOTAL; i++) {
		if (_workarea->_sar[i].selected) 
			xmstring[j++] = XmStringCreateLtoR(
					_workarea->_sar[i].title, charset);
	}
	
	/* set the scrolled list items
	 */
	XtVaSetValues (_alarmlist,
			XmNscrollingPolicy, XmAUTOMATIC,
			XmNscrollBarDisplayPolicy, XmAS_NEEDED,
			XmNitems, 	xmstring,	
			XmNitemCount,	j,
			XmNvisibleItemCount, 6,
			0);

	/* attach list to the form
	 */
	XtVaSetValues (XtParent(_alarmlist),
			XmNtopAttachment,   XmATTACH_WIDGET,
			XmNtopWidget,       _sep,
			XmNleftAttachment,  XmATTACH_FORM,
			XmNrightAttachment,  XmATTACH_POSITION,
			XmNrightPosition,    70,
			XmNbottomAttachment,  XmATTACH_FORM,
			0);
	XtAddCallback (_alarmlist, XmNbrowseSelectionCallback, 
			&Alarm::ListCB, this);

	if (j != 0) {
		/* free the strings of the list items */
		for (i = 0; i < j; i++) 
			if (xmstring[i])
				XmStringFree (xmstring[i]);
		xmstring = NULL;
	}

	/* create a row column to hold the buttons for delete and set
	 * alarm and alarm values and beep/flash
	 */
	_alarmrc = XtVaCreateManagedWidget ("alarmrc",
					xmRowColumnWidgetClass, _form,
					XmNorientation,  XmVERTICAL,
					XmNshadowThickness,   2,
					XmNtopAttachment,  	XmATTACH_WIDGET,
					XmNtopWidget,  		_sep,
					XmNrightAttachment,  	XmATTACH_FORM,
					XmNleftAttachment,  XmATTACH_POSITION,
					XmNleftPosition,    70,
					XmNbottomAttachment,  XmATTACH_FORM,
					0);

	/* delete alarm push button goes first
	 */
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_deletealarm),charset);
	_deletealarm  = XtVaCreateManagedWidget ("deletealarm ",
				xmPushButtonWidgetClass, _alarmrc,
				XmNlabelString, 	xmstr,	
				XmNsensitive,		j == 0 ? False : True,
				0);
	XmStringFree (xmstr);
	XtAddCallback (_deletealarm, XmNactivateCallback,	
			(XtCallbackProc)DeleteAlarmCB, this);		
	
	/* beep toggle button
	 */
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_beep),charset);
	_beep = XtVaCreateManagedWidget ("beep",
				xmToggleButtonWidgetClass, _alarmrc,
				XmNlabelString, 	 xmstr,
				XmNset,			False,	
				XmNsensitive,		j == 0 ? False : True,
				XmNnavigationType,	XmTAB_GROUP,	
				0);
	XmStringFree (xmstr);

	/* flash toggle button
	 */
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_flash),charset);
	_flash = XtVaCreateManagedWidget ("flash",
				xmToggleButtonWidgetClass, _alarmrc,
				XmNlabelString, 	xmstr,
				XmNset,			False,	
				XmNnavigationType,	XmTAB_GROUP,	
				XmNsensitive,		j == 0 ? False : True,
				0);
	XmStringFree (xmstr);

	/* The label for alarm above
	 */
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_abovelabel),charset);
	_abovelabel = XtVaCreateManagedWidget ("abovelabel", 
					xmLabelWidgetClass, _alarmrc,
					XmNlabelString, 	xmstr, 
					XmNsensitive,		j == 0 ? 
								False : True,
					0);
	XmStringFree (xmstr);

	/* The text field to hold the alarm above value
	 */
	_abovetext =  XtVaCreateManagedWidget ("abovetext",
					xmTextFieldWidgetClass, _alarmrc,
					XmNcolumns, 	10,
					XmNsensitive,	j == 0 ? False : True,
					0);

	/* The label for alarm below
	 */
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_belowlabel),charset);
	_belowlabel = XtVaCreateManagedWidget ("belowlabel", 
					xmLabelWidgetClass, _alarmrc,
					XmNlabelString,		xmstr,
					XmNsensitive,	j == 0 ? False : True,
					0);
	XmStringFree (xmstr);

	/* The text field to hold the alarm below value
	 */
	_belowtext =  XtVaCreateManagedWidget ("belowtext",
						xmTextFieldWidgetClass,_alarmrc,
						XmNcolumns, 	10,
						XmNsensitive,	j == 0 ? 
								False : True,
						0);

	/* The Set alarm push button 
	 */
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_setalarm),charset);
	_setalarm  = XtVaCreateManagedWidget ("setalarm",
				xmPushButtonWidgetClass, _alarmrc,
				XmNlabelString, 	xmstr,	
				XmNsensitive,		j == 0 ? False : True,
				XmNnavigationType,	XmTAB_GROUP,	
				0);
	XmStringFree (xmstr);
	XtAddCallback (_setalarm, XmNactivateCallback,	
			(XtCallbackProc)SetAlarmCB, this);		

	/* Manage the form for the control area here
	 */
	XtManageChild (_form);

	/* create the form for the lower area to hold the
	 * action buttons
	 */
	_actionform = XtVaCreateManagedWidget ("actionform", xmFormWidgetClass, 
						_pane, 
						XmNfractionBase, 	7,
						NULL);

	/* set the function pointers to the appropriate callbacks
	 */
	ActionItems[0]._proc_ptr = (XtArgVal)Alarm::okCB;
	ActionItems[1]._proc_ptr = (XtArgVal)Alarm::cancelCB;
	ActionItems[2]._proc_ptr = (XtArgVal)Alarm::helpCB;

	/* create the action buttons
	 */
	int position = 1;
	XtTranslations			trans_table;
	trans_table = XtParseTranslationTable ("<Key>Return: ArmAndActivate()");
	for (i = 0; i < 3; i++)  {
		xmstr = XmStringCreateLtoR((char *)ActionItems[i]._label, 
						charset);
    		_actionbuttons[i] = XtVaCreateManagedWidget ("actionbuttons",
				xmPushButtonWidgetClass, _actionform, 
				XmNlabelString, 	xmstr,
//				XmNmnemonic, 		ActionItems[i]._mnem,
				XmNtopAttachment, 	XmATTACH_FORM,
				XmNbottomAttachment, 	XmATTACH_FORM,
				XmNleftAttachment, 	XmATTACH_POSITION,
				XmNleftPosition, 	position,
				XmNrightAttachment, 	XmATTACH_POSITION,
				XmNrightPosition, 	position + 1,
				XmNshowAsDefault, 	i == 0 ? True:False,
				XmNdefaultButtonShadowThickness, 	1,
				0);
		position += 2;
		XtAddCallback (_actionbuttons[i], XmNactivateCallback,	
			(XtCallbackProc)ActionItems[i]._proc_ptr, this);		
		XmStringFree (xmstr);

		XtOverrideTranslations (_actionbuttons[i], trans_table);
	}
	for(i = 0; i < 3; i++)
		registerMnemInfo(_actionbuttons[i], (char *)mnemonics[i], 
					MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(_w);

	/* manage the action form area
	 */
	XtManageChild (_actionform);

	/* make sure the action area does not resize
	 */
	XtVaGetValues (_actionbuttons[0], XmNheight, &h, 0);
	XtVaSetValues (_actionform, XmNpaneMaximum, h, XmNpaneMinimum, h, 0);

	/* manage the pane window and popup the transient shell
	 */
	XtManageChild (_shellForm);
	XtVaSetValues (_shellForm, 	XmNcancelButton, _actionbuttons[1], 
					XmNdefaultButton, _actionbuttons[0],
					0);
	XtPopup (_w, XtGrabNone);

	XmListSelectPos (_alarmlist, 1, True); 
}

Alarm::~Alarm ()
{
	_w = NULL;
}

/*****************************************************************
	Ok callback
*****************************************************************/
void Alarm::okCB (Widget w, XtPointer client_data, XtPointer ) 
{
	Alarm	*obj = (Alarm *) client_data;

	obj->ok (w);
}

/*****************************************************************
	Ok virtual function  - set the alarm for the last item
	selected in the list and destroy the popup. If the last
	item was not selected then popup an error dialog. If there
	were no options selected then ok button behaves like cancel
*****************************************************************/
void Alarm::ok(Widget w)
{
	if (!XtIsSensitive (_setalarm))
		cancel();
	else {
		/* If there was a selection
		 */
		if (_selected) {
			/* If the set alarm worked then pop off
			 */
			if (!setalarm())
				cancel();
		}
		else {
			theErrDialogMgr->postDialog (w, 
						I18n::GetStr (TXT_errdialog),
						I18n::GetStr (TXT_selectone));
			theErrDialogMgr->setModal();
		}
	}
}

/*****************************************************************
	Help callback
*****************************************************************/
void Alarm::helpCB (Widget w, XtPointer client_data, XtPointer ) 
{
	Alarm	*obj = (Alarm *) client_data;

	obj->help (w);
}

/*****************************************************************
	Call the Display Help routine to display help for this
	popup.
*****************************************************************/
void Alarm::help (Widget w)
{
	theHelpManager->DisplayHelp (w, &AlarmHelp);
}

/*****************************************************************
	Default cancel callback to pop the dialog off.  Calls
	virtual function cancel () which can be overridden
	in the derived class
*****************************************************************/
void Alarm::cancelCB (Widget, XtPointer client_data, XtPointer ) 
{
	Alarm	*obj = (Alarm *) client_data;

	obj->cancel ();
}

/*****************************************************************
	Pop off the dialog.  If the user needs to customize
	this then derive the virtual function in inherited
	class. Destroy the popup widget here.
*****************************************************************/
void Alarm::cancel()
{
	XtDestroyWidget(_w);
}

/*****************************************************************
	destroy callback uses this
*****************************************************************/
void Alarm::destroyCB (Widget , XtPointer client_data, XtPointer ) 
{
	Alarm	*obj = (Alarm *) client_data;

	obj->destroy ();
}

/*****************************************************************
	destroy the base widget
	Make sure you also set pane (popup's child) to null.
	So that when it returns it does not think that pane is
	still up.
*****************************************************************/
void Alarm::destroy()
{
	_w = _pane = NULL;
}

/*****************************************************************
	This callback is called from the list selection. Any time
	a list  item is selected.
*****************************************************************/
void Alarm::ListCB (Widget , XtPointer client_data, XtPointer call_data) 
{
	Alarm	*obj = (Alarm *) client_data;
	XmListCallbackStruct *cb = (XmListCallbackStruct *) call_data;

	obj->list (cb);
}

/*****************************************************************
	Once the list item is selected :
		set the _selected boolean to True;
		figure out which item in the sar table was selected.
		Save the position of this item in the sar list.
		Set the values for alarms, beep and flash with the
		values from the sar table.	
*****************************************************************/
void Alarm::list (XmListCallbackStruct *cb)
{
	char		buf[BUFSIZ];
	int		i;
	char		*title;
	
	_selected = True;
	_list_position = 0;
	XmStringGetLtoR (cb->item, charset, &title);
	for (i = 0; i < SAR_TOTAL; i++) {
		if (!strncmp (title, _workarea->_sar[i].title, 
				strlen (_workarea->_sar[i].title))) {
			
			XtVaSetValues (_abovetext, XmNvalue, "",  0);
			XtVaSetValues (_belowtext, XmNvalue, "",  0);

			/* Desensitize alarms if there have no alarms set
			 * for that sar option
			 */
			if (_workarea->_sar[i].is_alarm == 0)
				XtSetSensitive (_deletealarm, False);
			else 
				XtSetSensitive (_deletealarm, True);

			/* If the alarm is set, the display above field
			 * even if zero but not the below field 
			 */ 
			if (_workarea->_sar[i].is_alarm != 0) {
				sprintf (buf, "%d", 
					_workarea->_sar[i].alarm_over);
				XtVaSetValues (_abovetext, XmNvalue, buf,  0);

				if (_workarea->_sar[i].alarm_under != 0) {
					sprintf (buf, "%d", 
						_workarea->_sar[i].alarm_under);
					XtVaSetValues (_belowtext, 
							XmNvalue, buf,  
							0);
				}
			}
			/* Whatever beep/flash have been set at 
			 * use these values
			 */
			XtVaSetValues (_beep, 
					XmNset,	_workarea->_sar[i].beep,
					0);
			XtVaSetValues(_flash,
					XmNset,_workarea->_sar[i].flash,
					0);
			_list_position = i;
		}
	}
}

/*****************************************************************
	This callback is called from the set alarm button.
*****************************************************************/
void Alarm::SetAlarmCB (Widget , XtPointer client_data, XtPointer) 
{
	Alarm	*obj = (Alarm *) client_data;

	obj->setalarm ();
}
/***************************************************************** 
	This function gets the values of the alarms, beep and flash
	from the screen and sets the sar table values to these values.
	It uses the _list_position that we saved earlier when our
	item was selected.
*****************************************************************/
int Alarm::setalarm ()
{
	char 	*abovevalue, *belowvalue;
	int	beep, flash;

	/* reset the existing alarm values
	 */
	_workarea->_sar[_list_position].alarm_over =
	_workarea->_sar[_list_position].alarm_under = 0;
	_workarea->_sar[_list_position].beep = 
	_workarea->_sar[_list_position].flash = 0;
	_workarea->_sar[_list_position].is_alarm = False;
	_workarea->ResetSarToggle(_list_position);

	/* initialize the variables
	 */
	beep = flash = 0;

	/* First get all the beep/flash/above/below values
	 */
	XtVaGetValues (_beep, XmNset,  &beep, 0);
	XtVaGetValues (_flash, XmNset,  &flash, 0);
	XtVaGetValues (_abovetext, XmNvalue, &abovevalue, 0);
	XtVaGetValues (_belowtext, XmNvalue, &belowvalue, 0);

	/* If the alarm below value is zero - error return
	 */
	if (strlen (belowvalue) == 1 && atoi (belowvalue) == 0) {
		XtVaSetValues (_belowtext, XmNvalue, 0, 0);
		XmProcessTraversal (_belowtext, XmTRAVERSE_CURRENT);
		theErrDialogMgr->postDialog (_setalarm, 
				I18n::GetStr (TXT_errdialog),
				I18n::GetStr (TXT_nozero));
		theErrDialogMgr->setModal(); 
		return 1;
	}	

	/* If the above and below values are the same and they 
	 * are not zero - not allowed , reset and return.
	 */
	if ((atoi (abovevalue) == atoi(belowvalue)) && atoi(abovevalue) != 0) {
		XtVaSetValues (_abovetext, XmNvalue, 0, 0);
		XtVaSetValues (_belowtext, XmNvalue, 0, 0);
		XmProcessTraversal (_abovetext, XmTRAVERSE_CURRENT);
		theErrDialogMgr->postDialog (_setalarm, 
				I18n::GetStr (TXT_errdialog),
				I18n::GetStr (TXT_nosamevalues));
		theErrDialogMgr->setModal(); 
		return 1;
	}

	/* If the above values is null and the below value is null
	 * and the beep/flash values are not set then there has been
	 * no alarm set, fine go back
	 */
	if ((strlen (abovevalue) < 1) && (strlen(belowvalue) < 1)) {
		if (beep == 0 && flash == 0) {
			return 0;
		}
		/* However if the beep/flash have been set and the 
	 	 * above/below values were null that is not allowed
	 	 */
		else {
			theErrDialogMgr->postDialog (_setalarm, 
					I18n::GetStr (TXT_errdialog),
					I18n::GetStr (TXT_noalarmvalues));
			theErrDialogMgr->setModal(); 
			return 1;
		}
	}
	
	/* Check for errors in above/below values field and if none
	 * exists in both fields, set the table values and return success
	 * else reset the appropriate fields and set the tab cursor to
	 * right field and return
	 */
	if (!ErrorChecker (abovevalue)) {
		if (!ErrorChecker (belowvalue)) {
			_workarea->_sar[_list_position].is_alarm =  True;
			_workarea->_sar[_list_position].alarm_under = 
							atoi (belowvalue);
			_workarea->_sar[_list_position].alarm_over =
							atoi (abovevalue);
			_workarea->_sar[_list_position].flash = flash;
			_workarea->_sar[_list_position].beep = beep;
			XtSetSensitive (_deletealarm, True);
			return 0;
		}
		else {
			XtVaSetValues (_belowtext, XmNvalue, 0, 0);
			XmProcessTraversal (_belowtext, XmTRAVERSE_CURRENT);
		}
	}
	else {
		XtVaSetValues (_abovetext, XmNvalue, 0, 0);
		XmProcessTraversal (_abovetext, XmTRAVERSE_CURRENT);
	}
	return 1;
}

/*****************************************************************
	This error checker makes sure only numbers are entered
	in the alarm above and below fields;
*****************************************************************/
int Alarm::ErrorChecker (char *value)
{
	char 	c, *ptr;
	int	spacefound = 0, numfound = 0;

	/* validate for alphabets  or illegal characters
	 * minus sign etc.
	 */
	ptr = value;
	while (*ptr) {
		c = *ptr;
		if (isalpha (c) || ispunct (c) || (spacefound && isdigit(c))) {
			theErrDialogMgr->postDialog (_setalarm, 
					I18n::GetStr (TXT_errdialog),
					I18n::GetStr (TXT_numerical));
			theErrDialogMgr->setModal(); 
			return 1;
		}
		if (isspace(c)) {
			if (numfound)
				spacefound = 1;
		}
		if (isdigit(c))
			numfound = 1;
		ptr++;
	}

	/* if the alarm value entered is above the maximum scale
	 * allowed for that value then - error
	 */
	if (atoi (value) > _workarea->_sar[_list_position].scale) {
		theErrDialogMgr->postDialog (_setalarm, 
				I18n::GetStr (TXT_errdialog),
				I18n::GetStr (TXT_maxalarm));
		theErrDialogMgr->setModal(); 
		return 1;
	}

	/* if the number was large that it returns negative
	 * then return error
	 */
	if (atoi (value) < 0) {
		theErrDialogMgr->postDialog (_setalarm, 
				I18n::GetStr (TXT_errdialog),
				I18n::GetStr (TXT_numerical));
			theErrDialogMgr->setModal(); 
			return 1;
	}

#if 0
	/* if the alarm value entered is beyond 6 digits 
	 * then - error
	 */
	if (strlen (value) > 6) {
		theErrDialogMgr->postDialog (_setalarm, 
				I18n::GetStr (TXT_errdialog),
				I18n::GetStr (TXT_numerical));
			theErrDialogMgr->setModal(); 
			return 1;
	}
#endif

	return 0;
}

/*****************************************************************
	This callback is called from the delete alarm button.
*****************************************************************/
void Alarm::DeleteAlarmCB (Widget , XtPointer client_data, XtPointer ) 
{
	Alarm	*obj = (Alarm *) client_data;

	obj->deletealarm ();
}

/*****************************************************************
	It nullifies the alarms and resets the beep and flash values
	in the sar table.  Also clears the data from the screen.
*****************************************************************/
void Alarm::deletealarm ()
{
	_workarea->_sar[_list_position].alarm_over =
	_workarea->_sar[_list_position].alarm_under = 0;
	_workarea->_sar[_list_position].beep = 
	_workarea->_sar[_list_position].flash = 0;
	_workarea->_sar[_list_position].is_alarm = False;
	XtVaSetValues (_abovetext, XmNvalue, 0, 0);
	XtVaSetValues (_belowtext, XmNvalue, 0, 0);
	XtVaSetValues (_beep, XmNset, 0, 0);
	XtVaSetValues (_flash, XmNset, 0, 0);
	XtSetSensitive (_deletealarm, False);
	_workarea->ResetSarToggle(_list_position);
}
