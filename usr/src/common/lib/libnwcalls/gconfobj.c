/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gconfobj.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwconnec.h"
#include "ncpconn.h"

/*manpage*NWGetConnListFromObject*******************************************
SYNTAX:  NWCCODE N_API NWGetConnListFromObject
         (
            NWCONN_HANDLE conn,
            nuint32  objID,
            nuint32  srchConnNum,
            pnuint16 connListLen,
            pnuint32 connList
         )

REMARKS:

ARGS:  >  conn
       >  objID
       >  srchConnNum
      <   connListLen
      <   connList

INCLUDE: nwconnec.h

RETURN:  0x0000 Successful

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 31  Get Connection List From Object

CHANGES: 20 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetConnListFromObject
(
   NWCONN_HANDLE  conn,
   nuint32        objID,
   nuint32        searchConnNum,
   pnuint16       connListLen,
   pnuint32       connList
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP23s31GetConnListFromObj(&access, NSwap32(objID),
               searchConnNum, connListLen, connList));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gconfobj.c,v 1.7 1994/09/26 17:45:34 rebekah Exp $
*/
