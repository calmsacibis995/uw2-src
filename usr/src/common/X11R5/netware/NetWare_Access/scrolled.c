/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwaccess:scrolled.c	1.5"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/NetWare_Access/scrolled.c,v 1.5.2.1 1994/10/27 15:00:14 renuka Exp $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL							*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#ident	"@(#)netware:scrolled.c	1.0"
#endif
*/

#include <stdio.h>
#include <pwd.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>
#include <Xol/FList.h>
#include <Xol/ControlAre.h>
#include <Xol/ScrolledWi.h>
#include <Xol/OlCursors.h>
#include <Xol/RubberTile.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <Dt/Desktop.h>
#include <libDtI/DtI.h>
#include <libDtI/FIconBox.h>

#include "main.h" 
#include "scroll.h" 

/* forward declaration */

extern void		GUIError (Widget, char *);
extern int    	        Deauthenticate (Widget,ServerList *);
extern void    	        Authenticate (Widget,ServerList *);
extern int    	        User_List (Widget,ServerList *);
extern int    	        Volume_List (Widget,ServerList *);
extern char		*GetStr (char *idstr);
extern int 		ReadServers (Widget, ServerList *);

extern 	Widget		TopLevel;

ListItem * 		GetItems(ServerList *);
ListItem *	 	GetSelectedItems(ServerList *, int);
static int 		GetFileServerNameLength (ServerList *servers);
static int 		GetUserNameLength (ServerList *servers);
static void 		MakeFormatStrings(ServerList *servers);

/******************************************************************
	static declarations
******************************************************************/
static void             SetButtonLbls (ButtonItem *, int);
static void             ListSelectCB (Widget, XtPointer, XtPointer);
static void             ListUnSelectCB (Widget, XtPointer, XtPointer);
static void             ButtonSelectCB (Widget, XtPointer, XtPointer);
static void    	        Show_SelectButton (Widget,OlFlatCallData*,ServerList*);
static ListItem *	GetSavedItems(Widget, ServerList *, int *);
static char *		GetDirectory();

/*******************************************************************
	buttons for the login, logout, password, volume and user list
***********************************************************************/
static ButtonItem NetWareItems [] = {
    { (XtArgVal) TXT_mask, (XtArgVal) 0, (XtArgVal) MNEM_mask, 
	(XtArgVal) True, }, 					/* mask */
    { (XtArgVal) TXT_authenticate,(XtArgVal) 0, (XtArgVal) MNEM_login, },
								/*auth */
    { (XtArgVal) TXT_deauthenticate, (XtArgVal) 0, (XtArgVal) MNEM_deauth, },
								/* deau */
    { (XtArgVal) TXT_userlist, (XtArgVal) 0, (XtArgVal) MNEM_users, },/*users */
    { (XtArgVal) TXT_change, (XtArgVal) 0, (XtArgVal) MNEM_pwd, },/* change */
    { (XtArgVal) TXT_volume, (XtArgVal) 0, (XtArgVal) MNEM_vol, },/* vol.list */
};

/***************************************
	structure for the servers
***************************************/
ServerList  	Servers;

/**************************list fields *********************/
static String		ListFields [] = {
    XtNformatData, 
};

static HeaderFormatData       	ColHdrs [1];
static ListItem         	ColItem [] = {
    (XtArgVal) ColHdrs,
};
/**************************button fields *********************/
String ButtonFields [] = {
	XtNlabel, XtNset, XtNmnemonic, XtNdefault,
};
Arg                  arg[50];
int NumButtonFields = XtNumber (ButtonFields);

/*********************************************************************
		global variables
**********************************************************************/
char 	    *longformat = "%20s %s%s%s%s         %s%s%s%s%s%s     %8s\n";
char 	    *shortformat ="%20s                           %8s\n";
char 	    *headerformat = "%20s%15s%12s%13s\n";
static int  database_found = 0;
static char *FILENAME	= "/.NETWARE.DB";

char 		headerBuf[256];
char 		shortheaderBuf[256];
char 		listLongBuf[256];
char 		listShortBuf[256];
Widget		PARENT, scrolledWindow;

/*******************************************************
* Make scrolled list and fill with netware servers *
*******************************************************/
int
BuildScrolledList (Widget parent, Widget menubar)
{
	Widget		buttonPane;
	Widget 		maskButton;
    	int             numcount;
    	ListItem	*listItems;

	PARENT = parent;
  	ColHdrs [0].FileServers = (XtPointer) GetStr (TXT_FileServers);
  	ColHdrs [0].NodeAddress = (XtPointer) SPACES;
  	ColHdrs [0].NwAddress = (XtPointer) SPACES; 
        ColHdrs [0].userName = (XtPointer) GetStr (TXT_userName);


    	/***********read the list of servers **************/
	Servers.cnt = Servers.allocated = 0;
    	if (!ReadServers (parent, &Servers)) 
		return 1;
    	else {

    		/*
     		 * Create the list format strings 
     		 */
		MakeFormatStrings( &Servers);

    	Servers.headerWidget = XtVaCreateManagedWidget ("caption", 
		flatListWidgetClass, 	parent,
 		XtNgravity,          	(XtArgVal) NorthWestGravity,
 		XtNviewHeight,          (XtArgVal) 1,
                XtNexclusives,          (XtArgVal) True,
                XtNnoneSet,             (XtArgVal) True,
                XtNrecomputeHeight,     (XtArgVal) False,
                XtNrecomputeWidth,     	(XtArgVal) False,
                XtNitems,               (XtArgVal) ColItem,
                XtNnumItems,            (XtArgVal) 1,
                XtNitemFields,          (XtArgVal) ListFields,
                XtNnumItemFields,       (XtArgVal) XtNumber (ListFields),
                XtNformat,              (XtArgVal) shortheaderBuf,
                XtNtraversalOn,         (XtArgVal) False,
    		XtNweight, 		(XtArgVal) 0,
		0);

    	/*  create the scrolled window */
    	scrolledWindow = XtVaCreateManagedWidget ("scrolledWindow",
			scrolledWindowWidgetClass, parent,
			XtNgranularity,   	1,
			0);
		Servers.viewformat_flag = SHORTVIEW;
		database_found = 0;
    		Servers.set_toggle = False;
    		Servers.pgone_count = 0;
    		Servers.pgtwo_count = 0;

		/* if the read servers worked set up the flag and get items
 		 * to be displayed in the scrolled list 
 		 */
		listItems = GetSavedItems (parent, &Servers, &numcount);

		/* create the scrolled list in a scrolled window */
    		Servers.serverWidget = XtVaCreateManagedWidget ("flatlist",
			flatListWidgetClass, scrolledWindow,
			XtNexclusives,		(XtArgVal) False,
			XtNnoneSet,		(XtArgVal) True,
			XtNselectProc,		(XtArgVal) ListSelectCB,
			XtNclientData,		(XtArgVal) &Servers,
			XtNunselectProc,	(XtArgVal) ListUnSelectCB,
			XtNclientData,		(XtArgVal) &Servers,
			XtNitems,		(XtArgVal) listItems,
			XtNnumItems,		(XtArgVal) numcount, 
			XtNitemFields,		(XtArgVal) ListFields,
			XtNnumItemFields,	(XtArgVal) XtNumber(ListFields),
			XtNviewHeight,		(XtArgVal) 10,
			XtNweight, 		(XtArgVal) 1,
	   		XtNformat,		(XtArgVal) 
			(Servers.viewformat_flag == 0 ? longformat:shortformat),
			0);

		/* create the button area */
    		buttonPane = XtVaCreateManagedWidget ("buttonpane",
			panesWidgetClass, parent,
    			XtNweight, 		(XtArgVal) 0,
    			XtNrefPosition, 	(XtArgVal) OL_BOTTOM,
    			XtNshadowThickness, 	(XtArgVal) 0,
			0);

		/* create the buttons in the control area */
    		SetButtonLbls (NetWareItems, XtNumber (NetWareItems));
    		maskButton = XtVaCreateManagedWidget ("maskbutton",
			flatButtonsWidgetClass, parent,
			XtNweight,		(XtArgVal) 0,
			XtNtraversalOn,		(XtArgVal) True,
			XtNselectProc,		(XtArgVal) ButtonSelectCB,
			XtNgravity,		(XtArgVal) CenterGravity,
			XtNclientData,		(XtArgVal) &Servers,
			XtNexclusives,		(XtArgVal) True,
			XtNbuttonType,		(XtArgVal) OL_OBLONG_BTN,
			XtNitemFields,		(XtArgVal) ButtonFields,
			XtNnumItemFields,	(XtArgVal) NumButtonFields,
			XtNitems,		(XtArgVal) NetWareItems,
			XtNnumItems,		(XtArgVal) 
						XtNumber (NetWareItems), 0);
		return 0;
    	}	/* if the read servers worked */
}	 /* End of Buildscrolledlist () */

/*************************************************
		ListSelectCB * * Select system
**************************************************/
static void
ListSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    	ServerList          *servers = (ServerList *) client_data;
    	OlFlatCallData	*pFlatData = (OlFlatCallData *) call_data;

    	/* if it is first pg increase the count and set the item index
   	 * of the selected item to 1 indicating selection
	 */	 
    	if (servers->set_toggle == False) {
		servers->pgone_count++;
    		servers->item_index_count[pFlatData->item_index] = 1; 
    	}
	/* if it is second page increase the count of pg two and set the
	 * the selected item index to 1 indication selectionin pg 2
	 */
    	else {
		servers->pgtwo_count++;
    		servers->selected_item_index[pFlatData->item_index] = 1; 
    	}
    
}	/* End of ListSelectCB () */


