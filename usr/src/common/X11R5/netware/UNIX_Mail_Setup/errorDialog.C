#ident	"@(#)umailsetup:errorDialog.C	1.4"
//  errorDialog.C


#include	<Xm/Xm.h>		//  ... always needed
#include	<Xm/MessageB.h>		//  for the error dialog
#include	<Xm/TextF.h>		//  for XmTextFieldSetString()

#include	"dtFuncs.h"		//  for HelpText, GUI group library func defs
#include	"setup.h"		//  for the AppStruct struct
#include	"setup_txt.h"		//  the localized message database



//  External functions, variables, etc.

extern AppStruct	app;



//  Local functions, variables, etc.

void		errorDialog (Widget topLevel, char *errorText, setupVar_t *curVar);
static void	doErrOkActionCB (Widget w, setupVar_t *curVar, XmAnyCallbackStruct *);

Widget	errDialog = (Widget)0;





void
errorDialog (Widget topLevel, char *errorText, setupVar_t *curVar)
{
	Widget		dialog;
	Arg		args[6];
	Cardinal	argc = (Cardinal)0;
	XmString	title, message, ok;


	log1 (C_FUNC, "errorDialog(topLevel, errorText, varInError)");
	title   = XmStringCreateLocalized (app.title);
	message = XmStringCreateLtoR (getStr (errorText), XmSTRING_DEFAULT_CHARSET);
	ok      = XmStringCreateLocalized (getStr (TXT_OkButton));

	if (errorText)
	{
		XtSetArg (args[argc],	XmNautoUnmanage,	False);    argc++;
		XtSetArg (args[argc],	XmNdialogTitle,		title);    argc++;
		XtSetArg (args[argc],	XmNmessageString,	message);  argc++;
		XtSetArg (args[argc],	XmNcancelLabelString,	ok);       argc++;
		XtSetArg (args[argc],	XmNdeleteResponse,	XmDESTROY);argc++;
		XtSetArg (args[argc],	XmNdialogStyle,
					  XmDIALOG_PRIMARY_APPLICATION_MODAL); argc++;

		errDialog = dialog = XmCreateErrorDialog (topLevel, "errorDialog",
								args, argc);

		XtAddCallback (dialog, XmNdestroyCallback,
				(XtCallbackProc)doErrOkActionCB, (XtPointer)curVar);

		XtAddCallback (dialog, XmNcancelCallback, (XtCallbackProc)doErrOkActionCB,
						(XtPointer)curVar);

		if (getStr (MNEM_OkButton))
		{
			registerMnemInfo (XmMessageBoxGetChild (dialog,
				XmDIALOG_CANCEL_BUTTON), getStr (MNEM_OkButton),
				MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
		}

		XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_OK_BUTTON));
		XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));

		XtManageChild (dialog);

		if (getStr (MNEM_OkButton))
			registerMnemInfoOnShell (dialog);

		XtPopup (XtParent (dialog), XtGrabNone);
	}

	XmStringFree (ok);
	XmStringFree (message);
	XmStringFree (title);
	log1 (C_FUNC, "End  errorDialog()");

}	//  End  errorDialog ()



static void
doErrOkActionCB (Widget w, setupVar_t *curVar, XmAnyCallbackStruct *)
{
	VarEntry	*cData;
	char		*charValue;
	int		intValue;
	char            buff[256];


	log1 (C_FUNC, "doErrOkActionCB(w, curVar, XmAnyCallbackStruct *)");

	if (curVar)	//  We have an invalid variable value (as opposed to
	{		//  an error like the user doesn't have permission).
		cData = (VarEntry *)setupVarClientData (curVar);

		switch (setupVarType (curVar))
		{
		    case svt_string:
			setupVarGetValue (curVar, &charValue);
			XmTextFieldSetString (cData->var, charValue);
			log2 (C_ALL, "\tReset variable to ", charValue);
			break;

		    case svt_integer:
			if (setupVarGetValue (curVar, &intValue))
				buff[0] = '\0';
			else
				sprintf (buff, "%d", intValue);

			XmTextFieldSetString (cData->var, buff);
			log2 (C_ALL, "\tReset variable to ", buff);
			break;

		    case svt_flag:
		    case svt_password:
		    case svt_menu:
		    default:
			break;
		}

		log1 (C_ALL, "\tSetting focus to variable in error");

		//  Set focus to the variable in error.
		(void)XmProcessTraversal (cData->var, XmTRAVERSE_CURRENT);
	}

	XtPopdown (XtParent (w));
	errDialog = (Widget)0;
	log1 (C_ALL, "\tError dialog popped down, errDialog = 0");
	log1 (C_FUNC, "End  doErrOkActionCB()");

}	//  End  doErrOkActionCB ()
