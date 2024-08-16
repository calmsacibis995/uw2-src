#ident	"@(#)systemmon:MenuBar.C	1.3"
////////////////////////////////////////////////////////////////////
// MenuBar.C: 
/////////////////////////////////////////////////////////////////////
#include "Application.h"
#include "main.h"
#include "MenuBar.h"
#include "WorkArea.h"
#include "Prompt.h"
#include "Question.h"
#include "Options.h"
#include "Alarm.h"
#include "Select.h"
#include "Buttons.h"
#include "Help.h"
#include "i18n.h"
#include "ErrDialog.h"
#include <Xm/PushB.h>
#include <Xm/CascadeB.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

/*****************************************************************
		GLOBAL DEFINES
******************************************************************/
#define OPTION  	0 
#define LOG	 	1 
#define STOPLOG		2 
#define PLAYBACK	3 
#define SETTINGS	4 
/*#define PROCESS		5  */
#define ALARM		5 
#define EXIT  		6

/*****************************************************************
		EXTERN 
******************************************************************/
extern "C" void close_mets();

/*****************************************************************
		 HELP VARIABLES 
******************************************************************/
static HelpText TOCHelp = { HELP_FILE, TXT_tocHelp, 0, TXT_helptitle, };
extern HelpText AppHelp;

static ButtonItem MainItems [] = {
    { (XtArgVal) TXT_Actions, (XtArgVal) MNEM_Actions, },
    { (XtArgVal) TXT_View, (XtArgVal) MNEM_View, },
    { (XtArgVal) TXT_help, (XtArgVal) MNEM_help, },
};

static ButtonItem ActionItems [] = {
    { (XtArgVal) TXT_options, (XtArgVal) MNEM_options, (XtArgVal) OPTION,
	(XtArgVal)True,},
    { (XtArgVal) TXT_logtofile, (XtArgVal) MNEM_logtofile, (XtArgVal) LOG,
	(XtArgVal)True,},
    { (XtArgVal) TXT_stoplog, (XtArgVal) MNEM_stoplog,(XtArgVal) STOPLOG,
	(XtArgVal)False,},
    { (XtArgVal) TXT_playback,(XtArgVal) MNEM_playback,(XtArgVal) PLAYBACK,
	(XtArgVal)True,},
    { (XtArgVal) TXT_settings,(XtArgVal) MNEM_settings,(XtArgVal) SETTINGS,
	(XtArgVal)True,},
    { (XtArgVal) TXT_exit, (XtArgVal) MNEM_exit, (XtArgVal) EXIT, 
	(XtArgVal)True,},
};

static ButtonItem ViewItems [] = {
/*
    { (XtArgVal) TXT_process, (XtArgVal) MNEM_process, (XtArgVal) PROCESS,
	(XtArgVal)True,},
*/
    { (XtArgVal) TXT_alarm, (XtArgVal) MNEM_alarm, (XtArgVal) ALARM,
	(XtArgVal)True,},
};

static ButtonItem HelpItems [] = {
    { (XtArgVal) TXT_application, (XtArgVal) MNEM_application,
	  (XtArgVal) &AppHelp },		/* Application */
    { (XtArgVal) TXT_TOC, (XtArgVal) MNEM_TOC, (XtArgVal) &TOCHelp },								/* Table o' Contents */
    { (XtArgVal) TXT_helpDesk, (XtArgVal) MNEM_helpDesk, (XtArgVal) 0 },							/* Help Desk */
};

