/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwaccess:nwlogout.c	1.10"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/NetWare_Access/nwlogout.c,v 1.10 1994/09/22 16:14:58 renuka Exp $"

/*
**  Netware Unix Client 
**	Copyright Univel Inc. 1992
**	Author: Scott Harrison
	Modified: Renuka Veeramoney
**	Created: 06/06/91
**
**	MODULE: nwlogout
**	ABSTRACT: Utility to deauthenticate
*/
/*
#ifndef NOIDENT
#endif
*/

#include	<stdio.h>
#include	<string.h>
#include	<nw/nwcalls.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/FList.h>
#include <Xol/FButtons.h>
#include <Xol/ScrolledWi.h>
#include <Xol/ControlAre.h>
#include <Xol/Modal.h>

#include    "main.h"
#include    "scroll.h"

extern Widget		TopLevel;
extern char * 		GetStr (char *);
extern char 		*longformat, *shortformat;
extern void 		SetLabels ();
extern ListItem *  	GetItems (ServerList *);
extern ListItem *  	GetSelectedItems (ServerList *, int);

int    	        	Deauthenticate (Widget,	ServerList *);
static void    	        Setup_Deauth_Error (Widget, ServerList *,int *, int *);
static int    	        nwlogout (char *, int *);

/*******************************************************
			ERROR 
	functions ,format data, list item, user structures
*******************************************************/
typedef struct {
    XtPointer	text;
} ErrorFormat;

typedef struct {
    XtArgVal	formatData;
} ErrorListItem;

static String	ErrorListFields [] = {
    XtNformatData, 
};
static ErrorFormat	*Errorlist;
static ErrorListItem	*ElistItems;

static void 		CancelCB (Widget, XtPointer, XtPointer);
static MenuItem CommandItems [] = {
    { (XtArgVal) TXT_continue, (XtArgVal) MNEM_continue, (XtArgVal) True,
	  (XtArgVal) CancelCB, },			/* Cancel */
};
static String	ErrorMenuFields [] = {
    XtNlabel, XtNmnemonic, XtNdefault, XtNselectProc,
};
static int	NumErrorMenuFields = XtNumber (ErrorMenuFields);

static void    	        ErrorWindow (Widget, int);
static void 		PopdownCB (Widget, XtPointer, XtPointer);
static void 		MakeErrorList (char *, int);
static ErrorListItem *	GetErrorList(ErrorFormat *, int);

/*************************************************
	statically used variables 
**************************************************/
static int		size = 0;
static int		visible_item = 0;

/*************************************************
		Deauthenticate * * deauthenticate Selected itm 
**************************************************/
int
Deauthenticate (Widget widget, ServerList *servers)
{
    	int     	index = 0, errno, i, selected = 0, 
			did_not_logout[MAXSERVER]; 
    	int         searchi = 0, error_found = 0;
    	int         some_success = 0;

	/* for all servers in the list figure out each one */
    	for (i = 0; i < servers->cnt; i++)  {
		did_not_logout[i] = 0;
		/***************************************
		determine which pg is being selected from
		****************************************/
		switch (servers->set_toggle) {
		case False:
			if (servers->item_index_count[i] == 1) { 
				selected = 1;
				searchi  = i;
			}
			break;
		case True:
			if (servers->selected_item_index[i] == 1) { 
				selected = 1;
				searchi = servers->selected_value_index[i];
			}
			break;
		}
		/* if something was selected */
		if (selected) {
			selected = 0;

			/*********try to deauthenticate********/
			if (!nwlogout (servers->list[searchi].FileServers, 
						&errno))  {
				did_not_logout[i] = errno;	
				error_found = 1;
				index++;
			}  /* did not de-authenticate */
			else {
			/*printf("name %x\n",servers->list[searchi].userName);*/
				if (strcmp (servers->list[searchi].userName, 
						" ") == 0){
					did_not_logout[i] = 
						attached_not_authenticated;
					error_found = 1;
					index++;
				}
				else {
					did_not_logout[i] = successful;
					some_success = 1;
				}
			}  /* if the deauth worked */
		}  /* if selected */
    	}/* for all servers in the list */

	visible_item = searchi;

    	if (!error_found && !some_success)   {
		GUIError (widget, GetStr (TXT_noneselected)); 
		return 1;
    	}
    	else  
		Setup_Deauth_Error (widget, servers, &index, did_not_logout);

    	if (index > 0)
		return 1;
    	else
		return 0;		
}