/*************************************************
		ListUnSelect * * UnSelect item 
**************************************************/
static void
ListUnSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    	ServerList          *servers = (ServerList *) client_data;
    	OlFlatCallData	*pFlatData = (OlFlatCallData *) call_data;
	
    	/* if it is first pg decrease the count and set the item index
   	 * of the selected item to 0 indicating unselection
	 */	 
    	if (servers->set_toggle == False) {
		servers->pgone_count--;
    		servers->item_index_count[pFlatData->item_index] = 0; 
    	}
	/* if it is second page decrease the count of pg two and set the
	 * the selected item index to 0 indication unselection in pg 2
	 */
    	else {
		servers->pgtwo_count--;
    		servers->selected_item_index[pFlatData->item_index] = 0; 
    	}	
}	/* End of unselectcb () */


/*************************************************
		ButtonSelect * * Select Button 
**************************************************/
static void
ButtonSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    	ServerList          *servers = (ServerList *) client_data;
    	OlFlatCallData	*pFlatData = (OlFlatCallData *) call_data;
	
   	switch (pFlatData->item_index) {
		case 0:	
			Show_SelectButton(widget, pFlatData, servers);
			break;
		case 1: 
			Authenticate (widget, servers);
			break;
			
		case 2: 
        		/* set the clock */
        		XDefineCursor(XtDisplay (TopLevel), XtWindow (TopLevel),
                              GetOlBusyCursor (XtScreen (TopLevel)));
        		XSync (XtDisplay (TopLevel), 0);
			Deauthenticate (widget, servers);
    			/* reset the clock */
        		XDefineCursor(XtDisplay (TopLevel), XtWindow (TopLevel),
                        	GetOlStandardCursor (XtScreen (TopLevel)));
        		XSync (XtDisplay (TopLevel), 0);
			break;
		case 3:
			User_List (widget, servers);
			break;
		case 4:
			Change_pwd (widget, servers);
			break;
		case 5:
			Volume_List (widget, servers);
			break;
   	}
		
}	/* End of buttonselect () */
		
