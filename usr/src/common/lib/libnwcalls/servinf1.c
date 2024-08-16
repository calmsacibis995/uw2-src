/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:servinf1.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWGetFileServerInformation****************************************
SYNTAX:  NWCCODE N_API NWGetFileServerInformation
         (
            NWCONN_HANDLE  conn,
            pnstr8         serverName,
            pnuint8        majorVer,
            pnuint8        minVer,
            pnuint8        rev,
            pnuint16       maxConnections,
            pnuint16       maxConnectionsUsed,
            pnuint16       connectionsInUse,
            pnuint16       numVolumes,
            pnuint8        SFTLevel,
            pnuint8        TTSLevel
         );

REMARKS:

ARGS: >  conn
      <  serverName
      <  majorVer
      <  minVer
      <  rev
      <  maxConnections
      <  maxConnectionsUsed
      <  connectionsInUse
      <  numVolumes
      <  SFTLevel
      <  TTSLevel

INCLUDE: nwserver.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     NWGetFileServerVersionInfo

NCP:     n/a

CHANGES: 13 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetFileServerInformation
(
   NWCONN_HANDLE  conn,
   pnstr8         serverName,
   pnuint8        majorVer,
   pnuint8        minVer,
   pnuint8        rev,
   pnuint16       maxConns,
   pnuint16       maxConnsUsed,
   pnuint16       connsInUse,
   pnuint16       numVolumes,
   pnuint8        SFTLevel,
   pnuint8        TTSLevel
)
{
   VERSION_INFO verInfo;
   NWCCODE ccode;

   if((ccode = NWGetFileServerVersionInfo(conn, &verInfo)) == 0)
   {
      if (serverName)
         NWCMemMove(serverName, verInfo.serverName, (nuint) 48);
      if (majorVer)
         *majorVer = verInfo.fileServiceVersion;
      if (minVer)
         *minVer = verInfo.fileServiceSubVersion;
      if (rev)
         *rev = verInfo.revision;
      if (maxConns)
         *maxConns = verInfo.maximumServiceConnections;
      if (maxConnsUsed)
         *maxConnsUsed = verInfo.maxConnectionsEverUsed;
      if (connsInUse)
         *connsInUse = verInfo.connectionsInUse;
      if (numVolumes)
         *numVolumes = verInfo.maxNumberVolumes;
      if (SFTLevel)
         *SFTLevel = verInfo.SFTLevel;
      if (TTSLevel)
         *TTSLevel = verInfo.TTSLevel;
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/servinf1.c,v 1.7 1994/09/26 17:49:47 rebekah Exp $
*/
