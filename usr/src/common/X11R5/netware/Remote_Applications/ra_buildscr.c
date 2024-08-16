/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)remapps:ra_buildscr.c	1.9"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Remote_Applications/ra_buildscr.c,v 1.9 1994/09/26 17:43:21 plc Exp $"

/*--------------------------------------------------------------------
** Filename : dl_buildscr.c
**
** Description : This file contains functions to build and start the
**               double list screen.
**
** Functions : BuildDLScreen
**             InitIcon
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                        D E F I N E S
**------------------------------------------------------------------*/
#define OWNER_OF_WIDG  "widget_owner"

/*--------------------------------------------------------------------
**                       I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <X11/Shell.h>
#include <Dt/Desktop.h>
#include <Xol/ScrolledWi.h>
#include <Xol/FList.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>
#include <Xol/RubberTile.h>
#include <Xol/Caption.h>
#include <Xol/FooterPane.h>
#include <X11/cursorfont.h>

#include <X11/Xaw/Logo.h>
#include <X11/Xaw/Cardinals.h>

#include <ra_saptypes.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ra_hdr.h"


/*--------------------------------------------------------------------
**             I C O N   I N C L U D E   F I L E
**------------------------------------------------------------------*/
#include "Remote_Apps.icon"
#include "remApplServer16.xpm"
#include "remAppl16.xpm"

/*--------------------------------------------------------------------
**             X  R E S O U R C E S
**------------------------------------------------------------------*/
/*
 * Needs to be seen by ra_names.c
 */
struct _app_resources  app_resources;

#define Offset(field) XtOffsetOf(struct _app_resources, field)

static XtResource  resources[] = {
    {"tcpHostNames", "TcpHostNames",XtRString, sizeof(char *),
        Offset(tcpHostNames), XtRString, NULL
    },
    {"preferredHostNames", "PreferredHostNames",XtRString, sizeof(char *),
        Offset(preferredHostNames), XtRString, NULL
    },
};

void ReturnCB ( Widget w, XEvent *ev, String * strp, Cardinal * cp );
static XtActionsRec actions[] = {
    "CancelCB", (XtActionProc) CancelCB,
    "ReturnCB", (XtActionProc) ReturnCB,
    "HelpCB",   (XtActionProc) HelpCB,
    "LoginHelpCB",   (XtActionProc) LogHelpCB
};
static String  transTable = "#override <Key>Escape: CancelCB()";

/*--------------------------------------------------------------------
**    V A R I A B L E S   T O   I M P L E M E N T   C L E A N U P  
**------------------------------------------------------------------*/
static void ExitCB( Widget, XtPointer, XtPointer );

