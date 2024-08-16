/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwaccess:misc.c	1.2"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/NetWare_Access/misc.c,v 1.2 1994/08/18 15:49:58 rv Exp $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL, Inc.  			                   	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#endif
*/

#include <stdio.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/Modal.h>
#include <Xol/StaticText.h>
#include <Xol/FButtons.h>

#include "main.h"
#include "scroll.h"

static void	ErrorSelectCB (Widget, XtPointer, XtPointer);
static void	ErrorPopdownCB (Widget, XtPointer, XtPointer);
/*extern char	*gettxt (char *, char *);*/
extern char	*GetStr (char *idstr);
static Widget	text;
extern Boolean 	endApplication;

/* Lower Control Area buttons */
static String	LcaFields [] = {
    XtNlabel, XtNmnemonic, XtNdefault,
};

static struct {
    XtArgVal	lbl;
    XtArgVal	mnem;
    XtArgVal	dflt;
} LcaItems [1];

/* Error Notification
 *
 * Display a notice box with an error message.  The only button is a
 * "continue" button.
 */
void
GUIError (Widget widget, char *errorMsg)
{
    Widget		notice;
    static Boolean	first = True;
	char 		*mnem;

    if (first)
    {
	first = False;
	LcaItems [0].lbl = (XtArgVal) GetStr (TXT_continue);
	mnem = GetStr (MNEM_continue);
	LcaItems [0].mnem = (XtArgVal) mnem[0];
	LcaItems [0].dflt = (XtArgVal) True; 
    }

    notice = XtVaCreatePopupShell (GetStr (TXT_message), modalShellWidgetClass, widget,
				   0);

    /* Add the error message text */
    text = XtVaCreateManagedWidget ("errorTxt", staticTextWidgetClass, notice,
		XtNstring,		(XtArgVal) errorMsg,
		XtNalignment,		(XtArgVal) OL_CENTER,
    		XtNfont,		(XtArgVal) _OlGetDefaultFont (widget,
							OlDefaultNoticeFont),
		0);

    /* Add the continue button to the bottom */
    (void) XtVaCreateManagedWidget ("lcaButton",
		flatButtonsWidgetClass, notice,
		XtNtraversalOn,		(XtArgVal) True,
		XtNclientData,		(XtArgVal) notice,
		XtNselectProc,		(XtArgVal) ErrorSelectCB,
		XtNitemFields,		(XtArgVal) LcaFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (LcaFields),
		XtNitems,		(XtArgVal) LcaItems,
		XtNnumItems,		(XtArgVal) XtNumber (LcaItems),
		0);

    XtAddCallback (notice, XtNpopdownCallback, ErrorPopdownCB,
		   (XtPointer) 0);

    XtPopup (notice, XtGrabExclusive);
} /* End of GUIError () */

/* ErrorSelectCB
 *
 * When a button is pressed in the lower control area, popdown the notice.
 * The notice is given an client_data.
 */
static void
ErrorSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
/*
	char  *textfield;

	XtVaGetValues (text, XtNstring, &textfield, 0);
	if ((strncmp (textfield, GetStr (TXT_defaultconnfailed), 
			strlen(textfield)) == 0)||
		(strncmp (textfield, GetStr (TXT_propreadfail), 
			strlen(textfield)) == 0)|| 
		(strncmp (textfield, GetStr (TXT_Noserver), 
			strlen(textfield)) == 0) ||
		(strncmp (textfield, GetStr (TXT_connstatusfailed), 
			strlen(textfield)) == 0))
		exit (1);
*/
	if (endApplication == True)
		exit (0);
    	XtPopdown ((Widget) client_data);
} /* End of ErrorSelectCB () */

/* ErrorPopdownCB
 *
 * Destroy Error notice on popdown
 */
static void
ErrorPopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    	XtDestroyWidget (widget);
} /* End of ErrorPopdownCB () */

/* GetStr
 *
 * Get an internationalized string.  String id's contain both the filename:id
 * and default string, separated by the FS_CHR character.
 */
char *
GetStr (char *idstr)
{
    char	*sep;
    char	*str = idstr;

    	sep = (char *)strchr (idstr, FS_CHR);
	if (sep) {
    		*sep = '\0';
    		str = (char *)gettxt (idstr, sep + 1);
    		*sep = FS_CHR;
	}
    	return (str);
}	/* End of GetStr () */

/* SetLabels
 *
 * Set menu item labels and mnemonics.
 */
void
SetLabels (MenuItem *items, int cnt)
{
    char	*mnem;

    for ( ;--cnt>=0; items++)
    {
	items->lbl = (XtArgVal) GetStr ((char *) items->lbl);
	mnem = GetStr ((char *) items->mnem);
	items->mnem = (XtArgVal) mnem [0];
    }
}	/* End of SetLabels */

/*
 *		SetHelpLabels
 *
 * Set strings for help text.
 */
void
SetHelpLabels (HelpText *help)
{
    help->title = GetStr (help->title);
}	/* End of SetHelpLabels */

/*****************************************************************
	condition to determine which page and to decide on the
	index of the list items to be used
*****************************************************************/
int
condition_handler (ServerList *servers, int *searchi)
{
    int i, found = 0;

    /******************************************************
	determine which page you want to use for selection
    *******************************************************/
    for (i = 0; i < servers->cnt; i++) {
    	switch (servers->set_toggle) {
		case True:	
    			if (servers->pgtwo_count > 1) 
				return (1);
			if (servers->selected_item_index[i] == 1) {
				found = 1;
				*searchi = servers->selected_value_index[i];
			}
			break;
		case False:
    			if (servers->pgone_count > 1) 
				return (1);
			if (servers->item_index_count[i] == 1) {
				*searchi = i;
				found = 1;
			}
			break;
    	}  /* switch statement */
    }  /* for statement */

    if (!found)  
	return (2);
   
    if (strcmp (servers->list[*searchi].userName, " ") == 0) 
	return (3);
    
    return (0);
}

/**************************************************************************
Verify callback.  client_data is a pointer to the flag indicating if it's
 * ok to pop the window down.  Reset this flag back to false.
 ***************************************************************************/
void
VerifyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    Boolean	*pOk = (Boolean *) call_data;
    Boolean	*pFlag = (Boolean *) client_data;

    *pOk = *((Boolean *) client_data);
    *pFlag = False;
}	/* End of VerifyCB () */

/***************************************************************************
 * Cancel callback.  client_data is a pointer to the flag indicating if it's
 * ok to pop the window down.  Reset this flag back to false.
 ***************************************************************************/
void
CancelCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
  Widget      shell;
	
    shell = XtParent(widget);
    while (!XtIsShell (shell))
        shell = XtParent(shell);

    XtPopdown (shell);
   /* PopdownOK = True; */
}       /* End of CancelCB () */

void
set_clock (Widget w)
{
	Widget shell;
	shell = XtParent (w);

	while (!XtIsShell (shell))
		shell = XtParent (shell);

	XDefineCursor (	XtDisplay (shell), XtWindow (shell),
		 	GetOlBusyCursor (XtScreen (shell)));
  	XSync (XtDisplay (shell), 0);
}

void
reset_clock (Widget w)
{
	Widget shell;
	shell = XtParent (w);

	while (!XtIsShell (shell))
		shell = XtParent (shell);
   	XDefineCursor (	XtDisplay (shell), XtWindow (shell), 
			GetOlStandardCursor (XtScreen (shell)));
   	XSync (XtDisplay (shell), 0);
}

