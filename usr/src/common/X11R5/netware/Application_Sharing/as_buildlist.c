/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)appshare:as_buildlist.c	1.10"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Application_Sharing/as_buildlist.c,v 1.11 1994/09/26 17:44:37 plc Exp $"

/*--------------------------------------------------------------------
** Filename : as_buildlist.c 
**
** Description : This file contains functions to build and control the
**               App_Sharing window.
**
** Functions : BuildListWindow
**             BuildAppsItemList
**             FileDropProc
**             DoneProc
**             AddDirectory
**             CreateScriptFile
**             FreeItemList
**             ResizeDropSite
**             AddType
**             ErrorChecking
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                         D E F I N E S
**------------------------------------------------------------------*/
#define      OWNER_OF_VARS      "mine"

/*--------------------------------------------------------------------
**                           I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <X11/Shell.h>
#include <Xol/RubberTile.h>
#include <Xol/Caption.h>
#include <Xol/ScrolledWi.h>
#include <Xol/FList.h>
#include <Xol/FooterPane.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>

#include "as_listhdr.h"

#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <priv.h>

/*--------------------------------------------------------------------
**                 I C O N   I N C L U D E   F I L E
**------------------------------------------------------------------*/
#include "App_Sharing.icon"
#include "sharedAppl16.xpm"


/*--------------------------------------------------------------------
**              E X T E R N A L    P R O T O T Y P E S 
**------------------------------------------------------------------*/
extern Boolean	_DtamIsOwner (char *);


/*--------------------------------------------------------------------
**               G L O B A L   V A R I A B L E S
**------------------------------------------------------------------*/
OlDnDDropSiteID       dropSiteID; 

void ReturnCB ( Widget w, XEvent *ev, String * strp, Cardinal * cp );
static XtActionsRec actions[] = {
    "CancelCB", (XtActionProc) CancelCB,
    "ReturnCB", (XtActionProc) ReturnCB,
    "HelpCB",   (XtActionProc) HelpCB
};
String  transTable = "#override <Key>Escape: CancelCB()";

/*--------------------------------------------------------------------
**                           M A C R O S
**------------------------------------------------------------------*/
#define  IsExec( b )       b & S_IXUSR || \
                           b & S_IXGRP || \
                           b & S_IXOTH

