#ident	"@(#)prtsetup2:ps_ctrlprinter.C	1.10"
//--------------------------------------------------------------
// ps_ctrlprinter.c
//--------------------------------------------------------------

extern "C" {
#include <libMDtI/DesktopP.h>
#include <libMDtI/DnDUtil.h>
}

#include <Xm/Xm.h>
#include <Xm/LabelG.h>
#include <Xm/ToggleBG.h>
#include <Xm/RowColumn.h>

#include "ps_hdr.h"
#include "BasicComponent.h"
#include "ps_i18n.h"
#include "ps_xpmbutton.h"
#include "ps_mainwin.h"
#include "ps_win.h"
#include "ps_dialog.h"
#include "ps_ctrlprinter.h"
#include "ps_msg.h"
#include "mnem.h"

#include "lpsys.h"

/*----------------------------------------------------------------------------
 *
 */
PSCtrlPrinter::PSCtrlPrinter (Widget		parent,
							  char*			name,
							  PSPrinter*	selPrinter,
							  short			ptype,
							  action*		abi,
							  short			buttonCnt) 
			 : PSDialog (parent, name, selPrinter, ptype, abi, buttonCnt)
{
	XtAddCallback (abi[0].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSCtrlPrinter::ApplyCallback,
				   this);
	XtAddCallback (abi[1].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSCtrlPrinter::ResetCallback,
				   this);
	XtAddCallback (abi[2].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSCtrlPrinter::CancelCallback,
				   this);
	XtAddCallback (abi[3].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSCtrlPrinter::HelpCallback,
				   this);
	XtAddCallback (panedWindow (),
				   XmNhelpCallback,
				   (XtCallbackProc)&PSCtrlPrinter::HelpCallback,
				   this);

	turnOffSashTraversal ();

	if (/*!di->IsAdmin()*/!is_user_admin ()) {
		XtVaSetValues (abi[0].w, XmNsensitive, False, 0);
	}
	
#ifdef DIE
	title = XtMalloc (strlen (GetLocalStr (TXT_ctrlPrinter )) +
					  strlen (GetPrinterName ()) + 1);
	sprintf (title, GetLocalStr (TXT_ctrlPrinter), GetPrinterName ()); 
	XtVaSetValues (GetDialog(), XmNtitle, title, 0);
	XtFree (title);
#endif

	CreateCtrlArea ();
	registerMnemInfoOnShell(_w);
}

/*----------------------------------------------------------------------------
 *
 */
PSCtrlPrinter::~PSCtrlPrinter()
{
}

