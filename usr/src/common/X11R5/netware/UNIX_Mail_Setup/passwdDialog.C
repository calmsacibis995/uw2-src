#ident	"@(#)umailsetup:passwdDialog.C	1.11"
//	passwdDialog.C


#include	<iostream.h>		//  for cout()

#include	<Xm/Xm.h>
#include	<Xm/DialogS.h>
#include	<Xm/Form.h>
#include	<Xm/Label.h>
#include	<Xm/PanedW.h>
#include	<Xm/TextF.h>

#include	"actionArea.h"		//  for ActionAreaItem definition
#include	"setup.h"		//  for AppStruct
#include	"setup_txt.h"		//  the localized message database
#include	"mnem.h"		//  for mnemonic APIs and definitions



//  External variables, functions, etc.

extern void   doPwdOkActionCB    (Widget w, setupVar_t *curVar,XmAnyCallbackStruct *);
extern void   doPwdCancelActionCB(Widget w, setupVar_t *curVar,XmAnyCallbackStruct *);
extern void   doPwdHelpActionCB  (Widget, XtPointer, XmAnyCallbackStruct *);
extern void   exitPwdPopupCB     (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs);

extern Widget createActionArea (Widget parent, ActionAreaItem *actions,
			Cardinal numActions, Widget highLevelWid, mnemInfoPtr mPtr);
extern void   setActionAreaHeight (Widget actionArea);
extern void   passwdTextCB (Widget, char **password, XmTextVerifyCallbackStruct *cbs);

extern AppStruct	app;


//  Local variables, functions, etc.

void	createPasswdDialog (Widget parent, setupVar_t *pwdVar);

static Widget	createPwdControlArea (Widget parent, setupVar_t *pwdVar);
static Widget	createPwdTextField (Widget parent, setupVar_t *pwdVar);
static Widget	createPwdLabel (Widget parent, Dimension height, setupVar_t *pwdVar);



static ActionAreaItem pwdActions[] =
{//  label           mnemonic         which    sensitive   callback         clientData
 {TXT_OkButton,    MNEM_OkButton,    OK_BUTTON,    True,doPwdOkActionCB,    (XtPointer)0 },
 {TXT_CancelButton,MNEM_CancelButton,CANCEL_BUTTON,True,doPwdCancelActionCB,(XtPointer)0 },
 {TXT_HelpButton,  MNEM_HelpButton,  HELP_BUTTON,  True,doPwdHelpActionCB,  (XtPointer)0 }
};
static int	numPwdButtons = XtNumber (pwdActions);


Widget	pwdDialog = (Widget)0;



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void createPasswdDialog (Widget parent, setupVar_t *pwdVar)
//
//  DESCRIPTION:
//	Create the Password Dialog for entering the password again.
//
//  RETURN:
//	Nothing.
//

void
createPasswdDialog (Widget parent, setupVar_t *pwdVar)
{
	Widget		dialog, form, pane, actionArea;
	VarEntry	*cData = (VarEntry *)setupVarClientData (pwdVar);
	int		i;


	//  We'll create our own simple dialog.
	log1 (C_FUNC, "createPasswdDialog()");
	pwdDialog = dialog = XtVaCreatePopupShell ("MHS Mail Setup Password Validation",
				xmDialogShellWidgetClass, parent,
				XmNdeleteResponse,	XmDESTROY,
				NULL);

	form = XtVaCreateWidget ("pwdForm", xmFormWidgetClass, dialog,
				XmNdialogStyle,	XmDIALOG_FULL_APPLICATION_MODAL,
				NULL);

	//  Catch the XtdestroyWidget(us) event, so we can clean things up.
	XtAddCallback (dialog, XmNdestroyCallback,
			(XtCallbackProc)exitPwdPopupCB, (XtPointer)pwdVar);

	//  Catch the "Help" key (i.e., the F1 key), so we can display help.
	XtAddCallback (form, XmNhelpCallback, (XtCallbackProc)doPwdHelpActionCB,
								(XtPointer)0);

	//  The pane does some managing for us (between the control area and
	//  the action area), and also gives us a separator line.
	pane = XtVaCreateWidget ("pane",
				xmPanedWindowWidgetClass,	form,
				XmNsashWidth,		1,	//  0 = invalid so use 1
				XmNsashHeight,		1,	//  so user won't resize
				XmNleftAttachment,		XmATTACH_FORM,
				XmNrightAttachment,		XmATTACH_FORM,
				XmNtopAttachment,		XmATTACH_FORM,
				XmNbottomAttachment,		XmATTACH_FORM,
				NULL);

	//  Create the control area (main area of the window) of the "dialog".
	(void)createPwdControlArea (pane, pwdVar);

	//  Get a new mnemonic info record pointer. This must be done before
	//  creating anything with mnemonics.
	cData->p_mPtr = createMnemInfoRec ();
	log3 (C_PWD,"\tGot new mnemInfoRec (", cData->p_mPtr, ")");

	//  Create the action area (lower button area of the window) of the "dialog".
	for (i = 0 ; i < numPwdButtons ; i++)
		pwdActions[i].clientData = pwdVar;

	actionArea = createActionArea (pane, &pwdActions[0], numPwdButtons,
							form, cData->p_mPtr);
	applyMnemInfoOnShell (cData->p_mPtr, dialog);

	XtManageChild (pane);
	XtManageChild (form);
	XtManageChild (dialog);

	XtPopup (dialog, XtGrabNone);

	//  Get and set the height of the action area.  We want this area to
	//  remain the same height while the control area gets larger, when
	//  the user resizes the window larger.  Don't know if this can be
	//  reliably done before XtPopup().
	setActionAreaHeight (actionArea);

	log1 (C_FUNC, "End  createPasswdDialog()");

}	//  End  createPasswdDialog ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget	createPwdControlArea (Widget parent, setupVar_t *pwdVar)
//
//  DESCRIPTION:
//
//  RETURN:
//	Return the widget id of the control area of the homemade dialog.
//