/*--------------------------------------------------------------------
** Function : BuildListWindow
**
** Description : This function builds a window with a single scrollable
**               list.
**
** Parameters : WnStruc    *win - Pointer to a structure that contains 
**                                all the main window variables for the 
**                                application. 
**              int       *argc - Pointer to a count of the number
**                                of command line arguments.
**              char     **argv - Pointer to the command line 
**                                arguments.
**
** Return : None
**------------------------------------------------------------------*/
XtAppContext          appContext;
void BuildListWindow( WnStruc *win, int *argc, char **argv )
{
   OlDnDSiteRect         dropSiteRect;
   Arg                   args[5];
   Position              x;
   Position              y;
   Dimension             width;
   Dimension             height;
   Dimension             theight;
   unsigned char         *errorStr = NULL;
   XtTranslations        translations;

   InitIcon( &widg.toplevel, 
             ApplicationSharing_width, ApplicationSharing_height, 
             ApplicationSharing_ncolors, 
             ApplicationSharing_chars_per_pixel,
             ( char * ) ApplicationSharing_colors, 
             ( char * ) ApplicationSharing_pixels,
             win->iconName,  TRUE );
   /*----------------------------------------------------------------
   ** This code initializes and creates the format data list of items.
   **--------------------------------------------------------------*/
   shApplIcon = InitIcon( &widg.toplevel, 
                         sharedAppl16_width, sharedAppl16_height, 
                         sharedAppl16_ncolors, sharedAppl16_chars_per_pixel,
                         ( char * ) sharedAppl16_colors, 
                         ( char * ) sharedAppl16_pixels,
                          NULL, FALSE );

   /* This function will display an error message and not
   ** return if an error is detected */
   ErrorChecking( appContext );

   XtVaSetValues( widg.toplevel, 
                  XtNtitle, win->title,
                  NULL );

   numItems = BuildAppsItemList( &items, shApplIcon );
   if ( numItems > 0 )
       enableRemoteAppsSAP();
   else 
       disableRemoteAppsSAP();


   widg.appRubberTile = XtVaCreateManagedWidget( "Appl Rubber Tile",
                                                  rubberTileWidgetClass,
                                                  widg.toplevel,
                                                  XtNorientation, OL_VERTICAL,
                                                  XtNlayoutHeight, OL_MINIMIZE,
                                                  XtNlayoutWidth, OL_MINIMIZE,
						  XtNsetMinHints, FALSE,
                                                  ( String ) 0 );
   widg.appCaption = XtVaCreateManagedWidget( "Application Caption",
                                               captionWidgetClass,
                                               widg.appRubberTile, 
                                               XtNposition, OL_TOP,
                                               XtNlabel, win->appListLabel,
                                               XtNweight, 10,
                                               ( String ) 0 );
    widg.appWin     = XtVaCreateManagedWidget( "Application Window",
                                                scrolledWindowWidgetClass,
                                                widg.appCaption,
                                                ( String ) 0 );
    widg.appList    = XtVaCreateManagedWidget( "Application List",
                                                flatListWidgetClass,
                                                widg.appWin,
                                                XtNnoneSet, TRUE,
                                                XtNformat, "%p %s        %s",
                                                XtNitems, items,
                                                XtNnumItems, numItems,
                                                XtNitemFields, itemListFields,
                                                XtNnumItemFields, 1,
                                                XtNselectProc, AppSelectCB,
                                                XtNunselectProc, AppUnselectCB,
                                                ( String ) 0 );
    widg.footer     = XtVaCreateManagedWidget( "Footer",
                                                footerPanelWidgetClass,
                                                widg.appRubberTile,
                                                ( String ) 0 );
    widg.buttons    = XtVaCreateManagedWidget( "Buttons",
                                              flatButtonsWidgetClass,
                                              widg.footer,
                                              XtNitems, win->buttons,
                                              XtNnumItems, win->numButtonItems,
                                              XtNitemFields, buttonFields,
                                              XtNnumItemFields, numButtonFields,
                                              XtNnoneSet, TRUE,
                                              XtNdefault, FALSE,
                                              ( String ) 0 ); 
    widg.footerStr  = XtVaCreateManagedWidget( "Footer String",
                                                staticTextWidgetClass,
                                                widg.footer,
                                                XtNstring, win->footerLabel,
                                                ( String ) 0 );

    XtAddEventHandler( widg.appCaption, StructureNotifyMask,
                       FALSE, ResizeDropSite, NULL ); 
                                              
    XtRealizeWidget( widg.toplevel );

    XtSetArg( args[0], XtNinitialX, &x );
    XtSetArg( args[1], XtNinitialY, &y );
    XtSetArg( args[2], XtNheight, &height );
    XtSetArg( args[3], XtNwidth, &width );
    XtGetValues( widg.appCaption, args, 4 );
    dropSiteRect.x      = x;
    dropSiteRect.y      = y;
    dropSiteRect.width  = width;
    dropSiteRect.height = height;  
    dropSiteID = OlDnDRegisterWidgetDropSite( widg.appCaption,
                                           OlDnDSitePreviewNone,
                                           &dropSiteRect,
                                           1,
                                           ( OlDnDTMNotifyProc )FileDropProc,
                                           NULL,
                                           TRUE,
                                           NULL );
    /*
     * Set the minimum w/h to the calculated values.
     */
    XtSetArg( args[0], XtNheight, &theight );
    XtGetValues( widg.appWin, args, 1 );
    XtVaSetValues( widg.toplevel,
                   XtNminWidth, width,
                   XtNminHeight, height + theight,
                   ( String ) 0 );

    if ( isSystemSappingAType( 0x3E4 ) != NULL )
    {
        CopyInterStr( TXT_NO_SAPPING, &errorStr, 0 );
        displayErrorMsg( widg.footerStr, errorStr );
        XtFree( (XtPointer) errorStr );
        errorStr = NULL;
    }
    /*
     * Setup escape overrides so that we are Motif compliant
     */
    XtAppAddActions (appContext, actions, XtNumber (actions));
    translations = XtParseTranslationTable(transTable);
    XtOverrideTranslations(widg.appList, translations);
    XtOverrideTranslations(widg.buttons, translations);

    translations = XtParseTranslationTable("#override <Key>Return: ReturnCB()");
    XtOverrideTranslations(widg.buttons, translations);

    translations = XtParseTranslationTable("#override <Key>F1: HelpCB()");
    XtOverrideTranslations(widg.appList, translations);
    XtOverrideTranslations(widg.buttons, translations);

    XtAppMainLoop( appContext );
}


