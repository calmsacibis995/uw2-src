#ident	"@(#)umailsetup:controlArea.C	1.10"
//  controlArea.C



#include	<iostream.h>		//  for cout

#include	<Xm/Xm.h>		//  ... always needed
#include	<Xm/Form.h>
#include	<Xm/Label.h>
#include	<Xm/PushB.h>
#include	<Xm/PushBG.h>
#include	<Xm/RowColumn.h>
#include	<Xm/ScrolledW.h>
#include	<Xm/Separator.h>
#include	<Xm/Text.h>
#include	<Xm/TextF.h>
#include	<Xm/ToggleB.h>

#include	"controlArea.h"		//  for ButtonItem definition
#include	"dtFuncs.h"		//  for HelpText, GUI group library func defs
#include	"setup.h"		//  for AppStruct
#include	"setupAPIs.h"		//  for setupType_t definition
#include	"setup_txt.h"		//  the localized message database

#define		LABEL_LEFT_OFFSET	5
#define		VAR_OFFSET		3



//  External functions, variables, etc.

extern void	labelCB  (Widget w, setupVar_t *curVar, XmAnyCallbackStruct *);
extern void	focusCB  (Widget w, setupVar_t *curVar, XmAnyCallbackStruct *);
extern void	losingFocusCB (Widget, setupVar_t *curVar,
						   XmTextVerifyCallbackStruct *cbs);
extern void	passwdTextCB (Widget, char **password,
						XmTextVerifyCallbackStruct *cbs);
extern void	categoryCB (Widget w, XtPointer callData, XmAnyCallbackStruct *);
extern void	optionCB (Widget w, XtPointer callData, XmAnyCallbackStruct *);
extern void	resizeCB (Widget w, Widget clientData, XEvent *);
extern void	varListChgCB (Widget w, Widget clientData, XConfigureEvent *event);
extern void	toggleCB (Widget w, setupVar_t *curVar,
						XmToggleButtonCallbackStruct *cbs);
extern AppStruct	app;



//  Local functions, variables, etc.

Widget	createControlArea (Widget parent);
Widget	createVariableList (Widget parent, Boolean extended);
void	setVariableFocus (setupVar_t *var);

static Widget	createCategoryMenu (Widget parent,char *label,ButtonItem buttons[]);
static Widget	createOptionMenu (Widget parent, char *label, Widget *pulldown);
static void	createOptionButton (Widget parent, ButtonItem *button);
//static Widget	createVerticalSeparator (Widget parent, int leftPos);
static Widget	createControlAreaTitle (Widget parent, XmString text);
static Widget	createLabel (Widget parent, int maxLabelLen, setupVar_t *curVar);
static int	calcMaxLabelLen (setupWeb_t *web);
static int	calcMaxLabelWidth (setupWeb_t *web, XmFontList fontList);
static Widget	createVariable (Widget parent, setupVar_t *curVar);
static Widget	createDescriptionArea (Widget parent);

ButtonItem	optionItems[] =
{  // widget   index  label          mnemonic       callback      curVar
  {  (Widget)0,  0,   TXT_basic,     MNEM_basic,    categoryCB,   (setupVar_t *)0 },
  {  (Widget)0,  1,   TXT_extended,  MNEM_extended, categoryCB,   (setupVar_t *)0 },
	NULL
};


/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	Widget	createControlArea (Widget parent)
//
//  DESCRIPTION:
//
//  RETURN:
//	Return the widget id of the control area of the homemade dialog.
//

