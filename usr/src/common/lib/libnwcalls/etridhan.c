/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:etridhan.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpafp.h"

#include "nwcaldef.h"
#include "nwafp.h"

/*manpage*NWAFPGetEntryIDFromHandle*****************************************
SYNTAX:  NWCCODE N_API NWAFPGetEntryIDFromHandle
         (
            NWCONN_HANDLE  conn,
            pnuint8        NWHandle,
            pnuint16       volNum,
            pnuint32       AFPEntryID,
            pnuint8        forkIndicator
         );

REMARKS:

ARGS: >  conn
      >  NWHandle
      <  volNum (optional)
      <  AFPEntryID (optional)
      <  forkIndicator (optional)
         0 = data
         1 = resource

INCLUDE: nwafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 06  AFP Get Entry ID Form NetWare Handle

CHANGES: 27 Aug 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWAFPGetEntryIDFromHandle
(
   NWCONN_HANDLE  conn,
   pnuint8        NWHandle,
   pnuint16       volNum,
   pnuint32       AFPEntryID,
   pnuint8        forkIndicator
)
{
   nuint8  buTempVolNum;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   ccode = (NWCCODE) NWNCP35s6AFPGEntryIDFrmNWHan(&access, NWHandle,
               &buTempVolNum, AFPEntryID, forkIndicator);
   if (ccode == 0)
   {
      if (volNum)
         *volNum = (nuint16) buTempVolNum;

      if (AFPEntryID)
         *AFPEntryID = NSwap32(*AFPEntryID);
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/etridhan.c,v 1.7 1994/09/26 17:45:24 rebekah Exp $
*/
