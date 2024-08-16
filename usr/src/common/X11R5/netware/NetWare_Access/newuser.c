/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwaccess:newuser.c	1.3"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/NetWare_Access/newuser.c,v 1.3 1994/09/01 16:31:44 renuka Exp $"

/*
**	Copyright Univel Inc. 1992
**
**	Author: Renuka Veeramoney
**	Utility used to display users for setuid and execute new user.
*/
/*
#ifndef NOIDENT
#endif
*/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <shadow.h>
#include <ia.h>
#include <crypt.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>
#include <Xol/TextField.h>
#include <Xol/TextEdit.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <Xol/FList.h>
#include <Xol/ScrolledWi.h>

#include "main.h" 
#include "scroll.h" 

/********************************************************************
	* extern declaration 
*********************************************************************/
extern void		VerifyCB (Widget, XtPointer, XtPointer);
extern void		set_clock (Widget);
extern void		reset_clock (Widget);
extern void 		CancelCB (Widget, XtPointer, XtPointer);
extern int		condition_handler (ServerList *, int *);
extern void		HelpCB (Widget, XtPointer, XtPointer);
extern void		GUIError (Widget, char *);
extern char *		GetStr (char *);
extern void 		SetLabels (MenuItem *, int);
extern void 		SetHelpLabels (HelpText *);
extern Widget		TopLevel;

#define	NewLoginHelpSect	"50"

/*******************************************************
	format data, list item, user structures
*******************************************************/
typedef struct {
    XtPointer	login_name;
} NewLoginFormat;

typedef struct {
    XtArgVal	formatData;
} NewLoginListItem;

typedef struct {
    Widget		listWidget;
    NewLoginListItem	*listItems;
    NewLoginFormat	*list;
    unsigned		cnt;
    Boolean		selected;
    unsigned 		index;
    unsigned		allocated;
} NewLoginList;

static String	ListFields [] = {
    XtNformatData, 
};

/**********************************************
	header fields
***********************************************/
static NewLoginFormat	ColHdrs [1];
static NewLoginListItem		ColItem [] = {
    (XtArgVal) ColHdrs,
};

/***************************************************
	statically declared variables
****************************************************/
static NewLoginList 		NewUsers;
static Widget 			newuser_list, pwd_popup, pwd_widget, pwdfoot;

/********************************************************************
	* forward declaration 
*********************************************************************/
static int                      nwusers (Widget, NewLoginList *);
static Widget                   Make_user_sheet (Widget, NewLoginList *);
static void 			FreeNewLogins (NewLoginList *);
static void 			PopdownCB (Widget, XtPointer, XtPointer);
static void 			PwdPopdownCB (Widget, XtPointer, XtPointer);
static void 			ApplyCB (Widget, XtPointer, XtPointer);
static void 			UnSelectCB (Widget, XtPointer, XtPointer);
static void 			SelectCB (Widget, XtPointer, XtPointer);
static void 			PwdApplyCB (Widget, XtPointer, XtPointer);
static void 			DblSelectCB (Widget, XtPointer, XtPointer);
static void 			fork_process (Widget, NewLoginList *);
static int 			fork_exec_program (Widget, char*);
static NewLoginListItem		*GetNewLoginItems (NewLoginList *);

/**********************************************************************
 		Lower Control Area buttons 
***********************************************************************/
static HelpText NewLoginHelp = {
    TXT_NewLoginHelp, NewLoginHelpSect, 
};

static MenuItem CommandItems [] = {
    { (XtArgVal) TXT_save, (XtArgVal) MNEM_save, (XtArgVal) True,
	  (XtArgVal) ApplyCB, (XtArgVal) True, (XtArgVal) 0},/*Save */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CancelCB, },			/* Cancel */
    { (XtArgVal) TXT_helpW, (XtArgVal) MNEM_helpW, (XtArgVal) True,
  	(XtArgVal) HelpCB, (XtArgVal) False, (XtArgVal) &NewLoginHelp, },
							/* Help*/
};


static String	NewLoginMenuFields [] = {
    XtNlabel, XtNmnemonic, XtNsensitive, XtNselectProc, XtNdefault,
    XtNuserData, XtNpopupMenu,
};

static int	NumNewLoginMenuFields = XtNumber (NewLoginMenuFields);