/*--------------------------------------------------------------------
** Function : BuildAppsItemList   
**
** Description : This function is used to build an item list for each
**               file in the SHARED_APPS_DIR directory. 
**
** Parameters : itemList  **items - The item list is built and stored
**                                  in this variable.
**
** Return : The number of files found
**------------------------------------------------------------------*/
int BuildAppsItemList( itemList **items, Pixmap *icon )
{
    DIR           *Dp;
    struct dirent *dirPtr;  
    int           i = 0;
    itemList      *temp;
    unsigned char *filePath;
    Arg            args[1];

    Dp = opendir( SHARED_APPS_DIR );
    if ( Dp == NULL )
        return( 0 );
    for ( dirPtr = readdir( Dp ); dirPtr != NULL; dirPtr = readdir( Dp ) )
       if ( ( strcmp( dirPtr->d_name, "." ) != 0 )           &&
            ( strcmp( dirPtr->d_name, ".." ) != 0 )          && 
            ( strcmp( dirPtr->d_name, ".exportTemp" ) != 0 ) &&
            ( strcmp( dirPtr->d_name, ".exportUTemp" ) != 0 ) )
                i++;
    closedir( Dp );
    
    *items = ( itemList * ) XtMalloc( i * ( sizeof( itemList ) ) ); 
    temp =  *items;

    Dp = opendir( SHARED_APPS_DIR );
    for ( dirPtr = readdir( Dp ); dirPtr != NULL; dirPtr = readdir( Dp ) )
    {
       if ( ( strcmp( dirPtr->d_name, "." ) != 0 )           &&
            ( strcmp( dirPtr->d_name, ".." ) != 0 )          &&
            ( strcmp( dirPtr->d_name, ".exportTemp" ) != 0 ) &&
            ( strcmp( dirPtr->d_name, ".exportUTemp" ) != 0 ) )
        {
            temp->itemPtr = ( Item * )XtMalloc( sizeof( Item ) );
            temp->itemPtr->name = ( unsigned char * ) 
                              XtMalloc( strlen( dirPtr->d_name ) + 1 );
            strcpy( ( char * )temp->itemPtr->name, dirPtr->d_name );
            memcpy( &temp->itemPtr->icon, icon, sizeof ( Pixmap ) );

            filePath = ( unsigned char * )XtMalloc( strlen( SHARED_APPS_DIR ) + 
                                 strlen( dirPtr->d_name ) + 1 + 1 );
            sprintf( ( char * )filePath, "%s/%s", 
                    SHARED_APPS_DIR, dirPtr->d_name ); 
            AddType(( unsigned char ** )&temp->itemPtr->type, filePath, FALSE );
            XtFree( ( XtPointer )filePath );
            temp++;
        }
    }
    closedir( Dp );
    SortAppsItemList( items, i );
    return( i );
}


