/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:63.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP63ScanNext******************************************************
SYNTAX:   N_EXTERN_LIBRARY( NWRCODE )
          NWNCP63ScanNext
          (
             pNWAccess         pAccess,
             nuint8           buVolNum,
             nuint16          suDirID,
             nuint8           buSrchAttrs,
             nuint8           buSrchPathLen,
             pnstr8           pbstrSrchPath,
             pnuint16         psuIterHnd,
             pNWNCPSrchInfo   pSrchInfo,
          )

REMARKS: This call returns information about a file or a directory. It is called
         iteratively after a call is made to File Search Initialize (function 62).
         If the client sets the Subdirectory bit of the SearchAttributes byte,
         directory information will be returned. Otherwise, file information will be
         returned.

         The information returned is a direct copy of a Novell server's internal file
         header. This call does not return the ID of the file owner.

         The SearchSequence in the request and in the reply is the file offset in the
         directory file. The file server increments this number by one in the reply.
         Setting this field to "-1" will restart the search. When SearchSequence is
         set to -1, SearchAttributes can be modified to alter the search mask.

         The DOS "DIR" command is accomplished in NetWare using this call. First, all
         of the non-directory files are obtained. To do so, File Search Init is
         called, followed by repeated calls to File Search Continue with
         SearchAttributes set to return normal, non-directory files. When all of the
         non-directory files have been obtained, the server returns a reply message
         with a No Files Found Completion Code and SearchSequence set to -1; the
         client (NetWare shell) then calls File Search Continue with Sequence Number
         set to -1 and with SearchAttributes set to return directory files. Then, File
         Search Continue is repeatedly called to obtain all of the subdirectories.

ARGS: <> pAccess
      >  buVolNum
      >  suDirID
      >  buSrchAttrs
      >  buSrchPathLen
      >  pbstrSrchPath
      <> psuIterHnd
      <  pSrchInfo

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x89FF  No Files Found

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     22 02  Scan Directory Information
         62 --  File Search Initialize

NCP:     63 --  File Search Continue

CHANGES: 7 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP63ScanNext
(
   pNWAccess         pAccess,
   nuint8           buVolNum,
   nuint16          suDirID,
   nuint8           buSrchAttrs,
   nuint8           buSrchPathLen,
   pnstr8           pbstrSrchPath,
   pnuint16         psuIterHnd,
   pNWNCPSrchInfo   pSrchInfo
)
{
   #define NCP_FUNCTION    ((nuint) 63)
   #define NCP_REQ_LEN     ((nuint) 7)
   #define NCP_REPLY_LEN   ((nuint) 32)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 1)

   nint32   lCode;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];

   abuReq[0] = buVolNum;
   NCopyLoHi16(&abuReq[1], &suDirID);
   NCopyHiLo16(&abuReq[3], psuIterHnd);
   abuReq[5] = buSrchAttrs;
   abuReq[6] = buSrchPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrSrchPath;
   reqFrag[1].uLen  = (nuint) buSrchPathLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NCopyHiLo16(psuIterHnd, &abuReply[0]);

      if (*(pnuint16)&abuReply[30] == 0xD1D1)  /*Directory */
      {
         NCopyLoHi16(&pSrchInfo->d.suReserved1, &abuReply[2]);
         NWCMemMove(pSrchInfo->d.abstrDirName, &abuReply[4], (nuint) 14);
         pSrchInfo->d.buDirAttributes = abuReply[18];
         pSrchInfo->d.buDirAccessRights = abuReply[19];
         NCopyHiLo16(&pSrchInfo->d.suCreateDate, &abuReply[20]);
         NCopyHiLo16(&pSrchInfo->d.suCreateTime, &abuReply[22]);
         NCopyHiLo32(&pSrchInfo->d.luOwningObjectID, &abuReply[24]);
         NCopyLoHi16(&pSrchInfo->d.suReserved2, &abuReply[28]);
         NCopyLoHi16(&pSrchInfo->d.suDirStamp, &abuReply[30]);
      }
      else
      {
         NCopyLoHi16(&pSrchInfo->f.suReserved, &abuReply[2]);
         NWCMemMove(pSrchInfo->f.abstrFileName, &abuReply[4], (nuint) 14);
         pSrchInfo->f.buAttrs = abuReply[18];
         pSrchInfo->f.buExeType = abuReply[19];
         NCopyHiLo32(&pSrchInfo->f.luSize, &abuReply[20]);
         NCopyHiLo16(&pSrchInfo->f.suCreationDate, &abuReply[24]);
         NCopyHiLo16(&pSrchInfo->f.suAccessedDate, &abuReply[26]);
         NCopyHiLo16(&pSrchInfo->f.suModifiedDate, &abuReply[28]);
         NCopyHiLo16(&pSrchInfo->f.suModifiedTime, &abuReply[30]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/63.c,v 1.7 1994/09/26 17:38:43 rebekah Exp $
*/