Widget
createControlArea (Widget parent)
{
	Widget		controlArea, option, sep1, sep2, varLabel, descLabel;
	Boolean		haveOption = False, descRight = True;
	Dimension	maxLabelWidth;
	Dimension	totLineWidth;
	Dimension	height = (Dimension)0, width = (Dimension)0;
	XmFontList	fontList;	//  for the high level form labelFontList


	log1 (C_FUNC, "createControlArea()");
	//  Create the control area of the dialog.  We use a form as the base.
	controlArea = XtVaCreateManagedWidget ("ctrlform",
			xmFormWidgetClass,		parent,
			NULL);

	//  Create the option menu so the user can select basic or extended mode.
	if (app.haveBasic && app.haveExtended)
	{
		haveOption = True;

		option = createCategoryMenu (controlArea, TXT_category, optionItems);

		XtVaSetValues (option,	XmNtopAttachment,	XmATTACH_FORM,
				XmNtopOffset,			5,
				XmNleftAttachment,		XmATTACH_POSITION,
				XmNleftPosition,		35,
				NULL);

		sep1 = XtVaCreateManagedWidget ("horizSeparator",
				xmSeparatorWidgetClass,		controlArea,
				XmNorientation,			XmHORIZONTAL,
				XmNtopAttachment,		XmATTACH_WIDGET,
				XmNtopWidget,			option,
				XmNtopOffset,			5,
				XmNleftAttachment,		XmATTACH_FORM,
				XmNrightAttachment,		XmATTACH_FORM,
				NULL);
	}

	//  Get the labelFontList from the high level form, and calculate
	//  the longest label width using that font so that we can determine
	//  how wide the scrolled window variable list should be.
	XtVaGetValues (app.highLevelWid, XmNlabelFontList, &fontList, NULL);
	maxLabelWidth = calcMaxLabelWidth (app.web, fontList);
	log2 (C_ALL, "\tmaxLabelWidth (in pixels) = ", maxLabelWidth);

	if (maxLabelWidth > 140)
	{
		descRight = False;
		log1 (C_ALL, "\tDescription area on bottom (maxLabelWidth > 140)");
	}
	else
		log1 (C_ALL, "\tDescription area on right (maxLabelWidth <= 140)");

	//  Create a vertical separator and attach it to the control area form.
//	sep = createVerticalSeparator (controlArea, 57);     //  57 is left attach pos

	if (descRight)
	{
		sep2 = XtVaCreateManagedWidget ("verticalSeparator",
				xmSeparatorWidgetClass,		controlArea,
				XmNorientation,			XmVERTICAL,
				XmNseparatorType,		XmNO_LINE,
				XmNtopAttachment,		XmATTACH_WIDGET,
				XmNtopWidget,	      haveOption? sep1 : controlArea,
				XmNbottomAttachment,		XmATTACH_FORM,
				XmNleftAttachment,		XmATTACH_POSITION,
				XmNleftPosition,		60,
				NULL);
	}

	//  Create the title of the left-side variable area of the control area.
	varLabel = createControlAreaTitle (controlArea,
				    XmStringCreateLocalized (getStr (TXT_leftTitle)));

	XtVaSetValues (varLabel,XmNtopAttachment,		XmATTACH_WIDGET,
				XmNtopWidget,	      haveOption? sep1 : controlArea,
				XmNtopOffset,			6,
				XmNleftAttachment,		XmATTACH_FORM,
				XmNrightAttachment,		XmATTACH_WIDGET,
				XmNrightWidget,	      descRight? sep2 : controlArea,
				NULL);

	XmStringExtent (fontList, XmStringCreateLocalized("GgPXwy9MhLQOFsepZAui"),
								   &width, &height);
	totLineWidth = maxLabelWidth + width + LABEL_LEFT_OFFSET + 2 * VAR_OFFSET;
	log3 (C_ALL, "\ttotLineWidth=", totLineWidth,
			" (maxLabelWidth+width+LABEL_LEFT_OFFSET+2*VAR_OFFSET)");
	log6 (C_ALL, "\t\twidth=", width, ", LABEL_LEFT_OFFSET=", LABEL_LEFT_OFFSET,
						", VAR_OFFSET=", VAR_OFFSET);
	log2 (C_ALL, "\tWIDTH_OFFSET=", WIDTH_OFFSET);
	log3 (C_ALL, "\tSetting scrolled window width to app.varListWidth = ",
		 45+totLineWidth+WIDTH_OFFSET, " (45+totLineWidth+WIDTH_OFFSET)");
	app.varListWidth = 45+totLineWidth+WIDTH_OFFSET;

	//  Create the left scrolled window, and attach it to the form.
	app.varWin = XtVaCreateManagedWidget ("scrolled_w",
				xmScrolledWindowWidgetClass,	controlArea,
				XmNscrollingPolicy,		XmAUTOMATIC,
				XmNscrollBarDisplayPolicy,	XmAS_NEEDED,
				XmNvisualPolicy,		XmVARIABLE,
				XmNtopAttachment,		XmATTACH_WIDGET,
				XmNtopWidget,			varLabel,
				XmNtopOffset,			1,
				XmNbottomAttachment,  descRight? XmATTACH_FORM :
								      XmATTACH_NONE,
				XmNbottomOffset,		6,
				XmNleftAttachment,		XmATTACH_FORM,
				XmNleftOffset,			6,
				XmNrightAttachment,		XmATTACH_WIDGET,
				XmNrightWidget,	      descRight? sep2 : controlArea,
				XmNrightOffset,			2,
				XmNwidth,			app.varListWidth,
				XmNheight,	      descRight? (Dimension)230 ://240
							       (Dimension)180,
				NULL);

	//  Create the managed "Variable" list (containing textfields, etc.)
	//  that go in the scrolled window.
	app.varList = createVariableList (app.varWin, app.haveBasic? False : True);

	//  Create the title of the right-side description area of the control area.
	descLabel = createControlAreaTitle (controlArea,
				 XmStringCreateLocalized (getStr (TXT_rightTitle)));

	//  Create the Description area window.  Since XmCreateScrolledText()
	//  is used, the widget returned is that of the Text widget, not of
	//  the scrolled window.  When the scrolled window widget id is required
	//  (for attachments, for instance) you'll see XtParent(app.descArea)
	//  used below, but just app.descArea when Text resources are being set.
	app.descArea = createDescriptionArea (controlArea);

	if (descRight)
	{
		XtVaSetValues (descLabel,
				XmNtopAttachment,		XmATTACH_WIDGET,
				XmNtopWidget,	     haveOption? sep1 : controlArea,
				XmNtopOffset,			6,
				XmNleftAttachment,		XmATTACH_WIDGET,
				XmNleftWidget,			sep2,
				XmNrightAttachment,		XmATTACH_FORM,
				NULL);
	}
	else
	{
		XtVaSetValues (descLabel,
				XmNleftAttachment,		XmATTACH_FORM,
				XmNrightAttachment,		XmATTACH_FORM,
				XmNbottomAttachment,		XmATTACH_WIDGET,
				XmNbottomWidget,	XtParent(app.descArea),
				XmNbottomOffset,		1,
				NULL);

		XtVaSetValues (app.descArea,
				XmNrows,			5,
				NULL);

		XtVaSetValues (app.varWin,
				XmNbottomAttachment,		XmATTACH_WIDGET,
				XmNbottomWidget,		descLabel,
				XmNbottomOffset,			6,
				NULL);
	}

	//  Attach the description area  to the form.
	XtVaSetValues (XtParent (app.descArea),
			XmNtopAttachment, descRight? XmATTACH_WIDGET: XmATTACH_NONE,
				XmNtopWidget,			descLabel,
				XmNtopOffset,			1,
				XmNbottomAttachment,		XmATTACH_FORM,
				XmNbottomOffset,		6,
				XmNleftAttachment,		XmATTACH_WIDGET,
				XmNleftWidget,	    descRight? sep2 : controlArea,
				XmNleftOffset,			6,
				XmNrightAttachment,		XmATTACH_FORM,
				XmNrightOffset,			6,
				NULL);

	return (controlArea);

}	//  End  createControlArea ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget	createCategoryMenu (Widget parent, char *label,
//							      ButtonItem buttons[])
//
//  DESCRIPTION:
//	Create the Category option menu. This is displayed at the top of the
//	application window and has the Basic or Extended mode options.
//	Note that this function is never called if the application has only
//	one of these modes.
//
//  RETURN:
//	Return the widget id of the option menu widget.
//

