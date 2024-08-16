/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)remapps:ra_getlists.c	1.3"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Remote_Applications/ra_getlists.c,v 1.4 1994/08/15 20:43:02 plc Exp $"

/*--------------------------------------------------------------------
** Filename : dl_getlists.c
**
** Description : This file contains functions to get the server list,
**               and functions to get the second list.
**
** Functions : GetServerList
**             GetXApplList
**             GetServerName
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                          I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <ra_saptypes.h>

#include <sys/types.h>
#include <sys/stat.h>  
#include <sys/uio.h>
#include <sys/utsname.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "ra_hdr.h"

#include <errno.h>

#define BUFSIZE 1024

/*--------------------------------------------------------------------
** Function : GetServerList
**
** Description : This function gets a list of servers that are 
**               advertising with the Novell SAP (Service Advertising
**               Protocol). 
**
** Parameters : int SAPType - build a list for this service type
**              nameList **serverList - build server name list here
**
** Return : 0 on Success
**          Error code on failure
**------------------------------------------------------------------*/
int GetServerList ( int SAPType, nameList **serverList )
{   
     nameList        *temp   = NULL;
     nameList        *save   = NULL;
     char            *line   = NULL;
     int              retCode;
     int              i;
     struct utsname   name;
     static char     *xHostName = NULL;
     char **tempServerList = NULL;
     int serverCnt;
     char            *rc   = NULL;

     *serverList = NULL;

     if ( xHostName == NULL )
     {
         if ( ( uname( &name ) ) != -1 )
         {
             char *ptr;
             xHostName = XtMalloc( strlen( name.nodename ) + 1 );
             strcpy( xHostName, name.nodename );
             for( ptr = xHostName; *ptr != '\0'; ptr++ )
                 *ptr = toupper( *ptr );
         }
     }

     if ( isSystemSappingAType( PERSONAL_EDITION ) != NULL )
         goto EXIT_GETSERVERLIST;
     if ((rc = getSappingServerList(SAPType, &tempServerList, &serverCnt)) != NULL )
     {
         goto EXIT_GETSERVERLIST;
     }
     for(i=0;i<serverCnt;i++ )
     {
         line = tempServerList[i];
         if ( strcoll( line, xHostName ) == 0 )
             continue;
         if ( strlen( line ) == 0 )
         {
             *serverList = NULL;
             goto EXIT_GETSERVERLIST;
         }
         if ( temp == NULL )
         {
             if ( ( temp = (nameList *) XtMalloc (sizeof (nameList ))) == NULL )
                 goto EXIT_GETSERVERLIST;
             *serverList = temp; 
         }

         temp->name = ( unsigned char * ) XtMalloc( strlen( line ) + 1 );
         if ( temp->name == NULL )
         {
             *serverList = NULL;
             goto EXIT_GETSERVERLIST;
         }
         strcpy( ( char * )temp->name, line );
         temp->next = (nameList * ) XtMalloc (sizeof (nameList ));
         if ( temp->next == NULL )
         {
            *serverList = NULL;
            goto EXIT_GETSERVERLIST;
         }
         save = temp;
         temp = temp->next;
     }

EXIT_GETSERVERLIST:
    if ( tempServerList ) 
        XtFree( ( XtPointer )tempServerList );
    if ( temp ) 
        XtFree( ( XtPointer )temp );
    if ( save ) 
    {
       XtFree( ( XtPointer )save->next );
       save->next = NULL;
    }
    return(0);
}

/*--------------------------------------------------------------------
** Function : GetXApplList 
**
** Description : This function returns a list of exported X applications
**               for the selected server.
**
** Parameters : char *serverName - get a list of X Applications for this
**                                 server.
**              nameList **applList - X application list built here
**
**
** Return : 0 on Success
**          Error code on failure
**------------------------------------------------------------------*/
int GetXApplList ( char *serverName, Pixmap *icon ) 
{
   char          *passwd       = NULL;
   char          *userName     = NULL;

   userName = XtMalloc( L_cuserid );
   userName = cuserid( userName );
   if ( userName[0] == '\0' )
   {
       XtFree( userName );
       userName = NULL;
   }
   BuildLoginMsg( widg.serverWin, serverName, &userName, &passwd, icon );
}


