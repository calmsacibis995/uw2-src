#ident	"@(#)umailsetup:setupWin.C	1.12"
//  setupWin.C


#include	<iostream.h>		//  for cout()

#include	<Xm/Xm.h>
#include	<Xm/Form.h>
#include	<Xm/PanedW.h>

#include	"actionArea.h"		//  for ActionAreaItem definition
#include	"mnem.h"		//  for mnemonic functions and constants
#include	"setup.h"		//  for AppStruct
#include	"setupAPIs.h"		//  for setupType_t definition
#include	"setup_txt.h"		//  the localized message database



//  External variables, functions, etc.

extern Widget  createControlArea (Widget parent);
extern Widget  createActionArea (Widget parent, ActionAreaItem *actions,
		    Cardinal numActions, Widget highLevelWid, mnemInfoPtr mPtr);

extern void    doOkActionCB    (Widget, XtPointer clientData, XmAnyCallbackStruct *);
extern void    doResetActionCB (Widget, XtPointer clientData, XmAnyCallbackStruct *);
extern void    doCancelActionCB(Widget w,XtPointer clientData,XmAnyCallbackStruct *);
extern void    doHelpActionCB  (Widget, XtPointer, XmAnyCallbackStruct *);

extern AppStruct	app;


//  Local variables, functions, etc.

Widget	createSetupWindow (Widget topLevel, Widget *actionArea);
void	resizeCB (Widget w, Widget clientData, XEvent *xev);

ActionAreaItem actionItems[] =
{ //   label         mnemonic          which    sensitive  callback       clientData
 {TXT_OkButton,    MNEM_OkButton,    OK_BUTTON,    True, doOkActionCB,    (XtPointer)0 },
 {TXT_ResetButton, MNEM_ResetButton, RESET_BUTTON, True, doResetActionCB, (XtPointer)0 },
 {TXT_CancelButton,MNEM_CancelButton,CANCEL_BUTTON,True, doCancelActionCB,(XtPointer)0 },
 {TXT_HelpButton,  MNEM_HelpButton,  HELP_BUTTON,  True, doHelpActionCB,  (XtPointer)0 }
};
int	numButtons = XtNumber (actionItems);




/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	Widget createSetupWindow (Widget topLevel, Widget *actionArea)
//
//  DESCRIPTION:
//	Create the General Setup window, using the topLevel received.
//	Also, store the actionarea widget id in the actionArea widget pointer.
//
//  RETURN:
//	The widget id of the setup window dialog (and the action area widget id).
//

Widget
createSetupWindow (Widget topLevel, Widget *actionArea)
{
	Widget	dialog, pane;
	int		i;


	log3 (C_FUNC, "createSetupWindow(topLevel=", topLevel, ")");

	//  Get the mnemonic info record pointer. This must be done before
	//  creating anything with mnemonics.
	app.mPtr = createMnemInfoRec ();

	//  We'll use the dialog format, using a form as the "dialog shell".
	dialog = XmCreateForm (topLevel, "dialog", (ArgList)0, (Cardinal)0);
	app.highLevelWid = dialog;

	//  The pane does some managing for us (between the control area and
	//  the action area), and also gives us a separator line.
	pane = XtVaCreateWidget ("pane",
				xmPanedWindowWidgetClass,	dialog,
				XmNsashWidth,		1,	//  0 = invalid so use 1
				XmNsashHeight,		1,	//  so user won't resize
				XmNleftAttachment,		XmATTACH_FORM,
				XmNrightAttachment,		XmATTACH_FORM,
				XmNtopAttachment,		XmATTACH_FORM,
				XmNbottomAttachment,		XmATTACH_FORM,
				NULL);

	//  Create the control area (main area of the window) of the "dialog".
	(void)createControlArea (pane);

	//  Initialize the button client data.
	for (i = 0 ; i < numButtons ; i++)
		actionItems[i].clientData = app.web;

	//  Create the action area (lower button area of the window) of the "dialog".
	*actionArea = createActionArea (pane, &actionItems[0], XtNumber (actionItems),
								dialog, app.mPtr);

#ifdef MNEMONICS
	//  Set up the mnemonic info for all focus widgets in this dialog.
	applyMnemInfoOnShell (app.mPtr, app.highLevelWid);
#endif MNEMONICS

	XtManageChild (pane);
	XtManageChild (dialog);

	return (dialog);

}	//  End  createSetupWindow ()




/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	resizeCB (Widget w, Widget clientData, XEvent *xEvent)
//
//  DESCRIPTION:
//	We capture the scrolled window resize events because the form inside it
//	will not resize.  Since we want the widgets inside the form to grow and
//	shrink width-wise within the scrolled window (in some cases), we set the
//	width of the form to fit the scrolled window.
//
//  RETURN:
//	Nothing.
//

void
resizeCB (Widget w, Widget clientData, XEvent *)
{
	Dimension	width;


	log1 (C_FUNC, "resizeCB()");

	//  Get the width of the scrolled window so we can resize the form within it.
	XtVaGetValues (w,		XmNwidth,		&width,
					NULL);
	log2 (C_ALL, "\tNew size of scrolled window = ", width);

	//  Let the width grow (but not shrink) with the scrolled window when
	//  resizing.  This way, horizontal scrollbars will appear only if the
	//  user makes the window too narrow, and if the user makes the window
	//  wider, the variables (esp. text fields, integers, and passwds) will
	//  stretch out.
	//  Don't let the height change, so that the scrollbars will appear if needed.
	if (width >= app.varListWidth)
	{
		log2(C_ALL,"\tSW width >= form width: Setting form width to ",
						    width-WIDTH_OFFSET);
		XtVaSetValues (clientData,	XmNwidth,	width-WIDTH_OFFSET,
						NULL);
	}
	else	//  new width < original size,... keep it that way for scrollbars
	{
		log2(C_ALL,"\tSW w < form width: Setting form w=",app.varListWidth);
		XtVaSetValues (clientData,	XmNwidth,	app.varListWidth,
						NULL);
	}
}
