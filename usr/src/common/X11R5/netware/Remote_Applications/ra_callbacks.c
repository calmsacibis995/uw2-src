/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)remapps:ra_callbacks.c	1.6"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Remote_Applications/ra_callbacks.c,v 1.5 1994/09/28 14:31:52 plc Exp $"

/*--------------------------------------------------------------------
** Filename : dl_callbacks.c
**
** Description : This file contains callback functions for the double
**               list window.
**
** Functions : ServerSelCB
**             ApplSelectCB
**             HelpCB
**             CancelCB
**             MkIconCB
**             LaunchCB
**             ServUnselectCB
**             AppUnselectCB
**             CleanupCB
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                         D E F I N E S
**------------------------------------------------------------------*/
#define   OWNER_OF_STRINGS     "mine"       

/*--------------------------------------------------------------------
**                         I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Dt/Desktop.h>
#include <Xol/OlCursors.h>
#include <Xol/FList.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <signal.h>

void sig_alrm();
extern int errno;

#include <stdio.h>
#include <sys/utsname.h>
#include "ra_saptypes.h"
#include "ra_hdr.h"

#include <errno.h>
#include <limits.h>
#include <priv.h>

/*--------------------------------------------------------------------
** Function : ServerSelCB
**
** Description : This is the callback function that is executed when
**               a server is selected.
**
** Parameters : See parameters for callback routines
**
** Return : None
**------------------------------------------------------------------*/
void ServerSelCB ( Widget w, XtPointer clientData, XtPointer callData )
{
    OlFlatCallData    *data;
    Pixmap            *icon;
    itemList          *item_ptr;
    char              *messageLabel = NULL;
    Arg                args[1];
    int                i = 0;
    char              *serviceLabel; 

    data = ( OlFlatCallData * ) callData;
    icon = ( Pixmap * )         clientData;
    item_ptr = ( itemList *) data->items;
    item_ptr += data->item_index;

    XtVaSetValues( widg.serverList, XtNsensitive, FALSE, ( String ) 0 );

    if ( serverName != NULL )    
        XtFree ( serverName );
    serverName = ( char * ) XtMalloc( strlen( item_ptr->itemPtr->name ) + 1 );
    if ( serverName != NULL )
        strcpy( serverName, item_ptr->itemPtr->name );

    /*-------------------------------------------------------------
    ** Display footer message so user knows that we are querying
    ** the remote machine
    **-----------------------------------------------------------*/
    CopyInterStr( TXT_QUERY_MESS, &messageLabel, 1, serverName );
    XtVaSetValues( widg.footerStr, 
                   XtNstring, messageLabel,
                   ( String ) 0 );

    XtSetArg( args[0], XtNsensitive, FALSE ); i++;
    OlFlatSetValues( widg.buttons, 0, args, i );
    OlFlatSetValues( widg.buttons, 1, args, i );
    CopyInterStr( TXT_SERVICE_LABEL, &serviceLabel, 0 );
    i = 0;
    XtSetArg( args[0], XtNlabel, serviceLabel ); i++;
    XtSetValues( widg.serviceCaption, args, i );
    XtFree( serviceLabel );
    GetXApplList( serverName, icon );
}



/*--------------------------------------------------------------------
** Function : ApplSelectCB
**
** Description : This is the callback function when an application is
**               selected from the Applications on * : list. 
**
** Parameters : Widget w             - 
**              XtPointer clientData - 
**              XtPointer callData   - 
**
** Return : None
**------------------------------------------------------------------*/
void ApplSelectCB( Widget w, XtPointer clientData, XtPointer callData )
{
    OlFlatCallData    *data;
    itemList          *item_ptr;
    Arg                args[1];
    int                i = 0;


    data = ( OlFlatCallData * ) callData;
    item_ptr = ( itemList *) data->items;
    item_ptr += data->item_index;
    
    if ( applicationName != NULL )    
    { 
        XtFree ( applicationName );
        applicationName = NULL;
    }
    applicationName = ( char * )XtMalloc( strlen( item_ptr->itemPtr->name ) + 1 );
    if ( applicationName != NULL )
        strcpy( applicationName, item_ptr->itemPtr->name );

    XtSetArg( args[0], XtNsensitive, TRUE ); i++;
    OlFlatSetValues( widg.buttons, 0, args, i );
    OlFlatSetValues( widg.buttons, 1, args, i );
} 