/*--------------------------------------------------------------------
** Function : BuildDLScreen
**
** Description : This function contstructs the double list screen.
**
** Parameters : screenInfo scrInfo - The information to configure 
**                                   the double window screen.
**              int  *argc    - pointer to count of command line parms 
**                              from main
**              char **argv   - command line parms from main 
**
** Return : 0 on Success
**          Error code on failure
**------------------------------------------------------------------*/
XtAppContext         any_context;
int BuildDLScreen ( int *argc, char **argv, screenInfo *scrInfo )
{
    long                 serial;
    DtRequest            request;
    Arg                  args[6];
    int                  j;
    Display              *display;   
    XtTranslations       translations;


    
/*    SetupErrHndlr( any_context, ErrHndlr );*/

    XtGetApplicationResources (widg.top, (XtPointer)&app_resources, resources,
                   XtNumber (resources), NULL, 0);

    DtInitialize( widg.top );
    display = XtDisplay( widg.top );

    timer_cursor   = XCreateFontCursor( display, XC_watch );
    
    XtSetArg ( args[0], XtNwidth, scrInfo->initialWidth );
    XtSetArg ( args[1], XtNheight, scrInfo->initialHeight );
    XtSetArg ( args[2], XtNminWidth, 150 );
    XtSetArg ( args[3], XtNtitle, scrInfo->title );
    XtSetValues ( widg.top, args, 4 ); 

    
    InitIcon ( &widg.top, 
               RemoteApplications_width, RemoteApplications_height, 
               RemoteApplications_ncolors, 
               RemoteApplications_chars_per_pixel, 
               (char *)RemoteApplications_colors, 
               (char *)RemoteApplications_pixels, 
               scrInfo->iconName, TRUE ); 
    scrInfo->remServicesIcon = InitIcon ( &widg.top, 
                                  remAppl16_width, remAppl16_height,
                                  remAppl16_ncolors, remAppl16_chars_per_pixel,
                                  (char *)remAppl16_colors, 
                                  (char *)remAppl16_pixels,
                                   NULL, FALSE );
    scrInfo->remServersIcon = InitIcon ( &widg.top, 
                               remApplServer16_width, remApplServer16_height,
                               remApplServer16_ncolors, 
                               remApplServer16_chars_per_pixel,
                               (char *)remApplServer16_colors, 
                               (char *)remApplServer16_pixels,
                               NULL, FALSE );
    MergeInResourceHostNames( &scrInfo->serverNames, scrInfo->numServerItems);
    BuildItemList( scrInfo->serverNames, &scrInfo->numServerItems,
                   &scrInfo->serverItems, scrInfo->remServersIcon );
    SortAppsItemList( &scrInfo->serverItems, scrInfo->numServerItems );
    SortForPreferredHosts( &scrInfo->serverItems, scrInfo->numServerItems );
    XtVaGetValues( widg.top, XtNminWidth, &j, NULL );

    if ( scrInfo->serverNames == NULL || 
         chdir("/tmp") != 0 )
    {
         item *okButton;
         char *temp;
         Widget buttons;
         Widget noServersWidg;
         char   * noServers;
         struct stat buf;

         XtSetArg ( args[0], XtNheight, 120 );
         XtSetValues ( widg.top, args, 1 ); 

         if ( isSystemSappingAType( PERSONAL_EDITION ) != SUCCESS )
             CopyInterStr( TXT_NOT_SAPPING, &noServers, 0 );
         else if ( chdir("/tmp") != 0 )
             CopyInterStr( TXT_CANT_CHDIR, &noServers, 0 );
         else
             CopyInterStr( TXT_NO_SERVERS, &noServers, 0 );   

         okButton = ( item * ) XtMalloc( sizeof( item ) );
         CopyInterStr( okButtonItems->label, &temp, 0 );
         okButton->label = temp;
         CopyInterStr( okButtonItems->mnemonic, &temp, 0 ); 
         okButton->mnemonic = ( XtPointer ) temp[0];
         okButton->select = okButtonItems->select; 
         okButton->sensitive = okButtonItems->sensitive;
         widg.baseRubberTile = XtVaCreateManagedWidget ( "baseRubberTile",
                                               rubberTileWidgetClass,
                                               widg.top,
                                               XtNorientation, OL_VERTICAL,
					       XtNsetMinHints, FALSE,
                                               ( String ) 0 );
         noServersWidg = XtVaCreateManagedWidget ( "No Servers",
                                                  staticTextWidgetClass,
                                                  widg.baseRubberTile,
                                                  XtNstring, noServers,
                                                  XtNalignment, OL_CENTER,
                                                  ( String ) 0 );
         buttons = XtVaCreateManagedWidget ( "buttons",
                                        flatButtonsWidgetClass,
                                        widg.baseRubberTile,
                                        XtNitems, okButton,
                                        XtNnumItems, numOkButtonItems,
                                        XtNitemFields, itemFields,
                                        XtNnumItemFields, numItemFields,
                                        XtNnoneSet, TRUE,
                                        XtNdefault, FALSE,
                                        ( String ) 0 );
         XtRealizeWidget ( widg.top );
         XtVaSetValues( widg.top,
                        XtNminWidth, 350,
                        XtNminHeight, 150,
                        ( String ) 0 );
         XtAppMainLoop ( any_context );   
    }
    widg.baseRubberTile = XtVaCreateManagedWidget ( "baseRubberTile",
                                               rubberTileWidgetClass,
                                               widg.top,
					       XtNsetMinHints, FALSE,
                                               ( String ) 0 );
    XtAddCallback( widg.top, XtNdestroyCallback, CleanupCB, NULL );
    widg.dblScrRubberTile = XtVaCreateManagedWidget ( "dblScrRubberTile",
                                           rubberTileWidgetClass,
                                           widg.baseRubberTile,
                                           XtNorientation, OL_HORIZONTAL,
                                           XtNweight, DBL_LIST_WEIGHT,
                                           ( String ) 0 );
    widg.footer = XtVaCreateManagedWidget ( "footer",
                                             footerPanelWidgetClass,
                                             widg.baseRubberTile,
                                             ( String ) 0 );
    widg.buttons = XtVaCreateManagedWidget ( "buttons",
                                        flatButtonsWidgetClass,
                                        widg.footer,
                                        XtNitems, scrInfo->buttonItems,
                                        XtNnumItems, scrInfo->numButtonItems,
                                        XtNitemFields, itemFields,
                                        XtNnumItemFields, numItemFields,
                                        XtNclientData, ( XtPointer ) scrInfo,
                                        XtNnoneSet, TRUE,
                                        XtNdefault, FALSE,
                                        ( String ) 0 );


    widg.footerStr    = XtVaCreateManagedWidget ( "footerStr",
                                                  staticTextWidgetClass,
                                                  widg.footer,
                                                  ( String ) 0 );
    widg.serverCaption = XtVaCreateManagedWidget ( "serverCaption",
                                              captionWidgetClass,
                                              widg.dblScrRubberTile,
                                              XtNposition, OL_TOP,
                                              XtNlabel, scrInfo->serverLabel,
                                              XtNlayoutHeight, OL_IGNORE,
                                              XtNlayoutWidth, OL_IGNORE,
                                              ( String ) 0 );
    widg.serverWin   = XtVaCreateManagedWidget ( "serverWin",
                                            scrolledWindowWidgetClass,
                                            widg.serverCaption,
                                            ( String ) 0 );
    widg.serverList  = XtVaCreateManagedWidget ( "serverList",
                                flatListWidgetClass,
                                widg.serverWin,
                                XtNnoneSet, TRUE,
                                XtNformat, "%p %s",
                                XtNitems, scrInfo->serverItems,
                                XtNnumItems, scrInfo->numServerItems,
                                XtNitemFields, itemListFields,
                                XtNnumItemFields, numItemListFields,
                                XtNselectProc, ServerSelCB,  
                                XtNunselectProc, ServUnselectCB,
                                XtNclientData, scrInfo->remServicesIcon,
                                ( String ) 0 );
    widg.serviceCaption = XtVaCreateManagedWidget ( "serviceCaption",
                                              captionWidgetClass,
                                              widg.dblScrRubberTile,
                                              XtNposition, OL_TOP,
                                              XtNlabel, scrInfo->serviceLabel,
                                              XtNlayoutHeight, OL_IGNORE,
                                              XtNlayoutWidth, OL_IGNORE,
                                              ( String ) 0 );
    widg.serviceWin   = XtVaCreateManagedWidget ( "serviceWin",
                                             scrolledWindowWidgetClass,
                                             widg.serviceCaption,
                                             ( String ) 0 );
   widg.serviceList   = NULL;
    
#ifdef NEVER
/* 
   This code appears to be left over from some experimental dev
   work.  Original developer did not remember why it is here.
   No other code references extraCaption, extraWin so I have
   defed it out for now.
*/
    if ( strcmp(scrInfo->extraLabel, "NULL") )
    {
        widg.extraCaption = XtVaCreateManagedWidget ( "serviceCaption",
                                                captionWidgetClass,
                                                widg.dblScrRubberTile,
                                                XtNposition, OL_TOP,
                                                XtNlabel, scrInfo->extraLabel,
                                                ( String ) 0 );
        widg.extraWin   = XtVaCreateManagedWidget ( "serviceWin",
                                                scrolledWindowWidgetClass,
                                                widg.extraCaption,
                                                ( String ) 0 );
    }
#endif
    memset( &request, 0, sizeof( request ) );
    request.header.rqtype           = DT_ADD_TO_HELPDESK;
    request.display_help.help_dir   = scrInfo->helpDir;
    request.display_help.file_name  = scrInfo->helpFile;
    request.display_help.app_name   = scrInfo->title;
    request.display_help.icon_file  = scrInfo->iconFile;
    request.display_help.source_type = DT_OPEN_HELPDESK;

    OlAddCallback( widg.top, XtNwmProtocol, ExitCB, NULL );

    XtAppAddActions (any_context, actions, XtNumber (actions));
    translations = XtParseTranslationTable(transTable);
    XtOverrideTranslations(widg.buttons, translations);
    XtOverrideTranslations(widg.serverList, translations);
    XtOverrideTranslations(widg.serviceWin, translations);

    translations = XtParseTranslationTable("#override <Key>Return: ReturnCB()");
    XtOverrideTranslations(widg.buttons, translations);

    translations = XtParseTranslationTable("#override <Key>F1: HelpCB()");
    XtOverrideTranslations(widg.serverList, translations);
    XtOverrideTranslations(widg.serviceWin, translations);
    XtOverrideTranslations(widg.buttons, translations);
    XtVaSetValues( widg.top, XtNuserData, (XtPointer) scrInfo, NULL);

    XtRealizeWidget ( widg.top );
    XtVaSetValues( widg.top,
                   XtNminWidth, scrInfo->initialWidth,
                   XtNminHeight, scrInfo->initialHeight,
                   ( String ) 0 );
    XtAppMainLoop ( any_context );   
}