//--------------------------------------------------------------
// creates the control area portion for the Control Printer dialog.
//--------------------------------------------------------------
void
PSCtrlPrinter::CreateCtrlArea ()
{
	XmString					str;
	XmString					accept;
	XmString					reject;
	XmString					enabled;
	XmString					disabled;

	// Get and create the status label

	str = XmStringCreateSimple ("");
	_status = XtVaCreateManagedWidget ("statusLbl", 
									   xmLabelGadgetClass, GetCtrlArea(), 
									   XmNlabelString, str,  
									   XmNleftAttachment, XmATTACH_FORM,
									   XmNtopAttachment, XmATTACH_FORM,
									   0);
	XmStringFree (str);

	// Create a Row Column for the "New Requests" buttons

	str	= XmStringCreateSimple ("New Requests");
	accept = XmStringCreateSimple ("Accept");
	reject = XmStringCreateSimple ("Reject");

	_newReqsRC = XtVaCreateWidget ("NewRequests", xmRowColumnWidgetClass,
								  GetCtrlArea(), 
								  XmNradioBehavior, True,
								  XmNradioAlwaysOne, True,
								  XmNorientation, XmHORIZONTAL, 
								  XmNtopAttachment, XmATTACH_WIDGET,
								  XmNtopWidget, _status,
								  XmNleftAttachment, XmATTACH_FORM,
								  0);
	_newReqLbl = XtVaCreateManagedWidget ("newReqLbl", 
										  xmLabelGadgetClass, _newReqsRC, 
										  XmNlabelString, str,  
										  0);
	_accept = XtVaCreateManagedWidget ("Accept", 
									   xmToggleButtonGadgetClass, _newReqsRC, 
									   XmNlabelString, accept,
									   0);
	XtAddCallback (_accept, XmNvalueChangedCallback, AcceptCallback, this);
	_reject = XtVaCreateManagedWidget ("Reject", 
									   xmToggleButtonGadgetClass, _newReqsRC, 
									   XmNlabelString, reject,
									   0);

	XmStringFree (str);
	XmStringFree (accept);
	XmStringFree (reject);

	XtManageChild (_newReqsRC);

	// Create a Row Column for the "Printer Enabled" buttons

	str	  = XmStringCreateSimple ("Printer");
	enabled  = XmStringCreateSimple ("Enabled");
	disabled = XmStringCreateSimple ("Disabled");

	_enabledRC = XtVaCreateWidget ("Enabled", xmRowColumnWidgetClass,
								   GetCtrlArea(),
								   XmNradioBehavior, True,
								   XmNradioAlwaysOne, True,
								   XmNorientation, XmHORIZONTAL,
								   XmNtopAttachment, XmATTACH_WIDGET,
								   XmNtopWidget, _newReqsRC,
								   XmNleftAttachment, XmATTACH_FORM,
								   0);
	_printerLbl = XtVaCreateManagedWidget ("printerLbl", 
										   xmLabelGadgetClass, _enabledRC, 
										   XmNlabelString, str,  
										   0);
	_enabled = XtVaCreateManagedWidget ("Enabled", 
										xmToggleButtonGadgetClass, _enabledRC,
										XmNlabelString, enabled,
										0);
	XtAddCallback (_enabled, XmNvalueChangedCallback, EnabledCallback, this);
	_disabled = XtVaCreateManagedWidget ("Disabled",
										 xmToggleButtonGadgetClass, _enabledRC,
										 XmNlabelString, disabled,
										 0);

	XmStringFree (str);
	XmStringFree (enabled);
	XmStringFree (disabled);

	XtManageChild (_enabledRC);

	// Now get and set the actual status values
	SetCtrls ();

	ShowDialog ();
}

//--------------------------------------------------------------
// called when the state of the _accept button is changed.
//--------------------------------------------------------------
void
PSCtrlPrinter::AcceptCallback (Widget, XtPointer data, XtPointer callData)
{
	PSCtrlPrinter*				obj = (PSCtrlPrinter*)data;

	obj->Accept ((XmToggleButtonCallbackStruct*)callData);
}

