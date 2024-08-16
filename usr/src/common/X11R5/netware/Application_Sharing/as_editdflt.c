/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)appshare:as_editdflt.c	1.2"
#ident	"@(#)as_editdflt.c	2.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Application_Sharing/as_editdflt.c,v 1.1 1994/02/01 22:46:06 renu Exp $"

/*--------------------------------------------------------------------
** Filename : as_editdflt.c
**
** Description : This file contains functions to bring up a pop-up
**               window that allows the user to select which 
**               template he/she would like to edit. 
**               
**
** Functions :
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                           I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/FButtons.h>
#include <Xol/Caption.h>
#include "as_listhdr.h"


/*--------------------------------------------------------------------
**          F U N C T I O N    P R O T O T Y P E S
**------------------------------------------------------------------*/
void shutdownEditDflt( Widget, XtPointer, XtPointer );
void XSelectCB   ( Widget, XtPointer, XtPointer );
void TSelectCB   ( Widget, XtPointer, XtPointer );
void SelApplyCB  ( Widget, XtPointer, XtPointer );
void SelCancelCB ( Widget, XtPointer, XtPointer );
void SelHelpCB   ( Widget, XtPointer, XtPointer );



/*--------------------------------------------------------------------
**                      V A R I A B L E S 
**------------------------------------------------------------------*/
static buttonItems  selectItemsTemp[] = {
   { TXT_X_SCRIPT,   TXT_X_M_SCRIPT,   ( XtPointer ) XSelectCB },   
   { TXT_T_SCRIPT,   TXT_T_M_SCRIPT,   ( XtPointer ) TSelectCB }
}; 
static int numSelectItems = XtNumber( selectItemsTemp );

static buttonItems  selButtonItemsTemp[] = {
   { TXT_APPLY,      TXT_M_APPLY,      ( XtPointer ) SelApplyCB   },
   { TXT_CANCEL,     TXT_M_CANCEL,     ( XtPointer ) SelCancelCB  },
   { TXT_HELP,       TXT_M_HELP,       ( XtPointer ) SelHelpCB    }
};
static int numSelButtonItems = XtNumber( selButtonItemsTemp );

static int        selChoice;
static Boolean    applyFlag;

Widget            popup;



/*--------------------------------------------------------------------
** Function : DestroyCB
**
** Description :
**
** Parameters : As per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void SelDestroyCB( Widget w, XtPointer clientData, XtPointer callData )
{
    char *tempAppName;

    if ( applyFlag == TRUE )
    { 
        if ( selChoice == X_SCRIPT ) 
            tempAppName = x_script;
        else 
            tempAppName = t_script;
        EditFile( w, tempAppName );
    }
    XtDestroyWidget( w );
}

/*--------------------------------------------------------------------
** Function : SelectDfltScript 
**
** Description : This function pops up a window that allows the user
**               to select the script he/she would like to edit.
**
** Parameters : w         - the widget that requested the popup window
**              selScript - the selected script
**              
**
** Return :
**------------------------------------------------------------------*/
int SelectDfltScript( Widget w )
{
    Widget  upper;
    Widget  lower;
    Widget  footer; 
    Widget  caption;
    Widget  select;
    Widget  selectButtons;
    buttonItems   *selectItems;
    buttonItems   *selButtonItems;
    char   *title;
    char   *selectScript;

    selChoice = X_SCRIPT;
    applyFlag = FALSE;


    CopyInterStr( TXT_DFLT_EDIT_TITLE, &title, 0 ); 
    
    popup = XtVaCreatePopupShell( "Dflt Edit Popup",
                                   popupWindowShellWidgetClass,
                                   XtParent( w ),
                                   XtNtitle, title,
                                   ( String ) 0 );
    XtAddCallback( popup, XtNpopdownCallback, SelDestroyCB, w );
    
    XtVaGetValues( popup,
                   XtNupperControlArea, &upper,
                /*   XtNlowerControlArea, &lower,*/
                   XtNfooterPanel,      &footer,
                   ( String ) 0 );

    CopyInterStr( TXT_SELECT_SCRIPT, &selectScript, 0 ); 
    caption = XtVaCreateManagedWidget( "Sel Script",
                                captionWidgetClass,
                                upper,
                                XtNlabel, selectScript,  
                                ( String ) 0 );

    BuildButtonItems( &selectItems, selectItemsTemp, numSelectItems );
    select = XtVaCreateManagedWidget( "Select Buttons",
                                flatButtonsWidgetClass,
                                caption, 
                                XtNbuttonType, OL_RECT_BTN,
                                XtNitems,  selectItems,
                                XtNnumItems, numSelectItems,
                                XtNitemFields, buttonFields,
                                XtNnumItemFields, numButtonFields,
                                XtNexclusives, TRUE,
                                XtNnoneSet, FALSE,
                                XtNclientData, &selChoice,
                                ( String ) 0 );
    BuildButtonItems( &selButtonItems, selButtonItemsTemp, numSelButtonItems );
    selectButtons = XtVaCreateManagedWidget( "Select Buttons",
                                       flatButtonsWidgetClass,
                                       footer,
                                       XtNbuttonType, OL_OBLONG_BTN,
                                       XtNitems, selButtonItems,
                                       XtNnumItems, numSelButtonItems,
                                       XtNitemFields, buttonFields,
                                       XtNnumItemFields, numButtonFields,
                                       ( String ) 0 ); 
    XtPopup( popup, XtGrabExclusive );
}


/*--------------------------------------------------------------------
** Function : XSelectCB
**
** Description :
**
** Parameters : As per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void XSelectCB( Widget w, XtPointer clientData, XtPointer callData )
{
   int *sel;
   
    sel = ( int * ) clientData;
   *sel = X_SCRIPT;
}   


/*--------------------------------------------------------------------
** Function : TSelectCB
**
** Description :
**
** Parameters : As per callback functions
**
** Return : None 
**------------------------------------------------------------------*/
void TSelectCB( Widget w, XtPointer clientData, XtPointer callData )
{
   int *sel;
   
    sel = ( int * ) clientData;
   *sel = T_SCRIPT;
}


/*--------------------------------------------------------------------
** Function : SelApplyCB
**
** Description :
**
** Parameters : As per callback funcions
**
** Return : None
**------------------------------------------------------------------*/
void SelApplyCB( Widget w, XtPointer clientData, XtPointer callData )
{
    applyFlag = TRUE;
    XtPopdown( popup );
}


/*--------------------------------------------------------------------
** Function : SelCancelCB
**
** Description :
**
** Parameters : As per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void SelCancelCB( Widget w, XtPointer clientData, XtPointer callData )
{
    XtPopdown( popup );
}


/*--------------------------------------------------------------------
** Function : SelHelpCB
**
** Description :
**
** Parameters : As per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void SelHelpCB( Widget w, XtPointer clientData, XtPointer callData )
{
}

