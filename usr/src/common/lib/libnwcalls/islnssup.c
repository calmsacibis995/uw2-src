/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:islnssup.c	1.5"
#if defined N_PLAT_WNT && defined N_ARCH_32
#include <windows.h>
#endif

#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwmisc.h"
#include "nwcint.h"
#include "nwvol.h"
#include "nwundoc.h"
#include "nwnamspc.h"
#include "nwserver.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwclocal.h"

/*manpage*NWIsLNSSupportedOnVolume******************************************
SYNTAX:  NWCCODE N_API NWIsLNSSupportedOnVolume
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path
         )

REMARKS: Queries the file server and returns a nonzero if long names are
         supported on the target volume and 0 if not.

ARGS:

INCLUDE: nwmisc.h

RETURN:  0   LNS not supported on volume
         !0  LNS is supported

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: Art 9/22/93
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWIsLNSSupportedOnVolume
(
   NWCONN_HANDLE   conn,
   NWDIR_HANDLE    dirHandle,
   pnstr8          path
)
{
#if defined(N_PLAT_OS2) || (defined N_PLAT_WNT && defined N_ARCH_32)
   nuint16 volNum;
   nint8 volName[17], NWPTR tPath;
   nuint8 supportedFlag = 0;
#ifdef N_PLAT_OS2
   pnuint8 pTemp;
   nuint8 reqBuf[8];
#else
   nuint16 serverVer;
   nuint8 nsList[256];
   nuint8 listLen, i;

   if(NWGetFileServerVersion(conn, &serverVer))
      return 0;

   if(serverVer < 3110)
      return 0;
#endif

   if(dirHandle)
   {
      if(NWGetVolumeInfoWithHandle(conn, dirHandle, volName,
         NULL, NULL, NULL, NULL, NULL, NULL))
      {
         return (0);
      }
   }
   else
   {
      if(!path)
         return (0);

      NWCMemMove( volName, path, 16 );
      volName[16] = (char) 0;

      tPath = volName;
      while(*tPath && *tPath != ':')
         tPath = NWNextChar(tPath);

      *tPath = (char) 0;

      volName[NWLTruncateString(volName, 16)] = (char) 0;
   }

   if(NWGetVolumeNumber(conn, volName, &volNum))
      return (0);

#ifdef N_PLAT_OS2
   pTemp = &supportedFlag;

   NCopy32(&reqBuf[0], &pTemp);  /* yes, we're copying an address here */
   NCopy16(&reqBuf[4], &volNum);
   NCopy16(&reqBuf[6], &conn);

   if (NWCCallGate(_NWC_LNS_SUP_ON_VOL, reqBuf))
      return (0);
#else
   if(NWGetNSLoadedList(conn, (nuint8) volNum, (nuint8) sizeof(nsList), nsList, &listLen))
      return 0;

   for(i = 0; i < listLen; i++)
   {
      if(nsList[listLen] == NW_NS_OS2)
      {
         supportedFlag = 1;
         break;
      }
   }

#endif
   return (supportedFlag);
#else
   path      = path;
   dirHandle = dirHandle;
   conn      = conn;
   return (N_FALSE);
#endif
}

#if 0    /* Obsolete this function.  It should not be exported. */
#ifdef N_PLAT_OS2
NWCCODE N_API NWToggleLNS(NWCONN_HANDLE conn, nuint16 LNSFlag)
{
   conn = conn;
   return (NWCCallGate(_NWC_TOGGLE_LNS, &LNSFlag));
}
#endif
#endif

#ifdef __NWGetCurNS
#undef __NWGetCurNS
nuint16 N_API __NWGetCurNS
(
   NWCONN_HANDLE conn,
   NWDIR_HANDLE dirHandle,
   pnstr8 path
);
#endif

nuint16 N_API __NWGetCurNS
(
   NWCONN_HANDLE conn,
   NWDIR_HANDLE dirHandle,
   pnstr8 path
)
{
#if defined(N_PLAT_OS2) || (defined N_PLAT_WNT && defined N_ARCH_32)
   if(dirHandle != 1 && NWIsLNSSupportedOnVolume(conn, dirHandle, path))
      return (NW_NS_OS2);
   else
#else
   conn = conn;
   dirHandle = dirHandle;
   path = path;
#endif
   return (NW_NS_DOS);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/islnssup.c,v 1.7 1994/09/26 17:47:40 rebekah Exp $
*/