static Widget
createPwdControlArea (Widget parent, setupVar_t *pwdVar)
{
	Widget	controlArea, label, pwdTxt;
	XtWidgetGeometry returnGeo;


	//  Create the control area of the dialog.  We use a form as the base.
	controlArea = XmCreateForm (parent, "ctrlform", (ArgList)0, (Cardinal)0);

	pwdTxt = createPwdTextField (controlArea, pwdVar);

	//  Get the height of the textfield to make the label the same height.
	XtQueryGeometry (pwdTxt, (XtWidgetGeometry *)0, &returnGeo);

	label = createPwdLabel (controlArea, returnGeo.height, pwdVar);

	XtVaSetValues (label,
			XmNtopAttachment,		XmATTACH_FORM,
			XmNtopOffset,			3,	// : 6,
			XmNleftAttachment,		XmATTACH_FORM,
			XmNleftOffset,			5,
			XmNbottomAttachment,		XmATTACH_FORM,
			XmNbottomOffset,		6,
			NULL);

	XtVaSetValues (pwdTxt,
			XmNtopAttachment,		XmATTACH_FORM,
			XmNtopOffset,			3,	// : 6,
			XmNbottomAttachment,		XmATTACH_FORM,
			XmNbottomOffset,		6,
			XmNleftAttachment,		XmATTACH_WIDGET,
			XmNleftWidget,			label,
			XmNleftOffset,			3,
			XmNrightAttachment,		XmATTACH_FORM,
			XmNrightOffset,			5,
			NULL);

	XtManageChild (controlArea);

	return (controlArea);

}	//  End  createPwdControlArea ()



static Widget
createPwdLabel (Widget parent, Dimension height, setupVar_t *pwdVar)
{
	Widget		label;


	label = XtVaCreateManagedWidget ("label",
			xmLabelWidgetClass,		parent,
			XmNheight,			height,
			XmNalignment,			XmALIGNMENT_END,
			XmNrecomputeSize,		False,
			XmNlabelString,	XmStringCreateLocalized (setupVarLabel (pwdVar)),
			NULL);

	return (label);

}	//  End  createPwdLabel ()


static Widget
createPwdTextField (Widget parent, setupVar_t *pwdVar)
{
	Widget	var;
	Pixel	bg;
	VarEntry	*cData;


	var = XtVaCreateManagedWidget ("textF",
			xmTextFieldWidgetClass,		parent,
			NULL);

	XtVaGetValues (var, XtNbackground, &bg, NULL);
	XtVaSetValues (var, XtNforeground,  bg, NULL);

	cData = (VarEntry *)setupVarClientData (pwdVar);

	XtAddCallback (var, XmNactivateCallback, (XtCallbackProc)doPwdOkActionCB,
						(XtPointer)pwdVar);

	XtAddCallback (var, XmNmodifyVerifyCallback, (XtCallbackProc)passwdTextCB,
						(XtPointer)(&(cData->p_2ndText)));

	return (var);

}	//  End  createPwdTextField ()
