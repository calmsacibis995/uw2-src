/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwaccess:ulist.c	1.8"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/NetWare_Access/ulist.c,v 1.9 1994/09/22 16:15:08 renuka Exp $"

/*
 * Copyright 1989, 1991 Unpublished Work of Univel, Inc. All Rights Reserved.
 *
 * THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL,
 * PROPRIETARY AND TRADE SECRET INFORMATION OF UNIVEL, INC. ACCESS
 * TO THIS WORK IS RESTRICTED TO (I) UNIVEL EMPLOYEES WHO HAVE A
 * NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
 * ASSIGNMENTS AND (II) ENTITIES OTHER THAN UNIVEL WHO HAVE
 * ENTERED INTO APPROPRIATE AGREEMENTS.
 * NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF UNIVEL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 */

/*  
**	Title:	userlist (utility)     
**       Date:	7/19/90    
**
*/

#include <stdio.h>
#include <nw/nwcalls.h>
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

/*****************************************************
	external declarations
******************************************************/
extern void 			GUIError (Widget, char *);
extern 	void			HelpCB (Widget, XtPointer, XtPointer);
extern 	char *			GetStr (char *);
extern  void			SetLabels (MenuItem *, int);
extern  void			SetHelpLabels (HelpText *);
extern  Widget			TopLevel;

/*************************************************
	user type structure for user info
****************************************************/
#define		UserHelpSect		"110"
#define 	LOGIN_TIME_SIZE 	7
#ifndef uint16
#define uint16		unsigned short
#endif

typedef struct {
	uint16	clientConnID;
	char	clientObjectName[MAX_OBJECT_NAME_LENGTH];
	uint8	loginTime[LOGIN_TIME_SIZE]; 	/* 7 bytes long */
} users;


/********************************************************
	format data, list item, userlist item structures
*******************************************************/
typedef struct {
    XtPointer	connection;
    XtPointer	name;
    XtPointer	logintime[6];
} UserFormat;

typedef struct {
    XtArgVal	formatData;
} UserListItem;

typedef struct {
    Widget		listWidget;
    UserListItem	*listItems;
    UserFormat		*list;
    unsigned		cnt;
    unsigned		allocated;
} UserList;

static String	ListFields [] = {
    XtNformatData, 
};

static Widget		headerWidget;

/********************************************************************
	* forward declaration 
*********************************************************************/
static int 			ReadUsers(Widget, UserList *, char *);
static void 			FreeUsers (UserList *);
static void 			PopdownCB (Widget, XtPointer, XtPointer);
static void 			HeaderSelectProc (Widget, XtPointer, XtPointer);
static UserListItem		*GetUserItems (UserList *);
static void 			MakeUserList (Widget, char *);
static int			GetUserItemsMaxLength (UserList *Users);
static void 			MakeFormatStrings(char *headerBuf,char *listBuf, UserList *Users);

/********************************************************************
	 external declaration 
*********************************************************************/
extern void			VerifyCB (Widget, XtPointer, XtPointer);
extern void 			CancelCB (Widget, XtPointer, XtPointer);

/**********************************************************************
 		Lower Control Area buttons 
***********************************************************************/
static HelpText UserHelp = {
    TXT_userHelp,  UserHelpSect,
};

static MenuItem CommandItems [] = {
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CancelCB, (XtArgVal) True},		/* Cancel */
    { (XtArgVal) TXT_helpW, (XtArgVal) MNEM_helpW, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False, (XtArgVal)&UserHelp, },/*Help */
};

static String	UserMenuFields [] = {
    XtNlabel, XtNmnemonic, XtNsensitive, XtNselectProc, XtNdefault,
    XtNuserData, XtNpopupMenu,
};

static int	NumUserMenuFields = XtNumber (UserMenuFields);

/**********************************************
	header fields
***********************************************/
static UserFormat		ColHdrs [1];
static UserListItem		ColItem [] = {
    (XtArgVal) ColHdrs,
};


