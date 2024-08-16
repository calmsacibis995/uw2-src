/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtusrinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetUserInfo*****************************************************
SYNTAX:  NWCCODE N_API NWGetUserInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        connNum,
            pnstr8         userName,
            NWFSE_USER_INFO NWPTR fseUserInfo
         )

REMARKS:

ARGS: >  conn
      >  connNum
      <  userName
      <  fseUserInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 04  User Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetUserInfo
(
   NWCONN_HANDLE  conn,
   nuint32        connNum,
   pnstr8         userName,
   NWFSE_USER_INFO NWPTR fseUserInfo
)
{
   NWCCODE ccode;
   nuint8 strLen;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if ((ccode = (NWCCODE)NWNCP123s4GetUserInfo(&access, connNum,
         (pNWNCPFSEVConsoleInfo) &fseUserInfo->serverTimeAndVConsoleInfo,
         &fseUserInfo->reserved, (pNWNCPFSEUserInfo) &fseUserInfo->userInfo,
         &strLen, userName)) == 0)
   {
      userName[strLen] = 0x00;
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtusrinf.c,v 1.7 1994/09/26 17:47:31 rebekah Exp $
*/

