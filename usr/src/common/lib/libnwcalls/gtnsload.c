/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtnsload.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"
#include "nwcaldef.h"
#include "nwnamspc.h"

/*manpage*NWGetNSLoadedList*************************************************
SYNTAX:  NWCCODE N_API NWGetNSLoadedList
         (
            NWCONN_HANDLE conn,
            nuint8  buVolNum,
            nuint8  buMaxListLen,
            pnuint8 pbuNSList,
            pnuint8 pbuActualListSize
         )

REMARKS: Gets the list of name spaces that are loaded on the specified volume.

         The NSLoadedList will contain a nuint8 entry for every name space
         that is loaded on the server. The buffer for NSLoadedList must be
         at least 6 nuint8s long (so listBufferSize should be at least 6).

ARGS: >  conn
      >  buVolNum
      >  buMaxListLen
      <  pbuNSList
      <  pbuActualListSize

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 24  Get Name Spaces Loaded List From Volume Number

CHANGES: 11 Aug 1993 - NWNCP Enabled - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetNSLoadedList
(
   NWCONN_HANDLE conn,
   nuint8  buVolNum,
   nuint8  buMaxListLen,
   pnuint8 pbuNSList,
   pnuint8 pbuActualListSize
)
{
   NWCCODE ccode;
   nuint8  buList[512];
   nuint16 suListLen;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = (NWCCODE)NWNCP87s24NSGetLoadedList(&access,
                                                  0,
                                                  buVolNum,
                                                  &suListLen,
                                                  buList)) == 0)
   {
      if((nuint8)suListLen > buMaxListLen)
         suListLen = buMaxListLen;

      NWCMemMove(pbuNSList, buList, suListLen);
      *pbuActualListSize = (nuint8)suListLen;
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtnsload.c,v 1.7 1994/09/26 17:47:17 rebekah Exp $
*/
