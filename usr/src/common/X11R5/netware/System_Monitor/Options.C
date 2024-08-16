#ident	"@(#)systemmon:Options.C	1.9"
////////////////////////////////////////////////////////////////////
// Options.C: Add to the graph panel 
/////////////////////////////////////////////////////////////////////
#include "Application.h"
#include "main.h"
#include "Options.h"
#include "WorkArea.h"
#include "Help.h"
#include "i18n.h"
#include "Buttons.h"
#include "ErrDialog.h"
#include <Xm/DialogS.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/ArrowB.h>
#include <Xm/Form.h>
#include <Xm/PanedW.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>
#include "mnem.h"

Options *theOptionsMgr = new Options ();

#define  MAXTIME 86400
#define  MINTIME 1	
#define  optionHelpSect		"30"

/****************************************************************
		 HELP VARIABLES 
******************************************************************/
static HelpText OptionsHelp = { 
//	HELP_FILE, TXT_appHelp, TXT_optionHelpSect, TXT_helptitle,
	HELP_FILE, TXT_appHelp, optionHelpSect, TXT_helptitle,
};

static ButtonItem ActionItems[] = {
	{(XtArgVal)TXT_apply, (XtArgVal)MNEM_apply, 0, },
	{(XtArgVal)TXT_cancel, (XtArgVal)MNEM_cancel, 0, },
	{(XtArgVal)TXT_help, (XtArgVal)MNEM_help, (XtArgVal) 0,},
};
static char *mnemonics[3];

/****************************************************************
	Options Class - CTOR . Initialize variables and set labels
	for action items and help values.
******************************************************************/
Options::Options () 
{
	_w = _pane = NULL;
	_workarea = NULL;
	_horizontal_set = _vertical_set = _proc_set = False;
	_which_proc = _incr = _range = 0;
	_arrow_timer_id = NULL;
}

