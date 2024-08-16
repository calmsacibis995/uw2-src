/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwaccess:nwlogin.c	1.12"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/NetWare_Access/nwlogin.c,v 1.11.2.1.2.1 1994/12/16 18:17:27 renuka Exp $"

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
#include <nw/nwclient.h>
#include <pwd.h>
#include <unistd.h>
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
#include <Xol/FList.h>
#include <Xol/Caption.h>

#include "main.h" 
#include "scroll.h" 
/*
#include <nw/nwconnec.h>
#include <nw/nwerror.h>
*/

/**************************************************************
	 forward declaration 
*************************************************************/
extern void 		CancelCB (Widget, XtPointer, XtPointer);
extern void 		HelpCB (Widget, XtPointer, XtPointer);
extern void 		VerifyCB (Widget, XtPointer, XtPointer);
extern void             GUIError (Widget, char *);
extern char *           GetStr (char *);
extern void           	SetLabels (MenuItem *, int);
extern void           	SetHelpLabels (HelpText *);
extern int           	Deauthenticate (Widget, ServerList *);
extern ListItem* 	GetItems  (ServerList *);
extern ListItem* 	GetSelectedItems  (ServerList *, int);
extern Widget		TopLevel;
extern char 		*longformat, *shortformat;
extern char		*AttachError (uint32);
extern char		*LoginError (uint32);

#define	LoginHelpSect	"20"

/**************************************************************
	 static declaration 
*************************************************************/
static void 		ApplyCB (Widget, XtPointer, XtPointer);
static void 		PopdownCB (Widget, XtPointer, XtPointer);
static void 		Redisplay (char *, ServerList *);
int 			LoginToNetWare (char *, char *, char *);
static void             MakePanel (Widget, ServerList *);
static void LoginFieldVerifyCB(Widget, XtPointer, XtPointer);

/**********************************************************
	 Menu items and fields 
***********************************************************/
static String	MenuFields [] = {
    XtNlabel, XtNmnemonic, XtNsensitive, XtNselectProc, XtNdefault,
    XtNuserData, XtNpopupMenu,
};

static int 		NumMenuFields = XtNumber (MenuFields);

/***************************************************************
	* HELP VARIABLES 
***************************************************************/
static HelpText LoginHelp = {
    TXT_loginHelp,   LoginHelpSect,
};

/***********************************************************
	 Lower Control Area buttons 
*************************************************************/
static MenuItem CommandItems [] = {
    { (XtArgVal) TXT_save, (XtArgVal) MNEM_save, (XtArgVal) True,
	  (XtArgVal) ApplyCB, (XtArgVal) True, },	/* Save */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CancelCB, },			/* Cancel */
    { (XtArgVal) TXT_help, (XtArgVal) MNEM_help, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal)False, (XtArgVal) &LoginHelp, },/*Help*/
};

/******************************************************
	global variables
********************************************************/
static Widget 		login;
static Widget 		login_popup;
static Widget 		pwd;
static Widget 		foot;
static int    		searchi;
static char		errmsg[BUFSIZ];