/*--------------------------------------------------------------------
** Function : HelpCB
**
** Description : This function is the select callback function for the
**               "Help" button.
**
** Parameters : as per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void HelpCB ( Widget w, XtPointer clientData, XtPointer callData )
{
    screenInfo    *scrInfo;
    DtRequest     request;
    long          serial;
    char      *tag;

    XtVaGetValues( widg.top, XtNuserData, &scrInfo, NULL );

    memset( &request, 0, sizeof( request ) );
    request.header.rqtype = DT_DISPLAY_HELP;
    request.display_help.app_name = scrInfo->title;
    request.display_help.app_title = scrInfo->title;
/*    request.display_help.help_dir = scrInfo->helpDir;*/
    request.display_help.file_name = scrInfo->helpFile;
    request.display_help.icon_file = scrInfo->iconFile;    
    request.display_help.sect_tag = HELP_SECT_TAG_10;
    request.display_help.source_type = DT_SECTION_HELP /* DT_OPEN_HELPDESK*/;

    serial = DtEnqueueRequest( XtScreen( widg.top ),
                               _HELP_QUEUE( XtDisplay( widg.top ) ),
                               _HELP_QUEUE( XtDisplay( widg.top ) ),
                               XtWindow( widg.top ),
                               &request );
}


/*--------------------------------------------------------------------
** Function : CancelCB
**
** Description : This function is the select callback function for the
**               "Cancel" button.
**
** Parameters : Widget w -
**              XtPointer clientData - 
**              XtPointer callData - 
**
** Return : None
**------------------------------------------------------------------*/
void CancelCB ( Widget w, XtPointer clientData, XtPointer callData )
{ 
    AccessXhost( w, NULL, CLEANUP_XHOST );
    exit( 0 );
}


