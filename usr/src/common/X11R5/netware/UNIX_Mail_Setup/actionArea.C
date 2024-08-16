#ident	"@(#)umailsetup:actionArea.C	1.8"
#include	<Xm/Xm.h>
#include	<Xm/Form.h>
#include	<Xm/PushB.h>

#include	"actionArea.h"		//  for ActionAreaItem definition
#include	"dtFuncs.h"		//  for HelpText, GUI group library func defs
#include	"mnem.h"		//  for mnemonic functions and constants
#include	"setup.h"		//  for AppStruct



//  External variables, functions, etc.

extern AppStruct	app;
extern int		numButtons;


//  Local variables, functions, etc.

Widget	createActionArea (Widget parent, ActionAreaItem *actions, Cardinal numActions,
						Widget highLevelWid, mnemInfoPtr mPtr);
void	setActionAreaHeight (Widget actionArea);
void	setButtonSensitivity (ActionAreaItem *buttons, int thisButton,
								Boolean sensitivity);




/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	Widget createActionArea (Widget parent, ActionAreaItem *actions,
//			Cardinal numActions, Widget highLevelWid, mnemInfoPtr mPtr)
//
//  DESCRIPTION:
//	Create the action area (the bottom row buttons) of the custom dialog, as a
//	child of the parent widget id passed in, using the information contained in
//	actions.  numActions is how many buttons are created, and highLevelWid is
//	the high-level Motif widget id used for setting the XmNdefaultButton and the
//	XmNcancelButton resources.
//
//  RETURN:
//	Return the widget id of the action area (in this case a form).
//

Widget
createActionArea (Widget parent, ActionAreaItem *actions, Cardinal numActions,
						Widget highLevelWid, mnemInfoPtr mPtr)
{
	Widget	actionArea, pbutton;
	int	i;


	//  Create the form to put the buttons on.
	actionArea = XtVaCreateWidget ("action_area",
				xmFormWidgetClass,		parent,
				XmNfractionBase,		TIGHTNESS*numActions-1,
//				XmNskipAdjust,			True,
				XmNbottomAttachment,		XmATTACH_WIDGET,
				XmNbottomWidget,		parent,
				NULL);

	//  Create the action area buttons, setting the label and callback for each one.
	for (i = 0 ; i < numActions ; i++)
	{
		pbutton = XtVaCreateManagedWidget (getStr (actions[i].label),
			   xmPushButtonWidgetClass,	actionArea,
			   XmNsensitive,		actions[i].sensitive,
			   XmNleftAttachment,    i? XmATTACH_POSITION : XmATTACH_FORM,
			   XmNleftPosition,		TIGHTNESS*i,
			   XmNtopAttachment,		XmATTACH_FORM,
			   XmNbottomAttachment,		XmATTACH_FORM,
			   XmNrightAttachment,		i != numActions-1?
						XmATTACH_POSITION : XmATTACH_FORM,
			   XmNrightPosition,		TIGHTNESS*i + (TIGHTNESS-1),
			   XmNshowAsDefault,		i == 0,
			   XmNdefaultButtonShadowThickness,	1,
			   NULL);

		if (actions[i].callback)
			XtAddCallback (pbutton, XmNarmCallback,
				(XtCallbackProc)(actions[i].callback),
							actions[i].clientData);

#ifdef MNEMONICS
		if (getStr (actions[i].mnemonic))
		{
			addMnemInfo (mPtr, pbutton, getStr (actions[i].mnemonic),
						MNE_GET_FOCUS),
			addMnemCBInfo (mPtr,(XtCallbackProc)(actions[i].callback),
						(XtPointer)(actions[i].clientData));
			nextMnemInfoSlot (mPtr);
		}
#endif MNEMONICS

		//  Set the default button (so Return or Enter activates OK button)
		if (actions[i].which == OK_BUTTON)
			XtVaSetValues (highLevelWid, XmNdefaultButton, pbutton, NULL);

		//  Set the cancel button (so ESC key activates Cancel button)
		if (actions[i].which == CANCEL_BUTTON)
			XtVaSetValues (highLevelWid, XmNcancelButton, pbutton, NULL);
	}

	XtManageChild (actionArea);
	return (actionArea);

}	//  End  createActionArea ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void setActionAreaHeight (Widget actionArea)
//
//  DESCRIPTION:
//	Get and fix the height of the action area.  We want this area to
//	remain the same height while the control area gets larger, when
//	the user resizes the window larger.  Unfortunately, we cannot do
//	this at the time we create the action area, but rather we must
//	do it after topLevel realization time.
//
//  RETURN:
//	Nothing.
//

void
setActionAreaHeight (Widget actionArea)
{
	Dimension	height;

	XtVaGetValues (actionArea,	XmNheight,	&height,	NULL);
	XtVaSetValues (actionArea,	XmNpaneMaximum,	height,
					XmNpaneMinimum,	height,
					NULL);
}	//  End  setActionAreaHeight ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	Widget setButtonSensitivity (ActionAreaItem *buttons, int thisButton,
//								Boolean sensitivity)
//
//  DESCRIPTION:
//	Set the sensitivity of "thisButton" to the "sensitivity" value.
//
//  RETURN:
//	Nothing.
//

void
setButtonSensitivity (ActionAreaItem *buttons, int thisButton, Boolean sensitivity)
{
	int	i;


	for (i = 0 ; i < numButtons ; i++)
	{
		if (buttons[i].which == thisButton)
		{
			buttons[i].sensitive = sensitivity;
			break;
		}
	}

}	//  End  setButtonSenitivity ()
