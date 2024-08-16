/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s30.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s30OpenCreateFileOrDir**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s30OpenCreateFileOrDir
         (
            pNWAccess          pAccess,
            nuint8            buNamSpc,
            nuint8            buDataStream,
            nuint8            buOpenCreateMode,
            nuint8            buReserved,
            nuint16           suSrchAttr,
            nuint16           suReserved2,
            nuint32           luRetInfoMask,
            nuint32           luCreateAttr,
            nuint16           suDesiredRights,
            pNWNCPCompPath    pCompPath,
            pnuint8           pbuNWFileHandleB4,
            pnuint8           pbuOpenCreateAction,
            pnuint8           pbuReserved3,
            pNWNCPEntryStruct pEntry
         );

REMARKS: This NCP replaces Open/Create File or Subdirectory (0x2222  87  01).
         This NCP will create or open the file depending on the OpenCreateMode
         field.  Note, however, that subdirectories may be created by the client
         but not opened by the client.

         The CreateAttributes field is used to set the attributes in the DOS
         name space.  More information on this field is given in the
         Introduction to File System NCPs.

         The SearchAttributes field, ReturnInfoMask field, NetWareInfoStruct
         field, and NetWareFileNameStruct field are explained in more detail
         in the Introduction to File System NCPs.

ARGS: <> pAccess
       > buNamSpc
       > buDataStream
       > buOpenCreateMode
       > buReserved
       > suSrchAttr
       > suReserved2
       > luRetInfoMask
       > luCreateAttr
       > suDesiredRights
       > pCompPath
      <  pbuNWFileHandleB4
      <  pbuOpenCreateAction (optional)
      <  pbuReserved3 (optional)
      <  pEntry

INCLUDE: ncpfile.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:     87 01  Open/Create File or Subdirectory

NCP:     87 30  Open/Create File or Subdirectory

CHANGES: 17 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s30OpenCreateFileOrDir
(
   pNWAccess          pAccess,
   nuint8            buNamSpc,
   nuint8            buDataStream,
   nuint8            buOpenCreateMode,
   nuint8            buReserved,
   nuint16           suSrchAttr,
   nuint16           suReserved2,
   nuint32           luRetInfoMask,
   nuint32           luCreateAttr,
   nuint16           suDesiredRights,
   pNWNCPCompPath    pCompPath,
   pnuint8           pbuNWFileHandleB4,
   pnuint8           pbuOpenCreateAction,
   pnuint8           pbuReserved3,
   pNWNCPEntryStruct pEntry
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 30)
   #define REQ_LEN         ((nuint) 19)
   #define REPLY_LEN       ((nuint) 83)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 2)
   #define NAME_LEN        ((nuint) 256)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buDataStream;
   abuReq[3] = buOpenCreateMode;
   abuReq[4] = buReserved;
   NCopyLoHi16(&abuReq[5], &suSrchAttr);
   NCopyLoHi16(&abuReq[7], &suReserved2);
   NCopyLoHi32(&abuReq[9], &luRetInfoMask);
   NCopyLoHi32(&abuReq[13], &luCreateAttr);
   NCopyLoHi16(&abuReq[17], &suDesiredRights);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pCompPath->abuPacked;
   reqFrag[1].uLen  = pCompPath->suPackedLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pEntry->abuName;
   replyFrag[1].uLen  = NAME_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);

   if(lCode == 0)
   {
      NCopyHiLo32(pbuNWFileHandleB4, abuReply);

      if (pbuOpenCreateAction)
         *pbuOpenCreateAction = abuReply[4];
      if (pbuReserved3)
         *pbuReserved3 = abuReply[5];

      NWNCPUnpackEntryStruct(pEntry, &abuReply[6], luRetInfoMask);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s30.c,v 1.8 1994/09/26 17:39:37 rebekah Exp $
*/