static Widget
createCategoryMenu (Widget parent, char *label, ButtonItem buttons[])
{
	Widget	pulldown, option;
	int	i;


	log1 (C_FUNC, "createCategoryMenu()");
	option = createOptionMenu (parent, label, &pulldown);

	for (i = 0 ; buttons[i].label != NULL ; i++)
	{
		createOptionButton (pulldown, &buttons[i]);
	}

	return (option);

}	//  End  createCategoryMenu ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget createOptionMenu (Widget parent, char *label, Widget*pulldown)
//
//  DESCRIPTION:
//	Create an option menu.  This is either the Category option menu at the
//	top of the application (with Basic or Extended options) if there are
//	both modes available, or, it is a type of variable, (that shows up in
//	the Variables list.
//
//  RETURN:
//	Return the widget id of the option menu widget.
//

static Widget
createOptionMenu (Widget parent, char *label, Widget *pulldown)
{
	Widget	option;
	Arg	args[2];
	Cardinal argc = 0;


	*pulldown = XmCreatePulldownMenu (parent, "optionPulldown", NULL, 0);

	XtSetArg (args[argc],	XmNsubMenuId,	*pulldown);	argc++;
	XtSetArg (args[argc],	XmNlabelString,	XmStringCreateLocalized
					(getStr (label))); argc++;

	option = XmCreateOptionMenu (parent, (getStr (label)), args, argc);

	//  Manage option menu, not pulldown menu.
	XtManageChild (option);

	return (option);

}	//  End  createOptionMenu ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static void	createOptionButton (Widget parent, ButtonItem *buttonItem)
//
//  DESCRIPTION:
//
//  RETURN:
//	Nothing.
//