//--------------------------------------------------------------
// Member function called from AcceptCallback.
//--------------------------------------------------------------
void
PSCtrlPrinter::Accept (XmToggleButtonCallbackStruct* state)
{
	_newReqsState = state->set;
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCtrlPrinter::EnabledCallback (Widget, XtPointer data, XtPointer callData)
{
	PSCtrlPrinter*				obj = (PSCtrlPrinter*)data;

	obj->Enabled ((XmToggleButtonCallbackStruct*)callData);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCtrlPrinter::Enabled (XmToggleButtonCallbackStruct* state)
{
	_enabledState = state->set;
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCtrlPrinter::ApplyCallback (Widget, XtPointer clientData, XtPointer)
{
	PSCtrlPrinter*				obj = (PSCtrlPrinter*)clientData;

	obj->Apply ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCtrlPrinter::Apply ()
{
	LpState  					accepting = Lp_On;
	LpState  					enabled = Lp_On;
	LpWhen   					when = Lp_Requeue;
	char*						prtName;
	char*						activeJob;
	Boolean 					acceptingState;
	Boolean 					enabledState;
	Boolean						faultedState;

	if (!_enabledState) {
		enabled = Lp_Off; 
	}
	if (!_newReqsState) {
		accepting = Lp_Off;
	}

	prtName = GetPrinterName ();
	LpAcceptEnable (prtName, accepting, enabled, when); 
	if (LpPrinterStatus (prtName,
						 &activeJob,
						 &acceptingState,
						 &enabledState,
						 &faultedState)) {
	}
	SetCtrls ();	
 } 

/*----------------------------------------------------------------------------
 *
 */
void
PSCtrlPrinter::ResetCallback (Widget, XtPointer clientData, XtPointer)
{
	PSCtrlPrinter*				obj = (PSCtrlPrinter*)clientData;

	obj->Reset ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCtrlPrinter::Reset ()
{
	SetCtrls ();
} 

/*----------------------------------------------------------------------------
 *
 */
void
PSCtrlPrinter::CancelCallback (Widget, XtPointer clientData, XtPointer)
{
	PSCtrlPrinter*				obj = (PSCtrlPrinter*)clientData;

	obj->Cancel ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCtrlPrinter::Cancel ()
{
	XtUnmanageChild (_w);
	d_delete = True;
} 

/*----------------------------------------------------------------------------
 *
 */
void
PSCtrlPrinter::HelpCallback (Widget, XtPointer clientData, XtPointer)
{
	PSCtrlPrinter*				obj = (PSCtrlPrinter*)clientData;

	obj->Help ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSCtrlPrinter::Help ()
{
	helpDisplay (DT_SECTION_HELP, GetLocalStr (TXT_helpTitle), TXT_ctrlSect);
} 

//--------------------------------------------------------------
// gets the printer status and sets the fields on the screen.
//--------------------------------------------------------------
void
PSCtrlPrinter::SetCtrls ()
{
	char*						status = NULL;
	char*						statusLine = NULL;
	XmString					str;
	char*						prtName;
	char*						activeJob;
	char*						tempStatus;
	Boolean						accepting;
	Boolean						enabled;
	Boolean						faulted;

	prtName = GetPrinterName ();

	if (!LpPrinterStatus (prtName,
						  &activeJob,
						  &accepting,
						  &enabled,
						  &faulted)) {
		status = GetLocalStr (TXT_noStatus);
	}
	else {
		if (activeJob) {
			status = GetLocalStr (TXT_printingPrintf);
			statusLine = new char[strlen (status) + strlen (prtName) +
								  strlen (activeJob) + 1];
			sprintf (statusLine, status, prtName, activeJob);
		}
		else
			if (faulted) {
				status = GetLocalStr (TXT_faultedPrintf);
			}
			else {
				if (!enabled) {
					_enabledState = False;
					tempStatus = GetLocalStr (TXT_disabledPrintf);
					status = XtMalloc (strlen (tempStatus) + 1);
					strcpy (status, tempStatus);
				}
				else {
					_enabledState = True;
					status = GetLocalStr (TXT_idlePrintf);
				}
			}
	}
	if (accepting) {
		_newReqsState = True;
	}
	if (statusLine == NULL) {
	   statusLine = new char[ strlen (status) + strlen (prtName) + 1 ];
	   sprintf (statusLine, status, prtName);
	}
	SetButtons ();
	str = XmStringCreateSimple (statusLine);
	XtVaSetValues (_status, XmNlabelString, str, NULL);
	delete (statusLine);
	XmStringFree (str);
}

//--------------------------------------------------------------
//	sets the _enabled, _disabled, _accept, and _reject Toggle Buttons.
//--------------------------------------------------------------
void
PSCtrlPrinter::SetButtons ()
{
	if (_enabledState) {
		XtVaSetValues (_enabled, XmNset, True, 0);
		XtVaSetValues (_disabled, XmNset, False, 0);
	}
	else {
		XtVaSetValues (_disabled, XmNset, True, 0);
		XtVaSetValues (_enabled, XmNset, False, 0);
	}

	if (_newReqsState) {
		XtVaSetValues (_accept, XmNset, True, 0);
 		XtVaSetValues (_reject, XmNset, False, 0);
	}
	else {
		XtVaSetValues (_reject, XmNset, True, 0);
		XtVaSetValues (_accept, XmNset, False, 0);
	}
}

