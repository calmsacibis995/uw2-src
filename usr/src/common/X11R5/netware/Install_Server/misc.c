/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)instlsrvr:misc.c	1.2"
#ident	"@(#)misc.c	9.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Install_Server/misc.c,v 1.1 1994/02/01 22:57:22 renu Exp $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL Inc.                     			*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#ident	"@(#)misc.c	1.0"
#endif
*/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Notice.h>
#include <DtI.h>

#include "main.h"

/**************************************************************
		forward declarations
**************************************************************/
extern char  	*gettxt ();
void 		Error ();
void     HelpCB ();
void     CancelCB ();
void     VerifyCB ();
void     DisplayHelp ();
void     QuestionDialog ();
void     cancel_func ();
static int	 Remove_pkgs (Widget);
extern int	 pipe_command ();
extern Widget	foot_msg;

/* GetStr
 *
 * Get an internationalized string.  String id's contain both the filename:id
 * and default string, separated by the FS_CHR character.
 */
char *
GetStr (char *idstr)
{
    char	*sep;
    char	*str;

    sep = strchr (idstr, FS_CHR);
    *sep = 0;
    str = gettxt (idstr, sep + 1);
    *sep = FS_CHR;

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
    if (help->section)
	help->section = GetStr (help->section);
}	/* End of SetHelpLabels */

 /*****************************************************************
 * Set button item labels.
 *****************************************************************/
void
SetButtonLbls (ButtonItem *items, int cnt)
{
  char	*mnem;
  
  /* label the remaining items */
  for ( ; --cnt>=0; items++) {
	 items->lbl = (XtArgVal) GetStr ((char *) items->lbl);
	 mnem = GetStr ((char *) items->mnem);
	 items->mnem = (XtArgVal) mnem [0];
  }
}	/* End of SetButtonLbls */

/************************************************************
		error
**************************************************************/
#include <Xol/Modal.h>
#include <Xol/StaticText.h>
#include <Xol/FButtons.h>

static void	ErrorSelectCB (Widget, XtPointer, XtPointer);
static void	ErrorPopdownCB (Widget, XtPointer, XtPointer);

/* Lower Control Area buttons */
static String	LcaFields [] = {
    XtNlabel, XtNmnemonic,  XtNdefault, 
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
Error (Widget widget, char *errorMsg)
{
    	Widget		notice;
    	static Boolean	first = True;
	char 		*mnem;

    if (first)
    {
	first = False;
	LcaItems [0].lbl = (XtArgVal) GetStr (TXT_continue);
	mnem =  GetStr (MNEM_continue);
	LcaItems [0].mnem = (XtArgVal) mnem[0];
	LcaItems [0].dflt = (XtArgVal) True;
    }

    notice = XtVaCreatePopupShell ("Message", modalShellWidgetClass, widget,
				   0);

    /* Add the error message text */
    XtVaCreateManagedWidget ("errorTxt", staticTextWidgetClass, notice,
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
} /* End of Error () */

/* ErrorSelectCB
 *
 * When a button is pressed in the lower control area, popdown the notice.
 * The notice is given an client_data.
 */
static void
ErrorSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
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

/********************************************************************* 
 * HelpCB
 * Display help.  userData in the item is a pointer to the HelpText data.
 *********************************************************************/
void
HelpCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    	OlFlatCallData	*flatData = (OlFlatCallData *) call_data;
    	MenuItem		*selected;

	XtVaSetValues (foot_msg, XtNstring, SPACES, 0);
    	selected = (MenuItem *) flatData->items + flatData->item_index;
    	DisplayHelp (widget, (HelpText *) selected->userData);
}	/* End of HelpCB () */


/**********************************************************************
 * DisplayHelp
 * Send a message to dtm to display a help window.  If help is NULL, then
 * ask dtm to display the help desk.
***********************************************************************/
void
DisplayHelp (Widget widget, HelpText *help)
{
    DtRequest			*req;
    DtDisplayHelpRequest	displayHelpReq;
    Display			*display = XtDisplay (widget);
    Window			win = XtWindow (XtParent (XtParent (widget)));

    req = (DtRequest *) &displayHelpReq;
    displayHelpReq.rqtype = DT_DISPLAY_HELP;
    displayHelpReq.serial = 0;
    displayHelpReq.version = 1;
	 displayHelpReq.client = win;
    displayHelpReq.nodename = NULL;

    if (help)
    {
	displayHelpReq.source_type =
	    help->section ? DT_SECTION_HELP : DT_TOC_HELP;
	displayHelpReq.app_name = GetStr (TXT_appName);
	displayHelpReq.app_title = GetStr (TXT_appName);
	displayHelpReq.title = help->title;
	displayHelpReq.help_dir = NULL;
	displayHelpReq.file_name = help->file;
	displayHelpReq.sect_tag = help->section;
    }
    else
	displayHelpReq.source_type = DT_OPEN_HELPDESK;

    (void)DtEnqueueRequest(XtScreen (widget), _HELP_QUEUE (display),
			   _HELP_QUEUE (display), win, req);
}	/* End of DisplayHelp () */

/* CancelCB */
void
CancelCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	cancel_func (widget);
}

/* generic cancel function */
void 
cancel_func  (Widget widget)
{
  Widget      shell;
  
  shell = XtParent(widget);
  while (!XtIsShell (shell))
	 shell = XtParent(shell);
  XtPopdown (shell);
}	/* End of CancelCB () */

/* VerifyCB
 *
 * Verify callback.
 */
void
VerifyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
  Boolean     *pOk = (Boolean *) call_data;
  Boolean     *pFlag = (Boolean *) client_data;
  
  *pOk = *((Boolean *) client_data);
  *pFlag = False;
}       /* End of VerifyCB () */