/***************************************************
	statically declared variables
****************************************************/
static UserList 	Users;
static Widget 		user_list;
static char		*titleLbl;

/***********************************************************************
		user list for the netware servers
***********************************************************************/
int
User_List (Widget parent, ServerList *servers)
{
    	int i, err = 0, found = 0, searchi = 0;
	
    	/******************************************************
		determine which page you want to use for selection
    	*******************************************************/
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
        	if (err)  {
			GUIError (parent, GetStr (TXT_cantshowuser));
			return (1);
    		}
    	}  /* for statement */
	
    	if (!found)  {
		GUIError (parent, GetStr (TXT_nouserselected));
		return (1);
    	}
    	if (strcmp (servers->list[searchi].userName, " ") == 0) {
		GUIError (parent, GetStr (TXT_notbeenauth));
		return (1);
    	}
	
    	if (user_list) {
		XtPopdown (user_list);
 	}
	XDefineCursor ( XtDisplay (TopLevel), XtWindow (TopLevel), 
		       	GetOlBusyCursor (XtScreen (TopLevel)));
	XSync (XtDisplay (TopLevel), 0);

	MakeUserList (parent, servers->list[searchi].FileServers);

	XDefineCursor ( XtDisplay (TopLevel), XtWindow (TopLevel), 
		        GetOlStandardCursor (XtScreen (TopLevel)));
	XSync (XtDisplay (TopLevel), 0);
}	/* End of userlist () */