/*************************************************
		ShowSELECT * * show Selected itm 
**************************************************/
static void
Show_SelectButton ( Widget widget, OlFlatCallData *pFlatData,
ServerList *servers)
{
    	int 		i, j = 0;
    	static int          first = 0;
    	ListItem *          SelectItems;
	XtWidgetGeometry	size;

    	if (database_found) {
		first = 1;
		database_found = 0;
    	}
    	/*    if the view is long or short format then set the headers 
	 *    accordingly
         */	
    	if (servers->viewformat_flag == LONGVIEW)  {
  		ColHdrs [0].NwAddress = (XtPointer) GetStr (TXT_NwAddress);
  		ColHdrs [0].NodeAddress = (XtPointer)GetStr (TXT_NodeAddress); 
		XtVaSetValues (	servers->headerWidget, 
                XtNitems,               (XtArgVal) ColItem,
                XtNnumItems,            (XtArgVal) 1,
                XtNitemsTouched,        (XtArgVal) True,
                XtNformat,        	(XtArgVal) headerBuf,
		0);
	}
    	else if  (servers->viewformat_flag == SHORTVIEW)  {
  		ColHdrs [0].NwAddress = (XtPointer) SPACES;
  		ColHdrs [0].NodeAddress = (XtPointer) SPACES; 
		XtVaSetValues (	servers->headerWidget, 
                XtNitems,               (XtArgVal) ColItem,
                XtNnumItems,            (XtArgVal) 1,
                XtNitemsTouched,        (XtArgVal) True,
                XtNformat,        	(XtArgVal) shortheaderBuf,
		0);
	}

    	/* switch the toggle flag first to see which is being requested
     	 *  - show select or show all 
	 */
     	switch (first) {
     		case 0:	

        	/* if it is show select -  show all servers selected */
   		servers->pgtwo_count = 0;

		/******************************************************
	    	first count the no. of servers selected and set the toggle
		**********************************************************/
     		for (i = 0; i < servers->cnt; i++) {
			servers->selected_item_index[i] = 0;
   	    		if (servers->item_index_count[i] == 1)  { 
				first = 1;
				servers->selected_value_index[j++] = i; 
       			}
		}

		/*********************************************************
	 	if nothing was selected then error message
         	********************************************************/
     		if (!first)
			GUIError(widget, GetStr (TXT_cantshowselect)); 
     		else {
			/******************************************************
			set the toggle button to true and show only selected
			******************************************************/
			servers->set_toggle = True;
    			OlVaFlatSetValues (widget, pFlatData->item_index,
			   		XtNlabel,(XtArgVal) GetStr (TXT_unmask),
			   		0);

    			SelectItems = GetSelectedItems (servers, 
							servers->pgone_count);
    			XtVaSetValues ( servers->serverWidget, 
				XtNformat, 	(XtArgVal) 
						(servers->viewformat_flag == 0 ?
						longformat : shortformat ),
				XtNitems,	(XtArgVal) SelectItems,
				XtNnumItems,	(XtArgVal) servers->pgone_count, 				XtNitemsTouched,(XtArgVal) True,
				0);
	
			/******* show all the marked guys as selected********/
     			for (i = 0; i < servers->cnt; i++)  {
				if (servers->selected_item_index[i] == 1) {
       					OlVaFlatSetValues (
					servers->serverWidget,  i, 
					XtNset, 	(XtArgVal) True, 
					0);
				}	/* if item selected in pg 2 */
			} /* for all servers */
     		}/* if second page has selected items */

     		break;

     		case 1:

     		/* if it is show all  set the first flag back to 0 */
		first = 0;
	
		/* set the toggle switch to first page */
		servers->set_toggle = False;
	
		/* get the new set of items */
		SelectItems =  GetItems (servers);
	
		/* set the label of the button to the all or select */
       		OlVaFlatSetValues (widget, pFlatData->item_index,
		   	 XtNlabel, 	(XtArgVal) GetStr (TXT_mask), 
		   	 0);
		
		/*   set the list items in the scrolled list  */
    		XtVaSetValues ( servers->serverWidget, 
				XtNformat, 	(XtArgVal) 
				(servers->viewformat_flag == 0 ? 
				longformat : shortformat),
	       			XtNitems,	(XtArgVal) SelectItems,
	       			XtNnumItems,	(XtArgVal) servers->cnt,
               			XtNitemsTouched,        (XtArgVal) True,
				0);
			
		/******** show all the marked guys as selected *************/
     		for (i = 0; i < servers->cnt; i++)  {
			if (servers->item_index_count[i] == 1) {
       				OlVaFlatSetValues (servers->serverWidget, i, 
				   		XtNset, (XtArgVal) True, 
				   		0);
			}	/* if it is selected in pg 1*/
		}		/* for all servers */
		break;
    	}	/* switch statement */

	/* Reset the width of the toplevel window to the width of 
	 * the newly displayed scrolled window
	 */
	size.request_mode = CWWidth;
	XtQueryGeometry (scrolledWindow, NULL, &size);
	XtVaSetValues (PARENT,	XtNwidth, size.width, 0);
	XSync (XtDisplay (TopLevel), FALSE);
}