static void
createOptionButton (Widget parent, ButtonItem *button)
{

	button->widget = XtVaCreateManagedWidget ("button",
		xmPushButtonGadgetClass,	parent,
		XmNlabelString,	XmStringCreateLocalized (getStr (button->label)),
//		XmNmnemonic,	XmStringCreateLocalized (getStr (button->mnemonic)),
		NULL);

	XtAddCallback (button->widget, XmNactivateCallback,
			       (XtCallbackProc)button->callback, (XtPointer)button);

}	//  End  createOptionButton ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget	createVerticalSeparator (Widget parent, int leftPos)
//
//  DESCRIPTION:
//	Create a vertical separator, and attach it to the parent form.
//
//  RETURN:
//	Return the widget id of the vertical separator widget.
//

#ifdef NEVER
static Widget
createVerticalSeparator (Widget parent, int leftPos)
{
	Widget	separator;

	separator = XtVaCreateManagedWidget ("verticalSeparator",
				xmSeparatorWidgetClass,		parent,
				XmNorientation,			XmVERTICAL,
				XmNseparatorType,		XmNO_LINE,
				XmNtopAttachment,		XmATTACH_FORM,
				XmNbottomAttachment,		XmATTACH_FORM,
				XmNleftAttachment,		XmATTACH_POSITION,
				XmNleftPosition,		leftPos,
				NULL);
	return (separator);

}	//  End  createVerticalSeparator ()
#endif NEVER



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget	createControlAreaTitle (Widget parent, XmString text)
//
//  DESCRIPTION:
//	Create a label widget containing the text designated by text.
//
//  RETURN:
//	Return the widget id of the managed label widget.
//

static Widget
createControlAreaTitle (Widget parent, XmString text)
{
	Widget	title;

	title = XtVaCreateManagedWidget ("title",
				xmLabelWidgetClass,		parent,
				XmNlabelString,			text,
				NULL);
	return (title);

}	//  End  createControlAreaTitle ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	Widget	createVariableList (Widget parent, Boolean extended)
//
//  DESCRIPTION:
//	Create the subpart of the main part of the control area which is the list
//	of "Variable" "lines" (comprised of textfields, etc.).
//
//  RETURN:
//	Return the widget id of the variable list (the form, in this case).
//

