/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getdirli.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"

#include "nwcaldef.h"
#include "nwdirect.h"

/*manpage*NWGetDirSpaceLimitList********************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWCCODE ) NWGetDirSpaceLimitList
         (
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle,
            pnuint8  pbuReturnBuf
         );

REMARKS:

ARGS: >  conn
      >  dirHandle
      <  pbuReturnBuff

INCLUDE: nwdirect.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 35  Get Directory Disk Space Restriction

CHANGES: 14 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWCCODE )
NWGetDirSpaceLimitList
(
   NWCONN_HANDLE conn,
   NWDIR_HANDLE dirHandle,
   pnuint8 pbuReturnBuf
)
{
   NWCCODE ccode;
   nuint8 abuReq[4];
   nuint16 suNCPLen;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   suNCPLen = (nuint16) 2;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = (nuint8) 35;
   abuReq[3] = (nuint8) dirHandle;

   /*
      This function must call the requester directly because
      it returns a packed buffer and the NWNCP module
      returns an array of structures.

      BEWARE while porting this function!!!!
   */

   if ((ccode = (NWCCODE)NWCRequestSingle(&access, (nuint) 22, abuReq, (nuint) 4, pbuReturnBuf,
            (nuint) 512, NULL)) == 0)
   {
      nuint   i, n, iNumEntries;
      nuint32 luTemp;

      iNumEntries = pbuReturnBuf[0];

      for (i = 0; i < iNumEntries; i++)
      {
         n = i*9+2;

         NCopyLoHi32(&luTemp, &pbuReturnBuf[n]);
         NCopy32(&pbuReturnBuf[n], &luTemp);
         NCopyLoHi32(&luTemp, &pbuReturnBuf[n+4]);
         NCopy32(&pbuReturnBuf[n+4], &luTemp);
      }
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getdirli.c,v 1.7 1994/09/26 17:45:50 rebekah Exp $
*/

