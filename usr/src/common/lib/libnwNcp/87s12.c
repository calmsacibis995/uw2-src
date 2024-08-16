/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s12.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s12AllocDirHandle**********************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP87s12AllocDirHandle
         (
            pNWAccess       pAccess,
            nuint8         buNamSpc,
            nuint8         buReserved1,
            nuint16        suAllocMode,
            pNWNCPCompPath cPath,
            pnuint8        pbuDirHandle,
            pnuint8        pbuVolNum,
            pnuint8        pbuReservedB4,
         )

REMARKS: This is a NetWare 386 v3.11 call.  The suAllocMode field can be set
         as shown below.

                                 Bits
                     7   6   5   4   3   2   1   0

                  *********************************
                  * 0 * 0 * 0 * 0 * 0 * 0 * x * x *
                  *********************************
                                            *   *
                                            *   *
              reserved ******************** 1   1
              Special Temporary Handle **** 1   0
              Temporary Handle ************ 0   1
              Permanent Handle ************ 0   0

ARGS: <> pAccess
      >  buNamSpc
      >  buReserved1
      >  suAllocMode
      >  cPath
      <  pbuDirHandle
      <  pbuVolNum (optional)
      <  pbuReservedB4 (optional)

INCLUDE: ncpfile.h

RETURN:  0x0000 Successful

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     87 12  Allocate Short Directory Handle

CHANGES: 8 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s12AllocDirHandle
(
   pNWAccess       pAccess,
   nuint8         buNamSpc,
   nuint8         buReserved1,
   nuint16        suAllocMode,
   pNWNCPCompPath cPath,
   pnuint8        pbuDirHandle,
   pnuint8        pbuVolNum,
   pnuint8        pbuReservedB4
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 12)
   #define REQ_FRAGS       ((nuint) 2)
   #define REQ_LEN         ((nuint) 5)
   #define REPLY_FRAGS     ((nuint) 1)
   #define REPLY_LEN       ((nuint) 6)

   nint32   lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8  req[REQ_LEN], reply[REPLY_LEN];

   req[0] = NCP_SUBFUNCTION;
   req[1] = buNamSpc;
   req[2] = buReserved1;
   NCopyLoHi16(&req[3], &suAllocMode);

   reqFrag[0].pAddr = req;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = cPath->abuPacked;
   reqFrag[1].uLen  = (nuint) cPath->suPackedLen;

   replyFrag[0].pAddr = reply;
   replyFrag[0].uLen  = REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
                 REPLY_FRAGS, replyFrag, NULL);
   if(lCode == 0)
   {
      *pbuDirHandle = reply[0];

      if(pbuVolNum)
         *pbuVolNum = reply[1];

      if(pbuReservedB4)
         NWCMemMove(pbuReservedB4, &reply[2], 4);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s12.c,v 1.7 1994/09/26 17:39:16 rebekah Exp $
*/