Widget
createVariableList (Widget parent, Boolean extended)
{
	Widget		varList, lineForm = (Widget)0;
	setupVar_t	*curVar;
//	setupVar_t	*move;
	VarEntry	*cData;
	int		i, maxLabelLen;
	static Boolean	firstTime = True;


	log1 (C_FUNC, "createVariableList()");
	varList = XtVaCreateManagedWidget ("varform",
			xmFormWidgetClass,		parent,
			XmNresizePolicy,   firstTime? XmRESIZE_NONE : XmRESIZE_GROW,
			NULL);

	maxLabelLen = calcMaxLabelLen (app.web);

	//  Does setupWeMoveExtended() return 0 on failure????????????? Yes!!!!
//	move = setupWebMoveExtended (app.web, extended);
	setupWebMoveExtended (app.web, extended);

	//  Save the 1st variable, so we can focus it & also show the description.
	app.firstVar = curVar = setupWebMoveTop (app.web);
	for (i = 0 ; curVar ; curVar = setupWebMoveNext (app.web), i++)
	{
		cData = (VarEntry *)XtCalloc (1, (Cardinal)sizeof (VarEntry));

		setupVarClientDataSet (curVar, (cData));

		lineForm = XtVaCreateManagedWidget ("lineForm",
			xmFormWidgetClass,		varList,
			XmNtopAttachment,	i? XmATTACH_WIDGET : XmATTACH_FORM,
			XmNtopWidget,			lineForm,
			XmNleftAttachment,		XmATTACH_FORM,
			XmNrightAttachment,		XmATTACH_FORM,
			XmNnavigationType,		XmNONE,
			NULL);

		XtAddCallback (lineForm, XmNfocusCallback, (XtCallbackProc)focusCB,
									curVar);

		cData->label = createLabel (lineForm, maxLabelLen, curVar);

		cData->var = createVariable (lineForm, curVar);

		XtVaSetValues (cData->label,
			XmNtopAttachment,		XmATTACH_FORM,
			XmNtopOffset,			i? 2 : 6,
			XmNleftAttachment,		XmATTACH_FORM,
			XmNleftOffset,			LABEL_LEFT_OFFSET,
			XmNbottomAttachment,		XmATTACH_FORM,
			NULL);

		XtVaSetValues (cData->var,
			XmNtopAttachment,		XmATTACH_FORM,
			XmNtopOffset,			i? 2 : 6,
			XmNleftAttachment,		XmATTACH_WIDGET,
			XmNleftWidget,			cData->label,
			XmNleftOffset,			VAR_OFFSET,
			XmNrightAttachment,		XmATTACH_FORM,
			XmNbottomAttachment,		XmATTACH_FORM,
			XmNrightOffset,			VAR_OFFSET,
			XmNnavigationType,		XmNONE,
			NULL);
	}

	XtVaSetValues (lineForm,
		XmNbottomAttachment,		XmATTACH_FORM,
		NULL);

	XtAddEventHandler (app.varWin, StructureNotifyMask, False,
			       (XtEventHandler)resizeCB, (XtPointer)varList);
	XtAddEventHandler (varList, ExposureMask, False,
			       (XtEventHandler)varListChgCB, (XtPointer)app.varWin);

	firstTime = False;

	return (varList);

}	//  End  createVariableList ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget createLabel (Widget parent, int maxLabelLen, setupVar_t *curVar)
//
//  DESCRIPTION:
//	Create a label but put it on a button that's invisible to the user
//	(shadowThickness = 0, etc.). When the user "touches the label", the
//	description area gets filled in with the appropriate text.
//
//  RETURN:
//	Return the widget id of the label widget.
//