static HelpText EnterpwdHelp = {
    TXT_EnterpwdHelp, 
};

static MenuItem PwdCommandItems [] = {
    { (XtArgVal) TXT_save, (XtArgVal) MNEM_save, (XtArgVal) True,
	  (XtArgVal) PwdApplyCB, (XtArgVal) True, (XtArgVal) &NewUsers},/*Save */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CancelCB, },			/* Cancel */
    { (XtArgVal) TXT_helpW, (XtArgVal) MNEM_helpW, (XtArgVal) True,
  	(XtArgVal) HelpCB, (XtArgVal) False, (XtArgVal) &EnterpwdHelp, },
							/* Help*/
};

/***********************************************************************
		newlogin_list list for the netware servers
***********************************************************************/
void
NewUserCB (Widget parent, XtPointer client_data, XtPointer call_data)
{
    	static Boolean		first = True;

    	if (newuser_list) 
		XRaiseWindow (XtDisplay (newuser_list),XtWindow (newuser_list));
	else {
		NewUsers.selected = False;
		NewUsers.index = 0;
    		if (nwusers (parent, &NewUsers) == 1)  
			GUIError (parent, GetStr (TXT_nousersfound));
    		else {
	    		/* Set Labels */
    			if (first) {
				first = False;
				SetLabels (CommandItems,XtNumber(CommandItems));
				SetHelpLabels (&NewLoginHelp);
				ColHdrs [0].login_name = (XtPointer) GetStr 	
							(TXT_loginname);
    			}
			newuser_list = Make_user_sheet (parent, &NewUsers);
      		}
	}
}

/***********************************************************************
		newlogin_list list for the netware servers
***********************************************************************/
static Widget
Make_user_sheet (Widget parent, NewLoginList *newusers)
{
    Widget		uca;
    Widget		lca;
    Widget		popup;
    Widget		lcaMenu;
    Widget		ucaMenuShell;
    Widget		ucaMenu;
    Widget		scrolledWindow;

    /* Create user sheet */
    popup = XtVaCreatePopupShell ("newuser",
		popupWindowShellWidgetClass, parent,
		XtNtitle,		(XtArgVal) GetStr (TXT_newusers),
		0);
   
    XtVaGetValues (popup,
		XtNlowerControlArea,	(XtArgVal) &lca,
		XtNupperControlArea,	(XtArgVal) &uca,
		0);

    /* Create a list of users   Add a flat list above this
     * with a single item in it to act as column headers.
     */
    newusers->listWidget = XtVaCreateManagedWidget ("colHdr",
		flatListWidgetClass, uca,
		XtNviewHeight,		(XtArgVal) 1,
		XtNexclusives,		(XtArgVal) True,
		XtNnoneSet,		(XtArgVal) True,
		XtNselectProc,		(XtArgVal) 0,
		XtNitems,		(XtArgVal) ColItem,
		XtNnumItems,		(XtArgVal) 1,
		XtNitemFields,		(XtArgVal) ListFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (ListFields),
		XtNformat,		(XtArgVal) " %s",
		XtNtraversalOn,		(XtArgVal) False,
		0);

    scrolledWindow = XtVaCreateManagedWidget ("scrolledWindow",
		scrolledWindowWidgetClass, uca,
		0);

    newusers->listItems = GetNewLoginItems (newusers);

    newusers->listWidget = XtVaCreateManagedWidget ("newuser_list",
		flatListWidgetClass, scrolledWindow,
		XtNviewHeight,		(XtArgVal) 5,
		XtNexclusives,		(XtArgVal) True,
		XtNnoneSet,		(XtArgVal) True,
		XtNselectProc,		(XtArgVal) SelectCB,
		XtNclientData,		(XtArgVal) newusers,
		XtNdblSelectProc,	(XtArgVal) DblSelectCB,
		XtNclientData,		(XtArgVal) newusers,
		XtNunselectProc,	(XtArgVal) UnSelectCB,
		XtNclientData,		(XtArgVal) newusers,
		XtNitems,		(XtArgVal) newusers->listItems,
		XtNnumItems,		(XtArgVal) newusers->cnt,
		XtNitemFields,		(XtArgVal) ListFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (ListFields),
		XtNformat,		(XtArgVal) "%s     ",
		0);

    /* We want an "cancel" and "reset" buttons in both the lower control
     * area and in a popup menu on the upper control area.
     */
    lcaMenu = XtVaCreateManagedWidget ("lcaMenu",
		flatButtonsWidgetClass, lca,
		XtNclientData,		(XtArgVal) newusers->listWidget,
		XtNitemFields,		(XtArgVal) NewLoginMenuFields,
		XtNnumItemFields,	(XtArgVal) NumNewLoginMenuFields,
		XtNitems,		(XtArgVal) CommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (CommandItems),
		0);

    ucaMenuShell = XtVaCreatePopupShell ("ucaMenuShell",
		popupMenuShellWidgetClass, uca,
		0);

    ucaMenu = XtVaCreateManagedWidget ("ucaMenu",
		flatButtonsWidgetClass, ucaMenuShell,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNclientData,		(XtArgVal) newusers->listWidget,
		XtNitemFields,		(XtArgVal) NewLoginMenuFields,
		XtNnumItemFields,	(XtArgVal) NumNewLoginMenuFields,
		XtNitems,		(XtArgVal) CommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (CommandItems),
		0);

    OlAddDefaultPopupMenuEH (uca, ucaMenuShell);

    /* Add callbacks to verify and destroy all widget when the property sheet
     * goes away
     */
    XtAddCallback (popup, XtNverify, VerifyCB, (XtPointer) &PopdownOK);
    XtAddCallback (popup, XtNpopdownCallback, PopdownCB,
		   (XtPointer) newusers->listItems);

    XtPopup (popup, XtGrabNone);
	return popup;
}	/* End of makenewuserlist () */


