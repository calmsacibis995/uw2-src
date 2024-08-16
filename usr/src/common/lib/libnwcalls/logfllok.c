/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:logfllok.c	1.5"
#ifdef N_PLAT_OS2
#include <os2def.h>
#include <bsedos.h>
#endif

#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwcaldef.h"
#define  FILE_LOCKS_ONLY     /* only use the locks part of nwfile.h */
#include "nwfile.h"
#include "nwclocal.h"
#include "nwserver.h"
#include "ncpsync.h"

#ifdef N_PLAT_OS2
#include "nwundoc.h"
#include "nwcint.h"
#include "nwmisc.h"
#endif

/*manpage*NWLogFileLock2****************************************************
SYNTAX:  NWCCODE N_API NWLogFileLock2
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            nuint8         lockFlags,
            nuint16        timeOut
         )

REMARKS: Logs the specified file for exclusive use by the
         workstation. If bit 0 of the Lock flag is set, the server will
         immediately attempt to lock the file.  The values for the lock
         flags are interpreted as follows:

         0x00 = Log File
         0x01 = Log and Lock the file

         A file may be locked by a client even if the file does not yet
         exist.  This reserves the file name for use by the client that
         locked it.

         The timeout value determines how long the call will attempt to
         lock the file before returning the completion code (0x89FE -
         Timeout Failure).  Timeout values are measured in ticks - a tick
         is equivalent to 1/18th of a second - If this value is 0, then
         there is now wait.  This call cannot lock files that are already
         logged and exclusively locked by other applications.

ARGS: >  conn
      >  dirHandle
      >  path
      >  lockFlags
      >  timeOut

INCLUDE: nwfile.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     03 --  Log File
         105 --  Log File

CHANGES: 3 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWLogFileLock2
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint8         lockFlags,
   nuint16        timeOut
)
{
#ifdef N_PLAT_OS2
   nuint32 timeStamp1, timeStamp2;
   nuint16 dif;
   nuint8  function, lockMade = 0;
   LOCK_SEM_NODE NWPTR lockHandle;
   NWCCODE ccode2;
   NW_FRAGMENT reqFrag[1];

   struct
   {
      nuint8   dirHandle;
      nuint8   lockFlag;
      nuint16  lockTimeout;
      nuint8   pathLen;
      nstr8    path[260];
   } reqBuf;

#endif

   NWCCODE ccode;
   nuint16 serverVer;
   nuint8 pathLen;
   nstr8 tempPath[260];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   ccode = NWGetFileServerVersion(conn, &serverVer);
   if(ccode)
      return (ccode);

   NWCStrCpy(tempPath, path);

   NWConvertToSpecChar(tempPath);

   pathLen = (nuint8) NWCStrLen(tempPath);

#ifdef N_PLAT_OS2
   NWCStrCpy(reqBuf.path, tempPath);
   reqBuf.dirHandle     = dirHandle;
   reqBuf.lockFlag      = lockFlags;
   reqBuf.pathLen       = (nuint8) NWCStrLen(reqBuf.path);
   reqBuf.lockTimeout   = NSwap16(timeOut);

   reqFrag[0].fragAddress= &reqBuf;
   reqFrag[0].fragSize   = 5 + reqBuf.pathLen;

   if(serverVer >= 3110)
      function = 105;
   else
      function = 3;

   if (NWCCallGate(_NWC_WAIT_LOCK_AVAIL, &conn))
      ccode = NWLockRequest(conn, function, 1, reqFrag, 0, NULL);
   else
   {
      NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp1);
      do
      {
         if (function == 3)
         {
            ccode = (NWCCODE) NWNCP3SyncLogFile(&access, (nuint8)dirHandle,
                        lockFlags, timeOut, pathLen,
                        tempPath);
         }
         else
         {
            ccode = (NWCCODE) NWNCP105SyncLogFile(&access, (nuint8)dirHandle,
                        lockFlags, timeOut, pathLen,
                        tempPath);
         }
         NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp2);
         if(timeStamp2 < timeStamp1)
            dif = (USHORT)((-1L - timeStamp1 + timeStamp2) >> 6);
         else
            dif = (USHORT)((timeStamp2 - timeStamp1) >> 6);

         if(ccode == 0x89ff && (timeOut == (nuint16)-1 || dif < timeOut))
         {
            if (lockMade == 0)
            {
               ccode2 = NWAddLockRequestToList(2, &lockHandle);
               if(ccode2)
                  return (ccode2);
               lockMade = 1;
            }
            ccode2 = DosSemSetWait(lockHandle,
                        timeOut == (nuint16)-1 ? (nuint32)-1L :
                        (timeOut - dif) * 56);
         }
         NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp2);
         if(timeStamp2 < timeStamp1)
            dif = (USHORT)((-1L - timeStamp1 + timeStamp2) >> 6);
         else
            dif = (USHORT)((timeStamp2 - timeStamp1) >> 6);
      } while (ccode == 0x89ff && (timeOut == (nuint16)-1 ||
               dif < timeOut));

      if (lockMade == 1)
         NWRemoveLockRequestFromList(2, lockHandle);
   }
   if(ccode == 0x89ff)
      return (0x89fe);

#else

   ccode = (NWCCODE) NWNCP3SyncLogFile(&access, (nuint8) dirHandle,
               lockFlags, timeOut, pathLen,
               tempPath);

#endif

   if(ccode == 0x897f)
      ccode = 0x897e;

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/logfllok.c,v 1.7 1994/09/26 17:47:48 rebekah Exp $
*/