/*--------------------------------------------------------------------
** Function : FileDropProc   
**
** Description : This function is called when an item is dropped on the
**               "Shared Applications" window.
**
** Parameters : 
**
** Return : TRUE
**------------------------------------------------------------------*/
Boolean   FileDropProc ( Widget w, Window win, Position x, Position y,
                        Atom selection, Time timestamp, 
                        OlDnDDropSiteID dropSiteID, 
                        OlDnDTriggerOperation operation,
                        Boolean    send_done,
                        Boolean    forwarded,
                        XtPointer  closure )
{
    Boolean  retCode;

    retCode = ( int ) DtGetFileNames( w,
                              selection,
                              timestamp,
                              send_done,
                              DoneProc,
                              NULL );
    return( TRUE );
}


/*--------------------------------------------------------------------
** Function : DoneProc   
**
** Description : This function is called by FileDropProc above.
**
** Parameters : As per callback functions 
**
** Return : None 
**------------------------------------------------------------------*/
void DoneProc( Widget w, XtPointer client_data, XtPointer call_data )
{
    DtDnDInfoPtr       dndInfo = ( DtDnDInfoPtr ) call_data;
    int                i;
    unsigned char     *fileName;
    struct stat        buf;
    int                retCode;
    int                appCnt = 0;
    unsigned char     *errorStr = NULL;
    itemList          *temp;
    Arg                args[1];

    for ( i=0, fileName = ( unsigned char * )dndInfo->files[i]; 
          i < dndInfo->nitems; 
          i++, fileName = ( unsigned char * )dndInfo->files[i] )
    {
        retCode = stat( ( char * )fileName, &buf );
        if ( buf.st_mode & S_IFDIR )
        {
            appCnt = AddDirectory( fileName );
            if ( appCnt )
            {
                if ( isSystemSappingAType( 0x3E4 ) != NULL )
                {
                    CopyInterStr( TXT_NO_SAPPING, &errorStr, 0 );
                    displayErrorMsg( widg.footerStr, errorStr );
                    XtFree( (XtPointer) errorStr );
                    errorStr = NULL;
                }
                goto UPDATE_AND_EXIT;
            }
            else
            {
                CopyInterStr( TXT_NO_FILES_COPIED, &errorStr, 1, fileName );
                XtVaSetValues( widg.footerStr, 
                               XtNstring, errorStr,
                               ( String ) 0 );
                goto EXIT_DONE_PROC;
            }
        }
        else if ( IsExec( buf.st_mode ) )      
        {
            CreateScriptFile( fileName );
            if ( isSystemSappingAType( 0x3E4 ) != NULL )
            {
               CopyInterStr( TXT_NO_SAPPING, &errorStr, 0 );
               displayErrorMsg( widg.footerStr, errorStr );
               XtFree( (XtPointer) errorStr );
               errorStr = NULL;
            }
            CopyInterStr( TXT_APPL_COPIED, &errorStr, 1, fileName );
            XtVaSetValues( widg.footerStr, 
                           XtNstring, errorStr,
                           ( String ) 0 );
            goto UPDATE_AND_EXIT;
        }
        else  
        {
            CopyInterStr( TXT_APP_NOT_EXE, &errorStr, 1, fileName );
            XtVaSetValues( widg.footerStr, 
                           XtNstring, errorStr,
                           ( String ) 0 );
        }
    }
UPDATE_AND_EXIT:
    FreeItemList( &items, numItems );
    numItems = BuildAppsItemList( &items, shApplIcon );    
    if ( numItems != 0 )
        enableRemoteAppsSAP();
    else 
        disableRemoteAppsSAP();
    XtVaSetValues( widg.appList,
                   XtNitems, items,
                   XtNnumItems,numItems,
		   XtNitemsTouched, TRUE,
                   ( String ) 0 );
   if ( applicationName != NULL )
       for ( i = 0, temp = items; i < numItems; i++, temp++ ) 
       {
           if ( strcmp( applicationName, ( char * )temp->itemPtr->name ) == 0 )
           { 
               XtSetArg( args[0], XtNset, TRUE );
               OlFlatSetValues( widg.appList, i, args, 1 );
               XtSetArg( args[0], XtNsensitive, TRUE );
               OlFlatSetValues( widg.buttons, 0, args, 1 );
               OlFlatSetValues( widg.buttons, 1, args, 1 );
               OlFlatSetValues( widg.buttons, 3, args, 1 );
/*
               XtVaSetValues( widg.appList, 
                              XtNviewItemIndex, i,
                              ( String ) 0 );*/
               break;
           }
       }
EXIT_DONE_PROC:
    if ( errorStr != NULL )
        XtFree( ( XtPointer )errorStr );
}


