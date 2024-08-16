#ident	"@(#)prtsetup2:ps_question.C	1.4"
/*----------------------------------------------------------------------------
 *	Filename: ps_question.c
 */

#include <Xm/Xm.h>
#include <Xm/MessageB.h>

#include "BasicComponent.h"
#include "ps_hdr.h"
#include "ps_mainwin.h"
#include "ps_i18n.h"
#include "ps_question.h"
#include "mnem.h"

/*----------------------------------------------------------------------------
 *	Constructor for the PSQuestion class
 */
PSQuestion::PSQuestion (Widget		parent,		// Parent Widget
						char*		msgStr, 	// "str:1" FS "text" format
						ClientInfo*	info)
		  : BasicComponent ("NotNeeded")
{
	XmString					msg;
	Arg							args[16];
	char*						tempTitle;
	Dimension					w;
	Dimension					h;

#ifdef DEBUG1
	cerr << "PSQuestion (" << this << ", " << msgStr << ") Constructor" << endl;
#endif

	d_info = info;

	msg = XmStringCreateLtoR (GetLocalStr (msgStr), XmFONTLIST_DEFAULT_TAG);
	tempTitle = GetLocalStr (TXT_psQuestionTitle);

	XtSetArg (args[0], XmNmessageString, msg);
	XtSetArg (args[1], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
	XtSetArg (args[2], XmNtitle, tempTitle);
	_w = XmCreateInformationDialog (parent, tempTitle, args, 3); 
	XmStringFree (msg);
	XtUnmanageChild (XmMessageBoxGetChild (_w, XmDIALOG_HELP_BUTTON));
	registerMnemInfo (XmMessageBoxGetChild (_w, XmDIALOG_OK_BUTTON),
					  GetLocalStr (TXT_OKMnem),
					  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfo (XmMessageBoxGetChild (_w, XmDIALOG_CANCEL_BUTTON),
					  GetLocalStr (TXT_cancelMnem),
					  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);

	XtAddCallback (_w, XmNokCallback, PSQuestion::OKCallback, this);
	XtAddCallback (_w, XmNcancelCallback, PSQuestion::CancelCallback, this);

	XtManageChild (_w);
	XtVaGetValues (XtParent (_w), XmNwidth, &w, XmNheight, &h, 0);
	XtVaSetValues (XtParent (_w),
				   XmNminWidth,
				   w,
				   XmNmaxWidth,
				   w,
		 		   XmNminHeight,
				   h,
				   XmNmaxHeight,
				   h,
				   0);
	registerMnemInfoOnShell(_w);
}

/*----------------------------------------------------------------------------
 *	Destructor for the PSQuestion class
 */
PSQuestion::~PSQuestion ()
{
#ifdef DEBUG1
	cerr << "PSQuestion (" << this << ") Destructor" << endl;
#endif

	XtUnmanageChild (_w);
}

/*----------------------------------------------------------------------------
 *	This function is called when the OK button is pushed on a message window.
 */
void
PSQuestion::OKCallback (Widget, XtPointer clientData, XtPointer)
{
	PSQuestion*					obj = (PSQuestion*)clientData;

	obj->OK ();
}

/*----------------------------------------------------------------------------
 *	Member function called from OKCallback 
 */
void
PSQuestion::OK ()
{
	((BasicComponent*)d_info->ptr)->ResponseCallback (d_info->type, True);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSQuestion::CancelCallback (Widget, XtPointer clientData, XtPointer)
{ 
	PSQuestion*					obj = (PSQuestion*)clientData;

	obj->Cancel ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSQuestion::Cancel ()
{
	((BasicComponent*)d_info->ptr)->ResponseCallback (d_info->type, False);
}