////////////////////////////////////////////////////////////////////
//	MenuBar widget class:	contains the bar for holding all 
//	the commands. 
////////////////////////////////////////////////////////////////////
MenuBar::MenuBar (Widget parent, char *name, WorkArea *workarea) 
			: BasicComponent (name)
{
	XtWidgetGeometry		size;
	int				i;

	_workArea = workarea;	
	_filename = NULL;

	/*	set the help labels	*/ 
	theHelpManager->SetHelpLabels ( &AppHelp);
	theHelpManager->SetHelpLabels ( &TOCHelp);
	
	/* Create the menuBar */
    	_w = XmCreateMenuBar ( parent, _name, NULL, 0);

	/* Set the labels for all the items in the MenuBar */
	Buttons::SetLabels (MainItems, XtNumber (MainItems));
	Buttons::SetLabels (ActionItems, XtNumber (ActionItems));
	Buttons::SetLabels (ViewItems, XtNumber (ViewItems));
	Buttons::SetLabels (HelpItems, XtNumber (HelpItems));

	/* Create the File Pull Down Menu to hold
	 * the Actions MenuBar
	 */
	_FilePullDown =  XmCreatePulldownMenu (_w, "Filepulldown", NULL, 0);
	
	/* Setup the cascade button for The Actions Button */
	XtVaCreateManagedWidget ("Actions", xmCascadeButtonWidgetClass, _w,
				XmNlabelString, XmStringCreateLtoR ( (char *)
						MainItems[0]._label, charset),
				XmNmnemonic, 	MainItems[0]._mnem,
				XmNsubMenuId,	_FilePullDown,
				NULL);	
	
	/* Under Actions set up all the button to hold the actions
	 */
	for (i = 0; i < XtNumber (ActionItems); i++) {

		_action_button[i] = XtVaCreateManagedWidget ("actionbutton",
					xmPushButtonWidgetClass, _FilePullDown,
					XmNlabelString, XmStringCreateLtoR(
					(char *)ActionItems[i]._label,charset),
					XmNmnemonic, 	ActionItems[i]._mnem,
					XmNuserData, ActionItems[i]._userdata,
					NULL);	

		XtAddCallback (_action_button[i], XmNactivateCallback, 
				&MenuBar::ActionsCB, (XtPointer)  this);
	}
	XtSetSensitive (_action_button[STOPLOG], False);

	/* Create the View Pull Down Menu to hold the view MenuBar
	 */
	_ViewPullDown =  XmCreatePulldownMenu (_w, "ViewPullDown", NULL, 0);
	
	/* Setup the cascade button for The Actions Button */
	/* Setup the cascade button for The View Button */
	XtVaCreateManagedWidget ("View", xmCascadeButtonWidgetClass, _w,
				XmNlabelString, XmStringCreateLtoR ( (char *)
						MainItems[1]._label, charset),
				XmNmnemonic, 	MainItems[1]._mnem,
				XmNsubMenuId,	_ViewPullDown,
				NULL);	
	
	/* Under View set up all the button to hold the process 
	 * alarms buttons 
	 */
	for (i = 0; i < XtNumber (ViewItems); i++) {

		_view_button[i] = XtVaCreateManagedWidget ("actionbutton",
					xmPushButtonWidgetClass, _ViewPullDown,
					XmNlabelString, XmStringCreateLtoR(
					(char *)ViewItems[i]._label,charset),
					XmNmnemonic, 	ViewItems[i]._mnem,
					XmNuserData, ViewItems[i]._userdata,
					NULL);	

		XtAddCallback (_view_button[i], XmNactivateCallback, 
				&MenuBar::ActionsCB, (XtPointer)  this);
	}
	
	
	/* Create the Help Pull Down Menu to hold
	 * the Help MenuBar to hold Title, TOC, HelpDesk
	 */
	_HelpPullDown =  XmCreatePulldownMenu (_w, "HelpPullDown", NULL, 0);

	_helpw = XtVaCreateManagedWidget ("Help", xmCascadeButtonWidgetClass, 
				_w,
				XmNlabelString, XmStringCreateLtoR ( (char *)
						MainItems[2]._label, charset),
				XmNmnemonic, 	MainItems[2]._mnem,	
				XmNsubMenuId,	_HelpPullDown,
				NULL);	
	
	/* Setup all the individual buttons to activate
	 * help under  Help pulldown menu
	 */
	for (i = 0; i < XtNumber (HelpItems); i++) {

		_helpbutton[i] = XtVaCreateManagedWidget ("OpenButton", 
				xmPushButtonWidgetClass, _HelpPullDown,
				XmNlabelString, XmStringCreateLtoR ( (char *)
						HelpItems[i]._label, charset),
				XmNmnemonic, 	HelpItems[i]._mnem,
				XmNuserData, 	HelpItems[i]._userdata,
				NULL);	

		XtAddCallback (_helpbutton[i], XmNactivateCallback, 
				&MenuBar::HelpCB, (XtPointer) this);
	}
	XtVaSetValues (_w, XmNmenuHelpWidget, _helpw, 0);

	/* Make sure the MenuBar does not resize
	 */
	size.request_mode = CWHeight;
	XtQueryGeometry (_w, NULL, &size);
	XtVaSetValues (_w,		/* set max. and min. size for pane */
			XmNpaneMinimum, size.height,
			XmNpaneMaximum, size.height,
			0);
}

