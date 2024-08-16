/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:chknwver.c	1.5"
#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWCheckNetWareVersion*********************************************
SYNTAX:  NWCCODE N_API NWCheckNetWareVersion
         (
            NWCONN_HANDLE  conn,
            nuint16        minVer,
            nuint16        minSubVer,
            nuint16        minRev,
            nuint16        minSFT,
            nuint16        minTTS,
            pnuint8        compatibilityFlag
         );

REMARKS:

ARGS: >  conn
      >  minVer
      >  minSubVer
      >  minRev
      >  minSFT
      >  minTTS
      <  compatibilityFlag

INCLUDE: nwserver.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     NWGetFileServerVersionInfo

NCP:     n/a

CHANGES: 9 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
*****************************************************************************/
NWCCODE N_API NWCheckNetWareVersion
(
   NWCONN_HANDLE  conn,
   nuint16        minVer,
   nuint16        minSubVer,
   nuint16        minRev,
   nuint16        minSFT,
   nuint16        minTTS,
   pnuint8        compatibilityFlag
)
{
   NWCCODE ccode;
   VERSION_INFO versionInfo;

   if((ccode = NWGetFileServerVersionInfo(conn,
            (VERSION_INFO N_FAR *) &versionInfo)) != 0)
   {
      *compatibilityFlag = (nuint8) 0xFF;  /* invalidate flag */
      return (ccode);
   }

   *compatibilityFlag = (nuint8) 0;

   if (minVer < versionInfo.fileServiceVersion)
      *compatibilityFlag = (nuint8)(*compatibilityFlag | VERSION_NUMBER_TOO_LOW);
   else if (minSubVer < versionInfo.fileServiceSubVersion)
      *compatibilityFlag = (nuint8)(*compatibilityFlag | VERSION_NUMBER_TOO_LOW);
   else if (minRev < versionInfo.revision)
      *compatibilityFlag = (nuint8)(*compatibilityFlag | VERSION_NUMBER_TOO_LOW);

   if (minSFT < versionInfo.SFTLevel)
      *compatibilityFlag = (nuint8)(*compatibilityFlag | SFT_LEVEL_TOO_LOW);

   if (minTTS < versionInfo.TTSLevel)
      *compatibilityFlag = (nuint8)(*compatibilityFlag | TTS_LEVEL_TOO_LOW);

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/chknwver.c,v 1.7 1994/09/26 17:44:20 rebekah Exp $
*/
