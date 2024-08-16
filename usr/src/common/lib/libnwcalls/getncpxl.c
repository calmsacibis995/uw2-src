/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getncpxl.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpext.h"
#include "nwncpext.h"
#include "nwserver.h"

/*manpage*NWGetNCPExtensionList*********************************************
SYNTAX:  NWCCODE N_API NWGetNCPExtensionsList
         (
            NWCONN_HANDLE conn,
            pnuint32  pluStartNCPExtID,
            pnuint16  psuItemsInList,
            pnuint32  pluNCPExtIDListB128
         );

REMARKS: Scans currently loaded NCP extensions

ARGS: <> pluStartNCPExtID
         The ID that will be used to get next NCP Extension list. Set
         to -1 for first iteration.

       < psuItemsInList
         The number of NCPExtensions in the ID list

       < pluNCPExtensionIDList
         list of loaded NCPExtensions (128 DWORDS)

INCLUDE: nwncpext.h

RETURN:  n/a

SERVER:  3.0 3.11 4.0

CLIENT:  DOS WIN OS2

SEE:     NWScanNCPExtensions

NCP:     36 04 Get List Of Loaded NCP Extensions IDs

CHANGES: 02 Dec 1992 - Written - jwoodbur
         21 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetNCPExtensionsList
(
  NWCONN_HANDLE conn,
  pnuint32 pluStartNCPExtID,
  pnuint16 psuItemsInList,
  pnuint32 pluNCPExtIDListB128
)
{
   nuint16 suServVer;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &suServVer)) != 0)
      return ccode;

   if(suServVer > 3110)
   {
      return((NWCCODE) NWNCP36s4GetNCPExtsList(&access, pluStartNCPExtID,
                        psuItemsInList, NULL, pluNCPExtIDListB128));

   }
   else
   {
      nuint16 suItems;

      for(suItems = 0; suItems < 128; suItems++)
      {
         if((ccode = NWScanNCPExtensions(conn, pluStartNCPExtID,
                              NULL, NULL, NULL, NULL, NULL)) != 0)
         {
            if(ccode == 0x89ff)
                  ccode = 0;
            break;
         }
         *(pluNCPExtIDListB128++) = *pluStartNCPExtID;
      }
      *psuItemsInList = suItems;
      return(ccode);
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getncpxl.c,v 1.7 1994/09/26 17:46:14 rebekah Exp $
*/
