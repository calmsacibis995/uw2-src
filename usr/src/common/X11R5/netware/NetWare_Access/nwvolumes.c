/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwaccess:nwvolumes.c	1.10"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/NetWare_Access/nwvolumes.c,v 1.11.4.1 1994/12/16 18:17:29 renuka Exp $"

/*
**  Netware Unix Client 
**	Copyright Novell Inc. 1990
**
**	Author: Hashem M Ebrahimi 
**	Modified: Renuka Veeramoney
**	Created: 12/06/91
**
**	MODULE:
**		nwvolumes.c - Displays the volumes of the specified server.
**
**	ABSTRACT:
**		Utility used to display NetWare volumes of a specified server.
*/
#include <stdio.h>
#include <string.h>
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

/********************************************************************
	* extern declaration 
*********************************************************************/
extern void		VerifyCB (Widget, XtPointer, XtPointer);
extern void 		CancelCB (Widget, XtPointer, XtPointer);
extern int		condition_handler (ServerList *, int *);
extern void		HelpCB (Widget, XtPointer, XtPointer);
extern void		GUIError (Widget, char *);
extern char *		GetStr (char *);
extern void 		SetLabels (MenuItem *, int);
extern void 		SetHelpLabels (HelpText *);
extern Widget		TopLevel;

#define	VolHelpSect	"120"

/*******************************************************
	format data, list item, vollist item structures
*******************************************************/
typedef struct {
    XtPointer	volume_name;
    XtPointer	total_blocks;
    XtPointer	available_blocks;
    XtPointer	total_entries;
    XtPointer	avail_entries;
} VolFormat;

typedef struct {
    XtArgVal	formatData;
} VolListItem;

typedef struct {
    Widget		listWidget;
    VolListItem		*listItems;
    VolFormat		*list;
    unsigned		cnt;
    unsigned		allocated;
} VolList;

static String	ListFields [] = {
    XtNformatData, 
};

/********************************************************************
	* forward declaration 
*********************************************************************/
static int 			nwvolumes (Widget, VolList *, char *);
static void 			FreeVols (VolList *);
static void 			PopdownCB (Widget, XtPointer, XtPointer);
static void 			SelectHeaderCB (Widget, XtPointer, XtPointer);
static VolListItem		*GetVolItems (VolList *);
static void 			MakeVolList (Widget, char *);
static int			GetVolItemsMaxLength (VolList *Vols);
static void 			MakeFormatStrings(char *headerBuf,char *listBuf, int volNameLength);

/**********************************************************************
 		Lower Control Area buttons 
***********************************************************************/
static HelpText VolHelp = {
    TXT_VolHelp,  VolHelpSect,
};

static MenuItem CommandItems [] = {
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CancelCB, (XtArgVal) True},		/* Cancel */
    { (XtArgVal) TXT_helpW, (XtArgVal) MNEM_helpW, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False, (XtArgVal) &VolHelp, },/*Help*/
};

static String	VolMenuFields [] = {
    XtNlabel, XtNmnemonic, XtNsensitive, XtNselectProc, XtNdefault,
    XtNuserData, XtNpopupMenu,
};

static int	NumVolMenuFields = XtNumber (VolMenuFields);

/**********************************************
	header fields
***********************************************/
static VolFormat	ColHdrs [1];
static VolListItem		ColItem [] = {
    (XtArgVal) ColHdrs,
};


/***************************************************
	statically declared variables
****************************************************/
static VolList 		volumes;
static Widget 		vol_list, headerWidget;
static char		*titleLbl;

/***********************************************************************
		volume list for the netware servers
***********************************************************************/
int
Volume_List (Widget parent, ServerList *servers)
{
    int retcode, searchi;

    retcode = condition_handler (servers, &searchi);
    switch (retcode) {
	case 1:
		GUIError (parent, GetStr (TXT_cantshowvolume));
		break;
	case 2:
		GUIError (parent, GetStr (TXT_novolselected));
		break;
	case 3:
		GUIError (parent, GetStr (TXT_volnotbeenauth));
		break;
	default:
    		if (vol_list) 
			XtPopdown (vol_list);
		XDefineCursor(XtDisplay (TopLevel),XtWindow (TopLevel),
			      GetOlBusyCursor (XtScreen (TopLevel)));
		XSync (XtDisplay (TopLevel), 0);

		MakeVolList(parent,servers->list[searchi].FileServers);
	
		XDefineCursor(XtDisplay (TopLevel),XtWindow (TopLevel),
			    GetOlStandardCursor (XtScreen (TopLevel)));
		XSync (XtDisplay (TopLevel), 0);
		break;
     }	/* switch statement*/
}	/* End of userlist () */