void OkCB( Widget w, XtPointer clientData, XtPointer callData )
{
    exit( 0 );
}



/*--------------------------------------------------------------------
** Function : ExitCB 
**
** Description : Function called when window is destroyed
**
** Parameters : Widget
**
** Return :
**------------------------------------------------------------------*/
static void ExitCB( Widget w, XtPointer clientData, XtPointer callData )
{
    OlWMProtocolVerify    *wmData = ( OlWMProtocolVerify * ) callData;

    if ( wmData->msgtype == OL_WM_DELETE_WINDOW ) 
    {
        XCloseDisplay( XtDisplay( widg.top ) );
        AccessXhost( w, NULL, CLEANUP_XHOST );          
        exit( 0 );
    }
}

#include <X11/keysym.h>
/*
 * Routine changes the current event keycode, "return", to a space keycode and 
 * resends the event.  This way we get Motif button action, when button has focus
 * a space bar or a return will cause a button press action.
 */
void ReturnCB ( Widget w, XEvent *ev, String * strp, Cardinal * cp )
{ 
    KeyCode sel;
    XKeyEvent * evp = (XKeyEvent *)ev;
    /*
     * Change keycode to a space character, the selection key, and resend the event
     */
    sel = XKeysymToKeycode(XtDisplay(w),XK_space);
    evp->keycode = sel;
    XSendEvent(XtDisplay(w),XtWindow(w),True,0xffffff,(XEvent *)evp);
}