/****************************************************************
	Post the dialog.  This public member function is called
	by other classes when the options menu needs to be posted.
******************************************************************/
void Options::postDialog (Widget parent, char *name, WorkArea *w) 
{
	char 		buf[8];
	int		i;
	Dimension 	h;
	XmString	xmstr;
	Arg		args[2];
	Widget	_sep;
	Widget	_shellForm;
	static int	first = 0;

	if (!first) {
		first = 1;
		for (i = 0; i < 3; i++ )
			mnemonics[i] = (char *)ActionItems[i]._mnem;
		Buttons::SetLabels (ActionItems, XtNumber (ActionItems));
		theHelpManager->SetHelpLabels ( &OptionsHelp);
	}

	/* if window exists then return
	 */
	if (_pane && XtIsManaged(XtParent(_pane)) ) 
		return;
	
	_workarea = w;

	_horizontal_set = _vertical_set = _proc_set = False;
	_incr = _range = 0;
	_which_proc = _workarea->GetCpu();
	_arrow_timer_id = NULL;

	/* create the form dialog to hold all the control area
	 * and the buttons	
	 */
	_w = XtVaCreatePopupShell (name, xmDialogShellWidgetClass, parent, 
				XmNtitle, I18n::GetStr (TXT_saroptions),
				NULL);
	XtAddCallback (_w, XmNdestroyCallback, &Options::destroyCB, this);

	//
	// Create an under-laying form widget, all others placed on top.
 	//  We get cancelButton and defaultButton behavior this way.
	//
	_shellForm = XtVaCreateWidget ("shellForm", xmFormWidgetClass, _w,
				NULL);
	// The PanedWindow widget was not being sized correctly.
//	_pane = XtVaCreateWidget ("Optionspane", xmPanedWindowWidgetClass, 
	_pane = XtVaCreateManagedWidget ("Optionspane", xmRowColumnWidgetClass, 
				_shellForm, 
				XmNsashWidth,   1,
				XmNsashHeight,  1,
				NULL);

	_control  = XtVaCreateWidget ("control", xmFormWidgetClass,  _pane, 0);

	_timerc = XtVaCreateManagedWidget ("timerc",
					xmRowColumnWidgetClass, _control,
					XmNorientation,  XmHORIZONTAL,
					0);
					
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_interval),charset);
	_interval = XtVaCreateManagedWidget ("interval",
				xmLabelWidgetClass, _timerc,
				XmNlabelString, 	xmstr,	
				0);
	XmStringFree (xmstr);

	_arrowup = XtVaCreateManagedWidget ("arrowup",
				xmArrowButtonWidgetClass, _timerc,
				XmNarrowDirection, 	XmARROW_UP,
				XmNuserData, 		1,
				0);
	XtAddCallback (_arrowup, XmNarmCallback, StartStopCB, this);
	XtAddCallback (_arrowup, XmNdisarmCallback, StartStopCB, this);

	_arrowdown = XtVaCreateManagedWidget ("arrowdown",
				xmArrowButtonWidgetClass, _timerc,
				XmNarrowDirection, 	XmARROW_DOWN,
				XmNuserData, 		-1,
				0);
	XtAddCallback (_arrowdown, XmNarmCallback, StartStopCB, this);
	XtAddCallback (_arrowdown, XmNdisarmCallback, StartStopCB, this);

	sprintf (buf, "%d", _workarea->GetTimer() / 1000);
	_timevalue =  XtVaCreateManagedWidget ("timevalue",
				xmTextFieldWidgetClass, _timerc,
				XmNvalue,	buf, 
				0);
	XtAddCallback (_timevalue, XmNvalueChangedCallback, ValueCB, this);

	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_seconds),charset);
	_seconds =  XtVaCreateManagedWidget ("seconds",
				xmLabelWidgetClass, _timerc,
				XmNlabelString,		xmstr, 
				0);
	XmStringFree (xmstr);

	_gridrc = XtVaCreateManagedWidget ("gridrc",
					xmRowColumnWidgetClass, _control,
					XmNorientation,  XmVERTICAL,
					XmNtopAttachment,  	XmATTACH_WIDGET,
					XmNtopWidget,  		_timerc,
					XmNrightAttachment,  	XmATTACH_FORM,
					XmNleftAttachment,  	XmATTACH_FORM,
					XmNbottomAttachment,  	XmATTACH_FORM,
					0);

	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_vertical),charset);
	_vertical = XtVaCreateManagedWidget ("vertical",
				xmToggleButtonWidgetClass, _gridrc,
				XmNlabelString,  xmstr,
				XmNset,		_workarea->DrawVT(),
				XmNalignment,		XmALIGNMENT_BEGINNING,
				0);
	XtAddCallback (_vertical, XmNvalueChangedCallback,	
			(XtCallbackProc)&VerticalCB, this);		
	XmStringFree (xmstr);

	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_horizontal),charset);
	_horizontal = XtVaCreateManagedWidget ("horizontal",
				xmToggleButtonWidgetClass, _gridrc,
				XmNlabelString, 	xmstr,	
				XmNset,		_workarea->DrawHZ(),
				XmNalignment,		XmALIGNMENT_BEGINNING,
				XmNnavigationType,	XmTAB_GROUP,
				0);
	XtAddCallback (_horizontal, XmNvalueChangedCallback,	
			(XtCallbackProc)&HorizontalCB, this);		
	XmStringFree (xmstr);

	/* create the pulldown option menu for the processor
	 * options. In case of one processor just show the label
	 * "This is a Single Processor"
	 */
	if (_workarea->NoCpus() == 1)  {
		xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_single), 
							charset);
		XtVaCreateManagedWidget ("procoptions", 
					xmLabelWidgetClass, _gridrc,
					XmNlabelString, xmstr,
					XmNalignment,	XmALIGNMENT_BEGINNING,
					0);  
		XmStringFree (xmstr);
	}
	else {
		_pulldown = XmCreatePulldownMenu(_gridrc,"pulldown", NULL, 0);

		xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_proc), charset);
		XtSetArg (args[0], XmNsubMenuId, _pulldown);
		XtSetArg (args[1], XmNlabelString, xmstr);
		_proc_menu = XmCreateOptionMenu(_gridrc,"proc_menu", args, 2);
		XmStringFree (xmstr);

		/* Create the pushbutton for Global as the first option */
		xmstr = XmStringCreateLtoR(I18n::GetStr(TXT_global), charset);
		_procbutton[0] = XtVaCreateManagedWidget ("procoptions", 
				xmPushButtonWidgetClass, _pulldown,
				XmNlabelString, xmstr,
				XmNuserData,	GLOBAL,
				0);  
		XtAddCallback(_procbutton[0], XmNactivateCallback, ProcCB, 
				this);
		XmStringFree (xmstr);

		/* all other processor options */
		for (i = 0; i < _workarea->NoCpus(); i++) {

			sprintf (buf, "%d", i);
			xmstr = XmStringCreateLtoR (buf, charset);
			_procbutton[i+1] = XtVaCreateManagedWidget(
					"procoptions", 
					xmPushButtonWidgetClass, _pulldown,
					XmNlabelString, xmstr,
					XmNuserData,	i,
					0);  
			XmStringFree (xmstr);

			XtAddCallback (_procbutton[i+1],  XmNactivateCallback,
					ProcCB, this);
		}
		XtManageChild (_proc_menu);

		/* Set the label to the value of the processor that was
	 	 * last chosen
	 	 */
		XtVaSetValues (_proc_menu, 
				XmNmenuHistory, _which_proc == GLOBAL ? 
				_procbutton[0] : _procbutton[_which_proc+1], 
				0);
	}
	
	XtManageChild (_control);

	// Added the separator which went away when the PanedWindow changed.
	_sep = XmCreateSeparator (_pane, "buttonSeparator", 0, 0);
	XtManageChild (_sep);

	/* create the form for the lower area to hold the
	 * action buttons
	 */
	_form = XtVaCreateWidget ("actionform", xmFormWidgetClass, _pane,
				XmNfractionBase, 7,
				XmNtopAttachment,  	XmATTACH_WIDGET,
				XmNtopWidget,  		_control,
				NULL);

	/* set the function pointers to the appropriate callbacks
	 */
	ActionItems[0]._proc_ptr = (XtArgVal)Options::okCB;
	ActionItems[1]._proc_ptr = (XtArgVal)Options::cancelCB;
	ActionItems[2]._proc_ptr = (XtArgVal)Options::helpCB;

	int position = 1;
	XtTranslations			trans_table;
	trans_table = XtParseTranslationTable ("<Key>Return: ArmAndActivate()");

	/* create the action buttons
	 */
	for (i = 0; i < 3; i++)  {
		xmstr = XmStringCreateLtoR((char *)ActionItems[i]._label, 
			charset);
    		_actionbuttons[i] = XtVaCreateManagedWidget ("actionbuttons",
				xmPushButtonWidgetClass, _form, 
				XmNlabelString, 	xmstr,
				XmNbottomAttachment, 	XmATTACH_FORM,
				XmNtopAttachment, 	XmATTACH_FORM,
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

	XtManageChild (_form);
	XtVaGetValues (_actionbuttons[0], XmNheight, &h, 0);
	XtVaSetValues (_form, XmNpaneMaximum, h, XmNpaneMinimum, h, 0);

	XtManageChild (_shellForm);
	XtVaSetValues (_shellForm, XmNcancelButton, _actionbuttons[1], 
			XmNdefaultButton, _actionbuttons[0],
			0);
	XtPopup (_w, XtGrabNone);
}

/****************************************************************
	Options	- DTOR
******************************************************************/
Options::~Options ()
{
	_w = NULL;
}

/*****************************************************************
	Help callback
*****************************************************************/
void Options::helpCB (Widget w, XtPointer client_data, XtPointer ) 
{
	Options	*obj = (Options *) client_data;

	obj->help (w);
}

void Options::help (Widget w)
{
	theHelpManager->DisplayHelp (w, &OptionsHelp);
}

/*****************************************************************
	Default cancel callback to pop the dialog off.  Calls
	virtual function cancel () which can be overridden
	in the derived class
*****************************************************************/
void Options::cancelCB (Widget w, XtPointer client_data, XtPointer ) 
{
	Options	*obj = (Options *) client_data;

	obj->cancel (w);
}

/*****************************************************************
	Pop off the dialog.  If the user needs to customize
	this then derive the virtual function in inherited
	class
*****************************************************************/
void Options::cancel(Widget)
{
	XtDestroyWidget(_w);
}

/*****************************************************************
	destroy callback uses this
*****************************************************************/
void Options::destroyCB (Widget, XtPointer client_data, XtPointer ) 
{
	Options	*obj = (Options *) client_data;

	obj->destroy ();
}

/*****************************************************************
	destroy the base widget
*****************************************************************/
void Options::destroy()
{
	_w = _pane = NULL;
}

void Options::StartStopCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Options	*obj = (Options *) client_data;
	XmArrowButtonCallbackStruct *cb = (XmArrowButtonCallbackStruct *)
					call_data;

	obj->start_stop(w, cb);
}