/*********************************************************************** 
		MakeUserList
 		Make the user list 
************************************************************************/
static void
MakeUserList (Widget parent, char *servername)
{
    /*Widget		popup;*/
    Widget		uca;
    Widget		lca;
    Widget		lcaMenu;
    Widget		ucaMenuShell;
    Widget		ucaMenu;
    Widget		scrolledWindow;
    static Boolean	first = True;
    char 		headerBuf[256];
    char 		listBuf[256];

    if (ReadUsers (parent, &Users, servername) == 1) 
		return ;

    /* Set Labels */
    if (first) {
	first = False;

	SetLabels (CommandItems, XtNumber (CommandItems));
	SetHelpLabels (&UserHelp);

	ColHdrs [0].connection = (XtPointer) GetStr (TXT_connection);
	ColHdrs [0].name = (XtPointer) GetStr (TXT_uname);
	ColHdrs [0].logintime[0] = (XtPointer) GetStr (TXT_logintime);
    }

    titleLbl = ( char *) XtMalloc (strlen (GetStr (TXT_ulist)) +  
				       strlen (servername) + 2);
    strcpy (titleLbl ,GetStr (TXT_ulist));
    strcat (titleLbl, servername);

    /* Create user sheet */
    user_list = XtVaCreatePopupShell ("user",
		popupWindowShellWidgetClass, parent,
		XtNtitle,		(XtArgVal) titleLbl,
		0);
   
    XtVaGetValues (user_list,
		XtNlowerControlArea,	(XtArgVal) &lca,
		XtNupperControlArea,	(XtArgVal) &uca,
		0);

    /*
     * Create the list format strings 
     */
    MakeFormatStrings(headerBuf,listBuf,&Users);

    /* Create a list of users   Add a flat list above this
     * with a single item in it to act as column headers.
     */
    headerWidget = XtVaCreateManagedWidget ("colHdr",
		flatListWidgetClass, uca,
		XtNviewHeight,		(XtArgVal) 1,
		XtNexclusives,		(XtArgVal) True,
		XtNnoneSet,		(XtArgVal) True,
		XtNselectProc,		(XtArgVal) HeaderSelectProc,
		XtNitems,		(XtArgVal) ColItem,
		XtNnumItems,		(XtArgVal) 1,
		XtNitemFields,		(XtArgVal) ListFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (ListFields),
		XtNformat,		(XtArgVal)headerBuf,
		XtNfont, 		(XtArgVal) _OlGetDefaultFont 
					(parent, OlDefaultFixedFont),
		XtNtraversalOn,		(XtArgVal) False,
		0);

    scrolledWindow = XtVaCreateManagedWidget ("scrolledWindow",
		scrolledWindowWidgetClass, uca,
		0);

    Users.listItems = GetUserItems (&Users);

    Users.listWidget = XtVaCreateManagedWidget ("user_list",
		flatListWidgetClass, scrolledWindow,
		XtNviewHeight,		(XtArgVal) 5,
		XtNexclusives,		(XtArgVal) True,
		XtNnoneSet,		(XtArgVal) True,
		XtNitems,		(XtArgVal) Users.listItems,
		XtNnumItems,		(XtArgVal) Users.cnt,
		XtNitemFields,		(XtArgVal) ListFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (ListFields),
		XtNfont, 		(XtArgVal) _OlGetDefaultFont 
					(parent, OlDefaultFixedFont),
		XtNformat,		(XtArgVal) listBuf,
		0);

    /* We want an "cancel" and "reset" buttons in both the lower control
     * area and in a popup menu on the upper control area.
     */
    lcaMenu = XtVaCreateManagedWidget ("lcaMenu",
		flatButtonsWidgetClass, lca,
		XtNclientData,		(XtArgVal) Users.listWidget,
		XtNitemFields,		(XtArgVal) UserMenuFields,
		XtNnumItemFields,	(XtArgVal) NumUserMenuFields,
		XtNitems,		(XtArgVal) CommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (CommandItems),
		0);

    ucaMenuShell = XtVaCreatePopupShell ("ucaMenuShell",
		popupMenuShellWidgetClass, uca,
		0);

    ucaMenu = XtVaCreateManagedWidget ("ucaMenu",
		flatButtonsWidgetClass, ucaMenuShell,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNclientData,		(XtArgVal) Users.listWidget,
		XtNitemFields,		(XtArgVal) UserMenuFields,
		XtNnumItemFields,	(XtArgVal) NumUserMenuFields,
		XtNitems,		(XtArgVal) CommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (CommandItems),
		0);

    OlAddDefaultPopupMenuEH (uca, ucaMenuShell);

    /* Add callbacks to verify and destroy all widget when the property sheet
     * goes away
     */
    XtAddCallback (user_list, XtNverify, VerifyCB, (XtPointer) &PopdownOK);
    XtAddCallback (user_list, XtNpopdownCallback, PopdownCB,
		   (XtPointer) Users.listItems);

    XtPopup (user_list, XtGrabNone);

}	/* End of MakePropertySheet () */


/*****************************************************************
 * PopdownCB
 * Destroy the popup widget and free associated data.
 * client_data is pointer to dynamically allocated items list.
 *****************************************************************/
static void
PopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtDestroyWidget (widget);
    FreeUsers (&Users);
    XtFree (client_data);
    XtFree (titleLbl);
    user_list = (Widget) 0;
}	/* End of PopdownCB () */

/***************************************************************
 * GetUserItems
 * Format the Users list into items for the flat list.
 ***************************************************************/
static UserListItem *
GetUserItems (UserList *Users)
{
    UserListItem		*items;
    register UserListItem	*pItem;
    register UserFormat		*pData;
    register		i;

    items = pItem = (UserListItem *) 
		     XtMalloc (Users->cnt * sizeof (UserListItem));
    pData = Users->list;
    for (i = Users->cnt; --i>=0; pItem++, pData++) {
	pItem->formatData = (XtArgVal) pData;
    }

    return (items);
}	/* End of GetUserItems () */
/***************************************************************
 * GetUserItemsMaxLength
 * Retrieve the maximum length of the user name.
 ***************************************************************/
