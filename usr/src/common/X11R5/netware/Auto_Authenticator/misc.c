/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoauthent:misc.c	1.7"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Auto_Authenticator/misc.c,v 1.8 1994/09/06 19:18:08 plc Exp $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL, Inc.  			                   	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#endif
*/

#include <stdio.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/MessageB.h>
#include <Xm/Protocols.h>
#include "dtFuncs.h"



#include "main.h"
#include "mnem.h"

static void	ErrorSelectCB (Widget, XtPointer, XtPointer);
static void	ErrorPopdownCB (Widget, XtPointer, XtPointer);
#ifdef __cplusplus
extern "C" char	*gettxt (char *, char *);
#else
extern char	*gettxt (char *, char *);
#endif
extern char	*GetStr (char *idstr);

char 	*global_msg;
int		global_errType;

/* GuiError Notification
 *
 * Display a notice box with an error message.  The only button is a
 * "continue" button.
 */
void
GuiError (Widget widget, char *errorMsg, int errType)
{
	XmString xmstr;
    Arg arglist[20];
    Cardinal num_args;
	Widget error;
	Widget shell;
	Atom WM_DELETE_WINDOW;
	XmString okXmString;


	global_errType = errType;
	global_msg = XtMalloc (strlen (errorMsg) + 1);
	strcpy (global_msg, errorMsg);

    xmstr = XmStringCreateSimple(global_msg);
    num_args = 0;
	XtSetArg(arglist[num_args], XmNautoUnmanage, False);num_args++;
	XtSetArg(arglist[num_args], XmNdialogStyle, 
	         XmDIALOG_FULL_APPLICATION_MODAL);num_args++;
    XtSetArg(arglist[num_args],XmNmessageString,xmstr);num_args++;
    XtSetArg(arglist[num_args],XmNtitle,GetStr(TXT_errorPanel));num_args++;
	if (errType == WARNING ) 
	{
    	XtSetArg(arglist[num_args],XmNtitle,GetStr(TXT_warningPanel));num_args++;
		error = XmCreateWarningDialog(widget,"error", arglist,num_args);
	}
	else
	{
    	XtSetArg(arglist[num_args],XmNtitle,GetStr(TXT_errorPanel));num_args++;
    	error = XmCreateErrorDialog(widget, "error", arglist,num_args);
	}
    XmStringFree(xmstr);
	okXmString = XmStringCreateLtoR (GetStr(TXT_ok),
	                                 XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(error, XmNokLabelString, okXmString, NULL);
	XmStringFree(okXmString);
    XtUnmanageChild(XmMessageBoxGetChild(error,XmDIALOG_HELP_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(error,XmDIALOG_CANCEL_BUTTON));
	XtAddCallback(error,XmNokCallback,
                    (XtCallbackProc)ErrorSelectCB,(XtPointer)error);
	XtAddCallback(error,XmNdestroyCallback,
                    (XtCallbackProc)ErrorPopdownCB,(XtPointer)error);

    /* 
	 * Find the dialog's shell widget 
	 */
    for (shell = error; !XtIsShell(shell); shell = XtParent(shell));

    /* 
	 * Don't want toolkit to handle Close action so tell it to do nothing
	 */
    XtVaSetValues(shell, XmNdeleteResponse, XmDO_NOTHING, NULL);

	/*
	 * Install the WM_DELETE_WINDOW atom 
	 */
    WM_DELETE_WINDOW = XmInternAtom(XtDisplay(shell),"WM_DELETE_WINDOW",False);

    /* 
	 * Add callback for the WM_DELETE_WINDOW protocol 
	 */
    XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW, ErrorSelectCB, error);

	registerMnemInfo(XmMessageBoxGetChild(error, XmDIALOG_OK_BUTTON), 
				GetStr(MNEM_ok), MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(error);

    XtManageChild(error);
    XtPopup(XtParent(error),XtGrabExclusive);

} /* End of GuiError () */

/* ErrorSelectCB
 *
 * When the button is pressed, popdown the notice.
 * The notice is given an client_data.
 */
static void
ErrorSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	XtDestroyWidget((Widget)client_data);
	if ( global_errType == FATAL_ERROR)
	{
		sendCloseFolderReq(widget);
		exit (0);
	}
} /* End of ErrorSelectCB () */

/* ErrorPopdownCB
 *
 * Destroy Error notice on popdown
 */
static void
ErrorPopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	if (global_msg)
		XtFree (global_msg);
   	XtDestroyWidget (widget);
	if ( global_errType == FATAL_ERROR)
	{
		sendCloseFolderReq(widget);
		exit (0);
	}
} /* End of ErrorPopdownCB () */

/* GetStr
 *
 * Get an internationalized string.  String id's contain both the filename:id
 * and default string, separated by the FS_CHR character.
 */
char *
GetStr (char *idstr)
{
    return (getStr(idstr));
}	/* End of GetStr () */


