/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s31.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpconn.h"

/*manpage*NWNCP23s31GetConnListFromObj**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s31GetConnListFromObj
         (
            pNWAccess pAccess,
            nuint32  luObjID,
            nuint32  luSrchConnNum,
            pnuint16 psuConnListLen,
            pnuint32 pluConnList,
         )

REMARKS: This NCP will get a list of connections via the Object ID.

ARGS: <> pAccess
      >  luObjID
      >  luSrchConnNum
      <  psuConnListLen
      <  pluConnList

INCLUDE: ncpconn.h

RETURN:  0x0000 SUCCESSFUL

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 21  Get Object Connection List (old)
         23 27  Get Object Connection List

NCP:     23 31  Get Connection List From Object

CHANGES: 20 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s31GetConnListFromObj
(
   pNWAccess pAccess,
   nuint32  luObjID,
   nuint32  luSrchConnNum,
   pnuint16 psuConnListLen,
   pnuint32 pluConnList
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 31)
   #define NCP_STRUCT_LEN  ((nuint16) 9)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 514)

   nint32   lCode;
   nint i;
   nuint16 suNCPLen;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo32(&abuReq[3], &luObjID);
   NCopyLoHi32(&abuReq[7], &luSrchConnNum);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NCopyLoHi16(psuConnListLen, &abuReply[0]);
      for(i=0; i < (nint) *psuConnListLen; i++)
         NCopyLoHi32(&pluConnList[i], &abuReply[(4 * i)+2]);
   }

   return((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s31.c,v 1.7 1994/09/26 17:37:08 rebekah Exp $
*/