/*********************************************************************** 
		Makevolumelist
 		Make the volume list 
************************************************************************/
static void
MakeVolList (Widget parent, char *servername)
{
    	Widget		uca;
    	Widget		lca;
    	Widget		lcaMenu;
    	Widget		ucaMenuShell;
    	Widget		ucaMenu;
    	Widget		scrolledWindow;
    	static Boolean	first = True;
    	char 		headerBuf[256];
    	char 		listBuf[256];

    	if (nwvolumes (parent, &volumes, servername) == 1) { 
		return ;
	}

    	/* Set Labels */
    	if (first) {
		first = False;

		SetLabels (CommandItems, XtNumber (CommandItems));
		SetHelpLabels (&VolHelp);
	
		ColHdrs [0].volume_name = (XtPointer) GetStr (TXT_volumename);
		ColHdrs [0].total_blocks = (XtPointer) GetStr (TXT_totblocks);
		ColHdrs [0].available_blocks = (XtPointer) 
						GetStr (TXT_availblocks);
		ColHdrs [0].total_entries = (XtPointer) GetStr (TXT_totentries);
		ColHdrs [0].avail_entries = (XtPointer) GetStr 
							(TXT_availentries);
    	}

    	titleLbl = ( char *) XtMalloc (strlen (GetStr (TXT_volume)) 
			+ strlen(GetStr (TXT_for)) + strlen (servername) + 3);
    	strcpy (titleLbl, GetStr (TXT_volume));
    	strcat (titleLbl, GetStr (TXT_for));
    	strcat (titleLbl, servername);

    	/* Create user sheet */
    	vol_list = XtVaCreatePopupShell ("volume",
		popupWindowShellWidgetClass, parent,
		XtNtitle,		(XtArgVal) titleLbl,
		0);
   
    	XtVaGetValues (vol_list,
		XtNlowerControlArea,	(XtArgVal) &lca,
		XtNupperControlArea,	(XtArgVal) &uca,
		0);

    /*
     * Create the list format strings 
     */
    	MakeFormatStrings(headerBuf,listBuf, GetVolItemsMaxLength (&volumes));

    /* Create a list of users   Add a flat list above this
     * with a single item in it to act as column headers.
     */
    	headerWidget = XtVaCreateManagedWidget ("colHdr",
		flatListWidgetClass, uca,
		XtNviewHeight,		(XtArgVal) 1,
		XtNexclusives,		(XtArgVal) True,
		XtNnoneSet,		(XtArgVal) True,
		XtNselectProc,		(XtArgVal) SelectHeaderCB,
		XtNitems,		(XtArgVal) ColItem,
		XtNnumItems,		(XtArgVal) 1,
		XtNitemFields,		(XtArgVal) ListFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (ListFields),
		XtNformat,		(XtArgVal) headerBuf,
		XtNfont, 		(XtArgVal) _OlGetDefaultFont 
					(parent, OlDefaultFixedFont),
		XtNtraversalOn,		(XtArgVal) False,
		0);

    	scrolledWindow = XtVaCreateManagedWidget ("scrolledWindow",
		scrolledWindowWidgetClass, uca,
		0);

    	volumes.listItems = GetVolItems (&volumes);

    	volumes.listWidget = XtVaCreateManagedWidget ("vol_list",
		flatListWidgetClass, scrolledWindow,
		XtNviewHeight,		(XtArgVal) 5,
		XtNexclusives,		(XtArgVal) True,
		XtNnoneSet,		(XtArgVal) True,
		XtNitems,		(XtArgVal) volumes.listItems,
		XtNnumItems,		(XtArgVal) volumes.cnt,
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
		XtNclientData,		(XtArgVal) volumes.listWidget,
		XtNitemFields,		(XtArgVal) VolMenuFields,
		XtNnumItemFields,	(XtArgVal) NumVolMenuFields,
		XtNitems,		(XtArgVal) CommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (CommandItems),
		0);

    	ucaMenuShell = XtVaCreatePopupShell ("ucaMenuShell",
		popupMenuShellWidgetClass, uca,
		0);

    	ucaMenu = XtVaCreateManagedWidget ("ucaMenu",
		flatButtonsWidgetClass, ucaMenuShell,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNclientData,		(XtArgVal) volumes.listWidget,
		XtNitemFields,		(XtArgVal) VolMenuFields,
		XtNnumItemFields,	(XtArgVal) NumVolMenuFields,
		XtNitems,		(XtArgVal) CommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (CommandItems),
		0);

    	OlAddDefaultPopupMenuEH (uca, ucaMenuShell);

    /* Add callbacks to verify and destroy all widget when the property sheet
     * goes away
     */
    	XtAddCallback (vol_list, XtNverify, VerifyCB, (XtPointer) &PopdownOK);
    	XtAddCallback (vol_list, XtNpopdownCallback, PopdownCB,
		   (XtPointer) volumes.listItems);

    	XtPopup (vol_list, XtGrabNone);
}	/* End of makevolumelist () */