/******************************************************************
	Authentication procedure
*****************************************************************/
int
Authenticate (Widget parent, ServerList *servers)
{
  	int 			code, i, found = 0, err = 0;
  	char			*username;
   	uid_t			uid;
   	Boolean			dosinglelogin = False;
	struct passwd 	*pwd;

  /********************************************************************
	      FIND OUT IF MORE THAN ONE SELECTION HAS BEEN 
	      MADE OR NO SELECTION HAS BEEN MADE for all servers 
  ********************************************************************/
  for (i = 0; i < servers->cnt; i++) {
   	switch (servers->set_toggle) {
		case True:	
			if (servers->pgtwo_count > 1) 
				err = 1;
			if (servers->selected_item_index[i] == 1) {
				found = 1;
				searchi = servers->selected_value_index[i];
			}
			break;
			
		case False:
			if (servers->pgone_count > 1) 
				err = 1;
			if (servers->item_index_count[i] == 1) {
				searchi = i;
				found = 1;
			}
			break;
    	}  /* switch statement */
    }  /* for statement */

    /**********in case it is authentication **************/
    if (err) { 
	GUIError (parent, GetStr (TXT_cantauth));
	return (0);
    }
    if (!found) { 
	GUIError (parent, GetStr (TXT_noneauthselected));
	return (0);
    }

	/* Try single login before doing anything else.
	 * Get the user name for redisplay if single login is enabled.
	 */
	uid	= getuid();
	setpwent();
	while (pwd = getpwent()) {
		if (pwd->pw_uid == uid) {

			char			buffer[BUFSIZ]; 

			/* see if the home directory/.slogin file exists
			 * If it does then single login is disabled.
			 */
			strcpy (buffer,pwd->pw_dir);
			strcat (buffer, "/.slogin");
			if (access (buffer, F_OK) < 0) 
				dosinglelogin = True;
			break;
		}
	}
	endpwent();

	/* Try single login to see if it works.  It single login was enabled for
	 * the user then redisplay the list with the user name authenticated to
	 * netware server and return else popup the login panel
	 */
	if (dosinglelogin) {
		if (SLAuthenticateUidRequest(servers->list[searchi].FileServers, 
								uid) == 0){
			Redisplay (pwd->pw_name, servers);
			return;
		}
	}

    if (login_popup) 
	XRaiseWindow (XtDisplay (login_popup), XtWindow (login_popup)); 
    else 
   	MakePanel (parent, servers);

}  

/***************************************************************
	make the panel for authentication
***************************************************************/
static void 
MakePanel (Widget parent, ServerList *servers)
{
    char  	   	*titleLbl;
    static Boolean  	once = False;
    Widget   		popup, lcaMenu, lca, uca;
    Widget          	footer;
    Widget	   	caption, pwcaption;
    Pixel           	bg;

    titleLbl = GetStr (TXT_authtitle);
	
    if (once == False) {
	once = True;
    	SetLabels (CommandItems, XtNumber(CommandItems));
    	SetHelpLabels (&LoginHelp);
     }

    /* Create authentication panel */
    login_popup = XtVaCreatePopupShell ("authentication",
		popupWindowShellWidgetClass, parent,
		XtNtitle,		(XtArgVal) titleLbl, 
		0);

    XtVaGetValues (login_popup,
		XtNlowerControlArea,	(XtArgVal) &lca,
		XtNupperControlArea,	(XtArgVal) &uca,
		XtNfooterPanel,		(XtArgVal) &footer,
		0);

    caption = XtVaCreateManagedWidget ("logintext",
			captionWidgetClass, uca,
			XtNlabel, (XtArgVal)GetStr(TXT_login),
			NULL);

    login = XtVaCreateManagedWidget ("loginfield", 
			textFieldWidgetClass, caption,
			XtNcharsVisible, (XtArgVal)20,
			0);

    XtAddCallback (login, XtNverification,
                        LoginFieldVerifyCB,(XtPointer)NULL);

    pwcaption = XtVaCreateManagedWidget ("pwdtext",
			captionWidgetClass, uca,
			XtNlabel, (XtArgVal)GetStr(TXT_pwd),
			0);

    pwd = XtVaCreateManagedWidget ("pwdfield", 
			textFieldWidgetClass, pwcaption,
 			XtNcharsVisible, (XtArgVal)20,
			0);
    XtVaGetValues (pwd, XtNbackground, &bg, NULL);
    XtVaSetValues (pwd, XtNforeground, bg, 
			XtNfontColor, bg, NULL);

    foot = XtVaCreateManagedWidget ("footermsg",
		staticTextWidgetClass, footer, 0);

    /* We want an "apply" and "cancel" buttons in lower control area */
    lcaMenu = XtVaCreateManagedWidget ("lcaMenu",
		flatButtonsWidgetClass, lca,
		XtNclientData,          (XtArgVal) servers,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) CommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (CommandItems),
		0);

    /* Add callbacks to verify and destroy all widget when the panel
     * goes away
     */
    XtAddCallback (login_popup, XtNverify, VerifyCB, (XtPointer) &PopdownOK);
    XtAddCallback (login_popup, XtNpopdownCallback, PopdownCB, NULL);

    XtPopup (login_popup, XtGrabNone);
}	/* End of Authenticate */