/*--------------------------------------------------------------------
** Function : MkIconCB
**
** Description : This function is the select callback function for the
**               "Make Icon" button.
**
** Parameters : Widget w -
**              XtPointer clientData - 
**              XtPointer callData - 
**
** Return : None
**------------------------------------------------------------------*/
void MkIconCB ( Widget w, XtPointer clientData, XtPointer callData )
{
    FILE             *remApplFD = NULL;
    char             *homeDir;
    char             *commandLine;
    int               commandLen;
    char             *doNotEdit = NULL;
    char             *errorStr  = NULL;

    char             *iconFileFmt  = { "%s-%s.rem" };
    char             *iconFileName = NULL;

    char             *filePathFmt = { "%s%sApplications/%s" }; 
    char             *filePath;
    char             *messageLabel = NULL;
    char             *cmdLineArg0 = NULL;
    char             *cp = NULL;
    struct stat      sbuf;
    int              i;
    extern  char *   arg;

    
    iconFileName = XtMalloc( strlen( iconFileFmt )     +
                             strlen( serverName )      + 
                             strlen( applicationName ) + 1 );
    sprintf( iconFileName, iconFileFmt, serverName, applicationName );
    

	/*
	 * In case user has moved the Applications dir or the name
	 * has been internationalized.
	 *
	 * Get the command line arg
	 * Find last '/' in arg
	 * lstat file name
	 * if ( stat ok &&  stat == link && a '/' was found )
	 *    strip app name from arg
	 *    append iconFileName to arg
	 * else
 	 *    do old technique
	 */
	cmdLineArg0 = arg;
	cp = strrchr(cmdLineArg0,'/');
	i = lstat(cmdLineArg0,&sbuf);
	if ((i != -1) && (sbuf.st_mode & S_IFLNK) && cp )
	{
		*(cp + 1) = NULL;
    	filePath = XtMalloc( strlen( cmdLineArg0 ) + 
                         strlen( iconFileName ) + 1 );
		strcpy(filePath,cmdLineArg0);
		strcat(filePath,iconFileName);
	}
	else
	{
    	homeDir = ( char * ) getenv( "HOME" );
    	filePath = XtMalloc( strlen( filePathFmt ) + 
                         	strlen( homeDir ) +
                         	strlen( iconFileName ) + 1 );
    	if ( homeDir[strlen( homeDir ) - 1] == '/' ) 
    	{
        	sprintf( filePath, filePathFmt, homeDir, "", iconFileName );
    	}
    	else
        	sprintf( filePath, filePathFmt, homeDir, "/", iconFileName ); 
	}
   	remApplFD = fopen( filePath, "w" );

    if ( remApplFD != NULL )
    {
        fwrite( "# REMAPPLICON\n", 1, 14, remApplFD );

        CopyInterStr( TXT_DO_NOT_EDIT, &doNotEdit, 0 );
        fwrite( doNotEdit, 1, strlen( doNotEdit ), remApplFD ); 
        XtFree( doNotEdit );

        commandLen =  ( strlen( serverName ) > strlen( applicationName ) ?
                            strlen( serverName ) : strlen( applicationName ) ) 
                            + 10;
        commandLine = XtMalloc( commandLen + 4 ); 

        sprintf( commandLine, "\n*Server-%s\n", serverName );
        fwrite( commandLine,  1, strlen( commandLine ), remApplFD );

        sprintf( commandLine, "*App-%s\n", applicationName );
        fwrite( commandLine,  1, strlen( commandLine ), remApplFD ); 

        fclose( remApplFD );
        chmod( filePath, 0x777 );
        XtFree( commandLine );
        CopyInterStr( TXT_MKICON_MESS, &messageLabel, 1, applicationName );
        XtVaSetValues( widg.footerStr, 
                       XtNstring, messageLabel,
                       ( String ) 0 );
        XtFree( ( XtPointer ) messageLabel );
        XSync( XtDisplay( widg.top ), False );
    }
    else
    {
       CopyInterStr( TXT_CANT_CREATE_ICON, &errorStr, 0 );
       displayErrorMsg( w, errorStr );
       XtFree( errorStr ); 
    }
EXIT_MKICON:
    if ( iconFileName )
        XtFree( iconFileName );
    if ( filePath )
        XtFree( filePath );
    return; 
}


