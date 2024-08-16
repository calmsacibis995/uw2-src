#ident	"@(#)prtsetup2:ps_msg.C	1.5"
/*----------------------------------------------------------------------------
 *
 */

#include <Xm/Xm.h>
#include <Xm/MessageB.h>

#include "BasicComponent.h"
#include "ps_hdr.h"
#include "ps_i18n.h"
#include "ps_msg.h"
#include "mnem.h"

/*----------------------------------------------------------------------------
 *
 */
PSMsg::PSMsg (Widget parent, char* name, char* msgStr)
	 : BasicComponent (name)
{
	Widget						temp;
	char*						tempStr;
	char*						tempTitle;
	XmString					msg;
	XmString					title;
	Arg							args[16];
	Dimension					w;
	Dimension					h;

	tempStr = GetLocalStr (msgStr);
	tempTitle = GetLocalStr (TXT_psMsgTitle);

	msg = XmStringCreateLtoR (tempStr, XmSTRING_DEFAULT_CHARSET);
	title = XmStringCreateSimple (tempTitle);
	XtSetArg (args[0], XmNmessageString, msg);
	XtSetArg (args[1], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
	XtSetArg (args[2], XmNtitle, tempTitle);
	temp = XmCreateInformationDialog (parent, tempTitle, args, 3); 
	_w = temp;
	XtDestroyWidget (XmMessageBoxGetChild (temp, XmDIALOG_CANCEL_BUTTON));
	XtDestroyWidget (XmMessageBoxGetChild (temp, XmDIALOG_HELP_BUTTON));

	XtAddCallback (temp, XmNokCallback, PSMsg::OKCallback, this);
	registerMnemInfo (XmMessageBoxGetChild (temp, XmDIALOG_OK_BUTTON),
					  GetLocalStr (TXT_OKMnem),
					  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	XtManageChild (temp);

	XtVaGetValues (XtParent (temp), XmNwidth, &w, XmNheight, &h, NULL);
	XtVaSetValues (XtParent (temp),
				   XmNminWidth, w,
				   XmNmaxWidth, w,
				   XmNminHeight, h,
				   XmNmaxHeight, h,
				   0);
	XmStringFree (msg);
	XmStringFree (title);
	registerMnemInfoOnShell(_w);
}

/*----------------------------------------------------------------------------
 *
 */
PSMsg::~PSMsg()
{
	//XtDestroyWidget (XtParent (_w));
}

/*----------------------------------------------------------------------------
 *
 */
void
PSMsg::OKCallback (Widget, XtPointer data, XtPointer)
{ 
	delete ((PSMsg*)data);
}


/*----------------------------------------------------------------------------
 *
 */
PSError::PSError (Widget parent, char* message)
{
	char*						tempTitle;
	XmString					msg;
	XmString					title;
	Arg							args[16];

	tempTitle = GetLocalStr (TXT_psMsgTitle);
	msg = XmStringCreateLtoR (message, XmSTRING_DEFAULT_CHARSET);

	title = XmStringCreateSimple (tempTitle);
	XtSetArg (args[0], XmNmessageString, msg);
	XtSetArg (args[1], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
	XtSetArg (args[2], XmNtitle, tempTitle);
	d_w = XmCreateInformationDialog (parent, tempTitle, args, 3); 
	XtUnmanageChild (XmMessageBoxGetChild (d_w, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild (XmMessageBoxGetChild (d_w, XmDIALOG_HELP_BUTTON));

	XtAddCallback (d_w, XmNokCallback, PSError::OKCallback, this);
	registerMnemInfo (XmMessageBoxGetChild (d_w, XmDIALOG_OK_BUTTON),
					  GetLocalStr (TXT_OKMnem),
					  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	XtManageChild (d_w);

	XmStringFree (msg);
	XmStringFree (title);
	registerMnemInfoOnShell(d_w);
}

/*----------------------------------------------------------------------------
 *
 */
PSError::~PSError()
{
	XtDestroyWidget (d_w);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSError::OKCallback (Widget, XtPointer data, XtPointer)
{ 
	delete ((PSError*)data);
}