static Widget
createLabel (Widget parent, int maxLabelLen, setupVar_t *curVar)
{
	Widget		labelButton;
	XmString	blankLabel;
					//  buffer is a string of 128 chars (more
	char		buffer[129]=	//  than twenty-some spaces don't work).
 "12345678911234567892123456789312345678941234567895123456789612345678971234567898123456789912345678901234567890123456789212345678";


	if (!setupVarLabel (curVar))
		return ((Widget)0);

	buffer[maxLabelLen] = '\0';
	blankLabel = XmStringCreateLocalized (buffer);

	switch (setupVarType (curVar))
	{
		case svt_string  :
		case svt_integer :
		case svt_flag    :
		case svt_password:
		case svt_menu    :
			//  Create a button with a label instead of just a label.
			//  The user doesn't see it as a button, but we want to
			//  catch its "arm" event so we can display the description
			//  when the user touches the label (button).
			labelButton = XtVaCreateManagedWidget ("labelButton",
				xmPushButtonWidgetClass,	parent,
				XmNshadowThickness,		0,
				XmNhighlightThickness,		0,
				XmNtraversalOn,			False,
				XmNalignment,			XmALIGNMENT_END,
				XmNrecomputeSize,		False,
				XmNlabelString,			blankLabel,
				NULL);

			XtVaSetValues (labelButton, XmNlabelString,
			    XmStringCreateLocalized (setupVarLabel (curVar)), NULL);

			//  Set the "arm" CB so we can set the description field
			//  when the user "selects" the label.
			XtAddCallback (labelButton, XmNarmCallback,
						   (XtCallbackProc)labelCB, curVar);
			break;

		default:
			break;
	}

	return (labelButton);

}	//  End  createLabel ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static int	calcMaxLabelWidth (setupWeb_t *web, XmFontList fontList)
//
//  DESCRIPTION:
//	Calculate the longest width (in pixels) of the labels in the SetupVar
//	list using the fontList provided.
//
//  RETURN:
//	Return what was just calculated.
//

static int
calcMaxLabelWidth (setupWeb_t *web, XmFontList fontList)
{
	setupVar_t	*curVar;
	Dimension	maxHeight = (Dimension)0, maxWidth = (Dimension)0;
	Dimension	height = (Dimension)0, width = (Dimension)0;


	log1 (C_FUNC, "calcMaxLabelWidth(web, fontList)");
	setupWebMoveExtended (app.web, app.haveExtended? True : False);

	for (curVar = setupWebMoveTop(web) ; curVar ; curVar = setupWebMoveNext(web))
	{
		XmStringExtent (fontList, XmStringCreateLocalized
					 (setupVarLabel (curVar)), &width, &height);
		if (height > maxHeight)
			maxHeight = height;

		if (width > maxWidth)
			maxWidth = width;
	}

	return (maxWidth);

}	//  End  calcMaxLabelWidth ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	int	calcMaxLabelLen (setupWeb_t *web)
//
//  DESCRIPTION:
//	Calculate the longest label (in chars) in the SetupVar list.
//
//  RETURN:
//	Return what was just calculated.
//

int
calcMaxLabelLen (setupWeb_t *web)
{
	setupVar_t	*curVar;
	int		len, maxLen = 0;


	setupWebMoveExtended (app.web, app.haveExtended? True : False);

	for (curVar = setupWebMoveTop (web) ; curVar ; curVar = setupWebMoveNext (web))
	{
		if ((len = strlen (setupVarLabel (curVar))) > maxLen)
			maxLen = len;

	}

	log3 (C_FUNC, "calcMaxLabelLen(): longest label = ", maxLen, " (chars)");
	return (maxLen);

}	//  End  calcMaxLabelLen ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget createVariable (Widget parent, setupVar_t *curVar)
//
//  DESCRIPTION:
//
//  RETURN:
//	Nothing.
//

