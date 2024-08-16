/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:17s10.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP17s10GetPrintersQueue**************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP17s10GetPrintersQueue
         (
            pNWAccess pAccess,
            nuint8   buPrinterNumber,
            pnuint32 pluQueueID,
         );

REMARKS: This call returns the ObjectID of the queue servicing the specified printer.


ARGS: <> pAccess
       > buPrinterNumber
      <  pluQueueID

INCLUDE: ncpqms.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89FF  Bad Printer

SERVER:

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     17  10  Get Printer's Queue

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP17s10GetPrintersQueue
(
   pNWAccess pAccess,
   nuint8   buPrinterNumber,
   pnuint32 pluQueueID
)
{
   #define NCP_FUNCTION    ((nuint)   17)
   #define NCP_STRUCT_LEN  ((nuint)    2)
   #define NCP_SUBFUNCTION ((nuint8)  10)
   #define NCP_REPLY_LEN   ((nuint)    4)

   nint32   lCode;
   nuint8  abuReq[NCP_STRUCT_LEN+2],
           abuReply[NCP_REPLY_LEN];
   nuint16 suStructLen;

   suStructLen = NCP_STRUCT_LEN;
   NCopyHiLo16((pnuint16)&abuReq[0], &suStructLen);
   abuReq[2] = (nuint8)NCP_SUBFUNCTION;

   abuReq[3] = buPrinterNumber;

   lCode = NWCRequestSingle(pAccess, (nuint) NCP_FUNCTION, abuReq,
      (nuint) NCP_STRUCT_LEN+2, abuReply, (nuint) NCP_REPLY_LEN, NULL);

   if (lCode == 0)
   {
      NCopyHiLo32(pluQueueID, &abuReply[0]);
   }
   return ((NWRCODE)lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/17s10.c,v 1.7 1994/09/26 17:33:13 rebekah Exp $
*/
