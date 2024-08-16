/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:prspath2.c	1.2"
#ifdef N_PLAT_OS2
# include <os2def.h>
# include <bsedos.h>
#endif

#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwintern.h"
#include "nwundoc.h"
#include "nwdpath.h"
#include "nwserver.h"
#include "nwconnec.h"
#include "nwerror.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwclocal.h"
#include "nwdirect.h"

#define IsSlash(c)  (c == '/' || c == '\\')

/* Static Functions */


/*manpage*NWParsePath*******************************************************
SYNTAX:  NWCCODE N_API NWParsePathConnRef
         (
            pnstr8 path,
            pnstr8 serverName,
            pnuint32 connRef,
            pnstr8 volName,
            pnstr8 dirPath
         )

REMARKS: Parses the given path and returns network information
         associated with that path.

         The path specification can be any of:

           <drive>:path
           <vol>:path
           <server><vol>:path - the information is copied to the return buffers
           path - the current drive is used to determine all the information

ARGS: >  path
         The input path to be parsed

      <  serverName
         The server name on which the path resides

      <  conn
         The connection ID of path

      <  volName
         The volume name on which the path resides

      <  dirPath
         The directory path relative to the volume

INCLUDE: nwdpath.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: 21 Aug 1994 - Written - docox - Changed call to return connection
         reference instead of connection handle.
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWParsePathConnRef
(
   pnstr8   path,
   pnstr8   serverName,
   pnuint32 connRef,
   pnstr8   volName,
   pnstr8   dirPath
)
#if defined( N_PLAT_UNIX ) || defined( N_PLAT_NLM )
{
   pnstr8 tempPtr;
   nstr8 tmpPath[256];
   nstr8 tmpServer[49];
   nstr8 tmpVolume[17];
   NWCONN_HANDLE  tmpConnectionID = 0xFF;
   nuint16  serverLength,
            volLength,
            startServer,
            startVolume,
            startPath;
   nuint8   ccode;
   NWCCODE  retcode = 0;


   (void) NWCStrCpy(tmpPath, path);

/*
** Make all slashes a '\'
*/
   for(tempPtr = tmpPath; *tempPtr; tempPtr = NWNextChar(tempPtr))
   {
      if(*tempPtr == '/')
         *tempPtr = '\\';
   }

   ccode = IndexPath((pnstr8)tmpPath, &startServer, &serverLength,
                 &startVolume, &volLength, &startPath );
   if(ccode != 0)
      return( INVALID_PATH );

   if((serverLength > 48) || (volLength > 16))
      return( INVALID_PATH );