/******************************************************************
	Setup the errors or reset server list for deauthentication
*******************************************************************/
static void
Setup_Deauth_Error (Widget widget, ServerList *servers, int *index, 
			int *didnotgo)
{
    	ListItem 	*ResetItems;
    	int         	i, some_success_flag = False, searchi;
	int 		j = 0;
	char		buf[BUFSIZ];

    	/*****************************************************
	 if not deauthenticated then setup error messages,
	 else if deauthenticated then remove the user name	
    	****************************************************/ 
    	for (i = 0; i < servers->cnt; i++) {
		if (servers->set_toggle == False) 
			searchi  = i;
		else
			searchi = servers->selected_value_index[i];
		switch (*didnotgo) {
		/* SUCCESSFUL */
		case successful:
			some_success_flag = True; 
 			servers->list[searchi].userName = 
						(XtPointer) strdup (" ");
 			servers->shortlist[searchi].userName = 
						(XtPointer) strdup (" ");
			break;

		/* DID NOT ATTACH */
		case not_attached:
			some_success_flag = True; 
 			servers->list[searchi].userName = 
						(XtPointer) strdup (" ");
 			servers->shortlist[searchi].userName = 
						(XtPointer) strdup (" ");
			strcpy (buf, GetStr (TXT_Notattached));
			break;

		/* SORRY */
		case sorry:
			strcpy (buf, GetStr (TXT_logoutfail));
			break;

		/* CANNOT FIND */
		case cantfind:
			strcpy (buf, GetStr (TXT_Cantfind));
			break;
		/* ATTACHED NOT AUTHENTICATED  */
		case attached_not_authenticated:
			strcpy (buf, GetStr (TXT_attachednotauth));
			break;
		} 
		/* ERROR OCCURED */
		if (*didnotgo > 1) {
			strcat (buf, servers->list[searchi].FileServers);
			MakeErrorList (buf, j);
			j++;
		}
		/* MOVE ON */
		didnotgo++;

    	}      /*for all servers */		
	
    	/******************************************************
			display the error message
    	******************************************************/
    	if (*index > 0)  { 
		i = 0;
		ErrorWindow (widget, *index);
	}

    	/*********************************************************
	 	if deauthentication went thru then reset the user name
    	**********************************************************/
    	if (some_success_flag == True) {
		if (servers->set_toggle == True) { 
			ResetItems = GetSelectedItems (servers,
						servers->pgone_count);
    			XtVaSetValues (servers->serverWidget, 
    			XtNformat,		(XtArgVal) 
		      	(servers->viewformat_flag == 0 ? 
						longformat : shortformat),
    			XtNitems,		(XtArgVal) ResetItems,
    			XtNnumItems,		(XtArgVal) servers->pgone_count,
			0);

			for (i = 0; i < servers->cnt; i++)  {
				if (servers->selected_item_index[i] == 1)
    					OlVaFlatSetValues(servers->serverWidget,
						i,  XtNset,(XtArgVal) True, 0); 
			}  /* highlight the selected items again*/
    		}
		else {
    			ResetItems = GetItems (servers);
    			XtVaSetValues (servers->serverWidget, 
    			XtNformat,		(XtArgVal) 
		      		(servers->viewformat_flag == 0 ? 
				longformat : shortformat),
    			XtNitems,		(XtArgVal) ResetItems,
    			XtNnumItems,		(XtArgVal) servers->cnt, 
			0);
			for (i = 0; i < servers->cnt; i++)  {
				if (servers->item_index_count[i] == 1)
    					OlVaFlatSetValues(servers->serverWidget,
					i,  XtNset,(XtArgVal) True, 0); 
			}  /* highlight the selected items again*/
    		}
    	}

	/* set the visibility index to the last item selected */
    	XtVaSetValues (servers->serverWidget, XtNviewItemIndex,	visible_item,0);

}