/*************************************************************************** 
 * PopdownCB
 * Destroy the popup widget and free associated data.
 * client_data is pointer to dynamically allocated items list.
 ***************************************************************************/
static void
PopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtDestroyWidget (widget);
    login_popup = (Widget) 0;
}	/* End of PopdownCB () */

/*************************************************************************** 
 * ApplyCB
 * apply callback.  
 ***************************************************************************/
static void
ApplyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	ServerList  *servers = (ServerList *) client_data;
	char        *username;
	char  	    *passwd;
	int         code; 
	ListItem    *items;
	Widget	    shell;
	Time	    time;

	/* get the login and the password  */
	username = OlTextFieldGetString (login, NULL);

	/* get the time stamp */
	time = XtLastTimestampProcessed (XtDisplay(widget));

	/* if the username was not entered then
	 * go back
	 */
	if (strlen (username) < 1) {
		XtVaSetValues (foot, XtNstring, (XtArgVal) 
				GetStr (TXT_enteruser), 0);
		if (OlCanAcceptFocus(login, time)) 
			OlSetInputFocus(login, RevertToNone, time);
	}
	else { 
		passwd = OlTextFieldGetString (pwd, NULL);
		if (strlen (passwd) < 1)
			passwd = NULL;

  		/***********CREATE THE WATCH *********************/
  		XDefineCursor (XtDisplay (login_popup), XtWindow (login_popup),
			 	GetOlBusyCursor (XtScreen (login_popup)));
  		XSync (XtDisplay (login_popup), 0);

	    	code = LoginToNetWare(servers->list[searchi].FileServers, 
				username, passwd);

		/***********REMOVE THE WATCH *********************/
   		XDefineCursor(XtDisplay (login_popup), XtWindow (login_popup),
				GetOlStandardCursor (XtScreen (login_popup)));
   		XSync (XtDisplay (login_popup), 0);

		switch (code) {
			case 0: 
				Redisplay (username, servers);
    				XtPopdown (login_popup);
				break;
			case 2:
				OlTextEditClearBuffer (pwd);
				if (OlCanAcceptFocus(pwd, time))
	                           OlSetInputFocus(pwd, RevertToNone, time);
				break; 
			default:
				OlTextEditClearBuffer (pwd);
				OlTextEditClearBuffer (login);
				if (OlCanAcceptFocus(login, time))
	                           OlSetInputFocus(login, RevertToNone, time);
				break;
		} 	

		/*******set the message to the footer *********/
		XtVaSetValues (foot, XtNstring, errmsg, 0);
	} 			/* if the login was entered */


} /* End of ApplyCB () */

/***************************************************************************
		Redisplay the authenticated items with the user name
 ***************************************************************************/
static void
Redisplay (char *username, ServerList *servers)
{
	int		i, item_position = 0;
	ListItem 	*items;

    	servers->list[searchi].userName = (XtPointer) strdup (username);
    	servers->shortlist[searchi].userName = (XtPointer) strdup (username);

    	switch (servers->set_toggle) {
	case True:
		items = GetSelectedItems (servers, servers->pgone_count);
			
		XtVaSetValues (servers->serverWidget, 
				XtNformat,  	(XtArgVal) 
						(servers->viewformat_flag == 0 ?
						longformat : shortformat),
				XtNitems, 	(XtArgVal) items,  
				XtNitemsTouched, (XtArgVal) True,  
				XtNnumItems, 	(XtArgVal)servers->pgone_count,
				0);
		for (i = 0; i < servers->cnt; i++) {
			if (servers->selected_item_index[i] == 1) {
				OlVaFlatSetValues (servers->serverWidget, i,
				XtNset, 	(XtArgVal) True, 
				0);
				item_position = i;
			}
		}
		break;
	case False:
		items = GetItems (servers);
		XtVaSetValues (servers->serverWidget, 
				XtNformat,  	(XtArgVal) 
						(servers->viewformat_flag == 0 ?
						longformat : shortformat),
				XtNitems, 	(XtArgVal) items,  
				XtNitemsTouched, (XtArgVal) True,  
				XtNnumItems, 	(XtArgVal)servers->cnt,
				0);
		for (i = 0; i < servers->cnt; i++) {
			if (servers->item_index_count[i] == 1) {
				OlVaFlatSetValues (servers->serverWidget, i,
				XtNset,	 (XtArgVal) True,
				0);
				item_position = i;
			}
		}
		break;
    	}	
	XtVaSetValues (servers->serverWidget, 
		XtNviewItemIndex,(XtArgVal) item_position, 
		0);
}