/*--------------------------------------------------------------------
** Function : BuildXApplList 
**
** Description : This function returns a list of exported X applications
**               for the selected server.
**
** Parameters : char *serverName - get a list of X Applications for this
**                                 server.
**              nameList **applList - X application list built here
**
**
** Return : 0 on Success
**          Error code on failure
**------------------------------------------------------------------*/
int BuildXApplList ( char *serverName, nameList **applList, 
                     char **errorStr, char *userName, char *passwd ) 
{
   int            retCode      = SUCCESS;
   int            cCode;
   int            i;
   struct stat    errorStat;
   char          *commandLine  = NULL;
   char          *line         = NULL;
   nameList      *temp         = NULL;
   
   nameList      *save         = NULL;
   Boolean        hasApps      = FALSE;
   int            ioFD         = -1;
   int            errFD        = -1;
   int            localErrFD   = -1;
   char          *fileName = NULL;
   int            charsRead;
   char          *start;
   
   

   static char *command  = { "ls -F /usr/X/lib/app-defaults/.exportApps" };

   close( 2 );
   GenRandomTempFName( &fileName );
   mktemp( fileName ); 
   localErrFD = creat( fileName, O_RDWR );

   temp = *applList = ( nameList * ) XtMalloc ( sizeof ( nameList ) ); 

   ioFD = rexec( &serverName, REXEC_SOCKET, userName, 
                 passwd, command, &errFD );
   if ( ioFD == -1 )
   {
      int charsread;

      retCode = FAILURE;
      *errorStr = XtMalloc( MAX_REXEC_ERR );
      close( localErrFD );
      localErrFD = open( fileName, O_RDONLY );
      charsread = read( localErrFD, *errorStr,  MAX_REXEC_ERR - 1 );
      if ( charsread == -1 )
      {
         XtFree( *errorStr );
         CopyInterStr( TXT_UNABLE_TO_CONNECT, errorStr, 1, serverName );
      }
      else
          (*errorStr)[charsread - 1] = '\0';
      goto EXIT_GETAPPLLIST;
   }

   line = XtMalloc( BUFSIZE );
   while ( ( charsRead = read( ioFD, line, BUFSIZE ) ) > 0 )
      for ( i = 0, start = line; charsRead > 0; charsRead--, i++ )
      {
        if ( line[i] == '*' )
        {
             hasApps = TRUE;
             line[i] = '\0';
             temp->name = ( unsigned char * ) XtMalloc( strlen( start ) + 1 );
             strcpy( ( char * )temp->name, start );
             temp->next = ( nameList * ) XtMalloc( sizeof ( nameList ) ) ;
             save = temp;
             temp = temp->next;
        }
        else if ( line[i] == '\n' )
            start = &line[i + 1];
      }
   *errorStr = XtMalloc( MAX_REXEC_ERR );
   i = 0;
   while( ( read( errFD, &((*errorStr)[i]), 1 ) ) != 0 )
       i++;
   if ( i == 0 )
       *errorStr = NULL;
   else
       (*errorStr)[i] = '\0';
   
EXIT_GETAPPLLIST: /* Cleanup and head for home */
   if ( hasApps == FALSE && *errorStr == NULL )
   { 
      CopyInterStr( TXT_NO_APPLICATIONS, errorStr, 1, serverName );
      retCode = NO_APPS_FOUND;     
   }
  if ( temp != NULL )
     XtFree ( ( XtPointer )temp );
  if ( hasApps == FALSE )
     *applList = NULL;
  if ( save != NULL )
     save->next = NULL;
  if ( commandLine )
     XtFree( commandLine );
  if ( line )
     XtFree( line );
  if ( fileName )
     XtFree( fileName );
  if ( ioFD != -1 )
     close( ioFD );
  if ( errFD != -1 )
     close( errFD );
  if ( localErrFD != -1 )
     close( localErrFD );
  unlink( fileName );
  return( retCode );
}


/*--------------------------------------------------------------------
** Function : GetServerName
**
** Description : This function takes a line from a SAP file and NULL
**               terminates the line after the Server name. The 
**               string passed back effectively contains just the 
**               server name.
**
** Parameters : char *str - buffer containing a line from a SAP file.
**
** Return :
**------------------------------------------------------------------*/
void GetServerName( char *str )
{
    while( !isspace( *str ) )
        str++;
    *str = '\0';
}