void Options::start_stop(Widget w, XmArrowButtonCallbackStruct *cb)
{
	XtVaGetValues (w, XmNuserData, &_incr, 0);

	if (cb->reason == XmCR_ARM)
		change_value ((XtPointer)this, cb->event->type==ButtonPress
				|| cb->event->type == KeyPress);
	else if (cb->reason == XmCR_DISARM)	{
		XtRemoveTimeOut (_arrow_timer_id);
	}
}

void Options::change_value (XtPointer client_data, XtIntervalId id)
{
	Options *obj = (Options *)client_data;
	char		buf[8];

	obj->GetTimeValue ();

	/* If the value is 86400 and you were trying to add 1 to it
	 * then you reset to 1
	 */
	if (obj->_range == MAXTIME && obj->_incr == 1)
		obj->_range =  MINTIME;

	/* If the value was 1 and you were trying to go further down
	 * you reset the value to 86400
	 */
	else if (obj->_range == MINTIME && obj->_incr == -1) 
		obj->_range =  MAXTIME;

	else 
		/* otherwise you just increment counters
		 * like you are supposed to
		 */
		obj->_range += obj->_incr;
	sprintf (buf, "%d", obj->_range);
	XtVaSetValues (obj->_timevalue, 
			XmNvalue, 	buf, 
			0);

	obj->_arrow_timer_id = XtAppAddTimeOut ( theApplication->appContext(),
					200, (XtTimerCallbackProc)
					&Options::change_value,(XtPointer)obj);
}