static int 
GetUserItemsMaxLength (UserList *Users)
{
    register UserFormat		*pData;
    register		i;
    register		x;
    register		length = 0;

    pData = Users->list;
    for (i = Users->cnt; --i>=0; pData++) {
    	x = strlen(pData->name);
    		if ( x > length )
    			length = x;
    }
    return (length);
}	/* End of GetUserItemsMaxLength () */

/************************************************************
	free the user data
**************************************************************/
static void
FreeUsers (UserList *Users)
{
    register	i;
    int		index;

    for (i = Users->cnt; --i>=0; ) {
	XtFree (Users->list[i].name);
	XtFree (Users->list[i].connection);
	for (index = 0; index < 6; index++) 
		XtFree (Users->list[i].logintime[index]);
    }

    Users->cnt = 0;
}	/* End of FreeUsers () */

/******************************************************************
			Flat List Format string creation routine
			Specific to UserList 
*******************************************************************/
static void
MakeFormatStrings(char *headerBuf,char *listBuf, UserList *Users)
{
    int i;
    int userNameLength;
    char buffer[256];

    /*
     *  ID number is a maximum of 5 chars wide, ensure that as
     *  a minimum length.
     */
    if ( (i = strlen(ColHdrs[0].connection)) <=5 )
    {
        i  = 5;
    }
    strcpy(headerBuf,buffer);
    strcat(headerBuf,"%");
    sprintf(buffer,"%d",i);
    strcat(headerBuf,buffer);
    strcat(headerBuf,"s    ");

    /*
     * Up to this point list string format is same as header string format
     * so copy header into list format string
     */
    strcpy(listBuf,headerBuf);

    /*
     *  Username can be up to 48 chars wide.  We get the widest name
     *  and use that as the field width unless the header length is
     *  greater.
     */
    userNameLength = GetUserItemsMaxLength (Users);
    if ( (i = strlen(ColHdrs[0].name)) <=userNameLength )
    {
        i = userNameLength;
    }
    strcat(headerBuf," %");
    strcat(listBuf,"%");

    sprintf(buffer,"%d",i);
    strcat(headerBuf,buffer);
    strcat(listBuf,buffer);
    strcat(headerBuf,"s    ");
    strcat(listBuf,"s    ");

    /*
     * Need at least 20 characters to display login time information
     */
    if (( i = strlen(ColHdrs[0].logintime[0])) < 20 )
    {
        i = 20;
    }
    strcat(headerBuf," %");
    sprintf(buffer,"%d",i);
    strcat(headerBuf,buffer);
    strcat(headerBuf,"s");

    /*
     * Finish list format string for login time information
     */
    strcat(listBuf,"%s %s%s  %s%s%s\n");
}

