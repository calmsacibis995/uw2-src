#ident	"@(#)umailsetup:controlAreaCBs.C	1.14"
//  controlAreaCBs.C

#include	<iostream.h>		//  for cout()
#include	<stdlib.h>		//  for strtol ()
#include	<errno.h>		//  to get errno

#include	<Xm/Xm.h>		//  ... always needed
#include	<Xm/Text.h>		//  for XmTextSetString ()
#include	<Xm/TextF.h>		//  for XmTextFieldGetString ()

#include	"controlArea.h"		//  for ButtonItem
#include	"mnem.h"		//  for mnemonic functions and constants
#include	"setup.h"		//  for AppStruct
#include	"setup_txt.h"		//  the localized message database



extern Widget	createVariableList (Widget parent, Boolean extended);
extern void	errorDialog (Widget topLevel, char *errorText, setupVar_t *curVar);
extern void	resizeCB (Widget w, Widget clientData, XEvent *);
extern void	setCursor (int cursorType, Window window, Boolean flush);
extern void	createPasswdDialog (Widget parent, setupVar_t *pwdVar);
extern void	setVariableFocus (setupVar_t *var);

extern AppStruct	app;
extern Widget		pwdDialog;
extern Widget		errDialog;


void	categoryCB (Widget, XtPointer callData, XmAnyCallbackStruct *);
void	optionCB (Widget w, XtPointer callData, XmAnyCallbackStruct *);
void	varListChgCB (Widget w, Widget clientData, XConfigureEvent *event);
void	labelCB (Widget w, setupVar_t *curVar, XmAnyCallbackStruct *);
void	focusCB (Widget w, setupVar_t *curVar, XmAnyCallbackStruct *);
void	losingFocusCB (Widget, setupVar_t *curVar, XmTextVerifyCallbackStruct *cbs);
void	passwdTextCB (Widget, char **password, XmTextVerifyCallbackStruct *cbs);
void	toggleCB (Widget w, setupVar_t *curVar, XmToggleButtonCallbackStruct *cbs);
Boolean	inputIsSetToThisApp (Widget widget);


static Widget	lastFocused = (Widget)0;
Boolean	pwdChanged = False;
Boolean pwdActivate = False;





/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	categoryCB (Widget w, XtPointer callData, XmAnyCallbackStruct *)
//
//  DESCRIPTION:
//
//  RETURN:
//	Nothing.
//

void
categoryCB (Widget, XtPointer callData, XmAnyCallbackStruct *)
{
	Boolean		extended = (Boolean)(((ButtonItem *)callData)->index);
	setupVar_t	*curVar;


	log1 (C_FUNC, "categoryCB()");

	//  When extended is True, extended is turned on.
	XtVaSetValues (app.extended, XmNsensitive, extended? False : True,  NULL);
	XtVaSetValues (app.basic,    XmNsensitive, extended? True  : False, NULL);

	XtRemoveEventHandler (app.varWin, StructureNotifyMask, False,
			  (XtEventHandler)resizeCB, (XtPointer)app.varList);

	//  Set the wait cursor 'cuz this takes awhile.
	setCursor (C_WAIT, app.window, C_FLUSH);

	//  Free the client data memory (cData) for each variable we had.
	for (curVar = setupWebMoveTop (app.web) ; curVar ;
						curVar = setupWebMoveNext (app.web))
	{
		XtFree ((char *)setupVarClientData (curVar));
	}

	XtDestroyWidget (app.varList);

	//  It looks better when the whole scrolled window is unmapped.
	//  Otherwise the user sees garbage in it.
	XtUnmapWidget (app.varWin);

	//  Create the managed "Variable" list (containing textfields, etc.)
	//  that go in the scrolled window.
	app.varList = createVariableList (app.varWin, extended? True : False);

	//  Set the cursor back to the arrow.
	setCursor (C_POINTER, app.window, C_FLUSH);

	XtRealizeWidget (app.varList);
	XtMapWidget (app.varWin);

	//  Set the input focus to the first variable in the list, and
	//  make sure the description for that variable is displayed.
	setVariableFocus (app.firstVar);

#ifdef MNEMONICS
	//  Reset the mnemonic info for all focus widgets in this dialog.
	reApplyMnemInfoOnShell (app.mPtr, app.highLevelWid);
#endif MNEMONICS

}	//  End  categoryCB ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	optionCB (Widget w, XtPointer callData, XmAnyCallbackStruct *)
//
//  DESCRIPTION:
//	This is called when an option menu pulldown item button is selected.
//	All we do is set the current value of the setup variable by telling it
//	which button index was selected, and make sure we are showing the
//	current description if it is not already being displayed.
//
//  RETURN:
//	Nothing.
//

