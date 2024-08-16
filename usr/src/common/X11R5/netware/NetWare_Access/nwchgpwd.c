/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwaccess:nwchgpwd.c	1.12"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/NetWare_Access/nwchgpwd.c,v 1.14 1994/09/22 16:14:56 renuka Exp $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL							*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#endif
*/

#include <stdio.h>
#include <string.h>
#include <nw/nwcalls.h> 
#include <nct.h> 
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/PopupWindo.h>
#include <Xol/MenuShell.h>
#include <Xol/Form.h>
#include <Xol/BulletinBo.h>
#include <Xol/FButtons.h>
#include <Xol/TextField.h>
#include <Xol/TextEdit.h>
#include <Xol/ScrolledWi.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>

#include "main.h" 
#include "scroll.h" 

/**********************************************************
	 extern declaration 
************************************************************/

extern void             GUIError (Widget, char *);
extern char *           GetStr (char *);
extern char *           ChangePasswordErrors(uint32);
extern void           	SetLabels (MenuItem *, int);
extern void           	SetHelpLabels (HelpText *);
extern void 		CancelCB (Widget, XtPointer, XtPointer);
extern void 		VerifyCB (Widget, XtPointer, XtPointer);
extern void 		reset_clock (Widget);
extern void 		set_clock (Widget);
extern void 		HelpCB (Widget, XtPointer, XtPointer);

#define 	PwdHelpSectTag	"90"
/****************************************************************** 
	Menu items and fields 
*****************************************************************/

static String	PwdMenuFields [] = {
    XtNlabel, XtNmnemonic, XtNsensitive, XtNselectProc, XtNdefault,
    XtNuserData, XtNpopupMenu,
};
static int 		NumPwdMenuFields = XtNumber (PwdMenuFields);

/*************************************************************
 Lower Control Area buttons 
*************************************************************/
static void 		ApplyCB (Widget, XtPointer, XtPointer);
static void 		PopdownCB (Widget, XtPointer, XtPointer);

/* HELP VARIABLES */
static HelpText PwdHelp = {
    TXT_pwdHelp,  PwdHelpSectTag,
};

static MenuItem PwdCommandItems [] = {
    { (XtArgVal) TXT_save, (XtArgVal) MNEM_save, (XtArgVal) True,
	  (XtArgVal) ApplyCB, (XtArgVal) True, },	/* Save */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CancelCB, },			/* Cancel */
    { (XtArgVal) TXT_help, (XtArgVal) MNEM_help, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal)False,(XtArgVal)&PwdHelp, }, /*help*/
};

/**********************************************************
	structure passed between eventhandlers
***********************************************************/
typedef struct {
	Widget 		chg_pwd;
	char		objectName[MAX_OBJECT_NAME_LENGTH];
	char		serverName[NWMAX_SERVER_LENGTH];
	Widget 		pwd_widget, foot, caption;
	int		num_times, done_once;
	Boolean		OLD;
	char 		*oldpasswd, *newpasswd;
}  PWD_STRUCT;

/********************************************************************
	statically declared routines
********************************************************************/
static int 		nwchgpwd (PWD_STRUCT *, char *, char *);
static int 		check_pwd (PWD_STRUCT *, char *);
static void 		clear_buffer (Widget);
static void             MakePwdPanel (PWD_STRUCT *,Widget, ServerList *, int);

/********************************************************************
	statically declared variables
********************************************************************/
static PWD_STRUCT      	pwdstruct;
static Widget		ancestor;

/******************************************************************
	change pwd procedure
*****************************************************************/
int
Change_pwd (Widget parent, ServerList *servers)
{
    int 		code, searchi;
	
    code = condition_handler (servers, &searchi);
    switch (code) {
	case 1:
		GUIError (parent, GetStr (TXT_cantshowpwd));
		break;
	case 2:
		GUIError (parent, GetStr (TXT_nopwdselected));
		break;
	case 3:
		GUIError (parent, GetStr (TXT_pwdnotbeenauth));
		break;
	default:
    		if (pwdstruct.chg_pwd) 
			XtPopdown (pwdstruct.chg_pwd);
		set_clock (parent);
		MakePwdPanel (&pwdstruct, parent, servers, searchi);
		reset_clock (parent);
		break;
     }	/* switch statement*/
}	/* End of chg_pwd () */

