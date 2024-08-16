/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwaccess:update.c	1.2"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/NetWare_Access/update.c,v 1.2 1994/08/18 15:50:22 rv Exp $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL							*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#endif
*/

#include <stdio.h>
#include <pwd.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/FButtons.h>
#include <Xol/FList.h>
#include <Xol/OlCursors.h>
#include <Xol/RubberTile.h>
#include <Xol/StaticText.h>
#include <Dt/Desktop.h>
#include <libDtI/DtI.h>
#include <libDtI/FIconBox.h>

#include "scroll.h" 

/* forward declaration */

extern ListItem * 	GetItems(ServerList *);
extern ListItem * 	GetSelectedItems(ServerList *, int);
extern ServerList	Servers;
extern char		*longformat, *shortformat;
extern Widget		TopLevel;

static void		Show_clock ();
static void		Remove_clock ();

/******************************************************************
	Update call back - reset all flags and get the new list
******************************************************************/
void
UpdateCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	int 			i = 0, index = 0, j = 0, second_i;
	int			pgone_index = 0, pgtwo_index = 0, searchi;
	char			*serverlist[MAXSERVER], *selectlist[MAXSERVER];
	ServerList		*servers;
	ListItem		*listItems;

	/**********SET THE CLOCK**************/
  	Show_clock ();

	/* set pointer to the whole Server List */
	servers = &Servers;

	/* for all servers in old list, if the 
	 * server was selected store servername 
	 * for future comparisons 
	 */
	for (i = 0; i < servers->cnt; i++) 
		if (servers->item_index_count[i] == 1) 
			serverlist[index++] = 
				strdup(servers->list[i].FileServers);
	/* store the count of all old servers 
	 * that have been selected 
	 */
	pgone_index = index;

	/* if it was second page then store all 
	 * selected servers here 
	 */
	index = 0;
	if (servers->set_toggle == True) {
		for (i = 0; i < servers->cnt; i++) {
			if (servers->selected_item_index[i] == 1) {
				searchi = servers->selected_value_index[i];
				selectlist[index++] = 
				strdup(servers->list[searchi].FileServers);
			}
		}
	}
	pgtwo_index = index;

#if 0
	Servers.cnt = Servers.allocated = 0;

	/* for all the servers in the previous list 
	 * free all used up malloced space
	 */
	for (i = 0; i < Servers.cnt; i++) {

		/* free the file server name in the long and short list */
		XtFree  (Servers.list[i].FileServers);
		XtFree  (Servers.shortlist[i].FileServers);

		/* free the network addresses */
		for (index = 0 ; index < 4; index++) 
			XtFree  (Servers.list[i].NwAddress[index]);

		/* free the node addresses */
		for (index = 0 ; index < 6; index++) 
			XtFree (Servers.list [i].NodeAddress[index]);

		/* free the user names */
		XtFree (Servers.list [i].userName);
		XtFree (Servers.shortlist [i].userName);
	}

	/* free the server list  - both long and short */
	XtFree ((char *)Servers.list);
	XtFree ((char *)Servers.shortlist);
	Servers.list = NULL;
	Servers.shortlist = NULL;
#endif

	/* Read the servers again using the nw api calls 
	 * and reset the scrolled list in the main window
	 */
	if (!ReadServers (widget, &Servers)) {
  		Remove_clock ();
		return; 
	}

	/* reset the indices to 0 */
	for ( i = 0; i < MAXSERVER; i++) {
		Servers.selected_item_index[i] = 0;
		Servers.selected_value_index[i] = 0;
		Servers.item_index_count[i] = 0;
	}
		
	/* set the page count one and page count two to 0 */
	Servers.pgone_count = Servers.pgtwo_count = 0;

	/* if the previously selected servers are in 
	 * the new list then set them to select again
	 *  - go thru all the servers in the new list 
	 */
	for (i = 0; i < servers->cnt; i++) {
		for (index = 0; index < pgone_index; index++) { 
			/* if server from the old list  exists  
		 	 * in the new list 
			 */
			if (strcmp (servers->list[i].FileServers,
					serverlist[index]) == 0) {
				/* set the selection flags here */
				servers->item_index_count[i] = 1;
				servers->selected_value_index[j++] = i;
				servers->pgone_count++;
				break;
			} 
		}/* for all servers in old list */
	} /* for all servers in the new list */

	/* However, if it is the Second Page 
	 * then you need to see if the selected servers 
	 * from the old list, exists in the new list
	 */
	if (servers->set_toggle == True) {
		second_i = 0;
		for (i = 0; i < servers->cnt; i++) {

			if (servers->item_index_count[i] == 1) {

				for (index = 0; index < pgtwo_index; index++) { 
					/* if the server in new list matches
					 * selected server from second page
					 * in the old list
					 */
					if (strcmp(servers->list[i].FileServers,
						selectlist[index]) == 0) {
						servers->selected_item_index
						[second_i] = 1;
						servers->pgtwo_count++;
						break;
					} 
				} /* for all old selected servers */

				second_i++;
			} /* if the server was selected in the new list */

		} /* for all servers in second page */
	} /* SECOND PAGE */

	/* get items in scrolled list format -  FIRST PAGE */
	if (servers->set_toggle == False) 
		listItems = GetItems (servers);
	else
		/* SECOND PAGE */
		listItems = GetSelectedItems (servers, servers->pgone_count);

	/* update the scrolled list in a scrolled window */
	XtVaSetValues (servers->serverWidget, 
		XtNitemsTouched,	(XtArgVal) True,
		XtNitems,		(XtArgVal) listItems,
		XtNnumItems,		(XtArgVal) servers->set_toggle == False
					 ? servers->cnt : servers->pgone_count,
	   	XtNformat,		(XtArgVal) 
		servers->viewformat_flag == 0 ? longformat : shortformat,
		0);

	/* set selected items to true */
	for (i = 0; i < servers->cnt; i++) {
		/* if it is the FIRST PAGE, then 
	    	 * flags are already set, use that
		 */
		if (servers->set_toggle == False) { 
			if (servers->item_index_count[i] == 1)
				OlVaFlatSetValues (servers->serverWidget, i,
					XtNset, 	(XtArgVal) True,
					0);
		}
		/* SECOND PAGE */
		else {
			if (servers->selected_item_index[i] == 1)
				OlVaFlatSetValues (servers->serverWidget, i,
					XtNset, 	(XtArgVal) True,
					0);
		}
	}
	
	/* free up storage used to preserve the 
	 * selected servers from the old lists
	 * in the first page and the second page
	 */ 
	for (i = 0; i < pgone_index; i++) 
		if (serverlist[i])
			free (serverlist[i]);
	for (i = 0; i < pgtwo_index; i++) 
		if (selectlist[i])
			free (selectlist[i]);

	/**********REMOVE THE WATCH *********************/
  	Remove_clock ();
}

/*******************************************************
	create the clock cursor
*******************************************************/
static void
Show_clock ()
{
	XDefineCursor (XtDisplay (TopLevel), XtWindow (TopLevel),
		 	GetOlBusyCursor (XtScreen (TopLevel)));
  	XSync (XtDisplay (TopLevel), 0);
}

/*******************************************************
	remove the clock cursor
*******************************************************/
static void
Remove_clock ()
{
   	XDefineCursor(XtDisplay (TopLevel), XtWindow (TopLevel),
			GetOlStandardCursor (XtScreen (TopLevel)));
   	XSync (XtDisplay (TopLevel), 0);
}
