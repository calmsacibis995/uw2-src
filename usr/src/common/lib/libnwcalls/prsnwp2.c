/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:prsnwp2.c	1.1"
#if defined (N_PLAT_WNT) || defined (N_PLAT_UNIX)

#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwintern.h"
#include "nwconnec.h"
#include "nwerror.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwclocal.h"
#include "nwdpath.h"

#define IsSlash(c)  (c == '/' || c == '\\')

/*manpage*NWParseNetWarePath************************************************
SYNTAX:  NWCCODE N_API NWParseNetWarePathConnRef
         (
            pnstr8   path,
            pnuint32 connRef,
            NWDIR_HANDLE NWPTR dirHandle,
            pnstr8   newPath
         )

REMARKS: Parses a path and returns its connHandle, dirHandle and path.

         If the path to be parsed is not complete or is relative to the
         current directory, the missing information will be obtained so
         that a complete path specification is returned.

         If the path is on a local drive, an error is returned.  If the
         path specifies a file server name and there are no connections to
         that file server, the NO_CONNECTIONS error is returned.

         NOTE:  Under OS/2 this call returns a dirHandle, and a path
                relative to it; under DOS the call returns 0 for a
                dirHandle, and fully qualified volume path (vol:path)

ARGS: >  path
      <  connRef
      <  dirHandle
      <  newPath

INCLUDE: nwdpath.h

RETURN:

CLIENT:  DOS WIN OS2 NT

SEE:

CHANGES: 21 Aug 1994 - Written - DOCOX - Changed call to return connection
         reference instead of connection number.
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWParseNetWarePathConnRef
(
   pnstr8               path,
   pnuint32             connRef,
   NWDIR_HANDLE NWPTR   dirHandle,
   pnstr8               newPath
)
#if defined(N_PLAT_UNIX) || defined(N_PLAT_NLM)
{
   nuint8      ccode;
   NWCCODE     retcode;
   nstr8       tmpPath[300];
   nuint16     startServer, serverLen, startVol, volLen, startPath;
   pnstr8      pathPtr;
   nuint32     scanIndex = 0;
   NWCDeclareAccess(access);

   *dirHandle = 0;
   (void) NWCStrCpy( (char *)tmpPath, (char *)path );

   ccode = IndexPath(tmpPath,
                     &startServer,
                     &serverLen,
                     &startVol,
                     &volLen,
                     &startPath);
    if(ccode) {
        return( INVALID_PATH );
    }

    if(startVol > (nuint16)0)
    {
/*
**  There is  a server and a volume name.  Use the server name to get
**  the connID, then put the rest of the path in path.
*/
      tmpPath[startVol - 1] = 0;
      *connRef = 0;
	  /* 
	  ** If success is not returned on the first call to NWCScanConnInfo, 
	  ** then there is no connection to the server and NO_CONNECTION error
	  ** shoulb be returned.  INVALID_PATH is deceiving. 
         		NWC_MATCH_EQUALS | NWC_RETURN_LICENSED | NWC_RETURN_UNLICENSED,
	  */
      retcode = (NWCCODE)NWCScanConnInfo(&access, &scanIndex, 
				NWC_CONN_INFO_SERVER_NAME, serverLen, 
				(pnuint8)&tmpPath[startServer],
         		NWC_MATCH_EQUALS,
         		NWC_CONN_INFO_CONN_REF, sizeof(nuint32), connRef, connRef);

      if (retcode ) {
         (void) NWCStrCpy((char *)newPath, (char *)&tmpPath[startServer]);
         return( NO_CONNECTION_TO_SERVER );
      }
      (void) NWCMemCpy((char *)newPath, (char *)&tmpPath[startVol],
                                    volLen);
      newPath[volLen++] = ':';
      newPath[volLen] = '\0';
      (void) NWCStrCat((char *)newPath, (char *)&tmpPath[startPath]);
    }
    else
    {
        return( INVALID_PATH );
    }

/*
**  Clean up the path by setting slashes to '\\', removing '..'
**  from the start of a path ( we don't want relative paths ).
*/
    ccode = CleanPath(newPath);
    if(ccode)
        return( 0x8800 | ccode );

    if(newPath[0] == '.' && newPath[1] == '.')
        return( INVALID_PATH );

    pathPtr = newPath;
    while( *pathPtr && *pathPtr != ':')
        pathPtr = NWNextChar(pathPtr);

    if( *pathPtr == ':' && *(pathPtr + 1) == '.' &&
                *(pathPtr + 2) == '.')
        return( INVALID_PATH );


    return( 0 );
}