/***************************************************************
	make the panel for changing pwd
***************************************************************/
static void 
MakePwdPanel (PWD_STRUCT *spwd, Widget parent, ServerList *servers, int searchi)
{
    	static Boolean  	once = False;
    	Widget   		lcaMenu, lca, uca;
    	Widget          	footer;
    	Pixel           	bg, fg;
	int			retcode = 0;
	
	ancestor = parent;
	spwd->done_once = spwd->num_times = 0;
	spwd->OLD  = False;
	spwd->oldpasswd = spwd->newpasswd  = NULL;

    	strcpy (spwd->objectName, servers->list[searchi].userName);
    	strcpy (spwd->serverName, servers->list[searchi].FileServers);

    	if (once == False) {
		once = True;
    		SetLabels (PwdCommandItems, XtNumber(PwdCommandItems));
    		SetHelpLabels (&PwdHelp);
     	}

	/* first check if the pwd is needed at all */
	retcode = check_pwd (spwd, NULL);
	if (retcode == INVALID_CONNECTION) {
		GUIError (parent, GetStr (TXT_invalidconn));
		return;
	}

    	/* Create pwd panel */
    	spwd->chg_pwd = XtVaCreatePopupShell ("pwd",
		popupWindowShellWidgetClass, parent,
		XtNtitle,		(XtArgVal) GetStr (TXT_chgpwd),
		0);

    	XtVaGetValues (spwd->chg_pwd,
		XtNlowerControlArea,	(XtArgVal) &lca,
		XtNupperControlArea,	(XtArgVal) &uca,
		XtNfooterPanel,		(XtArgVal) &footer,
		0);

	switch (retcode) {
	/* without pwd worked */
	case 0:
    		spwd->caption = XtVaCreateManagedWidget ("pwdtext",
			captionWidgetClass, uca,
			XtNlabel, (XtArgVal)GetStr(TXT_newpwd),
			0);

    		spwd->pwd_widget = XtVaCreateManagedWidget ("pwdfield", 
			textFieldWidgetClass, spwd->caption,
 			XtNcharsVisible, (XtArgVal)20,
			0);
    		XtVaGetValues (spwd->pwd_widget, XtNbackground, &bg, NULL);
    		XtVaSetValues (spwd->pwd_widget, XtNforeground, bg, 
			XtNfontColor, bg, NULL);
		break;
	/* did not work without pwd */
	default:
		spwd->OLD = True;
    		spwd->caption = XtVaCreateManagedWidget ("oldpwd",
			captionWidgetClass, uca,
			XtNlabel, (XtArgVal)GetStr(TXT_oldpwd),
			NULL);

    		spwd->pwd_widget = XtVaCreateManagedWidget ("oldpwdfield", 
			textFieldWidgetClass, spwd->caption,
			XtNcharsVisible, (XtArgVal)20,
			0);
    		XtVaGetValues (spwd->pwd_widget, XtNbackground, &bg, NULL);
    		XtVaGetValues (spwd->pwd_widget, XtNforeground, &fg, NULL);
    		XtVaSetValues(spwd->pwd_widget, XtNforeground, bg, 
					XtNfontColor, bg, NULL);
		break;
	} /* switch the retcode */

	/* footer */
    	spwd->foot = XtVaCreateManagedWidget ("footermsg",
				staticTextWidgetClass, footer, 0);

    	/* We want an "apply" and "cancel" buttons in lower control area */
    	lcaMenu = XtVaCreateManagedWidget ("lcaMenu",
		flatButtonsWidgetClass, lca,
		XtNclientData,          (XtArgVal) spwd,
		XtNitemFields,		(XtArgVal) PwdMenuFields,
		XtNnumItemFields,	(XtArgVal) NumPwdMenuFields,
		XtNitems,		(XtArgVal) PwdCommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (PwdCommandItems),
		0);

    	/* Add callbacks to verify and destroy all widget when the panel
     	 * goes away
     	 */
     	XtAddCallback (spwd->chg_pwd, XtNverify, VerifyCB, 
						(XtPointer) &PopdownOK);
    	XtAddCallback (spwd->chg_pwd, XtNpopdownCallback, PopdownCB, spwd);
    	XtPopup (spwd->chg_pwd, XtGrabNone);
}	/* End of pwd */

/*************************************************************************** 
 * ApplyCB
 * apply callback.  
 ***************************************************************************/