MenuBar::~MenuBar()
{
	_workArea = NULL;
}

void MenuBar::ActionsCB(Widget w, XtPointer clientData, XtPointer )
{
	MenuBar *obj	=  (MenuBar *) clientData;

	obj->actions(w);
}

void MenuBar::actions(Widget w)
{
	int	cb;
	char 	buf[BUFSIZ], procno[BUFSIZ];


	XtVaGetValues (w, XmNuserData, &cb, 0);
	switch (cb) {

		case OPTION:	theOptionsMgr->postDialog (_w, "Options", 	
								_workArea);
				break;
		
		case LOG:	strcpy (buf, _workArea->GetLogFileName());
				sprintf (procno, ".%d", _workArea->GetCpu());
				strcat (buf, procno);
				thePromptMgr->postPrompt (_w, I18n::GetStr 
					(TXT_prompt), I18n::GetStr 
					(TXT_promptlabel),  buf, this);
				break;

		case  STOPLOG:  StopLoggingFile ();		
				break;
		case SETTINGS:	
				_workArea->SaveSettings(w);
				break;
		case  PLAYBACK:
				theSelectManager->postDialog (_w, "Select",
								_workArea);
				break;
		case ALARM:	
				theAlarmManager->postDialog (_w, "Alarms",
								_workArea);
				break;
		case  EXIT:
				close_mets();
				exit (0);
				break;
	}
}

////////////////////////////////////////////////////////////////////
//	Help Callback - When the help button is pushed 
//	call Dt Help with the Help class	
////////////////////////////////////////////////////////////////////
void MenuBar::HelpCB (Widget w, XtPointer clientData, XtPointer )
{
	MenuBar *obj	=  (MenuBar *) clientData;
	
	obj->help (w);
}

void MenuBar::help (Widget w)
{
	HelpText		*help;

	XtVaGetValues (w, XmNuserData, &help, 0);
	theHelpManager->DisplayHelp (w, help);
}

void MenuBar::OpenLogFile (int mode)
{
	/* open the log file in the mode that was passed
	 */
	_logFile.open (_filename, mode);

	/* release memory for filename */
	if (_filename)
		free(_filename);

	/* if the file has just been opened for output
	 * make sure the playback indicator is written first
	 */
	if (mode == ios::out)  {
		char buf[10];
		if (WriteLogFile (PLAYBACK_INDICATOR))
			return;
		sprintf (buf, "#%d\n", _workArea->GetCpu());
		if (WriteLogFile (buf))
			return;
	}

	/* if the file indicator did not exist then post error else
	 * gray out the save data button and sensitize the stop logging
	 * button and set the log flag to true
	 */
	if (!_logFile) {
		theErrDialogMgr->postDialog (_action_button[LOG], 
						I18n::GetStr (TXT_errdialog), 
						I18n::GetStr (TXT_nofile));
		theErrDialogMgr->setModal();
		return;
	}
	else {
		XtSetSensitive (_action_button[LOG], False);
		XtSetSensitive (_action_button[STOPLOG], True);
		_workArea->SetLog (True);
	}
}

void MenuBar::LogData(char *filename)
{
	/* create space for filename for use later by the OpenLogFile
	 * member function, because the OpenLogFile function is called
	 * by the Prompt and the Question classes
	 */
	_filename = strdup (filename);

	/* if file exists then post the question if user wants to append
	 * or overwrite else open the log file for output 
	 */
	if (access (filename, F_OK) == -1)  {
		OpenLogFile (ios::out);
	}
	else {
		theQuestionMgr->postQuestion (_action_button[LOG], 
						I18n::GetStr (TXT_logtitle), 
						I18n::GetStr (TXT_logq), this,
						LOGQ);
	}
}

int MenuBar::WriteLogFile (char *s)
{
	int		retcode = 0;

	while (*s)  {
		_logFile.put (*s++);
		if (!_logFile) { 
			_workArea->SetLog(False);
			theErrDialogMgr->postDialog (_w, I18n::GetStr 
				(TXT_errdialog),I18n::GetStr (TXT_logfailed));
			theErrDialogMgr->setModal();
			retcode = 1;
			break;
		}
	}
	return retcode;
}

void MenuBar::StopLoggingFile()
{
	_workArea->SetLog(False);
	if (_logFile)
		_logFile.close();
	XtSetSensitive (_action_button[STOPLOG], False);
	XtSetSensitive (_action_button[LOG], True);
}