/*****************************************************************
		 GetSelectedItems
 * Format the host list into items for the flat list.
 *****************************************************************/
ListItem *
GetSelectedItems (ServerList *servers, int count)
{
    	ListItem		*items;
    	register ListItem	*pItem;
    	register FormatData	*pData;
    	register ShortFormatData     *sData; 
    	register		i;
	
	/* malloc space for the list items to be returned for display at the  
	 * scrolled list
	 */
    	items = pItem = (ListItem *) XtMalloc (count * sizeof (ListItem));
    	switch (servers->viewformat_flag) {
	/* if it is short format */
		case 1:	

		/* set the short format ptr to the short list */
    		sData = servers->shortlist;

		/* for all servers store the short list items in the
		 * malloced list item
		 */
    		for (i = 0; i < servers->cnt; i++, sData++) {
			if (servers->item_index_count[i] == 1) {
				pItem->formatData = (XtArgVal) sData;
				pItem++;
			}
		}
		break;

		case 0:

		/* set the long format ptr to the long list */
   		pData = servers->list;
		/* for all servers store the long list items in the
		 * malloced list item
		 */
    		for (i = 0; i < servers->cnt; i++, pData++) {
			if (servers->item_index_count[i] == 1) {
				pItem->formatData = (XtArgVal) pData;
				pItem++;
			}
		}
		break;
    	}		/* switch statement */
	
    	return (items);
}	/* End of GetSelectedItems () */