static void
ApplyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	PWD_STRUCT  		*spwd = (PWD_STRUCT *) client_data;
	char		 	*repwd;
	/*static char		*oldpasswd = NULL;
	static char		*newpasswd = NULL;
	*/
	Time			time;
	int			retcode;

	/* get the time stamp */
	time = XtLastTimestampProcessed (XtDisplay(widget));
	OlSetInputFocus(spwd->pwd_widget, RevertToNone, time);

	if (spwd->OLD == True) {
		/*  get old pwd, store it in a static variable */
		spwd->oldpasswd = OlTextFieldGetString (spwd->pwd_widget, NULL);

		if (strlen (spwd->oldpasswd) < 1) {
			XtVaSetValues(spwd->foot, XtNstring, (XtArgVal)
				GetStr (TXT_enteroldpwd), 0);
			OlTextEditSetCursorPosition(spwd->pwd_widget,0,0,0);
			return;
		}
		/* if the pwd was entered check it out and if
		 * it is invalid then get out and pop it down
		 */
		set_clock (widget);
		if ((retcode = check_pwd (spwd, spwd->oldpasswd)) != 0) {
			/* check the old pwd to make sure it is right */
			char buffer[256];
			if (retcode == INVALID_CONNECTION)
				strcpy (buffer, GetStr (TXT_invalidconn));
			else
				strcpy (buffer, GetStr (TXT_invalidpwd));
			reset_clock (widget);
			XtPopdown (spwd->chg_pwd);
			GUIError(ancestor, buffer);
		}
		reset_clock (widget);

		/* ask to enter the new pwd */
		XtVaSetValues (spwd->caption, XtNlabel, 
				(XtArgVal) GetStr (TXT_newpwd), 0);

		/* clear new pwd buffer set the cursor to the beginning */
		clear_buffer (spwd->pwd_widget);

		/* dont come here again */
		spwd->OLD = False;
		XtVaSetValues (spwd->foot, XtNstring, (XtArgVal) " ", 0);
		return;
	}	/* done the old password parsing */

	if (!spwd->done_once) {

		/* if old pwd went thru, get new pwd and clear the buffer */
 		spwd->newpasswd = OlTextFieldGetString (spwd->pwd_widget, NULL);
	
		/* ask to retype the new pwd */
		XtVaSetValues (spwd->caption, XtNlabel, 
				(XtArgVal) GetStr (TXT_retypepwd), 0);
		XtVaSetValues (spwd->foot, XtNstring, (XtArgVal) " ", 0);

		/* clear new pwd buffer set the cursor to the beginning */
		clear_buffer (spwd->pwd_widget);

		/* dont come here again */
		spwd->done_once = 1;

	}	/* done the new password parsing */
	else  {
		/* get the retyped newpasswd */
		repwd = OlTextFieldGetString (spwd->pwd_widget, NULL);

		if (strcmp(spwd->newpasswd,repwd) != 0) {
			XtVaSetValues (spwd->foot,XtNstring, (XtArgVal)
					GetStr (TXT_nomatch), 0);
			XtVaSetValues (spwd->caption, XtNlabel, 
				(XtArgVal) GetStr (TXT_newpwd), 0);
			clear_buffer (spwd->pwd_widget);
			spwd->num_times++;
			/* if too many tries have been made sorry */
			if (spwd->num_times == 5)  {
				XtPopdown (spwd->chg_pwd);
				GUIError (ancestor, GetStr (TXT_toomany));
			}
			else {
				spwd->done_once = 0;
				return;
			}
		}
		
		set_clock (widget);
		/* change the pwd  and reset the static variables  */
		if ((nwchgpwd (spwd, spwd->oldpasswd, repwd)) == 0) {
			reset_clock (widget);
			XtPopdown (spwd->chg_pwd);
			GUIError (ancestor, GetStr (TXT_chgpwdsucc));
		}
		else {
			/* increment the count and reset variables */
			spwd->num_times++;
			if (spwd->num_times == 5)  {
				reset_clock (widget);
				XtPopdown (spwd->chg_pwd);
				GUIError (ancestor, GetStr (TXT_toomany));
			} /* if too many tries */
			else {
				spwd->done_once = 0;

				/* rename new passwd field */
				XtVaSetValues (spwd->caption, XtNlabel, 
					(XtArgVal) GetStr (TXT_newpwd), 0);

				/*  set the cursor position to the start */
				clear_buffer (spwd->pwd_widget);
			} /* not too many tries */

		} /* if change pwd did not work */

		reset_clock (widget);
	}/* if it is the second time */

} /* End of ApplyCB () */