/*****************************************************************
 * PopdownCB
 * Destroy the popup widget and free associated data.
 * client_data is pointer to dynamically allocated items list.
 *****************************************************************/
static void
PopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtDestroyWidget (widget);
    FreeVols (&volumes);
    XtFree (client_data);
    XtFree (titleLbl);
    vol_list = (Widget) 0;
}	/* End of PopdownCB () */

/***************************************************************
 * GetVolItems
 * Format the volume list into items for the flat list.
 ***************************************************************/
static VolListItem *
GetVolItems (VolList *Vols)
{
    VolListItem			*items;
    register VolListItem	*pItem;
    register VolFormat		*pData;
    register			i;

    items = pItem = (VolListItem *) 
		     XtMalloc (Vols->cnt * sizeof (VolListItem));
    pData = Vols->list;
    for (i = Vols->cnt; --i>=0; pItem++, pData++) {
	pItem->formatData = (XtArgVal) pData;
    }

    return (items);
}	/* End of GetVolItems () */
/***************************************************************
 * GetVolItems
 * Format the volume list into items for the flat list.
 ***************************************************************/
static int 
GetVolItemsMaxLength (VolList *Vols)
{
    register VolFormat		*pData;
    register			i;
    register			x;
    register			length = 0;

    pData = Vols->list;
    for (i = Vols->cnt; --i>=0; pData++) {
	x = strlen(pData->volume_name);
		if ( x > length )
			length = x;
    }
    return (length);
}	/* End of GetVolItemsMaxLength () */

/************************************************************
	free the user data
**************************************************************/
static void
FreeVols (VolList *Vols)
{
    register	i;
    int		index;

    for (i = Vols->cnt; --i>=0; ) {
	XtFree (Vols->list[i].volume_name);
	XtFree (Vols->list[i].total_blocks);
	XtFree (Vols->list[i].available_blocks);
	XtFree (Vols->list[i].total_entries);
	XtFree (Vols->list[i].avail_entries);
    }

    Vols->cnt = 0;
}	/* End of FreeVols () */

/******************************************************************
			Flat List Format string creation routine
			Specific to Volume List ( sorry )
			"%16s  %11s  %11s  %17s  %17s\n",
*******************************************************************/
static void
MakeFormatStrings(char *headerBuf,char *listBuf, int volumeMaxLength)
{
    int i;
    char buffer[256];

    /*
     *  Volume name can be up to 16 chars wide.  
     *  Use "volumeMaxLength" as the field width unless the header 
     *  length is greater.
     */
    if ( (i = strlen(ColHdrs[0].volume_name)) <=volumeMaxLength )
    {
        i = volumeMaxLength;
    }
    strcpy(headerBuf,"%");
    sprintf(buffer,"%d",i);
    strcat(headerBuf,buffer);
    strcat(headerBuf,"s  ");

    /*
     *  Total bytes  12 digits = 999,999,999,999 ( 999 giga )
     */
    if ( (i = strlen(ColHdrs[0].total_blocks)) <=12 )
    {
        i  = 12;
    }
    strcat(headerBuf,"%");
    sprintf(buffer,"%d",i);
    strcat(headerBuf,buffer);
    strcat(headerBuf,"s  ");

    /*
     *  Available bytes 
     */
    if ( (i = strlen(ColHdrs[0].available_blocks)) <=12 )
    {
        i  = 12;
    }
    strcat(headerBuf,"%");
    sprintf(buffer,"%d",i);
    strcat(headerBuf,buffer);
    strcat(headerBuf,"s  ");

    /*
     *  Total dir entries
     */
    if ( (i = strlen(ColHdrs[0].total_entries)) <=17 )
    {
        i  = 17;
    }
    strcat(headerBuf,"%");
    sprintf(buffer,"%d",i);
    strcat(headerBuf,buffer);
    strcat(headerBuf,"s  ");

    /*
     *  Available dir entries
     */
    if ( (i = strlen(ColHdrs[0].avail_entries)) <=17 )
    {
        i  = 17;
    }
    strcat(headerBuf,"%");
    sprintf(buffer,"%d",i);
    strcat(headerBuf,buffer);
    strcat(headerBuf,"s  ");
    /*
     * List string format is same as header string format
     * so copy header into list format string
     */
    strcpy(listBuf,headerBuf);
}