/*****************************************************************
		 GetItems
 * Format the host list into items for the flat list.
 *****************************************************************/
ListItem *
GetItems (ServerList *servers)
{
    	ListItem		*items;
    	register ListItem	*pItem;
    	register FormatData	*pData;
    	register ShortFormatData *sData; 
    	register		i;
	
    	items = pItem = (ListItem *) XtMalloc(servers->cnt * sizeof (ListItem));

    	/* if the format is short or long then use the appropriate structures
     	 * for listing the items in the scrolled list
     	 */
    	switch (servers->viewformat_flag) {
		case 1:	
    			sData = servers->shortlist;
    			for (i = servers->cnt; --i >= 0; pItem++, sData++) {
				pItem->formatData = (XtArgVal) sData;
			}
			break;
		case 0:
    			pData = servers->list;
    			for (i = servers->cnt; --i >= 0; pItem++, pData++) {
				pItem->formatData = (XtArgVal) pData;
			} 
			break;
    	} 			/* switch statement for long or short */
	
    	return (items);
}	/* End of GetItems () */
/*****************************************************************
		 GetUserNameLength
 * Get the maximum length of the user names.
 *****************************************************************/
static int 
GetUserNameLength (ServerList *servers)
{
    	register FormatData	*pData;
    	register ShortFormatData *sData; 
    	register		i,x;
    	register		length = 0;

    	switch (servers->viewformat_flag) {
		case 1:	
    			sData = servers->shortlist;
    			for (i = servers->cnt; --i >= 0; sData++) {
					x = strlen(pData->userName);
					if ( x > length)
						length = x;
			}
			break;
		case 0:
    			pData = servers->list;
    			for (i = servers->cnt; --i >= 0;  pData++) {
					x = strlen(pData->userName);
					if ( x > length)
						length = x;
			} 
			break;
    	} 			/* switch statement for long or short */
    	return (length);
}	/* End of GetItems () */
/*****************************************************************
		 GetFileServerNameLength
 * Get the maximum length of the server names.
 *****************************************************************/
static int 
GetFileServerNameLength (ServerList *servers)
{
    	register FormatData	*pData;
    	register ShortFormatData *sData; 
    	register		i,x;
    	register		length = 0;

    	switch (servers->viewformat_flag) {
		case 1:	
    			sData = servers->shortlist;
    			for (i = servers->cnt; --i >= 0; sData++) {
					x = strlen(pData->FileServers);
					if ( x > length)
						length = x;
			}
			break;
		case 0:
    			pData = servers->list;
    			for (i = servers->cnt; --i >= 0;  pData++) {
					x = strlen(pData->FileServers);
					if ( x > length)
						length = x;
			} 
			break;
    	} 			/* switch statement for long or short */
    	return (length);
}	/* End of GetItems () */

/*****************************************************************
		 SetButtonLbls
 * Set button item labels.
 *****************************************************************/