/*--------------------------------------------------------------------
** Function : AddDirectory    
**
** Description : This function is get all the executables from a 
**               directory. It also creates a script file for each
**               application.
**
** Parameters : unsigned char *dirName - directory to get files from.  
**
** Return : The number of executables in the directory  
**------------------------------------------------------------------*/
int AddDirectory( unsigned char *dirName )
{
    DIR           *Dp;
    struct dirent *dirPtr;  
    struct stat    buf;
    unsigned char *fileBuf;
    int            retCode;
    int           appCnt = 0;

    Dp = opendir( ( char * )dirName );
    for ( dirPtr = readdir( Dp ); dirPtr != NULL; dirPtr = readdir( Dp ) )
    {
        fileBuf = ( unsigned char * ) XtMalloc( strlen( ( char * )dirName ) + 
                                         strlen( dirPtr->d_name ) + 1  + 1 );
        sprintf( ( char * )fileBuf, "%s/%s", dirName, dirPtr->d_name );
        retCode = stat( ( char * )fileBuf, &buf );
        if ( (retCode == 0)               &&
             (!( buf.st_mode & S_IFDIR )) &&
             (IsExec( buf.st_mode )) ) 
        {
            CreateScriptFile( fileBuf );
            appCnt++;
        }
        XtFree( ( XtPointer )fileBuf );
    }
    closedir( Dp );
    return( appCnt );
}