static Widget
createVariable (Widget parent, setupVar_t *curVar)
{
	Widget		var;
	Pixel		bg;
	int		i, intValue;
	char		*charValue;
	char		**varChoices;
	char		buff[256];
	VarEntry	*cData;


	switch (setupVarType (curVar))
	{
	    case svt_string:
	    case svt_password:
		if (setupVarGetValue (curVar, &charValue))
		{
//cout << "setupVarGetValue() failed." << endl;
//  call getErrorMsg to retrieve error string
			charValue = "";
		}

		if (!charValue)
			charValue = "";

		var = XtVaCreateManagedWidget ("textF",
				xmTextFieldWidgetClass,		parent,
				XmNresizeWidth,			True,
				NULL);

		//  Catch Return (or Enter) key so we can retrieve the
		//  current text and set it before we exit the application.
		XtAddCallback (var, XmNactivateCallback,
				(XtCallbackProc)losingFocusCB, (XtPointer)curVar);

		//  Catch the losing focus event so we can retrieve the
		//  current text and set it.
		XtAddCallback (var, XmNlosingFocusCallback,
					     (XtCallbackProc)losingFocusCB, curVar);

		if (setupVarType (curVar) == svt_password)
		{
			XtVaGetValues (var, XtNbackground, &bg, NULL);
			XtVaSetValues (var, XtNforeground,  bg, NULL);

			cData = (VarEntry *)setupVarClientData (curVar);
			XtAddCallback (var, XmNmodifyVerifyCallback,
			 (XtCallbackProc)passwdTextCB,(XtPointer)(&(cData->p_1stText)));
			break;
		}

		XmTextFieldSetString (var, charValue);
		break;		//  End  type svt_string or type svt_password

	    case svt_integer:
		if (setupVarGetValue (curVar, &intValue))
			buff[0] = '\0';
		else
			sprintf (buff, "%d", intValue);

		var = XtVaCreateManagedWidget ("textF",
				xmTextFieldWidgetClass,		parent,
				XmNresizeWidth,			True,
				NULL);

		XmTextFieldSetString (var, buff);

		XtAddCallback (var, XmNactivateCallback,
				(XtCallbackProc)losingFocusCB, (XtPointer)curVar);

		XtAddCallback (var, XmNlosingFocusCallback,
					     (XtCallbackProc)losingFocusCB, curVar);
		break;		//  End  type svt_integer

	    case svt_flag:
		setupVarGetValue (curVar, &intValue);

		cData = (VarEntry *)setupVarClientData (curVar);
		varChoices = setupVarChoices (curVar);

		var = XtVaCreateManagedWidget ("rowCol",
				xmRowColumnWidgetClass,	parent,
				XmNorientation,		XmHORIZONTAL,
				XmNpacking,		XmPACK_COLUMN,
				XmNradioBehavior,	True,
				XmNentryClass,		xmToggleButtonWidgetClass,
				XmNisHomogeneous,	True,
				XmNuserData,		curVar,
				NULL );

		cData->f_onBtn = XtVaCreateManagedWidget ("toggleButton2",
				xmToggleButtonWidgetClass,var,
				XmNlabelString,      (varChoices && varChoices[1]) ?
				    XmStringCreateLocalized (varChoices[1]) :
				    XmStringCreateLocalized (getStr (TXT_toggleOn)),
				XmNset,			intValue ? True : False,
				XmNuserData,		1,
				NULL);

		XtAddCallback (cData->f_onBtn, XmNvalueChangedCallback,
				       (XtCallbackProc)toggleCB, (XtPointer)curVar);

		cData->f_offBtn = XtVaCreateManagedWidget ("toggleButton1",
				xmToggleButtonWidgetClass,var,
				XmNlabelString,      (varChoices && varChoices[0]) ?
				    XmStringCreateLocalized (varChoices[0]) :
				    XmStringCreateLocalized(getStr (TXT_toggleOff)),
				XmNset,			intValue ? False : True,
				XmNuserData,		0,
				NULL );
			

		XtAddCallback (cData->f_offBtn,	XmNvalueChangedCallback,
					(XtCallbackProc)toggleCB, (XtPointer)curVar);
		break;

	    case svt_menu:
		{	Widget		pulldown;
			ButtonItem	*buttonData;

			//  setupVarGetValue() here, gives us the index of the
			//  current selection (which option menu pulldown button).
			setupVarGetValue (curVar, &intValue);

			cData = (VarEntry *)setupVarClientData (curVar);
			varChoices = setupVarChoices (curVar);

			//  Create the option menu here with no associated label,
			//  since we already created the label separately (earlier).
			var = createOptionMenu (parent, "", &pulldown);

			for (i = 0 ; varChoices[i] ; i++)
			{
				buttonData = (ButtonItem *)XtCalloc (1,
						    (Cardinal)sizeof (ButtonItem));
				buttonData->label = varChoices[i];
				buttonData->index = i;
				buttonData->callback = optionCB;
				buttonData->curVar = curVar;
				createOptionButton (pulldown, buttonData);

				//  By default, the first menu option is the current
				if (i == 0)
				   cData->m_origChoice = buttonData->widget;

				//  We could have a different button than the first
				//  specified as the current one.
				if (i == intValue)
				{
				   cData->m_origChoice = buttonData->widget;
				}
			}

			XtVaSetValues (var, XmNmenuHistory, cData->m_origChoice, NULL);
			break;
		}
	    default:
		break;
	}

	return (var);

}	//  End  createVariable ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget	createDescriptionArea (Widget parent)
//
//  DESCRIPTION:
//
//  RETURN:
//	Return the widget id of the ScrolledWindow..
//