void
SetButtonLbls (ButtonItem *items, int cnt)
{
	char		*mnem;

	/* if the database file for user is found the set the select
	 * button to show all else set it to show select
	 */
    	if (database_found) {
		items->lbl = (XtArgVal) GetStr (TXT_unmask);
	}
    	else {
		items->lbl = (XtArgVal) GetStr (TXT_mask);
	}
	mnem = GetStr (MNEM_mask);
	items->mnem = (XtArgVal) mnem[0];
    	cnt--; items++;
	/* label the remaining items */
    	for ( ; --cnt>=0; items++) {
		items->lbl = (XtArgVal) GetStr ((char *) items->lbl);
		mnem = GetStr ((char *)items->mnem);
		items->mnem = (XtArgVal) mnem[0];
	}
}	/* End of SetButtonLbls */

/*****************************************************************
 * ViewCB
 * Display short or long.  
 *********************************************************************/
void
ViewCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    	OlFlatCallData	*flatData = (OlFlatCallData *) call_data;
	ListItem		*items;
	int 			i;
	XtWidgetGeometry	size;

	/* if long format is chosen then set the viewformat flag to LONGVIEW
         * else set it to SHORTVIEW  
	 */
	Servers.viewformat_flag = (flatData->item_index == 1) ? SHORTVIEW : LONGVIEW;

	/* recreate the header caption depending on long or short format */
	if (Servers.viewformat_flag == LONGVIEW)  {
  		ColHdrs [0].NwAddress = (XtPointer) GetStr (TXT_NwAddress);
  		ColHdrs [0].NodeAddress = (XtPointer)GetStr (TXT_NodeAddress); 
		XtVaSetValues (	Servers.headerWidget, 
                XtNitems,               (XtArgVal) ColItem,
                XtNnumItems,            (XtArgVal) 1,
                XtNitemsTouched,        (XtArgVal) True,
                XtNformat,        	(XtArgVal) headerBuf,
		0);
	}
	else  {
  		ColHdrs [0].NwAddress = (XtPointer) SPACES;
  		ColHdrs [0].NodeAddress = (XtPointer) SPACES; 
		XtVaSetValues (	Servers.headerWidget, 
                XtNitems,               (XtArgVal) ColItem,
                XtNnumItems,            (XtArgVal) 1,
                XtNitemsTouched,        (XtArgVal) True,
                XtNformat,        	(XtArgVal) shortheaderBuf,
		0);
	}

	/***************************************************************
	 set up the list items for long or short format depending on 
	 which page is being displayed - select or all
	 ***************************************************************/
    	switch (Servers.set_toggle) {
		/**************SELECT ****************/
		case True:

		/* if it is page #2 then show only selected
		 *  servers.  Set the list items of the scrolled window
		 *  to long or short format depending on what is 
		 *  required. 
		 */
		items = GetSelectedItems (&Servers, Servers.pgone_count);
		XtVaSetValues (Servers.serverWidget, 
			XtNformat, 	(XtArgVal) 
					(Servers.viewformat_flag == 0 ?
					longformat : shortformat ),
			XtNitems, 	(XtArgVal) items,  
			XtNnumItems, 	(XtArgVal) Servers.pgone_count,
              			XtNitemsTouched,(XtArgVal) True,
			0);	
		/*  for all servers if server is selected set value to 
		 *  true
		 */
		for (i = 0; i < Servers.cnt; i++) {
			if (Servers.selected_item_index[i] == 1)
				OlVaFlatSetValues (Servers.serverWidget, i, 
					XtNset, 	(XtArgVal) True, 0);
		}
		break;

		/**************ALL ****************/
		case False:

		/* if it is show all page i.e #1 then show all the
		 * servers in long or short format as the case may
		 * be and set items in scrolled window
		 */
		items = GetItems (&Servers);
		XtVaSetValues (Servers.serverWidget, 
			XtNformat, 	(XtArgVal) 
					(Servers.viewformat_flag == 0 ?
					longformat : shortformat ),
			XtNitems, 	(XtArgVal) items,  
			XtNnumItems, 	(XtArgVal) Servers.cnt,
       			XtNitemsTouched,(XtArgVal) True,
			0);
		/*  for all servers if server is selected set value to 
		 *  true
		 */
		for (i = 0; i < Servers.cnt; i++) {
			if (Servers.item_index_count[i] == 1)
				OlVaFlatSetValues (Servers.serverWidget, i, 
						XtNset,	 (XtArgVal) True, 0);
		}
		break;

    	}		/*	switch if selected or all */	

	/* Reset the width of the toplevel window to the width of 
	 * the newly displayed scrolled window
	 */
	size.request_mode = CWWidth;
	XtQueryGeometry (scrolledWindow, NULL, &size);
	XtVaSetValues (PARENT,	XtNwidth, size.width, 0);
	XSync (XtDisplay (TopLevel), FALSE);
}