/*--------------------------------------------------------------------
** Function : LaunchCB
**
** Description : This function is the select callback function for the
**               "Launch" button.
**
** Parameters : As per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void LaunchCB ( Widget w, XtPointer clientData, XtPointer callData )
{
    char            *commandBuf = NULL;
    char            *command = { "\"/usr/X/lib/app-defaults/.exportApps/%s\" %s:0 &" };
    int              ioFD            = -1;
    int	             retCode;
    int              localErrFD;
    char            *messageLabel;

    char            *errorStr        = NULL;
    static char     *xhostDisplay    = NULL;
    struct utsname   name;
    char            *fileName        = NULL;
    int              charsRead;

    /*------------------------------------------------
    ** Attempt to add server name to the xhost file
    **----------------------------------------------*/
    retCode = AccessXhost( w, serverName, ADD_XHOST ); 
    if ( retCode != SUCCESS )
        goto EXIT_LAUNCH;

    /*------------------------------------------------
    ** If we don't have the xhost display, then get it.
    **----------------------------------------------*/  
    if ( xhostDisplay == NULL )
    {
        retCode = uname( &name );
        if ( retCode == -1 )
        {
           CopyInterStr( TXT_CANT_GET_XNAME, &errorStr, 0 );
           displayErrorMsg( w, errorStr );
           goto EXIT_LAUNCH;
        }
        else
        {
           xhostDisplay = XtMalloc( strlen( name.nodename ) + 1 );
           strcpy( xhostDisplay, name.nodename );
        }
    }    

    /*------------------------------------------------------------------
    ** Give the user some hints that we are launching the app.
    ** Change the cursor to clock, and display a message in the footer. 
    **----------------------------------------------------------------*/
    XDefineCursor( XtDisplay( widg.top ), XtWindow( widg.top ), timer_cursor );
    CopyInterStr( TXT_LAUNCH_MESS, &messageLabel, 1, applicationName );
    XtVaSetValues( widg.footerStr, 
                   XtNstring, messageLabel,
                   ( String ) 0 );
    XtFree( ( XtPointer ) messageLabel );
    XSync( XtDisplay( widg.top ), False );

    /*----------------------------------------------------------------
    ** Now actually make the remote call to launch the application.
    ** First set redirect out stderr to a file in case we get an 
    ** error locally.
    **--------------------------------------------------------------*/
    close( 2 );
    GenRandomTempFName( &fileName );
    mktemp( fileName );
    localErrFD = creat( fileName, O_RDWR );

    commandBuf = ( XtPointer ) XtMalloc( strlen( command ) +
                                         strlen( applicationName ) + 
                                         strlen( xhostDisplay ) + 1 );
    sprintf( commandBuf, command, applicationName, xhostDisplay );
    ioFD = rexec( &serverName, REXEC_SOCKET, currSess.userID, 
                  currSess.passwd, commandBuf, NULL );

    errorStr = ( XtPointer ) XtMalloc( MAX_REXEC_ERR );
    /*-------------------------------------
    ** Now, try to determine what happened.     
    **-----------------------------------*/
    if ( ioFD == -1 )
    { 
        close( localErrFD );
		charsRead = 1;
        localErrFD = open( fileName, O_RDONLY ); 
		if ( localErrFD != -1 )
        	charsRead = read( localErrFD, errorStr, MAX_REXEC_ERR - 2 );
        if ( charsRead == -1  || localErrFD == -1 )
        {
            XtFree( errorStr );
            CopyInterStr( TXT_UNABLE_TO_CONNECT, &errorStr, 1, serverName );
        }
        else
            errorStr[charsRead - 1] = '\0';
        if ( strlen( errorStr ) == 0 )
        {
           XtFree( ( XtPointer ) errorStr );
           CopyInterStr( TXT_NETWORK_ERROR, &errorStr, 0 );
        }
        CopyInterStr( TXT_NETWORK_ERROR, &messageLabel, 0 ); 
        XtVaSetValues( widg.footerStr, 
                       XtNstring, messageLabel,
                       ( String ) 0 );
        XtFree( ( XtPointer ) messageLabel );
        displayErrorMsg( w, errorStr );
    }
    else
    {
        fcntl( ioFD, F_SETFL, O_NDELAY | O_NONBLOCK | O_APPEND );
        sleep( 7 );
		memset(errorStr,0,MAX_REXEC_ERR);
        charsRead = read( ioFD, errorStr, 1 );

        if ( charsRead >  0 )
        {
			/*
			 * Alright, got an error message back
			 * give him/her 5 sec to give me the rest.
			 * Can't hang around here forever
			 */
            int i = 1;
			int jj;

			signal(SIGALRM,sig_alrm);
			alarm(5);

            fcntl( ioFD, F_SETFL, O_APPEND );
            while( ( ( jj = read( ioFD, &(errorStr[i]), 1 ) ) != 0 ) &&
                 ( i < ( MAX_REXEC_ERR - 1 ) ) )
			{
				if ( jj == -1 &&  errno == EINTR )
				{
					/* Alarm poped, break from the read */
					/* Display what we have */
					break;
				}
                i++;
			}
    		(void) alarm(0);
			signal(SIGALRM,(void (*)())SIG_DFL);

            errorStr[i] = '\0';
            if ( strlen( errorStr ) == 0 )
            {
               XtFree( ( XtPointer ) errorStr );
               CopyInterStr( TXT_NETWORK_ERROR, &errorStr, 0 );
            }
            if ( strlen( errorStr ) > 2 )
            {
                CopyInterStr( TXT_NETWORK_ERROR, &messageLabel, 0 ); 
                XtVaSetValues( widg.footerStr, 
                               XtNstring, messageLabel,
                               ( String ) 0 );
                XtFree( ( XtPointer ) messageLabel );
            	displayErrorMsg( w, errorStr );
            }
        }
    }
