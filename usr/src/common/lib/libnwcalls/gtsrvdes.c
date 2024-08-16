/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtsrvdes.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWGetFileServerDescription****************************************
SYNTAX:  NWCCODE N_API NWGetFileServerDescription
         (
            NWCONN_HANDLE  conn,
            pnstr8         companyName,
            pnstr8         rev,
            pnstr8         revDate,
            pnstr8         copyrightNotice
         );

REMARKS:

ARGS:
      > conn
      < companyName (optional)
      < rev (optional)
      < revDate (optional)
      < copyrightNotice (optional)

INCLUDE: nwserver.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 201  Get File Server Description Strings

CHANGES: 13 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetFileServerDescription
(
   NWCONN_HANDLE  conn,
   pnstr8         companyName,
   pnstr8         rev,
   pnstr8         revDate,
   pnstr8         copyrightNotice
)
{
   nuint16 len;
   NWCCODE ccode;
   pnstr8 pbstrTemp;
   nuint8 replyBuf[512];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   ccode = (NWCCODE) NWNCP23s201GetServerDescStr(&access, replyBuf);

   if (ccode == 0)
   {
      pbstrTemp = replyBuf;
      len = NWCStrLen(pbstrTemp) + 1;
      if(companyName)
         NWCMemMove(companyName, pbstrTemp, len);

      pbstrTemp += len;
      len = NWCStrLen(pbstrTemp) + 1;
      if(rev)
         NWCMemMove(rev, pbstrTemp, len);

      pbstrTemp += len;
      len = NWCStrLen(pbstrTemp) + 1;
      if(revDate)
         NWCMemMove(revDate, pbstrTemp, len);

      pbstrTemp += len;
      len = NWCStrLen(pbstrTemp) + 1;
      if(copyrightNotice)
         NWCMemMove(copyrightNotice, pbstrTemp, len);
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtsrvdes.c,v 1.7 1994/09/26 17:47:29 rebekah Exp $
*/