#else
{
   NWCCODE  ccode;
   nstr8    abstrTempPath[300];
   nuint16  suDriveNum, suStartServer, suStartVol, suStartPath;
   nuint16  suServerLen, suVolLen;
   pnstr8   pathPtr, pathPtr2;
   nuint32  scanIndex = 0;
   NWCDeclareAccess(access);


   *dirHandle = 0;
   NWCStrCpy(abstrTempPath, path);
   if((ccode = IndexPath(abstrTempPath, &suStartServer, &suServerLen,
            &suStartVol, &suVolLen, &suStartPath)) != 0)
      return (ccode);

   if(suStartVol > 0)
   {
      /* There is a server and volume name */
      abstrTempPath[suStartVol - 1] = 0;
      *connRef = 0;
      if ((ccode = (NWCCODE)NWCScanConnInfo(&access, &scanIndex, 
		NWC_CONN_INFO_SERVER_NAME,
         suServerLen, (pnuint8)&abstrTempPath[suStartServer],
         NWC_MATCH_EQUALS | NWC_RETURN_LICENSED | NWC_RETURN_UNLICENSED,
         NWC_CONN_INFO_CONN_REF, sizeof(nuint32), connRef, connRef)) != 0)
      {
         /* Return the name of the server that was not found */
         NWCStrCpy(newPath, &abstrTempPath[suStartServer]);
      }
      else
      {
         NWCMemMove(newPath, &abstrTempPath[suStartVol], suVolLen);
         newPath[suVolLen++] = ':';
         newPath[suVolLen]   = 0;
         NWCStrCat(newPath, &abstrTempPath[suStartPath]);
      }
   }
   else if(suStartPath > 2)
   {
      /* The volume name is specified */
      if((ccode = NWGetDefaultConnRef(connRef)) == 0)
         NWCStrCpy(newPath, abstrTempPath);
   }
   else
   {
      /* there is a path only. Check if there is a drive letter */
      if(suStartPath == 2)  /* There is a drive letter */
      {
         suDriveNum = *abstrTempPath;
         if(suDriveNum >= 'a' && suDriveNum <= 'z')
         suDriveNum -= '`';       /* this is ascii 96, which is char before 'a' */
         else
         suDriveNum -= '@';
      }
      else
         suDriveNum = 0;

#ifdef N_PLAT_OS2
      /* do not replace with NWGetDriveStatus since side effect is needed */
      ccode = NWGetDriveInformation(suDriveNum, MY_SESSION, conn, dirHandle,
                                    NULL, newPath);

#else
      if(IsSlash(abstrTempPath[suStartPath]))
         ccode = NWGetDrivePathConnRef(suDriveNum, MY_SESSION, connRef,
                     newPath, NULL);
      else
         ccode = NWGetDriveInfoConnRef(suDriveNum, MY_SESSION, connRef, NULL,
                     NULL, newPath);
#endif

      if(ccode == NOT_MY_RESOURCE)
      {
         *dirHandle = 0;
         *connRef      = 0;
         NWCStrCpy(newPath, abstrTempPath);
      }
      else if(!ccode)
      {

         if(IsSlash(abstrTempPath[suStartPath]))
         {
#ifdef N_PLAT_OS2
            NWCStrCpy(newPath, &abstrTempPath[suStartPath + 1]);
#else
            NWCStrCat(newPath, &abstrTempPath[suStartPath]);
#endif
         }
         else
         {
            /* we play a slight trick here to support OS/2. DOS will NEVER
               have a zero at newPath[0], since a vol name will always be there */
            if(newPath[0] == 0)
               NWCStrCpy(newPath, &abstrTempPath[suStartPath]);
            else
            {
               /* here, OS/2 will ALWAYS run to the end of the string, which is
                  just as well since that's what we want.
               */
               pathPtr = newPath;
               while(*pathPtr && *pathPtr != ':')
                  pathPtr = NWNextChar(pathPtr);

               pathPtr2 = pathPtr;
               while(*pathPtr2)
                  pathPtr2 = NWNextChar(pathPtr2);

               if(!(*pathPtr == ':' && *(pathPtr+1) == 0))
                  *pathPtr2++ = '\\';

               NWCStrCpy(pathPtr2, &abstrTempPath[suStartPath]);
            }
         }
      }
   }
   if(!ccode)
   {
      if((ccode = CleanPath(newPath)) == 0)
      {
         /* a .. cannot be the first characters of the newPath */

         if(newPath[0] == '.' && newPath[1] == '.')
            ccode = 0x899c;  /* Invalid Path */
         else
         {
            /* the rest of this code checks to make sure a
               .. does not follow a : */
            pathPtr = newPath;
            while(*pathPtr && *pathPtr != ':')
               pathPtr = NWNextChar(pathPtr);

            if(*pathPtr == ':' && *(pathPtr + 1) == '.' &&
                  *(pathPtr + 2) == '.')
               ccode = 0x899c;  /* Invalid Path */
         }
      }
   }
   return (ccode);
}
#endif

#endif /* #if defined (N_PLAT_WNT) || (N_PLAT_UNIX) */
/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/prsnwp2.c,v 1.1 1994/09/26 17:48:36 rebekah Exp $
*/