/*--------------------------------------------------------------------
** Function : CreateScriptFile
**
** Description : This function creates a new file by copying the 
**               contents of the template file.
**
** Parameters : char *filePath - Pathname of the file to create
**
** Return :
**------------------------------------------------------------------*/
int CreateScriptFile( unsigned char *filePath )
{
    unsigned char       *fileName        = NULL;
    unsigned char       *fileBuf         = NULL;
    unsigned char       *line            = NULL; 
    FILE                *applFD          = NULL;
    FILE                *templFD         = NULL;
    int                  i;
    Boolean              XAppFlag;
    unsigned char       *temp;
    unsigned char       *save;
    char                *tempFmt         = "\"%s\"\n";
    unsigned char       *errStr          = NULL;

    /*------------------------------------------
    ** First, strip off the path portion of the 
    ** filePath to get the actual filename
    **----------------------------------------*/
    for ( i=0; filePath[i] != '\0'; i++ )
        if ( filePath[i] == '/'  && filePath[i+1] != '\0' )
            fileName = &filePath[i];
    fileBuf = ( unsigned char * ) XtMalloc( strlen( SHARED_APPS_DIR ) +
                                 strlen( ( char * )fileName ) + 1 + 1 );
    sprintf( ( char * )fileBuf, "%s/%s", SHARED_APPS_DIR, fileName );
    /*--------------------------------------------------
    ** This code executes the ldd command to check and
    ** see if the X dynamic link libararies have been
    ** linked in. If they have we consider the app. an
    ** X application, otherwise we assume its a text
    ** based app. Not completely fool proof.
    **------------------------------------------------*/
    XAppFlag = IsXApp( filePath );       

    applFD = fopen( ( char * )fileBuf, "w" ); 
    if ( applFD != NULL )
    {
        templFD = fopen( TEMPLATE_FILE, "r" );
        if ( templFD != NULL )
        {
            line = ( unsigned char * ) XtMalloc( MAX_LINE_LEN + 1 );
            while ( ( fgets( ( char * )line, MAX_LINE_LEN, templFD ) ) != NULL )
            { 
                if ( ( temp = ( unsigned char * )
                     strstr( ( char * )line, appTypeLabel ) ) != NULL )
                { 
                    if (  XAppFlag == TRUE )
                    {
                        sprintf( ( char * )temp, tempFmt, XApplication );
                    }
                    else
                        sprintf( ( char * )temp, tempFmt, TApplication );
                }
                else if ( ( ( 
                  temp = ( unsigned char * )
                    strstr( ( char * )line, appLabel ) ) != NULL ) &&
                  ( line[0] != '#' ) )
                {
                      save = ( unsigned char * ) XtMalloc( MAX_LINE_LEN + 1 );
                      memcpy( save, line, temp - line ); 
                      sprintf( ( char * )save + ( temp - line ),
                               "\"%s\"", ( char * )filePath );
                      sprintf(( char * )save + strlen( ( char * )save),
                               "%s", 
                               temp + strlen( ( char * )appLabel ) );
                      strcpy( ( char * )line, ( char * )save );
                      XtFree( ( XtPointer )save );
                }
                fwrite( line, 1, strlen( ( char * )line ), applFD ); 
            }
            if ( applicationName != NULL )
                XtFree( applicationName );
            applicationName = XtMalloc( strlen( ( char * )fileName ) + 1 );
            strcpy( applicationName, ( char * )&fileName[1] ); 
        }
        else
        {
             CopyInterStr( TXT_NO_TEMP_ERR, &errStr, 0 ); 
             displayErrorMsg( widg.appCaption, errStr );
        }         
    }
    else
    {
             CopyInterStr( TXT_CANT_CREATE_SCRIPT, &errStr, 0 ); 
             displayErrorMsg( widg.appCaption, errStr );
    }
EXIT_CREATE_SCRIPT:
    if ( templFD ) 
        fclose( templFD );
    if ( applFD ) 
        fclose( applFD );
    if ( fileBuf ) 
    { 
        chmod( ( char * )fileBuf, S_IRWXU | S_IXGRP | S_IRGRP | S_IXOTH | S_IROTH ); 
        XtFree( ( XtPointer )fileBuf );
    }
    if ( line != NULL )
        XtFree( ( XtPointer ) line );
    if ( errStr != NULL )
        XtFree( ( XtPointer )errStr );
}


/*--------------------------------------------------------------------
** Function : FreeItemList 
**
** Description : This function frees all the data assoc. with an
**               itemList
**
** Parameters : itemArray - itemList to free
**              cntItems  - number of items in list
**
** Return : None
**------------------------------------------------------------------*/
void FreeItemList( itemList **itemArray, int cntItems )
{
    itemList     *temp; 
    int          i;

    for ( i=0, temp = *itemArray; 
          i < cntItems; 
          i++, temp++ )
    {
        XtFree( ( XtPointer )temp->itemPtr->name );
        XtFree( ( XtPointer )temp->itemPtr->type );
        XtFree( ( XtPointer )temp->itemPtr );
    }
}



/*--------------------------------------------------------------------
** Function : ResizeDropSite
**
** Description : This function is called when the Shared Application
**               window is resized
**
** Parameters :
**
** Return : None
**------------------------------------------------------------------*/
void ResizeDropSite( Widget w, XtPointer clientData, 
                     XEvent *event, Boolean *continueToDispatch )
{
    OlDnDSiteRect      dropSiteRect;
    Position           x;
    Position           y;
    Dimension          width;
    Dimension          height;
    Arg                args[4];

    XtSetArg( args[0], XtNx, &x );
    XtSetArg( args[1], XtNy, &y );
    XtSetArg( args[2], XtNheight, &height );
    XtSetArg( args[3], XtNwidth, &width );
    XtGetValues( widg.appCaption, args, 4 );
    dropSiteRect.x      = x;
    dropSiteRect.y      = y;
    dropSiteRect.width  = width;
    dropSiteRect.height = height;  
    OlDnDUpdateDropSiteGeometry( dropSiteID, 
                                 &dropSiteRect,
                                 1 );
}