void Options::GetTimeValue ()
{
	char		*value;

	XtVaGetValues (_timevalue, XmNvalue, &value, NULL);
	_range = atoi(value);
}

void Options::HorizontalCB (Widget, XtPointer client_data, XtPointer ) 
{
	Options	*obj = (Options *) client_data;

	obj->horizontal ();
}

void Options::horizontal ()
{
	_horizontal_set = True;
}

void Options::VerticalCB (Widget, XtPointer client_data, XtPointer ) 
{
	Options	*obj = (Options *) client_data;

	obj->vertical();
}

void Options::vertical ()
{
	_vertical_set = True;
}

/*****************************************************************
	Ok callback
*****************************************************************/
void Options::okCB (Widget w, XtPointer client_data, XtPointer ) 
{
	Options	*obj = (Options *) client_data;

	obj->ok (w);
}

/*****************************************************************
	Ok virtual function 
*****************************************************************/
void Options::ok(Widget w)
{
	GetTimeValue ();
	if (_range > MAXTIME || _range < MINTIME) {
		theErrDialogMgr->postDialog (w, I18n::GetStr (TXT_rangetitle),
						I18n::GetStr (TXT_rangerr));
		theErrDialogMgr->setModal();
	}
	else {
		_workarea->SetTimer(_range * 1000);
		_workarea->PlotGraph();

		if (_horizontal_set)  {
			XtVaGetValues(_horizontal, XmNset, &_horizontal_set, 0);
			_workarea->HorizontalLines (_horizontal_set);
		}

		if (_vertical_set) {
			XtVaGetValues (_vertical, XmNset, &_vertical_set, 0);
			_workarea->VerticalLines (_vertical_set);
		}

		/*******************************************************
 		stop logging if the log file is being written. Ask the 
		user if he wants to stop the logging or not.  If he does, 
		then stop the logging , set cpu chosen, removes plot 
		timeout and clear graph, so that an expose event is 
		generated and graph is re-drawn from the beginning,
		else reset the options field.
		********************************************************/
		if (_proc_set) 
			_workarea->IfLog(w);
		else
			XtDestroyWidget(_w);
	}
}

void Options::ProcCB (Widget w, XtPointer client_data, XtPointer ) 
{
	Options	*obj = (Options *) client_data;

	obj->processor(w);
}

/*****************************************************************
	If the processor has been changed, reset the proc_set
	flag and get the new processor #.
*****************************************************************/
void Options::processor(Widget w)
{
	int		i;

	XtVaGetValues (w, XmNuserData, &i, 0);
	if (i != _which_proc)  { 
		_which_proc = i;
		_proc_set = True;
 	}
}

/*****************************************************************
	Set the processor value and remove the graph plotted 
*****************************************************************/
void Options::SetProc()
{
	_workarea->SetCpu (_which_proc);
	_workarea->RemovePlotTimeout();
	_workarea->SetProcLabel();
}

void Options::ValueCB (Widget w, XtPointer client_data, XtPointer ) 
{
	Options	*obj = (Options *) client_data;

	obj->value(w);
}

void Options::value(Widget w)
{
	XmTextPosition 	position;

	position = XmTextFieldGetLastPosition (w);
	if (position > 5) {
		theErrDialogMgr->postDialog (w, I18n::GetStr (TXT_rangetitle),
						I18n::GetStr (TXT_rangerr));
		theErrDialogMgr->setModal();
	}
}
