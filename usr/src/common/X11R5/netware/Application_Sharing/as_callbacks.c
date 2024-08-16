/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)appshare:as_callbacks.c	1.7"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Application_Sharing/as_callbacks.c,v 1.8 1994/07/19 22:07:48 plc Exp $"

/*--------------------------------------------------------------------
** Filename : as_callbacks.c
**
** Description : This file contains the callback functions for the
**               Remote Application Launcher.
**
** Functions : EditCB
**             DeleteCB
**             CancelCB
**             HelpCB
**             AppSelectCB
**             AppUnselectCB
**             OkCB
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                           I N C L U D E S
**------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <priv.h>
#include <pwd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/FList.h>

#include "as_listhdr.h"


/*--------------------------------------------------------------------
** Function : EditCB
**
** Description : This function opens the currently selected file into 
**               the editor.
**
** Parameters : As per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void EditCB( Widget w, XtPointer clientData, XtPointer callData )
{
    EditFile( w, NULL );
}


/*--------------------------------------------------------------------
** Function : DeleteCB
**
** Description : This function deletes the currently selected file and
**               rebuilds the shared application list.
**
** Parameters :
**
** Return : None
**------------------------------------------------------------------*/
void DeleteCB ( Widget w, XtPointer clientData, XtPointer callData )
{
   unsigned char   *errorStr;
   unsigned char   *appPathName = NULL;
   int              i           = 0;
   Arg              args[1];

    if ( applicationName != NULL )
    {
        appPathName = ( unsigned char * )XtMalloc( strlen( SHARED_APPS_DIR ) +
                              strlen( applicationName )                      +
                              1               /* For / in path        */     +
                              1               /* For NULL termination */  ); 
        sprintf( ( char * )appPathName, "%s/%s", 
                  SHARED_APPS_DIR, applicationName );
        if ( ( unlink( ( char * )appPathName ) ) != 0 )
        {
            CopyInterStr( TXT_NO_APPL_TO_DEL, &errorStr, 0 );
            displayErrorMsg( w, errorStr );
            XtFree( ( XtPointer )errorStr );
        }
        else        
        {
            CopyInterStr( TXT_APPL_DELETED, &errorStr, 1, applicationName );
            XtVaSetValues( widg.footerStr,
                           XtNstring, errorStr, 
                           ( String ) 0 ); 
            XtFree( ( XtPointer )errorStr );
            FreeItemList( &items, numItems );
            numItems = BuildAppsItemList( &items, shApplIcon );
            if ( numItems != 0 )
        		enableRemoteAppsSAP();
            else 
        		disableRemoteAppsSAP();
            XtVaSetValues( widg.appList,
			   XtNitemsTouched, TRUE,
                           XtNitems, items,
                           XtNnumItems, numItems,
                           ( String ) 0 );
            XtSetArg( args[i], XtNsensitive, FALSE ); i++;
            OlFlatSetValues( widg.buttons, 0, args, i ); 
            OlFlatSetValues( widg.buttons, 1, args, i ); 
            OlFlatSetValues( widg.buttons, 3, args, i ); 
        }
    } 
    else
    {
        CopyInterStr( TXT_NO_APPL_TO_DEL, &errorStr, 0 );
        displayErrorMsg( w, errorStr );
        XtFree( ( XtPointer )errorStr );
    }
    if ( appPathName != NULL )
       XtFree( ( XtPointer )appPathName ); 
}



