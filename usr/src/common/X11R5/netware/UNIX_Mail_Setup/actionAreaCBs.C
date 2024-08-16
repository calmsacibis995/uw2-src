#ident	"@(#)umailsetup:actionAreaCBs.C	1.10"
//	actionAreaCBs.C

#include	<iostream.h>		//  for cout()
#include	<stdlib.h>		//  for strtol ()

#include	<Xm/Xm.h>
#include	<Xm/TextF.h>

#include	"dtFuncs.h"		//  for HelpText, GUI group library func defs
#include	"setup.h"		//  for AppStruct definiton
#include	"setupAPIs.h"		//  for setupType_t definition
#include	"setup_txt.h"		//  the localized message database



//  External variables, functions, etc.

extern void   shellWidgetDestroy (Widget w,XtPointer clientData,XmAnyCallbackStruct *cbs);
extern AppStruct	app;
extern HelpText		setupHelp;
extern Boolean		pwdChanged;
extern Boolean		pwdActivate;
extern Widget		pwdDialog;
extern Widget		errDialog;



//  Local variables, functions, etc.

void	doOkActionCB    (Widget w, XtPointer clientData, XmAnyCallbackStruct *);
Boolean	doApplyActionCB (Widget, XtPointer clientData, XmAnyCallbackStruct *);
void	doResetActionCB (Widget, XtPointer clientData, XmAnyCallbackStruct *);
void	doCancelActionCB(Widget w, XtPointer clientData, XmAnyCallbackStruct *);
void	doHelpActionCB  (Widget, XtPointer, XmAnyCallbackStruct *);

Boolean	doingOK = False;





void
doOkActionCB (Widget w, XtPointer clientData, XmAnyCallbackStruct *)
{

	log1 (C_FUNC, "doOkActionCB()");
	doingOK = True;

	if (pwdActivate)	//  Enter was pressed while in pwd field.
	{
		log1 (C_PWD, "\tpwdActivate is True (Enter pressed), returning..");
		return;
	}

	//  Alt-O (for Ok button) was pressed while either in a changed pwd field
	//  or while in a field that currently has an invalid value in it.  We
	//  return from here since we will come back again after the popup is done.
	if (pwdDialog || errDialog)
	{
		if (pwdDialog)
			log1 (C_PWD, "\tpwdDialog up (Alt-O pressed), returning..");
		else
			log1 (C_ALL, "\terrDialog up (Alt-O pressed), returning..");
		return;
	}

	if (doApplyActionCB ((Widget)0, clientData, (XmAnyCallbackStruct *)0))
		doCancelActionCB (w, clientData, (XmAnyCallbackStruct *)0);

}	//  End  doOkActionCB ()



Boolean
doApplyActionCB (Widget, XtPointer clientData, XmAnyCallbackStruct *)
{
	setupWeb_t	*web = (setupWeb_t *)clientData;


	log1 (C_FUNC, "doApplyActionCB()");
	setupWebApply (web);
	return (True);

}	//  End  doApplyActionCB ()



void
doResetActionCB (Widget, XtPointer clientData, XmAnyCallbackStruct *)
{
	setupWeb_t	*web = (setupWeb_t *)clientData;
	setupVar_t	*curVar;
	VarEntry	*cData;
	char		*charValue;
	char		buff[256];
	int		intValue;


	log1 (C_FUNC, "doResetActionCB()");

	if (pwdDialog)
	{
		XtDestroyWidget (pwdDialog);
		pwdDialog = (Widget)0;
		log1 (C_ALL, "\tDestroyed the pwd dialog! pwdDialog = 0");
	}

	if (errDialog)
	{
		XtDestroyWidget (errDialog);
		errDialog = (Widget)0;
		log1 (C_ALL, "\tDestroyed the err dialog! errDialog = 0");
	}

	setupWebReset (app.web);

	for (curVar = setupWebMoveTop (web) ; curVar ; curVar = setupWebMoveNext (web))
	{
		cData = (VarEntry *)setupVarClientData (curVar);

		switch (setupVarType (curVar))
		{
			case svt_string:
				setupVarGetValue (curVar, &charValue);
				XmTextFieldSetString (cData->var, charValue);
				break;

			case svt_integer:
				if (setupVarGetValue (curVar, &intValue))
					buff[0] = '\0';
				else
					sprintf (buff, "%d", intValue);

				XmTextFieldSetString (cData->var, buff);
				break;

			case svt_flag:
				setupVarGetValue (curVar, &intValue);
				if (intValue)
				{
				    XtVaSetValues (cData->f_onBtn,  XmNset, True,  NULL);
				    XtVaSetValues (cData->f_offBtn, XmNset, False, NULL);
				}
				else
				{
				    XtVaSetValues (cData->f_offBtn, XmNset, True,  NULL);
				    XtVaSetValues (cData->f_onBtn,  XmNset, False, NULL);
				}
				break;

			case svt_password:
				log1 (C_PWD, "\tClearing field, so passwdTextCB()");
				XmTextFieldSetString (cData->var, "");

				XtFree (cData->p_1stText);
				XtFree (cData->p_2ndText);
				cData->p_1stText = cData->p_2ndText = NULL;

				pwdChanged = pwdActivate = False;
				log1 (C_PWD, "\tXtFree'd & cleared 1st & 2nd fields");
				log1 (C_PWD, "\tpwdChanged = pwdActivate = False.");
				break;

			case svt_menu:
				setupVarGetValue (curVar, &intValue);
				XtVaSetValues (cData->var, XmNmenuHistory,
							cData->m_origChoice, NULL);
			default:
				break;
		}
	}

}	//  End  doResetActionCB ()



void
doCancelActionCB (Widget w, XtPointer clientData, XmAnyCallbackStruct *)
{
	log1 (C_FUNC, "doCancelActionCB()");

	//  Destroy the shell widget, which will cause exitSetupWinCB()
	//  to be called, which does a setupWebReset(), etc.

	shellWidgetDestroy (w, clientData, (XmAnyCallbackStruct*)0);

}	//  End  doCancelActionCB ()



void
doHelpActionCB (Widget, XtPointer, XmAnyCallbackStruct *)
{
	log1 (C_FUNC, "doHelpActionCB()");
	displayHelp (app.topLevel, &setupHelp);

}	//  End  doHelpActionCB ()