/***********************************************************
	nwlogout code - Huston Franklin 
**********************************************************/
static int
nwlogout(char *serverName, int *errno)
{
	int rc;
	NWCONN_HANDLE	connID;

	NWGetConnIDByName( serverName, &connID );
	if (connID == FALSE) {
		*errno = sorry;
		return 0;
	}
	    
	rc = NWLogout(connID, NULL);

	/* Hashem's fix.  If get connid by name works close connection
	 */
	NWCloseConn (connID);

	if (rc) {
		*errno = sorry;
		return 0;
	}
	else
		return 1;
}
		
/*****************************************************************
		scrolled error window 
 *****************************************************************/
static void
ErrorWindow (Widget w, int index)
{
    	static Boolean		first = True;
    	Widget		uca;
    	Widget		ca, rt, lca;
    	Widget		popup;
    	Widget		lcaMenu;
    	Widget		scrolledWindow;

    	popup = XtVaCreatePopupShell ("error",
		modalShellWidgetClass, w,
		XtNtitle,	(XtArgVal) GetStr (TXT_ErrorWindow),
		0);
	/* Set Labels */
    	if (first) {
		first = False;
		SetLabels (CommandItems,XtNumber(CommandItems));
    	}

    	scrolledWindow = XtVaCreateManagedWidget ("scrolledWindow",
		scrolledWindowWidgetClass, popup,
		0);

	ElistItems = GetErrorList (Errorlist, index);
    	lca = XtVaCreateManagedWidget ("list",
		flatListWidgetClass, scrolledWindow,
		XtNviewHeight,		(XtArgVal) 5,
		XtNexclusives,		(XtArgVal) True,
		XtNformat,		(XtArgVal) "%s",
		XtNnoneSet,		(XtArgVal) True,
		XtNitems,		(XtArgVal) ElistItems,
		XtNnumItems,		(XtArgVal) index,
		XtNitemFields,		(XtArgVal) ErrorListFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (ErrorListFields),
		0);

    /* We want an "cancel" and "reset" buttons in both the lower control
     * area and in a popup menu on the upper control area.
     */
    ca = XtVaCreateManagedWidget ("ca",
	controlAreaWidgetClass, popup,
	0);

    lcaMenu = XtVaCreateManagedWidget ("lcaMenu",
		flatButtonsWidgetClass, ca,
		XtNitemFields,		(XtArgVal) ErrorMenuFields,
		XtNnumItemFields,	(XtArgVal) NumErrorMenuFields,
		XtNitems,		(XtArgVal) CommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (CommandItems),
		0);

    /* Add callbacks to verify and destroy all widget when the property sheet
     * goes away
     */
    XtAddCallback (popup, XtNpopdownCallback, PopdownCB, (XtPointer)index);

    XtPopup (popup, XtGrabExclusive);
}	/* End of makenewuserlist () */

/*****************************************************************
		get the scrolled list
 *****************************************************************/
static ErrorListItem *
GetErrorList (ErrorFormat *list, int index)
{
    ErrorListItem			*items;
    register ErrorListItem		*pItem;
    register ErrorFormat		*pData;
    register				i;

    items = pItem = (ErrorListItem *)XtMalloc (index * sizeof (ErrorListItem));
    pData = list;
    for (i = index; --i>=0; pItem++, pData++) {
	pItem->formatData = (XtArgVal) pData;
    }
    return (items);
}	/* End of GetItems () */

/*****************************************************************
		make the scrolled error list
 *****************************************************************/
static  void
MakeErrorList(char *msg, int index)  
{
	if (index >= size) {
		size += SERVER_ALLOC_SIZE;
		Errorlist = (ErrorFormat *) XtRealloc ((char *) Errorlist,
	       			size  *  sizeof(ErrorFormat));
	}
	Errorlist [index].text = (XtPointer) strdup (msg);
}

/*****************************************************************
 * PopdownCB
 * Destroy the popup widget and free associated data.
 * client_data is pointer to dynamically allocated items list.
 *****************************************************************/
static void
PopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	int	i = 0;

    	XtDestroyWidget (widget);
	XtFree ((XtPointer) ElistItems);
	for (i = 0; i < (int) client_data;i++ )
		XtFree(Errorlist[i].text);
	size = 0;
}	/* End of PopdownCB () */

/*****************************************************************
	cancelCB
 *****************************************************************/
static void
CancelCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	Widget	shell;
    	shell = XtParent(widget);
    	while (!XtIsShell (shell))
        	shell = XtParent(shell);

    	XtPopdown (shell);
}       /* End of CancelCB () */

