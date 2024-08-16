#ident	"@(#)umailsetup:passwdDialogCBs.C	1.9"
//	passwdDialogCBs.C


#include	<iostream.h>		//  for cout()

#include	<Xm/Xm.h>
#include	<Xm/TextF.h>		//  for XmTextFieldGetString()

#include	"dtFuncs.h"		//  for HelpText, GUI group library func defs
#include	"setup.h"		//  for AppStruct
#include	"setup_txt.h"		//  the localized message database




//  External variables, functions, etc.

extern Boolean	doOkActionCB  (Widget, XtPointer clientData, XmAnyCallbackStruct *);
extern void	errorDialog (Widget topLevel, char *errorText, setupVar_t *curVar);
extern void	shellWidgetDestroy (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs);

extern AppStruct	app;
extern HelpText		setupHelp;
extern Boolean		pwdChanged;
extern Boolean		pwdActivate;
extern Widget		pwdDialog;
extern Boolean		doingOK;


//  Local variables, functions, etc.

void	doPwdOkActionCB    (Widget w, setupVar_t *curVar, XmAnyCallbackStruct *cbs);
void	doPwdCancelActionCB(Widget w, setupVar_t *curVar, XmAnyCallbackStruct *);
void	doPwdHelpActionCB  (Widget, XtPointer, XmAnyCallbackStruct *);
void	exitPwdPopupCB (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs);





//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void doPwdOkActionCB (Widget w, setupVar_t *curVar,
//						     XmAnyCallbackStruct *cbs)
//
//  DESCRIPTION:
//	This gets called when the "Ok" button has been pushed in the password
//	dialog popup window.  We compare what the user typed in to this
//	validation popup with what was typed in the first password field.
//	If it doesn't match, we pop up an error dialog, and put the input
//	focus on the first password field, which has been cleared.  If it
//	does match, we set the new password value using the API call.
//
//  RETURN:
//	Nothing.
//

void
doPwdOkActionCB (Widget w, setupVar_t *curVar, XmAnyCallbackStruct *cbs)
{
	VarEntry	*cData;


	log1 (C_FUNC, "doPwdOkActionCB()");

	if (cbs->reason == XmCR_ACTIVATE)
	{
		log1(C_PWD,"\treason == XmCR_ACTIVATE\nEnd  doPwdOkActionCB()");
		return;
	}

	cData = (VarEntry *)setupVarClientData (curVar);
	pwdChanged = False;
	log1 (C_PWD, "\tpwdChanged = False");

	if (strcmp (cData->p_2ndText, cData->p_1stText))
	{
		log5 (C_PWD, "\tPasswords (1st=", cData->p_1stText, ", 2nd=",
					    cData->p_2ndText, ") DON'T match");
		doingOK = False;
		shellWidgetDestroy (w,(XtPointer)curVar,(XmAnyCallbackStruct *)0);

		//  Set focus to "bad" password field.
		(void)XmProcessTraversal (cData->var, XmTRAVERSE_CURRENT);

		errorDialog (app.topLevel, TXT_invalPasswd, curVar);
	}
	else	//  First password matches the second.
	{
		log5 (C_PWD, "\tPasswords (1st=", cData->p_1stText, ", 2nd=",
					    cData->p_2ndText, ") DO match");
		if (setupVarSetValue (curVar, &(cData->p_1stText)))
		{
			log1 (C_API, "\tERR: setupVarSetValue (password) failed");
			;  //  Setting of variable failed.
		}
		log3 (C_API, "\tjust did setupVarSetValue (", cData->p_1stText, ")");

		shellWidgetDestroy (w,(XtPointer)curVar,(XmAnyCallbackStruct *)0);

		//  User hit Enter, Alt-O, or the Ok button after chging the
		//  1st pwd field.  The pwd validation popup came up, they
		//  entered a matching 2nd password, and now we must make
		//  the whole app go away.
		if (pwdActivate | doingOK)
		{
			pwdActivate = False;
			log1 (C_PWD, "\tpwdActivate was True, now it is False");
			doOkActionCB ((Widget)0, app.web, (XmAnyCallbackStruct *)0);
		}
	}
	log1 (C_FUNC, "End  doPwdOkActionCB()");

}	//  End  doPwdOkActionCB ()



//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void doPwdCancelActionCB (Widget w, setupVar_t *curVar,
//						     XmAnyCallbackStruct *)
//
//  DESCRIPTION:
//	This gets called when the "Cancel" button has been pushed (or the ESC
//	key was pressed) in the password dialog popup window.  We simply call
//	shellWidgetDestroy() which causes the associated XmNdestroyCallback
//	(exitPwdPopupCB()) for the shell widget of this window to be called.
//	That function is what performs the exit tasks such as clearing the
//	password fields, destroying the mnemInfoRec, freeing memeory, etc.
//
//  RETURN:
//	Nothing.
//

void
doPwdCancelActionCB (Widget w, setupVar_t *curVar, XmAnyCallbackStruct *)
{

	log1 (C_FUNC, "doPwdCancelActionCB()");

	shellWidgetDestroy (w, (XtPointer)curVar, (XmAnyCallbackStruct *)0);

	log1 (C_FUNC, "End  doPwdCancelActionCB()");

}	//  End  doPwdCancelActionCB ()



//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void exitPwdPopupCB (Widget w, XtPointer clientData,
//							XmAnyCallbackStruct *cbs)
//
//  DESCRIPTION:
//	This gets called (automatically) when XtDestroyWidget() has been called
//	either directly in this application (here, using shellWidgetDestroy())
//	or when the XmNdestroyCallback is executed because the window manager
//	executes an XtDestroyWidget() on us (like when the user presses Alt-F4,
//	(or selects the mwm Close button or double-clicks on the window manager
//	button).
//	Here, we destroy the mnemInfoRec, clear the first password text field,
//	free up the memory associated with both password fields, reinitialize
//	those pointers, and clear a couple of flags.
//
//  RETURN:
//	Nothing.
//

void
exitPwdPopupCB (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs)
{
	VarEntry	*cData;
	setupVar_t	*curVar = (setupVar_t *)clientData;


	log1 (C_FUNC, "exitPwdPopupCB()");
	cData = (VarEntry *)setupVarClientData (curVar);

	destroyMnemInfoRec (cData->p_mPtr);

	log1 (C_PWD, "\tClearing field so passwdTextCB()");
	XmTextFieldSetString (cData->var, "");

	XtFree (cData->p_1stText);
	XtFree (cData->p_2ndText);
	cData->p_1stText = cData->p_2ndText = (char *)0;
	log1 (C_PWD, "\tXtFree'd both pwd fields & set ptrs to 0");
	pwdDialog = (Widget)0;
	pwdChanged = False;
	log1 (C_PWD, "\tpwdDialog = 0, pwdChanged = False");
	log1 (C_FUNC, "End  exitPwdPopupCB()");

}	//  End  exitPwdPopupCB ()



//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void doPwdHelpActionCB (Widget, XtPointer, XmAnyCallbackStruct *)
//
//  DESCRIPTION:
//	This gets called when the "Help..." button has been pushed in the
//	password dialog popup window, or when the XmNhelpCallback gets
//	activated via the F1 key.  We simply call displayHelp().
//
//  RETURN:
//	Nothing.
//

void
doPwdHelpActionCB (Widget, XtPointer, XmAnyCallbackStruct *)
{
	log1 (C_FUNC, "doPwdHelpActionCB()");
	displayHelp (app.topLevel, &setupHelp);
	log1 (C_FUNC, "End  doPwdHelpActionCB()");

}	//  End  doPwdHelpActionCB ()