/*--------------------------------------------------------------------
** Function : AddType
**
** Description : This function adds the type string to a file
**
** Parameters : type     - Determine, allocate, and return the
**                         type field in this parameter.
**              filePath - file to search
**
** Return :
**------------------------------------------------------------------*/
int AddType( unsigned char **type, unsigned char *filePath, Boolean changeType )
{
      int             fileType;
      FILE           *fd = NULL;
      FILE           *fd2 = NULL;
      Boolean         foundFlag    = FALSE;
      char           *line         = NULL;
      char           *tempFile     = NULL;
      char           *command      = { "mv %s %s" };
      char           *commandLine;
      unsigned char  *errStr;


      fd = fopen( ( char * )filePath, "r" );
      if ( fd == NULL )
          goto EXIT_ADDTYPE;
      if ( changeType == TRUE ) 
      {
          GenRandomTempFName( &tempFile );
          mktemp( tempFile );
          fd2 = fopen( tempFile, "w" );
          if ( fd2 == NULL )
          { 
             CopyInterStr( TXT_CHG_TYPE_ERR, &errStr, 0 ); 
             displayErrorMsg( widg.appCaption, errStr );
             goto EXIT_ADDTYPE;
          }
      }
      line = XtMalloc( MAX_LINE_LEN );      
      while ( ( fgets( line, MAX_LINE_LEN - 1, fd ) ) != NULL ) 
      { 
         if ( strstr( line, Remappl ) != NULL  && strstr( line, "=" ) != NULL ) 
         { 
             if ( strstr( line, XApplication ) != NULL ) 
             {
                 if ( changeType == TRUE ) 
                 { 
                     sprintf( line, "%s=\"%s\"\n", Remappl, TApplication );
                     fwrite( line, 1, strlen( line ), fd2 );
                     CopyInterStr( TXT_T_APP, type, 0 ); 
                 } 
                 else
                     CopyInterStr( TXT_X_APP, type, 0 ); 
                 foundFlag = TRUE;
             } 
             else if ( strstr( line, TApplication ) != NULL )
             { 
                 if ( changeType == TRUE ) 
                 { 
                     sprintf( line, "%s=\"%s\"\n", Remappl, XApplication );
                     fwrite( line, 1, strlen( line ), fd2 );
                     CopyInterStr( TXT_X_APP, type, 0 ); 
                 }
                 else
                     CopyInterStr( TXT_T_APP, type, 0 ); 
                 foundFlag = TRUE;
             } 
         }  
         else if ( changeType == TRUE ) 
            fwrite( line, 1, strlen( line ), fd2 );
      } 
      fclose( fd );

EXIT_ADDTYPE:
    if ( foundFlag == FALSE ) 
         CopyInterStr( TXT_UNKNOWN_APP, type, 0 );
    if ( line != NULL ) 
        XtFree( line );
    if ( fd )
       fclose ( fd );
    if ( changeType == TRUE && fd2 != NULL )
    { 
	fclose( fd2 );
        chmod( ( char * )tempFile, S_IRWXU | S_IXGRP | S_IRGRP | S_IXOTH | S_IROTH ); 
        commandLine = XtMalloc( strlen( command ) + 
                                strlen( ( char * )filePath ) +
                                strlen( tempFile ) + 1 );
        sprintf( commandLine, command, tempFile, filePath );
        system( commandLine );   
        XtFree( commandLine );
        chmod( ( char * )filePath, S_IRWXU | S_IXGRP | S_IRGRP | S_IXOTH | S_IROTH ); 
    } 
    if ( tempFile )
    {
       unlink( tempFile );
       XtFree( ( XtPointer )tempFile );
    }
}

        
/*--------------------------------------------------------------------
** Function : ErrorChecking 
**
** Description : This function checks for error conditions that are
**               catasrophic, and displays the appropriate message. This 
**               function will not return if such an error is detected.
**
** Parameters : None
**
** Return : None
**------------------------------------------------------------------*/
void ErrorChecking( XtAppContext any_context )
{
    char         *errBuf = NULL;
    struct stat   buf;

    if ( chdir( "/tmp" ) == -1 )
        CopyInterStr( TXT_CANT_CHDIR, &errBuf, 0 );
    else if ( stat( SHARED_APPS_DIR, &buf ) == -1 )
       CopyInterStr( TXT_NO_SHARED_APPS_DIR, &errBuf, 0 );
    /*
     * Is user in the tfadmin database for this application?
     * ( /sbin/tfadmin -t AppName )
     */
    else if ( !_DtamIsOwner("App_Sharing") )
       CopyInterStr( TXT_NOT_PRIV, &errBuf, 0 );
    else
    {
        priv_t  buff[NPRIVS *2];
        int     count;

        /*
         * User was in tfadmin database, get users privilege set.
         * Privileges are given when app is envoked by tfadmin
         */
        count = procpriv(GETPRV, buff, NPRIVS * 2);
        if ( count <= 0 )
        {
            /* No privs or command failed, error out */
            CopyInterStr( TXT_NOT_PRIV, &errBuf, 0 );
        }
        else
        {
            int  isSetUidPriv = False;
            int  i;

            /*
             * We have privileges now check for the setuid privilege
             * Specific to this application
             */
	        for (i = 0; i < count; i++ )
            {
                if ( (P_SETUID | PS_WKG ) == buff[i] )
                {
                    isSetUidPriv = True;
                    break;
                } 
            }
            if ( isSetUidPriv == False )
                CopyInterStr( TXT_NOT_PRIV, &errBuf, 0 );
        }
    }
    
    if ( errBuf != NULL )
    {
       buttonItems   *okButton;
       char          *temp;
       Widget        buttons;
       Arg           args[2];


       okButton = ( buttonItems * ) XtMalloc( sizeof( buttonItems ) );
       CopyInterStr( okButtonItems->label, &temp, 0 );
       okButton->label = temp;
       CopyInterStr( okButtonItems->mnemonic, &temp, 0 );
       okButton->mnemonic = ( XtPointer ) temp[0];
       okButton->select = okButtonItems->select;
       okButton->sensitive = okButtonItems->sensitive;
       widg.appRubberTile = XtVaCreateManagedWidget( "appRubberTile",
                                              rubberTileWidgetClass,
                                              widg.toplevel,
                                              XtNorientation, OL_VERTICAL,
				              XtNsetMinHints, FALSE,
                                              ( String ) 0 );
       XtVaCreateManagedWidget( "Error Widget",
                                 staticTextWidgetClass,
                                 widg.appRubberTile,
                                 XtNstring, errBuf,
                                 XtNalignment, OL_CENTER,
                                 ( String ) 0 );
       buttons = XtVaCreateManagedWidget( "buttons",
                                 flatButtonsWidgetClass,
                                 widg.appRubberTile,
                                 XtNitems, okButton,
                                 XtNnumItems, 1,
                                 XtNitemFields, buttonFields,
                                 XtNnumItemFields, numButtonFields,
                                 XtNnoneSet, TRUE,
                                 XtNdefault, FALSE,
                                 ( String ) 0 );
       XtSetArg( args[0], XtNdefault, TRUE );
       OlFlatSetValues( buttons, 0, args, 1 );
       XtRealizeWidget( widg.toplevel );
       XtAppMainLoop( any_context );
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
