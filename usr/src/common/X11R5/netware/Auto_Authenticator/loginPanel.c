/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoauthent:loginPanel.c	1.8"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Login.C: A login panel for authentication 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>
#include <Xm/MwmUtil.h>
#include <Xm/DialogS.h>
#include <Xm/PanedW.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/Separator.h>
#include <Xm/SelectioB.h>
#include "main.h"
#include "dtFuncs.h"

#include "mnem.h"

extern int apply_func (Widget form, Widget widget, char *server);
char * GetStr (char *idstr);
    
static Widget _w, _form,  
              _login_label, _login_text, 
              _pwd_label, _pwd_text;

static char *passwd;
static 	char *usersName;
static 	char *serverName;

static XtIntervalId	timer;

static void	okCB (Widget, XtPointer, XtPointer);
static void	helpCB (Widget, XtPointer, XtPointer);
static void	cancelCB (Widget, XtPointer, XtPointer);
static void	destroyCB (Widget, XtPointer, XtPointer);
static void passwdTextCB(Widget, XtPointer ,XtPointer );
static void loginTextFocusCB(Widget, XtPointer ,XtPointer );
static void LoginActivateOverride ( Widget , XEvent *, String *, Cardinal *);
static void PasswordActivateOverride ( Widget , XEvent *, String *, Cardinal *);
static void passwdFocusCB ( Widget , XtPointer , XtPointer );
void setInputFocus ();
void TimerProc (Widget widget, XEvent *event, String *pars, Cardinal *npars);
char * getUserName();
char * getPassword();
void clearPassword(void);

static Display *display;

/**********************************************************
	Action  Overrides
*********************************************************/

static XtActionsRec actions[] = {
    "LoginActivateOverride",    NULL, 
    "PasswordActivateOverride", NULL, 
};

char *
getUserName()
{
	return(strdup(usersName));
}
char *
getPassword()
{
	if ( passwd != NULL )
		return(strdup(passwd));
	return(NULL);
}

void
clearPassword()
{
    if (passwd) 
	{
		passwd[0] = NULL;
	}
	XtVaSetValues (_pwd_text, XmNvalue, (XtArgVal)NULL, NULL);
}
/*
 *	Post the dialog onto the screen.  Setup the widgets and
 *	post the dialog.
 */