void
optionCB (Widget w, XtPointer callData, XmAnyCallbackStruct *)
{
	ButtonItem	*button = (ButtonItem *)callData;


	log1 (C_FUNC, "optionCB()");
	setupVarSetValue (button->curVar, &(button->index));
	log3 (C_API, "\tjust did setupVarSetValue(", button->index, ")");

	if (w != lastFocused)
	{
		XmTextSetString (app.descArea, setupVarDescription(button->curVar));
		lastFocused = w;
	}

}	//  End  optionCB ()



void
varListChgCB (Widget, Widget, XConfigureEvent *event)
{

	log1 (C_FUNC, "varListChgCB()");

	if (event->type != Expose)
		return;

	XtRemoveEventHandler (app.varList, ExposureMask, False,
			       (XtEventHandler)varListChgCB, (XtPointer)app.varWin);
}	//  End  varListChgCB ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	labelCB (Widget w, setupVar_t *curVar, XmAnyCallbackStruct *cbs)
//
//  DESCRIPTION:
//	The user clicked on a label (which is really a button) associated with
//	a variable.  Show the user the description for the variable, as well as
//	set the focus to that variable.
//
//  RETURN:
//	Nothing.
//

void
labelCB (Widget w, setupVar_t *curVar, XmAnyCallbackStruct *)
{
	
	log1 (C_FUNC, "labelCB()");

	if (w != lastFocused)
	{
		XmTextSetString (app.descArea, setupVarDescription (curVar));
		setVariableFocus (curVar);
		lastFocused = w;
	}

}	//  End  labelCB ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	focusCB (Widget w, setupVar_t *curVar, XmAnyCallbackStruct *)
//
//  DESCRIPTION:
//	This variable has received focus, either by clicking on it with the
//	mouse, or by tabbing or using the arrow keys.  Show the user the
//	descriptive text for this variable.
//
//  RETURN:
//	Nothing.
//

void
focusCB (Widget w, setupVar_t *curVar, XmAnyCallbackStruct *)
{
	log1 (C_FUNC, "focusCB()");

	//  Set the descriptive text for this particular variable iff this is a
	//  different widget than was last focused..

	if (w != lastFocused)
	{
		XmTextSetString (app.descArea, setupVarDescription (curVar));
		lastFocused = w;
	}

}	//  End  focusCB ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void  losingFocusCB (Widget, setupVar_t *curVar, XmAnyCallbackStruct *cbs)
//
//  DESCRIPTION:
//
//  RETURN:
//	Nothing.
//