/*****************************************************************
 * PopdownCB
 * Destroy the popup widget and free associated data.
 * client_data is pointer to dynamically allocated items list.
 *****************************************************************/
static void
PopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    	XtDestroyWidget (widget);
    	FreeNewLogins (&NewUsers);
	if (client_data)
    		XtFree (client_data);
    	newuser_list = (Widget) 0;
}	/* End of PopdownCB () */

/***************************************************************
 * GetNewLoginItems
 * Format the volume list into items for the flat list.
 ***************************************************************/
static NewLoginListItem *
GetNewLoginItems (NewLoginList *NewLogins)
{
    NewLoginListItem			*items;
    register NewLoginListItem		*pItem;
    register NewLoginFormat		*pData;
    register			i;

    items = pItem = (NewLoginListItem *) 
		     XtMalloc (NewLogins->cnt * sizeof (NewLoginListItem));
    pData = NewLogins->list;
    for (i = NewLogins->cnt; --i>=0; pItem++, pData++) {
	pItem->formatData = (XtArgVal) pData;
    }

    return (items);
}	/* End of GetNewLoginItems () */

/************************************************************
	free the user data
**************************************************************/
static void
FreeNewLogins (NewLoginList *NewLogins)
{
    register	i;
    int		index;

    for (i = NewLogins->cnt; --i>=0; ) 
	XtFree (NewLogins->list[i].login_name);

    NewLogins->cnt = 0;
}	/* End of FreeNewLogins () */

/************************************************************************
	ApplyCB
 	apply callback.  
 ***************************************************************************/
static void
ApplyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	if (NewUsers.selected == False)
		GUIError (widget, GetStr (TXT_selectuser));
	else
		(void) fork_process (widget, &NewUsers);
}

		
/************************************************************************
	SelectCB
 	select callback.  
 ***************************************************************************/
static void
SelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	NewLoginList	*newlogins = (NewLoginList *) client_data;
	OlFlatCallData 	*pFlatData = (OlFlatCallData *) call_data;
		
	newlogins->index = pFlatData->item_index;
	newlogins->selected = True;
}
		
/************************************************************************
	dblSelectCB
 	double select callback.  
 ***************************************************************************/
static void
DblSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	NewLoginList	*newlogins = (NewLoginList *) client_data;
	OlFlatCallData 	*pFlatData = (OlFlatCallData *) call_data;
		
	newlogins->index = pFlatData->item_index;
	newlogins->selected = True;
	OlVaFlatSetValues (widget, newlogins->index, 
				XtNset, True, 
				0);
	(void) fork_process (widget, newlogins);
}
		
/************************************************************************
	unselectcb
 	un select callback.  
 ***************************************************************************/