/*****************************************************************
 * SaveSelectCB
 * save the selected servers in a file 
 *********************************************************************/
void
SaveSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    	OlFlatCallData	*flatData = (OlFlatCallData *) call_data;
	int 		i, found = 0;
	FILE		*fp;
	char 		*dbname;

	/* set the clock */
        XDefineCursor(XtDisplay (TopLevel), XtWindow (TopLevel),
                      GetOlBusyCursor (XtScreen (TopLevel)));
        XSync (XtDisplay (TopLevel), 0);

	/* get the name of the database file */
	dbname = GetDirectory ();

	/* open the database file in write mode */
	fp = fopen (dbname, "w");

	/* for all selected servers store the servername  in
	 * the database file 
	 */
	for (i = 0; i < Servers.cnt; i++) {
		if (Servers.item_index_count[i] == 1) {
			found = 1;
			fprintf (fp, "%s\n", Servers.list[i].FileServers);
		}
	}
	/* close the file */	
	fclose (fp);

	XtFree (dbname);

	/* reset the clock */
        XDefineCursor(XtDisplay (TopLevel), XtWindow (TopLevel),
                      	GetOlStandardCursor (XtScreen (TopLevel)));
        XSync (XtDisplay (TopLevel), 0);

	/* if there was no server found then return and error and free
	 * the file name 
	 */
	if (!found)
		GUIError (widget, GetStr (TXT_noservers));
}
 

/*****************************************************************
		 GetSavedItems
 * Format the server list into items for the flat list.
 *****************************************************************/
static ListItem *
GetSavedItems (Widget widget, ServerList *servers, int *count)
{
	char 			 *ptr, line[BUFSIZ], *fileservers[2048];
    	ListItem		 *items;
    	register ListItem	 *pItem;
    	register ShortFormatData *sData; 
    	int			  total, tally, i, j, index;
	FILE 			 *fp;
	char			 *database_file;

	i = j = index = 0;

	/* get the name of the database file */
	database_file = GetDirectory ();
	
	/* open the database file 
	 * read the database file into a malloced buffer 
	 * close the file 
	 */
	tally = 0;
	if (fp = fopen(database_file,"r"))  { 
		while (fgets (line, BUFSIZ, fp)) {
			fileservers[tally] = strdup (line);
			tally++;
		}
		fclose (fp);
	}
	else {
		*count = Servers.cnt;
    		items = GetItems (&Servers);
		return items;
	}

	/* get a count of the servers from the database file
	 * that are there in the new list
	 */
	total = 0;
	for (i = 0; i < servers->cnt; i++) {
		for (j = 0; j < tally; j++) {
			ptr = strtok (fileservers[j], "\n");
			if (strcmp(ptr, servers->shortlist[i].FileServers) == 0)
				total++;
		}
	}

	/* malloc the items to be displayed */
	if (total != 0)
    		items = pItem = (ListItem *)XtMalloc(total * sizeof(ListItem));

	*count = total;

	/* set the ptr to the beg of the shortlist - SINCE 
	 * default is SHORTVIEW format
	 */
	sData = servers->shortlist;

	/* for all servers in our structure */
    	for (i = 0; i < servers->cnt; i++, sData++) {

		/* for all servers in the database file */ 
		for (j = 0; j < tally; j++) { 

			/* compare each servers till same */
			ptr = strtok (fileservers[j], "\n");
			if (strcmp(ptr,servers->shortlist[i].FileServers) == 0){

				/* set selection index to 1 for pg1 */
				servers->item_index_count[i] = 1;

				/* increment the page count */
				servers->pgone_count++;

				/* set the selected value index*/
				servers->selected_value_index[index++] = i;

				/* set the global flag to one */
				if (!database_found)
					database_found = 1;

				/* store the formatted item */
				pItem->formatData = (XtArgVal) sData;
				pItem++;

			}	/* if the fileservers are same */

		}	/* for all servers in stored file */

	}		/* for all servers in our structure */

	/* if the file was not found */
	if (!database_found) {
		*count = Servers.cnt;
    		items = GetItems (&Servers);
	}
	else
		/* set the toggle to true indicating
		 * second page of the scrolled list
		 */ 
		servers->set_toggle = TRUE;

	return items;
}