void
losingFocusCB (Widget, setupVar_t *curVar, XmTextVerifyCallbackStruct *cbs)
{
	VarEntry	*cData = (VarEntry *)setupVarClientData (curVar);
	char		*charValue, *err;
	long		longValue;


	log1 (C_FUNC, "losingFocusCB()");

	//  Find out if the variable is losing focus because of internal
	//  tabbing, CR, etc., rather than setting focus to a diff app.
	if (inputIsSetToThisApp (app.topLevel))
		log1 (C_ALL, "\tInput focus is INSIDE this app");
	else
	{
		log1 (C_ALL, "\tInput focus is OUTSIDE this app, returning...");
		return;
	}

	switch (setupVarType (curVar))
	{
		case svt_string:
			charValue = XmTextFieldGetString (cData->var);

			if (err = setupVarSetValue (curVar,
					*charValue? &charValue : (char **)0))
			{
			    if (*charValue)
			    {
				log6 (C_API, "\tERR (string): ",setupVarLabel(curVar),
					"\n\t\tsetupVarSetValue (", charValue,
					") failed:\n\t\t", err);
			    }
			    else
			    {
				log5 (C_API, "\tERR (string): ",setupVarLabel(curVar),
					"\n\t\tsetupVarSetValue ((char *0)",
					") failed:\n\t\t", err);
			    }
			    //  whatever happened to  getErrorMsg() for err msgs?
			}

			if (!err)
			    log3 (C_API, "\tsvt_string: just did setupVarSetValue(",
								charValue, ")");
			break;

		case svt_integer:
		    {	char	*endp = (char *)0;

			charValue = XmTextFieldGetString (cData->var);

			if (*charValue)
			{
				longValue = strtol (charValue, &endp, 0);

				if (*endp)
				{
				    log3 (C_ALL, "*endp has unconverted value (",
						*endp, ")");
				    //  Pop up the error dialog.
				    if (!errDialog)
				    	errorDialog (app.topLevel,
							TXT_invalIntChars, curVar);
				    break;
				}

				if (errno == ERANGE)
				{
				    log1 (C_ALL, "errno==ERANGE (out of range #)");
				    //  Pop up the error dialog.
				    if (!errDialog)
					errorDialog (app.topLevel,
							TXT_invalIntRange, curVar);
				    break;
				}
			}
		    }

			if (err = setupVarSetValue (curVar,
						*charValue? &longValue : (long *)0))
			{
			    if (*charValue)
			    {
			    	log6 (C_API, "\tERR (int): ", setupVarLabel (curVar),
				    "\n\t\tsetupVarSetValue (", longValue,
				    ") failed:\n\t\t", err);
			    }
			    else
			    {
			    	log5 (C_API, "\tERR (int): ", setupVarLabel (curVar),
				    "\n\t\tsetupVarSetValue ((long *)0",
				    "(int)) failed:\n\t\t", err);
			    }

			    //  Pop up the error dialog.
			    if (!errDialog)
				errorDialog (app.topLevel, err, curVar);
			}

			if (*charValue)
			    log3 (C_API, "\tsvt_integer: just did setupVarSetValue(",
							longValue, ")");
			else
			    log2 (C_API, "\tsvt_integer: just did setupVarSetValue(",
							"(long *)0)");
			break;

		case svt_password:
			//  Iff the field has changed, pop up the 2nd password dialog.
			if (pwdChanged)
			{
				if (cbs->reason == XmCR_ACTIVATE)
				{
					pwdActivate = True;
					log1 (C_PWD, "\tpwdActivate = True");
				}

				if (!pwdDialog)
					createPasswdDialog (app.topLevel, curVar);
			}
			break;

		case svt_flag:	//  Toggles get set at time of pushing toggle button
		case svt_menu:	//  Option menu selection gets set at selection time
			break;
		default:
			break;
	}

}	//  End  losingFocusCB ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void  passwdTextCB (Widget w, char **password, XmTextVerifyCallbackStruct *cbs)
//
//  DESCRIPTION:
//
//  RETURN:
//	Nothing.
//