/****************************************************************************
	NetWare Login - uses the new api
	NWCALLS api.
******************************************************************************/
int
LoginToNetWare(char *serverName, char *userName, char *password)
{
	int	rc;
	NWCONN_HANDLE	connID;

	/*	We'll try logging in with no password
	 *	if that fails then we'll prompt the
	 *	user for the password and try logging
	 *	in again.
	 */


	rc = NWAttach( serverName, &connID, 0 );
	/* Attach error, use the AttachError routine */
	if (rc && rc != NWERR_ALREADY_ATTACHED) {
		strcpy (errmsg, AttachError (rc));
		return 1;
	} 
	else if (connID == FALSE) {
		strcpy (errmsg, GetStr (TXT_invalidconn));
		return 1;
	}

	if (isAuthenticated (connID)) {
		strcpy (errmsg, GetStr (TXT_cantlogin));
		return 1;
	}

	rc = NWLogin( (NWCONN_HANDLE)connID, userName, password, NULL);
	if (!rc) {
		strcpy (errmsg, GetStr (TXT_loginsucc));
		return 0;
	}
	else {
		/*login with no passwd did not work so ask for passwd*/
		if (password == NULL && rc == NO_SUCH_OBJECT_OR_BAD_PASSWORD) {
			strcpy (errmsg, GetStr (TXT_enterpasswd));
			NWDetach (connID);
			return 2;
		}
		else  { 
			NWDetach (connID);
			strcpy (errmsg, LoginError (rc));
/* Replaced the following with the LoginError routine supplied by libnct
			switch( rc ){
				case CONNECTION_LOGGED_IN:
					strcpy (errmsg, GetStr(TXT_connlogged));
					break;
				case ACCOUNT_DISABLED:
					strcpy (errmsg, GetStr(TXT_disableacc));
					break;
				case INVALID_CONNECTION:
					strcpy (errmsg,GetStr(TXT_invalidconn));
					break;
				case NO_SUCH_OBJECT_OR_BAD_PASSWORD:
					strcpy (errmsg, GetStr (TXT_badpwd));
					break;
				case PASSWORD_HAS_EXPIRED_NO_GRACE:
					strcpy (errmsg, GetStr (TXT_nograce));
					break;
				case PASSWORD_HAS_EXPIRED:
					strcpy (errmsg, GetStr(TXT_pwdexpired));
					break;
	    			default: 
					strcpy (errmsg, GetStr(TXT_unexpected));
					break;
			}
*/
			return 1;
		}
	}
}
/******************************************************************
   Move to password field on "CR" in login field procedure 
 *******************************************************************/
static void
LoginFieldVerifyCB(Widget w, XtPointer client_data,XtPointer call_data)
{
    OlTextFieldVerify *tfv = ( OlTextFieldVerify *)call_data;
    Time            time;

    if ( tfv->reason == OlTextFieldReturn && strlen(tfv->string))
    {
        tfv->ok = False;
        time = XtLastTimestampProcessed(XtDisplay(w));
        if (OlCanAcceptFocus(pwd, time))
            OlSetInputFocus(pwd, RevertToNone, time);
    }
}