/*
** Check for a server and a volume specification
*/
   if(serverLength > (nuint16)0 || volLength > (nuint16)0)
   {
      if(serverLength > (nuint16)0)
      {
         (void) NWCMemCpy((char *)tmpServer, (char *)(tmpPath + startServer),
                     serverLength);
         tmpServer[serverLength] = '\0';
         retcode=NWGetConnectionID((pnuint8)tmpServer, NULL, &tmpConnectionID, NULL);
         if(retcode)
         {
            return((nuint16)retcode);
         }
      }
      if(volLength > (nuint16)0)
      {
         (void) NWCMemCpy((char *)tmpVolume, (char *)(tmpPath + startVolume),
                     volLength);
         tmpVolume[volLength] = '\0';
      }
      if(dirPath)
         (void) NWCStrCpy((char *)dirPath, (char *)(tmpPath + startPath));
   } else {
      /*
       **   There is either a drive letter or a path off of a default drive.
       **   Neither concept is supported for NWU, so we return an INVALID_PATH
       **   error.
       */
      return( INVALID_PATH );
   }

   if(dirPath)
   {
      ccode = CleanPath(dirPath);
      if(ccode)
         return( INVALID_PATH );
   }
   if(serverName != NULL)
   {
      (void) NWCStrCpy((char *)serverName, (char *)tmpServer);
   }
   if(volName != NULL) {
      (void) NWCStrCpy((char *)volName, (char *)tmpVolume);
   }
   if(connRef != NULL)
      *connRef = tmpConnectionID;

   return(0);
}
#else
{
   pnstr8   charPtr, tempPtr, tmp;
   nstr8    curDir[256], tPath[256], tSrv[49], tVol[17];
   NWDIR_HANDLE dirHandle;
   nuint16  suDrvNum, suSrvLen, suVolLen, i, suStartServer,
            suStartVolume, suStartPath;
   NWCCODE  ccode;
   nuint32  tConnRef;
   nuint32  scanIndex = 0;
#ifdef N_PLAT_OS2
   nuint16  suPathLen = 255;
#endif
   NWCDeclareAccess(access);

   NWCStrCpy(tPath, path);
   for(tempPtr = tPath; *tempPtr; tempPtr = NWNextChar(tempPtr))
   {
      if(*tempPtr == '/')
         *tempPtr = '\\';
   }

   ccode = IndexPath(tPath, &suStartServer, &suSrvLen, &suStartVolume,
                  &suVolLen, &suStartPath);
   if(ccode != 0)
      return ccode;

   if((suSrvLen > 48) || (suVolLen > 16))
      return(INVALID_PATH);

   if(suSrvLen > 0 && suVolLen > 0)
   { /* There is a server and volume name */

      NWCMemMove(tSrv, (tPath + suStartServer), suSrvLen);
      tSrv[suSrvLen] = '\0';
      NWCMemMove(tVol, (tPath + suStartVolume), suVolLen);
      tVol[suVolLen] = 0;
      if(dirPath)
         NWCStrCpy(dirPath, (tPath + suStartPath));

      tConnRef = scanIndex = 0;
      ccode = (NWCCODE)NWCScanConnInfo(&access, &scanIndex, NWC_CONN_INFO_SERVER_NAME,
         suSrvLen, (pnuint8)&tSrv,
         NWC_MATCH_EQUALS | NWC_RETURN_LICENSED | NWC_RETURN_UNLICENSED,
         NWC_CONN_INFO_CONN_REF, sizeof(nuint32), &tConnRef, &tConnRef);
   }
   else if(suVolLen > 1)
   {           /* The volume name is specified */
      for(tempPtr = &tPath[suStartVolume], i = 0;
            *tempPtr == '.' || *tempPtr == '\\' || *tempPtr == '/';
            tempPtr = NWNextChar(tempPtr), i++)
         ;

      NWCMemMove(tVol, tempPtr, suVolLen - i);
      tVol[suVolLen - i] = 0;
      if(dirPath)
         NWCStrCpy(dirPath, (tPath + suStartPath));
      if((ccode = NWGetDefaultConnRef(&tConnRef)) == 0)
      {
         scanIndex = 0;
         ccode = (NWCCODE)NWCScanConnInfo(&access, &scanIndex, NWC_CONN_INFO_CONN_REF,
            sizeof(nuint32), &tConnRef,
            NWC_MATCH_EQUALS | NWC_RETURN_LICENSED | NWC_RETURN_UNLICENSED,
            NWC_CONN_INFO_SERVER_NAME, sizeof(tSrv), &tConnRef, &tSrv);
      }
   }
   else
   { /* there is a drive letter or the path off of the default drive */

      nuint16 rootPath = 0;

      curDir[0] = (char) 0;                    /* init current directory         */

      if(suStartPath == 2)
      {                                  /* There is a drive letter */
         suDrvNum = (nuint16)*tPath;
         if(suDrvNum >= (nuint16)'a' && suDrvNum <= (nuint16)'z')
            suDrvNum -= (nuint16)('a' - 'A');
         suDrvNum -= (nuint16)('A' - 1);

         if(tPath[2] == '\\')              /* is this path relative to root  */
         {
            rootPath = 1;
            NWCStrCpy(&tPath[2], &tPath[3]);   /* remove the '\' */
         }
      }
      else
      {
         suDrvNum = __NWGetCurrentDrive();
         if(*tPath == '\\')                /* is this path relative to root  */
            rootPath = 1;
      }

      if(dirPath)
         *dirPath = 0;

      tSrv[0] = (char) 0;
      tVol[0] = (char) 0;
      tConnRef   = 0;

      ccode = NWGetDrivePathConnRef(suDrvNum, MY_SESSION, &tConnRef, curDir, NULL);
      if(ccode == NOT_MY_RESOURCE)
      {
         /* valid local drive */
         ccode = 0;
         tVol[0] = (char)(suDrvNum + 'A' - 1);
         tVol[1] = '\0';
         if(!rootPath)
            __NWGetCurrentDirectory(suDrvNum, curDir);
         else
            curDir[0] = (char) 0;
      }
      else if(ccode == 0)
      {
         /* Get file server name */
         scanIndex = 0;
         ccode = (NWCCODE)NWCScanConnInfo(&access, &scanIndex, NWC_CONN_INFO_CONN_REF,
            sizeof(nuint32), &tConnRef,
            NWC_MATCH_EQUALS | NWC_RETURN_LICENSED | NWC_RETURN_UNLICENSED,
            NWC_CONN_INFO_SERVER_NAME, sizeof(tSrv), &tConnRef, &tSrv);
/*         ccode = NWGetFileServerName(tConn, tSrv); */
         if(ccode == 0)
         {
            if(!rootPath)
            {
#ifdef N_PLAT_OS2
               tmp = &curDir[NWCStrLen(curDir)];
               tmp = NWPrevChar(curDir, tmp);
               if(*tmp != '\\')
               {
                  tmp = NWNextChar(tmp);
                  *tmp = '\\';
               }
               tmp = NWNextChar(tmp);
               *tmp = '\0';
#else
               tmp = curDir;
#endif

               NWGetDriveInfoConnRef(suDrvNum, MY_SESSION, &tConnRef, &dirHandle,
                        NULL, (pnstr8) tmp);
               if((*tmp == '\\') || (*tmp == '/'))
                  NWCStrCpy(tmp, tmp + 1);

            }

            for(tempPtr = (pnstr8 )curDir, i = 0;
                  *tempPtr != ':';
                  tempPtr = NWNextChar(tempPtr), i++)
               ;

            if(i <= 16)
               NWCMemMove(tVol, curDir, i);
            else
               return(INVALID_PATH);

            tVol[i] = '\0';
            i++;                  /* skip over the ':' after the vol*/
            NWCStrCpy(curDir, &curDir[i]);
         }
      }
      else
         return ccode;

      tempPtr = charPtr = &tPath[suStartPath];
      tempPtr = NWNextChar(tempPtr);
      if(dirPath)
      {
         NWCStrCpy(dirPath, curDir);
         if(!((*charPtr == '.') && (*tempPtr == 0))) /* if path is just '.' return */
         {
            NWCStrCat(dirPath, (pnstr8) "\\");
            NWCStrCat(dirPath, charPtr);
         }
      }
   }

   if(!ccode && dirPath)
      ccode = CleanPath(dirPath);

   if(serverName)
      NWCStrCpy(serverName, tSrv);

   if(volName)
      NWCStrCpy(volName, tVol);

   if(connRef)
      *connRef = tConnRef;

   return ccode;
}
#endif


/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/prspath2.c,v 1.3 1994/09/26 17:48:39 rebekah Exp $
*/