/*****************************************************************
		 get directory
 *****************************************************************/
static char *
GetDirectory ()
{
	char 		*dbname;
	struct passwd	*pwd;
	uid_t		uid;

	/* get the home directory of the current user */
	setpwent ();
	uid = geteuid ();	
	while (pwd = getpwent()) {
		if (uid	 == pwd->pw_uid)  {
		/* create a database file in his home directory */
			dbname = (char *)XtMalloc (strlen (pwd->pw_dir) + 
				 strlen (FILENAME) + 2);		 
			strcpy (dbname, pwd->pw_dir);
			strcat (dbname, FILENAME);
			break;
		}
     	} 
	endpwent ();
	return dbname;
}
/******************************************************************
			Flat List Format string creation routine
			Specific to the server information panel 
*******************************************************************/
static void
MakeFormatStrings( ServerList *servers)
{
    int i;
    char buffer[256];
    int nameLength;

    nameLength = GetFileServerNameLength(servers);
    /*
     *  File Server name can be up to 48 chars wide.  
     *  Use "nameLength" as the field width unless the header 
	 *  length is greater.
     */
    if ( (i = strlen(ColHdrs[0].FileServers)) <=nameLength )
    {
        i = nameLength;
    }
    strcpy(headerBuf,"%");
    strcpy(shortheaderBuf,"%");

    sprintf(buffer,"%d",i);
    strcat(headerBuf,buffer);
    strcat(shortheaderBuf,buffer);

    strcat(headerBuf,"s    ");
    strcat(shortheaderBuf,"s    ");

    strcpy(listShortBuf,shortheaderBuf);
    strcpy(listLongBuf,headerBuf);

    /*
     *  NetWare Addess is 8 characters wide, ensure that
     *  as a minimum
     */
    if ( (i = strlen(GetStr (TXT_NwAddress))) <=8 )
    {
    i  = 8;
    }
    strcat(headerBuf,"%");
    sprintf(buffer,"%d",i);
    strcat(headerBuf,buffer);
    strcat(headerBuf,"s    ");
    strcat(shortheaderBuf,"%s");
    strcat(listLongBuf, "%2s%2s%2s%2s    ");
    strcat(listShortBuf," ");
    for(i -= 8;i>0;i--)
    {
	    strcat(listLongBuf," ");
    }

    /*
     *  NetWare Node Addess is 12 characters wide, ensure that
     *  as a minimum
     */
    if ( (i = strlen(GetStr (TXT_NodeAddress))) <=12 )
    {
	    i  = 12;
    }
    strcat(headerBuf,"%");
    sprintf(buffer,"%d",i);
    strcat(headerBuf,buffer);
    strcat(headerBuf,"s    ");
    strcat(listLongBuf, "%2s%2s%2s%2s%2s%2s    ");
    strcat(shortheaderBuf,"%s");
    strcat(listShortBuf," ");
    for(i -= 12;i>0;i--)
    {
        strcat(listLongBuf," ");
    }
    /*
     * User name can be up to 48 chars, can't use that.
     * ( Window won't fit on 640x480 display. )
     * Get the maximum length and use it.
     */
    nameLength = GetUserNameLength(servers);
    if ( (i = strlen(ColHdrs[0].userName)) <=nameLength )
    {
        i  = nameLength;
    }
    strcat(headerBuf,"%");
    strcat(shortheaderBuf,"%");
    strcat(listShortBuf,"%");
    strcat(listLongBuf,"%");

    sprintf(buffer,"%d",i);
    strcat(headerBuf,buffer);
    strcat(shortheaderBuf,buffer);
    strcat(listShortBuf,buffer);
    strcat(listLongBuf,buffer);

    strcat(headerBuf,"s");
    strcat(shortheaderBuf,"s");
    strcat(listShortBuf,"s");
    strcat(listLongBuf,"s");

    /*
     * Redirect from the static format string, to 
     * "THE NEW DYNAMIC ONES"
     */
    longformat = listLongBuf;
    shortformat =listShortBuf;
}
