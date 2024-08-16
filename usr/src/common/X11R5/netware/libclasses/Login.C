#ident	"@(#)libclass:Login.C	1.3"
////////////////////////////////////////////////////////////////////
// Login.C: A login panel for authentication 
/////////////////////////////////////////////////////////////////////
#include "Login.h"
#include "main.h"
#include "msgs.h"
#include "i18n.h"
#include "Buttons.h"
#include <Xm/DialogS.h>
#include <Xm/PanedW.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Text.h>

static ButtonItem ActionItems[] = {
	{(XtArgVal)TXT_apply, (XtArgVal)MNEM_apply, },
	{(XtArgVal)TXT_cancel, (XtArgVal)MNEM_cancel, },
	{(XtArgVal)TXT_help, (XtArgVal)MNEM_help,},
};


////////////////////////////////////////////////////////////////////
//	The CTOR for Login class - sets up the help labels
////////////////////////////////////////////////////////////////////
Login::Login () 
{
	Buttons::SetLabels (ActionItems, XtNumber (ActionItems));
}

////////////////////////////////////////////////////////////////////
//	Post the dialog onto the screen.  Setup the widgets and
//	post the dialog.
////////////////////////////////////////////////////////////////////
void Login::postDialog (Widget parent, char *name, char *title ,char *) 
{
	char 		*tmp;
	XmString	xmstr;
	Dimension	h;
	int		i;

	/* 
	 * Set up the title for the screen here
	 */
	tmp = XtMalloc (strlen (title) + strlen (I18n::GetStr (TXT_loginto)) +
			2);
	strcpy (tmp, I18n::GetStr (TXT_loginto));
	strcat (tmp, title);

	if (_pane && XtIsManaged (_pane)) 
		return;

	/* 
	 * create the popup shell to hold all the control area
         * and the buttons
         */
        _w = XtVaCreatePopupShell (name,  xmDialogShellWidgetClass, parent,
                                   XmNtitle,	tmp,
                                   0);
        XtAddCallback (_w, XmNdestroyCallback,&Login::destroyCB, this);
	XtFree (tmp);

        /* 
	 * create the paned window to hold the forms  below
         */
        _pane = XtVaCreateWidget ("pane",xmPanedWindowWidgetClass, _w,
                                	XmNsashWidth, 1, XmNsashHeight, 1, 0);

	/* 
	 * create the control area  - Form widget to hold the login and
	 * password fields
	 */
        _control = XtVaCreateWidget ("control",xmRowColumnWidgetClass,_pane, 
					XmNorientation, 	XmVERTICAL,
					0);

	/* 
	 * create the login form to hold the login label and the login
	 * text field
	 */
	_loginform = XtVaCreateManagedWidget ("loginform", xmFormWidgetClass,
						_control, 0);

	/* 
	 * Login label field 
	 */
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_login), charset);
	_login_label = XtVaCreateManagedWidget ("login_label",
					xmLabelWidgetClass,	_loginform,
					XmNlabelString,		xmstr, 
					XmNalignment, 	XmALIGNMENT_BEGINNING,
					XmNleftAttachment, 	XmATTACH_FORM,
					XmNtopAttachment, 	XmATTACH_FORM,
					XmNbottomAttachment, 	XmATTACH_FORM,
					0);
	XmStringFree (xmstr);

	/*
	 *  Login text field
	 */
	_login_text = XtVaCreateManagedWidget ("login_text",
					xmTextWidgetClass,	_loginform,
					XmNleftAttachment, XmATTACH_POSITION,
					XmNleftPosition, 	30,
					XmNrightAttachment, 	XmATTACH_FORM,
					XmNtopAttachment, 	XmATTACH_FORM,
					XmNbottomAttachment, 	XmATTACH_FORM,
					0);

	/*
	 *  The password form manager to hold the pwd label/text fields
	 */
	_pwdform = XtVaCreateManagedWidget ("pwdform", xmFormWidgetClass,
					_control, 0);

	/*
	 *  create the Password label field
	 */
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_pwd), charset);
	_pwd_label = XtVaCreateManagedWidget ("_pwd_label",
					xmLabelWidgetClass,	_pwdform,
					XmNalignment, 	XmALIGNMENT_BEGINNING,
					XmNlabelString,		xmstr, 
					XmNleftAttachment, 	XmATTACH_FORM,
					XmNbottomAttachment, 	XmATTACH_FORM,
					XmNtopAttachment, 	XmATTACH_FORM,
					0);
	XmStringFree (xmstr);

	/* 
	 * create the Password text field
	 */
	_pwd_text = XtVaCreateManagedWidget ("pwd_text",
					xmTextWidgetClass,	_pwdform,
					XmNleftAttachment, XmATTACH_POSITION,
					XmNleftPosition, 	30,
					XmNrightAttachment, 	XmATTACH_FORM,
					XmNbottomAttachment, 	XmATTACH_FORM,
					XmNtopAttachment, 	XmATTACH_FORM,
					0);

	XtManageChild (_control);

	/*
	 *  create the form for the lower area to hold the
	 * action buttons
	 */
	_form = XtVaCreateWidget ("form", xmFormWidgetClass, _pane,
					XmNfractionBase, 7,
					0);
	
	/*
	 *  set the function pointers to the appropriate callbacks
	 */
	ActionItems[0]._proc_ptr = (XtArgVal)Login::okCB;
	ActionItems[1]._proc_ptr = (XtArgVal)Login::cancelCB;
	ActionItems[2]._proc_ptr = (XtArgVal)Login::helpCB;

	/*
	 *  create the action buttons
	 */
	int position = 1;
	for (i = 0; i < 3; i++)  {
		xmstr = XmStringCreateLtoR((char *)ActionItems[i]._label, 
					charset);
    		_actionbuttons[i] = XtVaCreateManagedWidget ("actionbuttons",
				xmPushButtonWidgetClass, _form, 
				XmNlabelString,		xmstr,
				XmNmnemonic, 		ActionItems[i]._mnem,
				XmNtopAttachment, 	XmATTACH_FORM,
				XmNbottomAttachment, 	XmATTACH_FORM,
				XmNleftAttachment, 	XmATTACH_POSITION,
				XmNleftPosition, 	position,
				XmNrightAttachment, 	XmATTACH_POSITION,
				XmNrightPosition, 	position + 1,
				0);
		XmStringFree (xmstr);
		position += 2;
		XtAddCallback (_actionbuttons[i], XmNactivateCallback,	
			(XtCallbackProc)ActionItems[i]._proc_ptr, this);		
	}
	
	/*
	 * Manage the action form area that holds the action buttons
	 * Set the max and min height of this area so that resize is
	 * prevented
	 */
	XtManageChild (_form);
	XtVaGetValues (_actionbuttons[0], XmNheight, &h, 0);
	XtVaSetValues (_form, XmNpaneMaximum, h, XmNpaneMinimum, h, 0);

	/* 	
	 * Manage the pane and then Popup the window
	 */
	XtManageChild (_pane);
	XtPopup (_w, XtGrabNone);
}