/*********************************************************
		display message
**********************************************************/
static void YesNoCB ();

/* Lower Control Area buttons */
static String	LowerFields [] = {
    	XtNlabel, XtNmnemonic, XtNdefault,
};

static struct {
    XtArgVal	lbl;
    XtArgVal	mnem;
    XtArgVal	dflt;
} LowerItems [2];

static Widget 	question;

void
QuestionDialog (Widget w, char *msg)
{
    	Widget		ca, text;
    	static Boolean	first = True;
	char 		*mnem;

    	if (first)
    	{
		first = False;
		LowerItems [0].lbl = (XtArgVal) GetStr (TXT_Ok);
		mnem = GetStr (MNEM_Ok);
		LowerItems [0].mnem = (XtArgVal) mnem[0];
		LowerItems [0].dflt = (XtArgVal) False;

		LowerItems [1].lbl = (XtArgVal) GetStr (TXT_No);
		mnem = GetStr (MNEM_No);
		LowerItems [1].mnem = (XtArgVal) mnem[0]; 
		LowerItems [1].dflt = (XtArgVal) True;
    	}

    	question = XtVaCreatePopupShell ("Message", noticeShellWidgetClass, w,
			XtNnoticeType,		(XtArgVal) OL_QUESTION,
				   	0);

	XtVaGetValues (question,XtNcontrolArea, &ca,
			 	XtNtextArea, &text, 0);

    	/* Add the message text */
    	XtVaSetValues (text,
			XtNstring,		(XtArgVal) msg,
			XtNalignment,		(XtArgVal) OL_CENTER,
    			XtNfont,		(XtArgVal) 
						_OlGetDefaultFont (w,
						OlDefaultNoticeFont),
			0);

    	/* Add the continue button to the bottom */
    	(void) XtVaCreateManagedWidget ("lcaButton",
		flatButtonsWidgetClass, ca,
		XtNtraversalOn,		(XtArgVal) True,
		XtNselectProc,		(XtArgVal) YesNoCB,
		XtNitemFields,		(XtArgVal) LowerFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (LowerFields),
		XtNitems,		(XtArgVal) LowerItems,
		XtNnumItems,		(XtArgVal) XtNumber (LowerItems),
		0);

	XtAddCallback (question, XtNpopdownCallback, ErrorPopdownCB,
			(XtPointer) 0);
    	XtPopup (question, XtGrabExclusive);
}

extern Widget		action_buttons;
extern Widget		pkg_btns;

/* YESNOCB
 *
 * When a button is pressed in the lower control area, popdown the notice.
 * The notice is given an client_data.
 */
static void
YesNoCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData		*p = (OlFlatCallData *) call_data;
	int			i = 0, retval = 0;
	char			*ptr;

	if (!p->item_index) {
		/* set su and remove the configuration file 
		 * and then disable the sapd for the install server 
		 */
		if (!Remove_pkgs (widget)) {
			if ((pipe_command(widget,RM_COMMAND,CONFIG_FILE)) == 0){
				if ((ptr = disableInstallSAP ()) != NULL){
					Error(XtParent(question), GetStr (ptr));
					return;
				}
			}
			else 
				retval = 1;
		}
		else 
			retval = 1;

		if (retval)
			Error (XtParent(question), GetStr (TXT_cantremove));
		else {
			/* sensitize the LOAD button and desensitize the rest */
			OlVaFlatSetValues (action_buttons, LOAD - 1, 
					XtNsensitive, (XtArgVal) True, 0);
			OlVaFlatSetValues (action_buttons, CONFIGURE - 1, 
					XtNsensitive, (XtArgVal) False, 0);
			OlVaFlatSetValues (action_buttons, REMOVE - 1, 
					XtNsensitive, (XtArgVal) False, 0);
			for (i = 0; i< 2; i++) 
				OlVaFlatSetValues (pkg_btns, i, 
					XtNsensitive, (XtArgVal) False, 0);
			/* set the footer to the error msg */
			XtVaSetValues (foot_msg, XtNstring, GetStr(TXT_removed), 					0);
    			XtPopdown (question);
		}
	}
	else
    		XtPopdown (question);
}

static int
Remove_pkgs (Widget widget) 
{
	char		*ptr, buf[BUFSIZ];
	struct stat 	stat_buf; 
	int		found = 0,i = 0;
	char 		filename[BUFSIZ];
	FILE		*fp;

  	/* if the file cannot be opened then return NULL */
  	if ((fp = fopen (CONFIG_FILE, "r")) == NULL) 
	 	return 1;
	
  	/* check the stat on the file, read it into a buffer and get the size
	 *  close the file after the fread and set a ptr to the buffer
	 */
  	if ((stat (CONFIG_FILE, &stat_buf)) == NULL) {
		if (stat_buf.st_size < 1) 
			return 1;
	 	fread(buf, sizeof(char), stat_buf.st_size, fp);
	 	fclose (fp);

	 	/* go thru the buffer and if either as or pe are found 	
		 * the remove pkg file and then return found */
	 	ptr = buf;

		/* while not eof */
	 	while (*ptr) {
			/* if the ptr points to /  and it is 
			 * get the filename into a buffer
			 */
			if (strncmp (SLASH, ptr, strlen (SLASH)) == 0) {
				i = 0;
				while (*ptr != '\t' && *ptr != ' ')
					filename[i++] = *ptr++;
				filename[i] = '\0';
				/* remove it using the pipe command */
				if ((pipe_command(widget,RM_FILE,filename))!=0) 
					return 1;
				found = 1;
		 	} /* if the file is found */
			if (found)
				break;
			else
				ptr++;
  		} /* while loop */
	} /* if the file is accessible */
	return (found == 1  ? 0 : 1); 
}