/*******************************************************
	read the users into the list item
********************************************************/
static int
nwvolumes(Widget parent, VolList *Vols, char *server)
{
	NWCONN_HANDLE		serverConnID;
	NWNUMBER		MaxVols, ConnInUse, MaxConns;
	NWVOL_NUM		VolNum;
	VOL_STATS		volInfo;
	char			buffer[BUFSIZ];
	Boolean			found, Netware_for_unix;
	int			rc;
	NWVOL_FLAGS         Removable;
	DIR_SPACE_INFO dirSpace;

	found = False;
   /************************************************************ 
    * Need the NetWare server connection ID to get volume info.
    ************************************************************/
	if ((rc = getConnHandle(server, &serverConnID)) != 0) {
	GUIError (parent, GetStr (TXT_nwattach));
	return (1);
    }

    /************************************************************
     * Get the NetWare server info to find out the max number of
     * volumes.
     ************************************************************/
    if ((rc = NWGetFileServerInformation (serverConnID, NULL, NULL, NULL, NULL,
			&MaxConns, NULL, &ConnInUse, &MaxVols, NULL, NULL))
			!= 0) {
	GUIError (parent, GetStr (TXT_cantgetserverinfo));
	return(1);
    }

    	/************************************************************
      	 * Get volume names.
     	************************************************************/
     	for (VolNum = 0; VolNum < MaxVols; VolNum++) {
		if ( NWGetDirSpaceInfo(serverConnID,0,VolNum,&dirSpace) != 0 )
			continue;

		if ( dirSpace.totalBlocks == 0 )
			continue;

		found = True;
		if (Vols->cnt >= Vols->allocated) { 
			Vols->allocated += 2;
			Vols->list = (VolFormat *) XtRealloc((char *)Vols->list,
	  	      		Vols->allocated * sizeof(VolFormat));
		}

		if (strlen (dirSpace.volName) < 1 && Vols->cnt == 0) {
			found = False;
			break;
		}

		Vols->list [Vols->cnt].volume_name = 
			(XtPointer) strdup ((char*)dirSpace.volName);

		/* total blocks */
		sprintf(buffer, "%u", dirSpace.totalBlocks *  512 *
						dirSpace.sectorsPerBlock);
		Vols->list[Vols->cnt].total_blocks = 
				(XtPointer) strdup (buffer);

		/* avail blocks */
		sprintf(buffer, "%u", (dirSpace.availableBlocks *  512 *
						dirSpace.sectorsPerBlock) +
						(dirSpace.purgeableBlocks *  512 *
						dirSpace.sectorsPerBlock));

		Vols->list[Vols->cnt].available_blocks = 
				(XtPointer) strdup (buffer);

		sprintf(buffer, "%u", dirSpace.totalDirEntries);
		Vols->list[Vols->cnt].total_entries = 
				(XtPointer) strdup (buffer);

		sprintf(buffer, "%u", dirSpace.availableDirEntries);
		Vols->list[Vols->cnt++].avail_entries = 
				(XtPointer) strdup (buffer);
     	} /* for each volume */
	
	if (found == True)
   		return 0;
	else {
		GUIError (parent, GetStr (TXT_Novolumes));
		return 1;
	}
}

/*****************************************************************
 * Select header - unsets it
 *****************************************************************/
static void
SelectHeaderCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlVaFlatSetValues  (headerWidget, 0, XtNset, (XtArgVal) False, 0);
}	/* End of SelectHeaderCB () */