static void
UnSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	NewLoginList	*newlogins = (NewLoginList *) client_data;
	OlFlatCallData 	*pFlatData = (OlFlatCallData *) call_data;
		
	newlogins->selected = False;
}

/*******************************************************
	read the users into the list item
********************************************************/
static int
nwusers(Widget parent, NewLoginList *NewLogins)
{
	int  		found = 0;
	struct passwd	*pwd ;
  
	setpwent ();
	while (pwd = getpwent()) {
		found = 1;
		if (NewLogins->cnt >= NewLogins->allocated) { 
			NewLogins->allocated += 2;
			NewLogins->list = (NewLoginFormat *)
				XtRealloc ((char *) NewLogins->list,
	       			NewLogins->allocated * 
				sizeof(NewLoginFormat));
		}
		NewLogins->list [NewLogins->cnt++].login_name = 
				(XtPointer) strdup (pwd->pw_name);
     	} 
	endpwent ();

	if (found)
   		return 0;
	else
		return 1;
}

/***************************************************************************
	fork the proccess
 ***************************************************************************/
static void
fork_process (Widget widget, NewLoginList *newlogins)
{
    	Widget   	lcaMenu, lca, uca, caption, foot;
    	Pixel           bg;
	static	int 	once;
	struct 	passwd	*pwd;
	
    	if (pwd_popup) { 
		XRaiseWindow (XtDisplay (pwd_popup),XtWindow (pwd_popup));
		return;
	}
	/* get the password for newuser and if there is no passwd 
	 * then fork and exec right now , else popup the pwd window
	 * etc.
	 */
	set_clock (widget);
	if ((fork_exec_program (widget, NULL)) == 0) {
		reset_clock (widget);
		return;
	}
	
    	if (once == False) {
		once = True;
    		SetLabels (PwdCommandItems, XtNumber(PwdCommandItems));
    		SetHelpLabels (&EnterpwdHelp);
     	}
	
    	/* Create pwd panel */
    	pwd_popup = XtVaCreatePopupShell ("enterpwd",
			popupWindowShellWidgetClass, widget,
			XtNtitle,	(XtArgVal) GetStr(TXT_enterpwd),
			0);
	
    	XtVaGetValues (pwd_popup,
			XtNlowerControlArea,	(XtArgVal) &lca,
			XtNupperControlArea,	(XtArgVal) &uca,
			XtNfooterPanel,		(XtArgVal) &foot,
			0);
	
    	caption = XtVaCreateManagedWidget ("enterpwd",
				captionWidgetClass, uca,
				XtNlabel, 	(XtArgVal) GetStr(TXT_change),
				NULL);
	
    	pwd_widget = XtVaCreateManagedWidget ("pwdtext", 
				textFieldWidgetClass, caption,
				XtNcharsVisible,	(XtArgVal)20,
				0);
	
    	XtVaGetValues (pwd_widget, XtNbackground, &bg, NULL);
    	XtVaSetValues (pwd_widget, XtNforeground, bg, XtNfontColor, bg, NULL);
	
    	pwdfoot = XtVaCreateManagedWidget ("footermsg", 
					staticTextWidgetClass, foot, 0);
	
    	/* We want an "apply" and "cancel" buttons in lower control area */
    	lcaMenu = XtVaCreateManagedWidget ("lcaMenu",
			flatButtonsWidgetClass, lca,
		XtNitemFields,		(XtArgVal)NewLoginMenuFields,
		XtNnumItemFields,	(XtArgVal)NumNewLoginMenuFields,
		XtNitems,		(XtArgVal)PwdCommandItems,
		XtNnumItems,		(XtArgVal)XtNumber 
					(PwdCommandItems),
		0);
	
    	/* Add callbacks to verify and destroy all widget when the panel
     	* goes away
     	*/
    	XtAddCallback (pwd_popup,XtNverify, VerifyCB, (XtPointer) &PopdownOK);
    	XtAddCallback (pwd_popup, XtNpopdownCallback, PwdPopdownCB, NULL);
	
	reset_clock (widget);
    	XtPopup (pwd_popup, XtGrabNone);
}	/* End of enter pwd */
	
/*************************************************************************** 
 * PwdPopdownCB
 * Destroy the popup widget and free associated data.
 * client_data is pointer to dynamically allocated items list.
 ***************************************************************************/
