#ident	"@(#)libclass:Dialog.C	1.4"
////////////////////////////////////////////////////////////////////
// Dialog.C: A dialog for messages 
/////////////////////////////////////////////////////////////////////
#include "Dialog.h"
#include "main.h"
#include "msgs.h"
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <iostream.h>

Dialog::Dialog (char *name) : BasicComponent (name)
{
}

Dialog::~Dialog () 
{
}

void Dialog::postDialog (Widget parent, char *title, char *msg) 
{
	parentWidget = parent;

	_ac = 0;
	XtSetArg (_al[_ac],XmNtitle, title); _ac++;
	XtSetArg (_al[_ac],XmNautoUnmanage, False); _ac++;

	switch (_type) {
	
		case ERROR: 	
			_w = XmCreateErrorDialog (parent, _name,_al, _ac);
			break;

		case INFORMATION: 	
			_w = XmCreateInformationDialog (parent, _name,_al, _ac);
			break;

		case QUESTION: 	
			_w = XmCreateQuestionDialog (parent, _name,_al, _ac);
			break;

		case WORKING: 	
			_w = XmCreateWorkingDialog (parent, _name,_al, _ac);
			break;

		case WARNING: 	
			_w = XmCreateWarningDialog (parent, _name,_al, _ac);
			break;
		case PROMPT: 	
			_w = XmCreatePromptDialog (parent, _name,_al, _ac);
			break;
	}
	switch (_type) {
	
		case ERROR: 	
		case INFORMATION: 	
		case QUESTION: 	
		case WORKING: 	
		case WARNING: 	
			okWidget = XmMessageBoxGetChild (_w, XmDIALOG_OK_BUTTON); 
			cancelWidget = XmMessageBoxGetChild (_w, XmDIALOG_CANCEL_BUTTON); 
			helpWidget = XmMessageBoxGetChild (_w, XmDIALOG_HELP_BUTTON); 
			break;
		case PROMPT: 	
			okWidget = XmSelectionBoxGetChild (_w, XmDIALOG_OK_BUTTON); 
			cancelWidget = XmSelectionBoxGetChild (_w, XmDIALOG_CANCEL_BUTTON);
			helpWidget = XmSelectionBoxGetChild (_w, XmDIALOG_HELP_BUTTON);
			break;
	}

	XtAddCallback (_w, XmNdestroyCallback,&Dialog::destroyCB, this);
	if (_type != PROMPT && _type != WORKING)  {
		if (msg)
			XtVaSetValues (_w, XmNmessageString, XmStringCreateLtoR
					(msg, charset), 0);
	}
}

void Dialog::registerCancelCallback (XtCallbackProc func, XtPointer userdata) 
{
	XtAddCallback (_w, XmNcancelCallback, func, userdata);
}

void Dialog::registerHelpCallback (XtCallbackProc func, XtPointer userdata) 
{
	XtRemoveAllCallbacks( helpWidget, XmNactivateCallback) ;
	XtAddCallback (_w, XmNhelpCallback, func, userdata);
	XtAddCallback (helpWidget, XmNactivateCallback, func, userdata);
}

void Dialog::registerOkCallback (XtCallbackProc func, XtPointer userdata) 
{
	XtAddCallback (_w, XmNokCallback, func, userdata);
}

void Dialog::unmanageOk () 
{
	XtUnmanageChild (okWidget); 
}

void Dialog::unmanageCancel () 
{
	XtUnmanageChild (cancelWidget); 
}

void Dialog::unmanageHelp () 
{
	XtUnmanageChild (helpWidget); 
}

void Dialog::setOkString (char *okstring, char *mnem) 
{

	XtVaSetValues (_w,
			XmNokLabelString, XmStringCreateLtoR(okstring, charset), 
			0); 
	if ( mnem != NULL )
	{
		registerMnemInfo(okWidget, mnem, MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	}

}

void Dialog::setHelpString (char *helpstring, char *mnem) 
{
	XtVaSetValues (_w,
			XmNhelpLabelString,XmStringCreateLtoR(helpstring,charset), 
			0); 
	if ( mnem != NULL )
	{
		registerMnemInfo(helpWidget, mnem, MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	}
}

void Dialog::setCancelString (char *cancelstring, char *mnem) 
{
	XtVaSetValues (_w,
			XmNcancelLabelString,XmStringCreateLtoR(cancelstring,
								charset), 
			0); 
	if ( mnem != NULL )
	{
		registerMnemInfo(cancelWidget, mnem, MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	}
}

/*****************************************************************
		destroy callback
*****************************************************************/
void Dialog::destroyCB (Widget,  XtPointer client_data, XtPointer ) 
{
	Dialog	*obj = (Dialog *) client_data;

	obj->destroy ();
}

/*****************************************************************
		destroy virtual function 
*****************************************************************/
void Dialog::destroy() 
{
	_w = NULL;
}

/*****************************************************************
		override BasicComponent virtual function 
*****************************************************************/
void Dialog::manage()
{
	registerMnemInfoOnShell(_w);
    XtManageChild ( _w );
}