/*--------------------------------------------------------------------
** Function : EditDfltCB
**
** Description : This function allows the user to edit the 
**               script template file ( .exportTemp ).
**
** Parameters : as per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void EditDfltCB( Widget w, XtPointer clientData, XtPointer callData )
{
    EditFile( w, x_script );    
}

/*--------------------------------------------------------------------
** Function : ChgeTypeCB
**
** Description : This function changes the type of an application from
**               an X application to a Text application, or vice versa.
**
** Parameters : as per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void ChgeTypeCB ( Widget w, XtPointer clientData, XtPointer callData )
{
    char *filePath;
    char *type = NULL;
    static itemList *temp;
    int       i;
    Arg       args[1];
    static Pixmap   *icon;

    
    filePath = XtMalloc( strlen( SHARED_APPS_DIR ) + 
                         strlen( applicationName ) + 1 + 1 );
    sprintf( filePath, "%s/%s", SHARED_APPS_DIR, applicationName );  
    AddType( ( unsigned char ** )&type, ( unsigned char * )filePath, TRUE );
    XtFree( filePath );
    temp = ( XtPointer ) XtMalloc( sizeof ( Item ) );
    XtSetArg( args[0], XtNformatData, temp );
    for ( i = 0; ;i++ )
    {
      OlFlatGetValues( widg.appList, i, args, 1 );
      if ( strcmp( ( char * )temp->itemPtr->name, applicationName ) == 0 )
      { 
          XtFree((XtPointer)temp->itemPtr->type);
          temp->itemPtr->type = type;
          OlVaFlatSetValues( widg.appList, i, XtNformatData, temp->itemPtr, 
                        ( String ) 0 );
          break;
      }
    } 
}

/*--------------------------------------------------------------------
** Function : CancelCB
**
** Description : This function exits the application.
**
** Parameters : as per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void CancelCB ( Widget w, XtPointer clientData, XtPointer callData )
{
    XtCloseDisplay( XtDisplay( w ) );
    exit( 0 );
}


/*--------------------------------------------------------------------
** Function : HelpCB
**
** Description : This function brings up the Help information for this 
**               appliction.
**
** Parameters :
**
** Return : None
**------------------------------------------------------------------*/
void HelpCB ( Widget w, XtPointer clientData, XtPointer callData )
{
    DtRequest     request;
    long          serial;
    char         *name;
    char         *tag;

    CopyInterStr( TXT_TITLE, &name, 0 );

    memset( &request, 0, sizeof( request ) );
    request.header.rqtype = DT_DISPLAY_HELP;
    request.display_help.app_name = name;
    request.display_help.app_title = name;
    request.display_help.file_name = helpFile;
    request.display_help.icon_file = iconFile;
    request.display_help.sect_tag = HELP_SECT_TAG_10;
    request.display_help.source_type = DT_SECTION_HELP;

    serial = DtEnqueueRequest( XtScreen( w ),
                               _HELP_QUEUE( XtDisplay( w ) ),
                               _HELP_QUEUE( XtDisplay( w ) ),
                               XtWindow( w ),
                               &request );
    XtFree( ( XtPointer )name );
}