/*****************************************************************
	DTOR - Null the widget upon destruction of the class.
	This delete of Login never occurs.
*****************************************************************/
Login::~Login ()
{
	_w = NULL;
}


/*****************************************************************
	Ok callback
*****************************************************************/
void Login::okCB (Widget w, XtPointer client_data, XtPointer ) 
{
	Login	*obj = (Login *) client_data;

	obj->ok (w);
}

/*****************************************************************
	Help callback
*****************************************************************/
void Login::helpCB (Widget w, XtPointer client_data, XtPointer ) 
{
	Login	*obj = (Login *) client_data;

	obj->help (w);
}

/*****************************************************************
	Default cancel callback to pop the dialog off.  Calls
	virtual function cancel () which can be overridden
	in the derived class
*****************************************************************/
void Login::cancelCB (Widget w, XtPointer client_data, XtPointer ) 
{
	Login	*obj = (Login *) client_data;

	obj->cancel (w);
}

/*****************************************************************
	Pop off the dialog.  If the user needs to customize
	this then derive the virtual function in inherited
	class
*****************************************************************/
void Login::cancel(Widget)
{
	XtDestroyWidget (_w);
}

/*****************************************************************
	destroy callback
*****************************************************************/
void Login::destroyCB (Widget , XtPointer client_data, XtPointer ) 
{
	Login	*obj = (Login *) client_data;

	obj->destroy ();
}

/*****************************************************************
	Pop off the dialog.  If the user needs to customize
	this then derive the virtual function in inherited
	class
*****************************************************************/
void Login::destroy()
{
	_w = _pane = NULL;
}