/****************************************************************
		check the old password here
*****************************************************************/
static int
check_pwd (PWD_STRUCT *spwd, char *oldpasswd)
{
	int 		ccode = 1;
	int i;
	NWCONN_HANDLE	serverConnID;

	NWGetConnIDByName( spwd->serverName, &serverConnID );
	if (serverConnID != FALSE) {
		/*
	 	 *  Upper case the password and user name
	 	 */
    		for(i=0; i < strlen(oldpasswd); i++)
        		oldpasswd[i] = toupper(oldpasswd[i]);
    		for(i=0; i < strlen(spwd->objectName); i++)
        		spwd->objectName[i] = toupper(spwd->objectName[i]);
		ccode = NWVerifyObjectPassword(serverConnID, 
			(NWsp)spwd->objectName, OT_USER, (NWsp)oldpasswd);
		/* Hashem's fix. If get connid by name succeeds close connection
	 	 */
		NWCloseConn (serverConnID);
	}
	return  ccode;
}

/*****************************************************************
		clear buffer macro
 *****************************************************************/
static void 
clear_buffer  (Widget w)
{
	OlTextEditClearBuffer (w);
	OlTextEditSetCursorPosition (w, 0, 0, 0);
}

/*****************************************************************
 * PopdownCB
 * Destroy the popup widget and free associated data.
 * client_data is pointer to dynamically allocated items list.
 *****************************************************************/
static void
PopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PWD_STRUCT *spwd = (PWD_STRUCT *) client_data;

    	if (spwd) {
    		spwd->done_once =  spwd->num_times = 0;
    		spwd->chg_pwd = (Widget) 0;
    		spwd->pwd_widget = (Widget) 0;
    		spwd->foot = (Widget) 0;
    		spwd->caption = (Widget) 0;
    		spwd->OLD = FALSE; 
		spwd->newpasswd = NULL;
		spwd->oldpasswd = NULL;
    	}
    	XtDestroyWidget (widget);
}	/* End of PopdownCB () */

/****************************************************************
	change the password over here 
*****************************************************************/
static int
nwchgpwd (PWD_STRUCT *spwd, char *oldpasswd, char *newpasswd)
{
	int 	rc, retcode;
	NWCONN_HANDLE	connID;


	NWGetConnIDByName( spwd->serverName, &connID );
	if (connID == FALSE) {
		XtVaSetValues (spwd->foot, XtNstring, 
				(XtArgVal) GetStr (TXT_invalidconn), 0);
		return 1;
	}

	rc = NWChangePassword((NWCONN_HANDLE)connID, spwd->objectName, 
				oldpasswd, newpasswd);
	if (!rc) {
		XtVaSetValues (spwd->foot, XtNstring, 
				(XtArgVal) GetStr (TXT_chgpwdsucc), 0);
		retcode = 0;
	}
	/* Changed the errorhandling to call libnct call instead of
	 * error handling ourselves in here
	 */
	else {
		XtVaSetValues (spwd->foot, XtNstring, 
				(XtArgVal) ChangePasswordErrors (rc), 0);
		retcode = 1;
	}
#if 0
	switch (rc) {
		case 0:
			XtVaSetValues (spwd->foot, XtNstring, 
				(XtArgVal) GetStr (TXT_chgpwdsucc), 0);
			retcode = 0;
			break;
		case PASSWORD_NOT_UNIQUE:
			XtVaSetValues (spwd->foot, XtNstring, 
				(XtArgVal) GetStr (TXT_duppwd), 0);
			retcode = 1;
			break;
		case SERVER_BINDERY_LOCKED:
			XtVaSetValues (spwd->foot, XtNstring, 
				(XtArgVal) GetStr (TXT_loginlock), 0);
			retcode = 1;
			break;
		case NO_SUCH_OBJECT_OR_BAD_PASSWORD:
			XtVaSetValues (spwd->foot, XtNstring, 
			(XtArgVal)GetStr(TXT_nosuchobject), 0);
			retcode = 1;
			break;
		case PASSWORD_TOO_SHORT:
			XtVaSetValues (spwd->foot, XtNstring, 
				(XtArgVal)GetStr(TXT_pwdshort), 0);
			retcode = 1;
			break;
		case INTRUDER_DETECTION_LOCK:
			XtVaSetValues (spwd->foot, XtNstring, 
				(XtArgVal)GetStr(TXT_intruderlock), 0);
			retcode = 1;
			break;
		default:
			XtVaSetValues (spwd->foot, XtNstring, 
			(XtArgVal) GetStr (TXT_failedpwdchg), 0);
			retcode = 1;
			break;
	} /* switch rc */
#endif

	/* Hashem's fix.  If get connid by name succeeds close connection
	 */
	NWCloseConn (connID);
		
	return retcode;
}