void
passwdTextCB (Widget, char **password, XmTextVerifyCallbackStruct *cbs)
{

	log5 (C_FUNC, "passwdTextCB(Widget, password=", password,
						"(*password=", *password, "), cbs)");
	log4 (C_PWD, "\tcbs->text->ptr=", cbs->text->ptr, ", cbs->text->length=",
								cbs->text->length);

	if (cbs->text->length > 1)
	{
		cbs->doit = False;		//  don't allow "paste" operations
		return;				//  make user *type* the password!
	}

	//  cbs->text->ptr == NULL happens with a backspace as well as when we
	//  set the textfield with a NULL (as in XmTextSetString(widget, "")
	if (cbs->text->ptr == NULL || cbs->text->length == 0)
	{
		if (strlen (*password) <= 0)
		{
			cbs->endPos = cbs->startPos = 0;
			return;			//  catch null password
		}

		//  Delete from here to end.
		cbs->endPos = strlen (*password);

		//  Backspace - terminate.
		(*password)[cbs->startPos] = '\0';
		return;
	}

	pwdChanged = True;
	log1 (C_PWD, "\tpwdChanged = True,... got a char..");

	//  cbs->text->length is always 1 or 0.

	//  Get enough space for the old text plus the new char plus a '\0' char.
	//  If password is NULL (the first time), XtRealloc simply calls XtMalloc
	//  for the memory.  It copies the old contents of password to the new place.
	if (*password)
	{
		*password = XtRealloc (*password, (Cardinal)(cbs->endPos + 2));
		log1 (C_PWD, "\t*password XtRealloc'd");
	}
	else
	{
		*password = XtCalloc ((Cardinal)1, (Cardinal)(cbs->endPos + 2));
		log1 (C_PWD, "\t*password XtCalloc'd");
	}

	strncat (*password, cbs->text->ptr, 1);
	(*password)[cbs->endPos + 1] = '\0';
	log2 (C_PWD, "\t*password now = ", *password);

	//  Change the text to a '*' (even though it is "invisible"), so
	//  no one can cut the password from the text field.
	cbs->text->ptr[0] = '*';

}	//  End  passwdTextCB ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	toggleCB (Widget w, setupVar_t curVar,
//						XmToggleButtonCallbackStruct *cbs)
//
//  DESCRIPTION:
//
//  RETURN:
//	Nothing.
//

void
toggleCB (Widget w, setupVar_t *curVar, XmToggleButtonCallbackStruct *cbs)
{
	int		state;
	

	log1 (C_FUNC, "toggleCB()");

	//  Set the descriptive text for this particular entry iff this is a
	//  different widget than was last focused..

	if (w != lastFocused)
	{
		XmTextSetString (app.descArea, setupVarDescription (curVar));
		lastFocused = w;
	}

	if (cbs->set)	//  This button is being SET (pushed in).
	{
		//  userData: 0 = Off button, 1 = On button
		XtVaGetValues (w, XmNuserData, &state, NULL);

		if (setupVarSetValue (curVar, &state))
		{
			log1 (C_API, "\tERR: setupVarSetValue(flag) failed");
			; //  Setting of variable failed.
		}
		log3 (C_API, "\tsvt_flag: just did setupVarSetValue(", state, ")");
	}

	//  Don't change state if this button is being released.

}	//  End  toggleCB ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	Boolean	inputIsSetToThisApp (Widget widget)
//
//  DESCRIPTION:
//	This function first finds the topLevel shell widget id of the widget,
//	and the window id for that shell, then asks X for the current input
//	focus window (and the window that would get focus next (which we don't
//	use).
//
//  RETURN:
//	True if the window id retrieved from X matches our own, False otherwise.
//

Boolean
inputIsSetToThisApp (Widget widget)
{
	Window	inputFocusWindow;
	Window	revertTo;
	Window	thisShellWindow;
	Widget	shell;;

	for (shell = widget ; !XtIsShell (shell) ; shell = XtParent (shell));

	thisShellWindow = XtWindow (shell);
	XGetInputFocus (XtDisplay (widget), &inputFocusWindow, (int *)&revertTo);

	if (inputFocusWindow == thisShellWindow)
		return (True);

	//  When focus is put on any option menu, the above calculation
	//  outcome is that the new inputFocusWindow != thisShellWindow.
	//  This is because the option menu is reparenting itself.
	//  For our needs, focus to an option menu should be treated
	//  like it is focus anywhere else in this "window", so we do
	//  the following additional tests.  Rather than replacing the
	//  above with the below, I have kept the above since we may
	//  need it for 2.0+ when we have more than one setup window.
	if (shell = XtWindowToWidget (app.display, inputFocusWindow))
	{
		if (app.appContext == XtWidgetToApplicationContext (shell))
			return (True);
	}

	return (False);

}	// End  inputIsSetToThisApp ()