/*--------------------------------------------------------------------
** Function : AppSelectCB
**
** Description : This function is called when an application is 
**               selected from the application list.
**
** Parameters : as per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void AppSelectCB ( Widget w, XtPointer clientData, XtPointer callData )
{
    OlFlatCallData  *data;
    itemList        *itemListPtr;
    int              i = 0;
    Arg              args[1];

   data = ( OlFlatCallData * ) callData;
   itemListPtr = ( itemList *) data->items;
   itemListPtr += data->item_index;

   if ( applicationName != NULL )
      XtFree ( applicationName ); 
   applicationName = ( char * ) XtMalloc
                     ( strlen( ( char * )itemListPtr->itemPtr->name ) + 1 );
   if ( applicationName != NULL )
       strcpy( applicationName, ( char * )itemListPtr->itemPtr->name ); 
   XtSetArg( args[i], XtNsensitive, TRUE ); i++;
   OlFlatSetValues( widg.buttons, 0, args, i ); 
   OlFlatSetValues( widg.buttons, 1, args, i ); 
   OlFlatSetValues( widg.buttons, 3, args, i ); 
}


/*--------------------------------------------------------------------
** Function : AppUnselectCB
**
** Description : This function is called when an application is 
**               deselected from the application list.
**
** Parameters : as per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void AppUnselectCB ( Widget w, XtPointer clientData, XtPointer callData )
{
    OlFlatCallData  *data;
    itemList        *itemListPtr;
    int              i = 0;
    Arg              args[1];
    
   data = ( OlFlatCallData * ) callData;
   itemListPtr = ( itemList *) data->items;
   itemListPtr += data->item_index;

   if ( applicationName != NULL )
     if ( strcmp( applicationName, ( char * )itemListPtr->itemPtr->name ) == 0 )
       {
          XtFree( applicationName );
          applicationName = NULL;
       }
   XtSetArg( args[i], XtNsensitive, FALSE ); i++;
   OlFlatSetValues( widg.buttons, 0, args, i ); 
   OlFlatSetValues( widg.buttons, 1, args, i ); 
   OlFlatSetValues( widg.buttons, 3, args, i ); 
}


/*--------------------------------------------------------------------
** Function : OkCB
**
** Description : This function is called when the Ok button is selected
**               on the fatal error screen.
**
** Parameters : as per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void OkCB ( Widget w, XtPointer clientData, XtPointer callData )
{
    exit( 0 );
}


/*--------------------------------------------------------------------
** Function : EditFile 
**
** Description : This function brings up a file in the text editor.
**
** Parameters : w - if errors occur this widget will be the parent of
**                  the error popup.
**              altAppName - the application to bring up in the text
**                           editor.
**
** Return : None
**------------------------------------------------------------------*/
int EditFile( Widget w, char *altAppName )
{
    char             *commandLine = NULL;
    char             *commandStr = { "exec %s %s/%s 2>%s &" };
    char             *errorFile = NULL;
    char             *errorStr = NULL;
    Boolean          freeEditor = FALSE;
    struct   stat    statBuf;
    int              retCode;
    char             *saveAppName;

    if ( altAppName != NULL )
    {
        saveAppName = applicationName;
        applicationName = altAppName;
    }
    
    if ( applicationName != NULL )
    {
        retCode = stat( DFLT_EDITOR, &statBuf );
        if ( retCode )
        {
            CopyInterStr( TXT_NO_EDITOR_ERR, &errorStr, 0 );
            displayErrorMsg( w, errorStr );
            XtFree( errorStr );
        }
        else
        {
            GenRandomTempFName( &errorFile );
            mktemp( errorFile );
            commandLine = ( char * ) XtMalloc( strlen( commandStr )       +
                                             strlen( DFLT_EDITOR )      +
                                             strlen( SHARED_APPS_DIR )  +
                                             strlen( applicationName )  + 
                                             strlen( errorFile ) + 1 );
    
            sprintf( commandLine, commandStr, 
                             DFLT_EDITOR, SHARED_APPS_DIR, 
                             applicationName, errorFile ); 
            noPrivSystem( commandLine,SHARED_APPS_DIR,applicationName );
            GetErrorMsg( errorFile, &errorStr );
            if ( errorStr != NULL )
            {
                displayErrorMsg( w, errorStr );
                XtFree( errorStr );
            }
            XtFree( commandLine );
        }
    }
    else      
    {
        CopyInterStr( TXT_NO_APPL_TO_EDIT, &errorStr, 0 );
        displayErrorMsg( w, errorStr );
        XtFree( errorStr );
    }
    if ( altAppName != NULL )
    {
        applicationName = saveAppName;
    }
    if ( errorFile != NULL )
    {
        XtFree( ( XtPointer )errorFile );
        unlink( errorFile );
    }
}  

noPrivSystem(s,dir,appName)
const char *s;
const char *dir;
const char *appName;
{
	struct passwd *pwd;
	char *cp;
	uid_t uid;


	if (fork() == 0) 
	{
		/*
		 * Change the ownership of the file to this user  
		 * tfadmin has verified permissions
		 */

		uid = getuid();
		pwd = getpwuid(uid);
		if ( pwd != NULL )
		{
			cp = malloc(strlen(dir) + strlen(appName) + 2);
			strcpy(cp,dir);
			strcat(cp,"/");
			strcat(cp,appName);

			setuid(0);
			chown(cp,pwd->pw_uid,pwd->pw_gid);
        	chmod(cp, S_IRWXU | S_IXGRP | S_IRGRP | S_IXOTH | S_IROTH ); 
			setuid(pwd->pw_uid);

			free(cp);
		}
		/*
		 * Clear privilges so that the X application can
		 * find it's libraries.
		 */
		procprivl(CLRPRV,pm_max(P_ALLPRIVS),(priv_t)0);
		system(s);
		exit(1);
	} 
}