Widget 
postDialog (Widget parent, char *title, char *argServerName, char *userName) 
{
	int		i = 0;
	Widget vsep, serverLabelWidget, serverNameWidget;

	int maxStringLength = 0;
	char *serverNameLabel;
	char *loginLabel;
	char *passwdLabel;
	char labelString[256];
	XmString okXmString;
	XmString helpXmString;
	XmString cancelXmString;

	int highlightThickness, shadowThickness, margin, labelMarginHeight;
	unsigned long bg;
	Widget shell;
    Atom WM_DELETE_WINDOW;
	long mwmFuncs;
    Arg args[4];
	XmString serverXmStr;
	XmString loginXmStr;
	XmString passwdXmStr;
	XmFontList fontList;
	Dimension height, width, maxWidth;


	/* 
	 * Check to see if we are still around
	 */
	if (_w && XtIsManaged (_w)) 
		return;

	display = XtDisplay(parent);
	passwd = NULL;
	actions[0].proc = LoginActivateOverride;
	actions[1].proc = PasswordActivateOverride;
	/*
	 * Save the users name, need it for clear testing 
	 */
	if ( userName != NULL )
		usersName = strdup(userName);
	else
		usersName = strdup("");
	serverName = strdup(argServerName);

   	/*
	 * create the prompt dialog container to hold the login and password fields 
   	 */
	i = 0;
	XtSetArg(args[i], XmNautoUnmanage, False);i++;
	_w = XmCreatePromptDialog(parent, "Prompt", args, i);
	XtVaSetValues(XtParent(_w), XmNtitle, title,NULL);

	okXmString = XmStringCreateLtoR (GetStr(TXT_ok),
	                                 XmSTRING_DEFAULT_CHARSET);
	cancelXmString = XmStringCreateLtoR (GetStr(TXT_cancel),
	                                     XmSTRING_DEFAULT_CHARSET);
	helpXmString = XmStringCreateLtoR (GetStr(TXT_help),
	                                   XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(_w, 
				XmNokLabelString, okXmString,
				XmNcancelLabelString, cancelXmString,
				XmNhelpLabelString, helpXmString,
				NULL);
	XmStringFree(okXmString);
	XmStringFree(helpXmString);
	XmStringFree(cancelXmString);
	/* 
	 * Don't show prompt dialog's selection label and text 
	 */
	XtUnmanageChild(XmSelectionBoxGetChild(_w,XmDIALOG_SELECTION_LABEL));
	XtUnmanageChild(XmSelectionBoxGetChild(_w, XmDIALOG_TEXT));

	/*
	 * Install callbacks on the buttons
	 */                
	XtAddCallback(_w,XmNokCallback,
					(XtCallbackProc)okCB,(XtPointer)NULL);
	XtAddCallback(_w,XmNcancelCallback,
					(XtCallbackProc)cancelCB,(XtPointer)NULL);
	XtAddCallback(_w,XmNhelpCallback,
						(XtCallbackProc)helpCB,(XtPointer)NULL);

	/*
	 * Disable these functions in the Window Managers menu
	 */
	for (shell = _w; !XtIsShell(shell); shell = XtParent(shell));
	XtVaGetValues(shell, XmNmwmFunctions, &mwmFuncs, NULL);
	mwmFuncs &= ~( MWM_FUNC_RESIZE | MWM_FUNC_ALL | 
					MWM_FUNC_MAXIMIZE | MWM_FUNC_MINIMIZE);
	XtVaSetValues(shell, XmNmwmFunctions, mwmFuncs, NULL);

	/* 
	 * Don't want toolkit to handle Close action so tell it to do nothing
	 */
   	 XtVaSetValues(shell, XmNdeleteResponse, XmDO_NOTHING, NULL);

	/*
	 * Install the WM_DELETE_WINDOW atom 
	 */
   	 WM_DELETE_WINDOW = XmInternAtom(XtDisplay(shell),"WM_DELETE_WINDOW",False);

	 /* 
	 * Add callback for the WM_DELETE_WINDOW protocol 
	 */
	XmAddWMProtocolCallback(shell,WM_DELETE_WINDOW,(XtCallbackProc)cancelCB,NULL);
	XtAddCallback (_w, XmNdestroyCallback,destroyCB, NULL);

	/* 
	 * create the Form widget to hold the Server name, login and
	 * password fields. Set shadow and highlightThickness to 0
	 */
	_form = XtVaCreateWidget ("form", xmFormWidgetClass, _w, 0);
	XtVaSetValues(_form, XmNshadowThickness,0,
						XmNhighlightThickness,0,
						NULL);

	/*
	 * Find the maximum length of all the label strings so that 
	 * we can align them properly
	 */
	serverNameLabel =GetStr(TXT_serverName);
	loginLabel = GetStr(TXT_login);
	passwdLabel = GetStr(TXT_pwd);
	i = strlen(serverNameLabel);
	(maxStringLength < i) ? maxStringLength=i:maxStringLength;
	i = strlen(loginLabel);
	(maxStringLength < i) ? maxStringLength=i:maxStringLength;
	i = strlen(passwdLabel);
	(maxStringLength < i) ? maxStringLength=i:maxStringLength;

	/*
	 * Find the maximum width of all the label strings so that 
	 * we can align them properly, proportional fonts make the
	 * strlen calculations only partially correct.
	 */
	serverXmStr = XmStringCreateLtoR (serverNameLabel,XmSTRING_DEFAULT_CHARSET);
	passwdXmStr = XmStringCreateLtoR (passwdLabel,XmSTRING_DEFAULT_CHARSET);
	loginXmStr = XmStringCreateLtoR (loginLabel,XmSTRING_DEFAULT_CHARSET);

	XtVaGetValues(shell,XmNlabelFontList,&fontList,NULL);
	XmStringExtent(fontList,serverXmStr,&maxWidth,&height);
	XmStringExtent(fontList,passwdXmStr,&width,&height);
	(maxWidth < width) ? maxWidth=width:maxWidth;
	XmStringExtent(fontList,loginXmStr,&width,&height);
	(maxWidth < width) ? maxWidth=width:maxWidth;

	/*
	 * Create the server label string 
	 * taking into account proportional width fonts.
	 */
	while(1)
	{
		labelString[0] = NULL;
		for (i = strlen(serverNameLabel); i < maxStringLength; i++)
		{
			strcat(labelString," ");
		}
		strcat(labelString,serverNameLabel);
		if ( serverXmStr != NULL )
			XmStringFree(serverXmStr);
		serverXmStr = XmStringCreateLtoR (labelString,XmSTRING_DEFAULT_CHARSET);
		XmStringExtent(fontList,serverXmStr,&width,&height);
		if (width  <= maxWidth )
			maxStringLength += 2;
		else
			break;
	}

	serverLabelWidget = XtVaCreateManagedWidget ("ServerName_label",
						xmLabelWidgetClass, _form,
						XmNlabelString, serverXmStr,
						XmNleftAttachment, XmATTACH_FORM,
						XmNtopAttachment, XmATTACH_FORM,
						0);

	/*
	 * Create the invisible Vertical separator that keeps everybody
	 * lined up
	 */
	vsep = XtVaCreateManagedWidget ("sep",
						xmSeparatorWidgetClass, _form,
						XmNorientation, XmVERTICAL,
						XmNbottomAttachment, XmATTACH_FORM,
						XmNtopAttachment, XmATTACH_FORM,
						XmNleftAttachment, XmATTACH_WIDGET,
						XmNleftWidget, serverLabelWidget,
						XmNseparatorType, XmNO_LINE,
						0);
	/*
	 * Create the server name label string widget
	 */
	serverNameWidget = XtVaCreateManagedWidget ("slc-univel",
						xmLabelWidgetClass, _form,
						XmNlabelString,
						XmStringCreateLtoR ((serverName),
										XmSTRING_DEFAULT_CHARSET),
						XmNrightAttachment, XmATTACH_FORM,
						XmNtopAttachment, XmATTACH_FORM,
						XmNleftAttachment, XmATTACH_WIDGET,
						XmNleftWidget, vsep,
						XmNalignment, XmALIGNMENT_BEGINNING,
						0);

	/*
	 * Create the login label and text widget
	 */
	_login_label = XtVaCreateManagedWidget ("Login_label",
						xmLabelWidgetClass, _form,
						XmNlabelString, loginXmStr,
						XmNrightAttachment, XmATTACH_WIDGET,
						XmNrightWidget, vsep,
						0);
	_login_text = XtVaCreateManagedWidget ("Login_text",
						xmTextFieldWidgetClass, _form,
						XmNrightAttachment, XmATTACH_FORM,
						XmNtopAttachment, XmATTACH_WIDGET,
						XmNtopWidget, serverNameWidget,
						XmNleftAttachment, XmATTACH_WIDGET,
						XmNleftWidget, vsep,
						XmNeditMode, XmSINGLE_LINE_EDIT,
						0);


	XtVaGetValues(_login_label, XmNmarginHeight,&labelMarginHeight,NULL);
	XtVaGetValues(_login_text, XmNshadowThickness,&shadowThickness,
						XmNhighlightThickness,&highlightThickness,
						XmNmarginHeight,&margin,
						NULL);
	XtVaSetValues (_login_label, 
						XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
						XmNtopWidget, _login_text,
						XmNmarginTop, margin + shadowThickness + 
						              highlightThickness - labelMarginHeight,
						NULL);

	/*
	 * Pre-load the user name in the login text field
	 */
	XtVaSetValues (_login_text, 
						XmNvalue, (XtArgVal)usersName,
						NULL);
	/*
	 * Create the Password label and text widgets
	 */
	_pwd_label = XtVaCreateManagedWidget ("Password_label",
						xmLabelWidgetClass, _form,
						XmNlabelString, passwdXmStr,
						XmNrightAttachment, XmATTACH_WIDGET,
						XmNrightWidget,	vsep,
						0);
	_pwd_text = XtVaCreateManagedWidget ("Passwd_text",
						xmTextWidgetClass, _form,
						XmNrightAttachment, XmATTACH_FORM,
						XmNtopAttachment, XmATTACH_WIDGET,
						XmNtopWidget, _login_text,
						XmNleftAttachment, XmATTACH_WIDGET,
						XmNleftWidget, vsep,
						XmNeditMode, XmSINGLE_LINE_EDIT,
						XmNverifyBell, False,
						XmNcursorPositionVisible, False,
						0);
	XtVaSetValues (_pwd_label, 
						XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
						XmNtopWidget, _pwd_text,
						XmNmarginTop, margin + shadowThickness + 
						              highlightThickness - labelMarginHeight,
						NULL);
	/*
	 * Don't want spies to see the users password
	 */
	XtVaGetValues (_pwd_text, XtNbackground, &bg, NULL);
	XtVaSetValues (_pwd_text, XtNforeground, bg, NULL);

	/*
	 * Add a mnemonic to each button
	 */

	registerMnemInfo(XmSelectionBoxGetChild(_w, XmDIALOG_OK_BUTTON), 
				GetStr(MNEM_ok), MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfo(XmSelectionBoxGetChild(_w, XmDIALOG_CANCEL_BUTTON), 
				GetStr(MNEM_cancel),MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfo(XmSelectionBoxGetChild(_w, XmDIALOG_HELP_BUTTON), 
				GetStr(MNEM_help), MNE_ACTIVATE_BTN | MNE_GET_FOCUS);

	registerMnemInfoOnShell(_w);

	XtRemoveAllCallbacks( XmSelectionBoxGetChild(_w, XmDIALOG_HELP_BUTTON), 
	                           XmNactivateCallback) ;
	XtAddCallback (XmSelectionBoxGetChild(_w, XmDIALOG_HELP_BUTTON), 
	                           XmNactivateCallback, helpCB, NULL);
	XtAddCallback(_w,XmNhelpCallback,
	                           (XtCallbackProc)helpCB,(XtPointer)NULL);

	/*
	 * Override the default activate actions for the two text fields
	 */
	XtAppAddActions (XtWidgetToApplicationContext(shell),actions, XtNumber (actions));
	XtOverrideTranslations(_login_text,
			XtParseTranslationTable("<Key>Return: LoginActivateOverride()"));
	XtOverrideTranslations(_pwd_text,
			XtParseTranslationTable("<Key>Return: PasswordActivateOverride()"));
	XtVaSetValues(_login_text,XmNuserData,NULL,NULL);
	XtVaSetValues(_pwd_text,XmNuserData,NULL,NULL);
	
	/*
	 * Make tab groups
	 */
	XmAddTabGroup(_login_text);
	XmAddTabGroup(_pwd_text);

	/*
	 * Install callbacks so we can look at/save users entered data
	 */
	XtAddCallback(_pwd_text,XmNmodifyVerifyCallback,
					(XtCallbackProc)passwdTextCB,(XtPointer)NULL);
	XtAddCallback(_pwd_text,XmNfocusCallback,
					(XtCallbackProc)passwdFocusCB,(XtPointer)NULL);

	/* 	
	 * Manage the form and dialog and set focus the the login text field
	 */
	XtManageChild (_form);
	XmProcessTraversal(_login_text, XmTRAVERSE_CURRENT);

	/*
	 * Causes Selection on entry
	 */
	XtAddCallback(_login_text,XmNfocusCallback,
					(XtCallbackProc)loginTextFocusCB,(XtPointer)NULL);
	/*
	 * Causes de-selection on exit
	 */
	XtAddCallback(_login_text,XmNlosingFocusCallback,
					(XtCallbackProc)loginTextFocusCB,(XtPointer)NULL);
	
	timer = XtAppAddTimeOut (XtWidgetToApplicationContext(_w), 
	                        60000, (XtTimerCallbackProc) TimerProc, NULL);	
	return(_w);
}

/*****************************************************************
	Ok callback
*****************************************************************/
void 
okCB (Widget w, XtPointer client_data, XtPointer call_data ) 
{
	char *textFieldValue;
	XtVaGetValues(_login_text,XmNvalue,&textFieldValue,NULL);
	if ( strcmp(usersName,textFieldValue) != 0 )
	{
		free(usersName);
		usersName = strdup(textFieldValue);
	}
	apply_func (_w, w, serverName);
}

/*****************************************************************
	Help callback
*****************************************************************/
void 
helpCB (Widget w, XtPointer client_data, XtPointer call_data) 
{
	
	extern HelpText AppHelp;
	Widget widget = _w;

    while (!XtIsShell (widget))
            widget = XtParent(widget);
	displayHelp (widget, &AppHelp);
}

/*****************************************************************
	Cancel callback 
	Pop off the dialog.  
*****************************************************************/
void
cancelCB (Widget w, XtPointer client_data, XtPointer call_data ) 
{
	sendCloseFolderReq(_w);
	XtDestroyWidget (_w);
	exit (0);
}

/*****************************************************************
	destroy callback
*****************************************************************/
void 
destroyCB (Widget w , XtPointer client_data, XtPointer call_data ) 
{
	_w = NULL;
}

/*****************************************************************
	Login text field focus callback
    Select or deselect the text field contents.
*****************************************************************/
void
loginTextFocusCB(Widget w, XtPointer client_data ,XtPointer call_data )
{

    XmTextVerifyCallbackStruct * cbs =(XmTextVerifyCallbackStruct *) call_data;

	if ( cbs->reason == XmCR_LOSING_FOCUS )
	{
		XmTextFieldClearSelection(_login_text,(XmTextPosition)0);
	}
	else if  ( cbs->reason == XmCR_FOCUS )
	{
		XmTextPosition first;
		XmTextPosition last;

		first = (XmTextPosition) 0;
		last = XmTextFieldGetLastPosition(_login_text);
		XmTextFieldSetInsertionPosition(_login_text, last);
		XmTextFieldSetSelection(_login_text, first, last, CurrentTime);
	}
}
/*****************************************************************
	Password text field modify callback
	Save each password character into the private member handling
    backspaces and paste operations.  Maximum one character at a
	time.
*****************************************************************/
void
passwdTextCB(Widget w, XtPointer client_data ,XtPointer call_data )
{
    XmTextVerifyCallbackStruct * cbs =(XmTextVerifyCallbackStruct *) call_data;
    char *newStr;

	if (timer)	
	{
		XtRemoveTimeOut (timer);
		timer = NULL;
	}
	/*
	 * Handle the backspace character case
	 */
    if (cbs->text->ptr == NULL || cbs->text->length == 0) 
	{
        if (strlen(passwd) <= 0) 
		{
        	cbs->endPos = cbs->startPos = 0;
			return; 
		}
        cbs->endPos = strlen(passwd); /* delete from here to end */
        passwd[cbs->startPos] = 0;    /* backspace--terminate */
        return;
    }

	/*
	 * don't allow "paste" operations 
	 * make the user *type* the password!
	 */
    if (cbs->text->length > 1) 
	{
        cbs->doit = False; 
        return; 
    }

	/*
	 * Save the new password character into our local buffer
	 * allocating more space each time.
	 */
    newStr = XtMalloc((unsigned int)cbs->endPos + 2 ); /* new char + NULL terminator */
    if (passwd) 
	{
        strncpy(newStr, passwd,(unsigned int)cbs->endPos);
        newStr[cbs->endPos] = NULL;
        XtFree(passwd);
    } 
	else
        newStr[0] = NULL;
    passwd = newStr;
    strncat(passwd, cbs->text->ptr, 1);
    passwd[cbs->endPos + 1] = 0;

	/*
	 * Change the text to a '*' incase someone wants
	 * to cut the password from the text field
	 */
	cbs->text->ptr[0] = '*';
}

/*****************************************************************
* Text field activate override routines
*****************************************************************/
void
LoginActivateOverride ( Widget widget, XEvent *xev, String *s, Cardinal *c)
{
	if (timer)	
	{
		XtRemoveTimeOut (timer);
		timer = NULL;
	}
	/*
	 * Send focus to the pass word field
	 */
	XmProcessTraversal(_pwd_text, XmTRAVERSE_CURRENT);
}
void
PasswordActivateOverride ( Widget widget, XEvent *xev, String *s, Cardinal *c)
{
	if (timer)	
	{
		XtRemoveTimeOut (timer);
		timer = NULL;
	}
	/*
	 * Send focus to the first OK button 
	 */
	XmProcessTraversal(XmSelectionBoxGetChild(_w, XmDIALOG_OK_BUTTON), 
	                         XmTRAVERSE_CURRENT);
	XtCallCallbacks(XmSelectionBoxGetChild(_w, XmDIALOG_OK_BUTTON),
						XmNactivateCallback, (XtPointer)NULL);
}
/*****************************************************************
* Clear the password field on focus accept
*****************************************************************/
void 
passwdFocusCB (Widget w, XtPointer client_data, XtPointer call_data ) 
{
	clearPassword();
}
/******************************************************************
	Timeout Procedure - If nothing happens then time out
 ******************************************************************/
void
TimerProc (Widget widget, XEvent *event, String *pars, Cardinal *npars)
{
	sendCloseFolderReq(_w);
	XtDestroyWidget (_w);
	XtCloseDisplay (display);
	exit (0);
}

/******************************************************************
	Sets input focus
 ******************************************************************/
void
setInputFocus()
{
	XmProcessTraversal(_login_text, XmTRAVERSE_CURRENT);
}