/*******************************************************
	read the users into the list item
********************************************************/
static int
ReadUsers(Widget parent, UserList *Users, char *server)
{
	int 			index, ccode, i, found = 0, f = 0;
	char 			ObjectName[MAX_OBJECT_NAME_LENGTH];
	users 			list[250];
	char			buffer[100];
	int 			compr();

	NWOBJ_ID 		ObjID;
	NWCONN_HANDLE 		ConnID;
	NWNUMBER 		numberOfConnections;
	NWNUMBER 		maxListElements;
	NWCONN_NUM 		connectionList[50];
	NWOBJ_ID 		clientObjectID;
	NWOBJ_TYPE 		clientObjectType;
	NWFSE_USER_INFO usrInfo;
	unsigned int x;
	char userName[50];

	ccode = getConnHandle(server, &ConnID);
	if (ccode) {
		GUIError(parent,GetStr (TXT_invalidconn));
		return 1;
   	}

   	ObjID = -1;
   	maxListElements = 50;
   	numberOfConnections = 0;

	/* scan the object for the users on the server 
	 * in a loop till there are no more users found
	 */
   	while ((ccode = NWScanObject(ConnID, (NWsp)"*", OT_USER, &ObjID,
			(NWsp)ObjectName, NULL, NULL, NULL, NULL)) == 0) {

		ccode = NWGetObjectConnectionNumbers(ConnID, (NWsp)ObjectName, 
				OT_USER, &numberOfConnections, connectionList, 
				maxListElements);
		if (ccode != 0) 
			continue;

		if (numberOfConnections <= 0) 
			continue;

		if (!found)
			found = 1;

		for (i = 0; i < (int)numberOfConnections; i++, f++) {

			/* store the connection number and the user name 
			 */
			list[f].clientConnID = connectionList[i];
			strcpy (list[f].clientObjectName, ObjectName);

			/* need to get the login time here 
			 */
			ccode = NWGetConnectionInformation( ConnID, 
						connectionList[i], 
						(NWsp) ObjectName, 
						&clientObjectType, 
						&clientObjectID,
		  				list[f].loginTime);
			if ( ccode != 0) {
				GUIError(parent,GetStr (TXT_failedconn));
				return(1);
			}
			if (f > 250) 
				break;
		}
   	}

	if (!found){
		GUIError (parent, GetStr (TXT_userfail));
		return (1);
	}
	else if (f < 1) {
		GUIError (parent, GetStr (TXT_userempty));
		return (1);
    	}
   	qsort ((char *)list, f, sizeof(users), compr);

   	for (i=0; i < f; i++) {

		if (Users->cnt >= Users->allocated) { 
			Users->allocated += 2;
			Users->list = (UserFormat *)
			      	XtRealloc ((char *) Users->list,
	  		      	Users->allocated * sizeof(UserFormat));
		}

		sprintf (buffer,"%5d", list[i].clientConnID);
		Users->list [Users->cnt].connection = (XtPointer)strdup(buffer);

		Users->list [Users->cnt].name = (XtPointer) strdup 
						(list[i].clientObjectName);

		sprintf(buffer,"19%d",list[i].loginTime[0]);
		Users->list[Users->cnt].logintime[0] = 
				(XtPointer) strdup (buffer);

		sprintf(buffer,"%d/",list[i].loginTime[1]);
		Users->list[Users->cnt].logintime[1] = 
				(XtPointer) strdup (buffer);

		sprintf(buffer,"%d",list[i].loginTime[2]);
		Users->list[Users->cnt].logintime[2] = 
				(XtPointer) strdup (buffer);

		/*
	 	 * Min/Max of 2 characaters, 0=pad char
	 	 */
		for (index = 3 ; index < 5; index++) {
			sprintf(buffer,"%02.2d:",list[i].loginTime[index]);
			Users->list[Users->cnt].logintime[index] = 
					(XtPointer) strdup (buffer);
		}
		sprintf(buffer,"%02.2d",list[i].loginTime[5]);
		Users->list[Users->cnt++].logintime[5] = 
				(XtPointer) strdup (buffer);

   	} 			/* for all users */

   	return 0;
}

/******************************************************************
			comparison routine
*******************************************************************/
int	compr (user1, user2)
users *user1, *user2;
{
	if (user1->clientConnID > user2->clientConnID)
		return(1);
	else
		return(-1);
}
/******************************************************************
			get a netware connection 
*******************************************************************/
int
getConnHandle(char *serverName,NWCONN_HANDLE *connID)
{
	NWCConnString     name;
	int	rc;

	name.pString = serverName;
	name.uStringType = NWC_STRING_TYPE_ASCII;
	name.uNameFormatType = NWC_NAME_FORMAT_BIND;

	rc = NWOpenConnByName(NULL, &name,(pnstr)"NCP_SERVER",
	                      NWC_OPEN_PUBLIC | NWC_OPEN_UNLICENSED,
	                      NWC_TRAN_TYPE_WILD, connID);
	return(rc);

}
/*****************************************************************
 * When header is selected unset the header
 *****************************************************************/
static void
HeaderSelectProc (Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlVaFlatSetValues ( headerWidget, 0, XtNset,  (XtArgVal) False, 0); 
}