static void
PwdPopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	XtDestroyWidget (widget);
	pwd_popup = (Widget) 0;
}	/* End of PopdownCB () */

/*************************************************************************** 
 * PwdApplyCB
 * Pwdapply callback.  
 ***************************************************************************/
static void
PwdApplyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData *pFlatData = (OlFlatCallData *) call_data;
	char 		*pw;

	set_clock (widget);
	/**************************************************
	clear the area in the footer panel
	**************************************************/
    	XtVaSetValues (pwdfoot, XtNstring, " ", 0);

	pw = OlTextFieldGetString (pwd_widget, NULL);
	if ((fork_exec_program (widget, pw)) == 0) {
		reset_clock (widget);
		if (pwd_popup)
			XtPopdown (pwd_popup);
	}
	else {
		reset_clock (widget);
		/* if the password is wrong then sorry */
    		XtVaSetValues (pwdfoot, XtNstring, GetStr (TXT_Sorry),0);
		/* clear the pwd buffer and reposition the cursor */
		OlTextEditClearBuffer (pwd_widget); 
		OlTextEditSetCursorPosition (pwd_widget, 0, 0, 0); 
	}

} /* End of PwdApplyCB () */

/* fork  and exec the program */
static int
fork_exec_program (Widget widget, char *pw)
{
	pid_t		chpid;
	uid_t		curr_uid;
	struct passwd 	*pwd;
	char 		*ia_pwdp, *nptr;
	uinfo_t		uinfo;

	int 		nhosts;
	int 		enabled = 0;

	/* List hosts that are in the access control list 
	 * to determine if access control has been enabled 
	 * for them or not
	 */
	XListHosts(XtDisplay(widget), &nhosts, &enabled);
	XFlush(XtDisplay(widget));

	/* get the password structure and current uid, then setuid to root*/
	pwd = getpwnam (NewUsers.list[NewUsers.index].login_name);

	curr_uid = getuid ();
	setuid (0);

	/* get the password for the user */
	nptr = pwd->pw_name;

	if (ia_openinfo (nptr, &uinfo)  || (uinfo == NULL))  {
		/* reset the uid back to the olduser in the parent */	
		setuid (curr_uid);
		return 1;
	}
	else	{
		ia_get_logpwd (uinfo, &ia_pwdp);

		/* reset the uid back to the olduser in the parent */	
		setuid (curr_uid);

		/* if passwords are the same as the password in the structure
	 	 * then execute the netware pgm for the new unixware user 
	 	 */
		if (strcmp (ia_pwdp, crypt (pw, ia_pwdp)) == 0) {
			if ((chpid = fork ()) <  0) 
    				XtVaSetValues (pwdfoot, XtNstring, 
						GetStr (TXT_Sorry), 0);
			else if (chpid == 0) { /* child */
				/* set the uid to the user */
				setuid (pwd->pw_uid);

				/* If access control is enabled disable it and 
	 		 	 * pass argument to NetWare_Access to re-enable
				 * it else just start NetWare_Access w/o any 
				 * arguments
	 		 	 */ 
				if ( enabled ) {
#ifdef DEBUG
printf("access control was enabled, disabling and setting enable arg\n");
#endif
					XDisableAccessControl (XtDisplay
								(widget));
					XFlush(XtDisplay(widget));
					if (execl("/usr/X/bin/NetWare_Access",
						"NetWare_Access", "-a",0) == -1)
						GUIError (widget, GetStr
							(TXT_execfailed));
				}
				else {
#ifdef DEBUG
printf("access control was not enabled\n");
#endif
					if (execl("/usr/X/bin/NetWare_Access",
						"NetWare_Access",0) == -1) 
						GUIError (widget, GetStr
							(TXT_execfailed));
				}
/* Pre-Access control code. 
				if (execl("/usr/X/bin/NetWare_Access",
						"NetWare_Access",0) == -1) 
					GUIError (widget, GetStr
							(TXT_execfailed));
*/
			} /* child */
			else { /* parent */
				setuid (curr_uid);
				return (0);
			} /* parent */
		}
		/* if password did not match */
		else 
			return(1);
	} 
	/* reset the uid back to the olduser in the parent */	
	setuid (curr_uid);
	return 0;
}