static Widget
createDescriptionArea (Widget parent)
{
	Widget	stext;
	Arg	args[5];
	Cardinal argc = 0;


	XtSetArg (args[argc],	XmNeditMode,		XmMULTI_LINE_EDIT); argc++;
	XtSetArg (args[argc],	XmNeditable,		False);		argc++;
	XtSetArg (args[argc],	XmNwordWrap,		True);		argc++;
	XtSetArg (args[argc],	XmNscrollHorizontal,	False);		argc++;

	stext = XmCreateScrolledText (parent, "stext", args, argc);
	XtManageChild (stext);

	return (stext);

}	//  End  createDescriptionArea ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void	setVariableFocus (setupVar_t *var)
//
//  DESCRIPTION:
//	Set the focus to the variable var (which is passed in).
//	We also make sure that this forces the description for this
//	variable to be shown in the description area.  Normally,
//	this is done for the first variable in the list after the
//	variable list has just been created, but it can be used anytime,
//	for any variable.  We do assume that all widgets are realized.
//
//  RETURN:
//	Nothing.
//

void
setVariableFocus (setupVar_t *var)
{
	VarEntry	*cData;
	int		intValue;


	log1 (C_FUNC, "setVariableFocus()");
	cData = (VarEntry *)setupVarClientData (var);

	switch (setupVarType (var))
	{
	    case svt_string:	//  Text field.
	    case svt_password:	//  Text field- invisible input & validation popup.
	    case svt_integer:	//  Text field that takes integers.
		(void)XmProcessTraversal (cData->var, XmTRAVERSE_CURRENT);
		break;

	    case svt_flag:	//  2 radio buttons.
		setupVarGetValue (var, &intValue);

		//  Turn button that was on, to off, then turn it back on, so
		//  that the valueChangedCB (toggleCB) gets called, so that the
		//  description area gets filled in.

		XmToggleButtonSetState (intValue? cData->f_onBtn : cData->f_offBtn,
								False, True);
		XmToggleButtonSetState (intValue? cData->f_onBtn : cData->f_offBtn,
								True, True);
		(void)XmProcessTraversal (intValue? cData->f_onBtn : cData->f_offBtn,
							XmTRAVERSE_CURRENT);
		break;

	    case svt_menu:	//  Option Menu.
		{	Widget option;

			//  Need to get the cascade button widget to set its focus.
			option = XmOptionButtonGadget (cData->var);
			(void)XmProcessTraversal (option, XmTRAVERSE_CURRENT);
		}
		break;

	    default:
		break;
	}

}	//  End  setVariableFocus ()