EXIT_LAUNCH:

    /*------------------------------------------------------
    ** If we have allocated resources then clean up. Set the
    ** cursor back to a pointer.
    **----------------------------------------------------*/
    if ( errorStr != NULL ) 
        XtFree( errorStr );
    if ( ioFD != -1 )
        close( ioFD ); 
    if ( localErrFD != -1 )
    {
        close( localErrFD );
        unlink( fileName );
        XtFree( fileName );
    }
    XUndefineCursor( XtDisplay( widg.top ), XtWindow( widg.top ) );
}



/*--------------------------------------------------------------------
** Function : AppUnSelectCB
**
** Description : This function is called when an application is 
**               unselected from the Remote Application List.
**
** Parameters : See parameters for Callback routines
**
** Return : None
**------------------------------------------------------------------*/
void AppUnselectCB( Widget w, XtPointer clientData, XtPointer callData )
{
    OlFlatCallData    *data;
    itemList          *itemListPtr;
    Arg                args[1];
    int                i = 0;

    data = ( OlFlatCallData * ) callData;
    itemListPtr = ( itemList * ) data->items;
    itemListPtr += data->item_index;

    if ( applicationName != NULL )
        if ( strcmp( applicationName, itemListPtr->itemPtr->name ) == 0 )
        {
            XtFree( applicationName );
            applicationName = NULL;
        }
    XtSetArg( args[0], XtNsensitive, FALSE ); i++;
    OlFlatSetValues( widg.buttons, 0, args, i );
    OlFlatSetValues( widg.buttons, 1, args, i );
}


/*--------------------------------------------------------------------
** Funciton : ServUnSelectCB 
**
** Description : This function is called when a server is 
**               unselected from the Remote Application Server 
**               List.
**
** Parameters : See parameters for Callback routines
**
** Return : None
**------------------------------------------------------------------*/
void ServUnselectCB( Widget w, XtPointer clientData, XtPointer callData )
{
    OlFlatCallData    *data;
    itemList          *itemListPtr;
    Arg                args[1];
    int                i = 0;
    char              *serviceLabel;

    data = ( OlFlatCallData * ) callData;
    itemListPtr = ( itemList * ) data->items;
    itemListPtr += data->item_index;

    if ( serverName != NULL )
        if ( strcmp( serverName, itemListPtr->itemPtr->name ) == 0 )
        {
            XtFree( serverName );
            serverName = NULL;
        }
    XtSetArg( args[0], XtNsensitive, FALSE ); i++;
    OlFlatSetValues( widg.buttons, 0, args, i );
    OlFlatSetValues( widg.buttons, 1, args, i );
    XtVaSetValues( widg.serviceList, 
		   XtNitems, NULL,
		   XtNnumItems, 0,
		   XtNitemsTouched, TRUE,
		   ( String ) 0 );
    CopyInterStr( TXT_SERVICE_LABEL, &serviceLabel, 0 );
    i = 0;
    XtSetArg( args[0], XtNlabel, serviceLabel ); i++;
    XtSetValues( widg.serviceCaption, args, i );
    XtFree( serviceLabel );
}


/*--------------------------------------------------------------------
** Funciton : CleanupCB 
**
** Description : This function is when the widget is going to be 
**               destroyed. Anything that needs to be done before
**               exiting can be done here.
**
** Parameters : See parameters for Callback routines
**
** Return : None
**------------------------------------------------------------------*/
void CleanupCB( Widget w, XtPointer clientData, XtPointer callData )
{
    AccessXhost( w, NULL, CLEANUP_XHOST );
}

/*--------------------------------------------------------------------
** Funciton : sig_alrm 
**
** Description : This function is called when the alarm timeout expires and
**               the SIGALRM handler has been pointed here. Used to terminate 
**               blocking reads, etc.
**          
**               CAUTION: Alarm handler is set to default. ( I know SVR4
**                        does it automatically, but who knows maybe they
**                        will fix it some day )
**
** Parameters : Should be SIGALRM
**
** Return : None
**------------------------------------------------------------------*/
void
sig_alrm(int signo )
{
	signal(SIGALRM,(void (*)())SIG_DFL);
	return;
}
