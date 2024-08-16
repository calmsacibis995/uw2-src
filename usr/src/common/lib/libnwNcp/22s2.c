/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s2.c	1.7"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s2ScanDirInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s2ScanDirInfo
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            pnuint16 psuIterHnd,
            pnstr8   pbstrDirNameB16,
            pnuint16 psuCreationDate,
            pnuint16 psuCreationTime,
            pnuint32 pluOwnerTrusteeID,
            pnuint8  pbuAccessRightsMask,
            pnuint8  pbuReserved,
         );

REMARKS: This call returns information about a file server directory.  The Directory
         Path supplied by the client can contain wildcard characters in the last name
         of the path; the last name of the Directory Path is used as a pattern against
         which directory names are matched.  When this call is used iteratively to
         search for wildcard patterns, the client should set the Starting Search
         Number to 1 on the first call.  On subsequent calls, the client should set
         the Starting Search Number to 1 plus the Next Search Number returned on the
         previous call.  The Starting Search Number and the Next Search Number are
         stored in high-low order.

         Directory Path will be null-padded.

         Creation Date and Time are in DOS format, except that the bytes are in
         high-low rather than low-high order.

         Owner Trustee ID contains the trustee ID of the object (user) that originally
         created the directory.

         The Access Rights Mask is a bit field containing maximum access privileges
         (see the introduction to the Directory Services chapter.

         This call does not have to be used for iterative searches.  A full or
         partial directory path can be specified, and the directory information for
         only that directory will be returned.  If the request message's Directory
         Path Length field is set to zero (0), the Directory Path can be omitted, and
         the directory information for the directory specified by Directory Handle
         will be returned.

ARGS: <> pAccess
      >  buDirHandle
      >  buPathLen
      >  pbstrPath
      <> psuIterHnd
      <  pbstrDirNameB16 (optional)
      <  psuCreationDate (optional)
      <  psuCreationTime (optional)
      <  pluOwnerTrusteeID (optional)
      <  pbuAccessRightsMask (optional)
      <  pbuReserved (optional)

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     62 --  File Search Initialize
         63 --  File Search Continue

NCP:     22 02  Scan Directory Information

CHANGES: 7 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s2ScanDirInfo
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint16 psuIterHnd,
   pnstr8   pbstrDirNameB16,
   pnuint16 psuCreationDate,
   pnuint16 psuCreationTime,
   pnuint32 pluOwnerTrusteeID,
   pnuint8  pbuAccessRightsMask,
   pnuint8  pbuReserved
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 2)
   #define NCP_STRUCT_LEN  ((nuint16) 5 + buPathLen)
   #define NCP_REQ_LEN     ((nuint) 7)
   #define NCP_REPLY_LEN   ((nuint) 28)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 1)

   nint32   lCode;
   nuint16  suNCPLen;
   nuint8   abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   NWCFrag  reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   NCopyHiLo16(&abuReq[4], psuIterHnd);
   abuReq[6] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = (nuint) buPathLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
                 NCP_REPLY_FRAGS, replyFrag, NULL);

   if(lCode == 0)
   {
      if(pbstrDirNameB16)
         NWCMemMove(pbstrDirNameB16, &abuReply[0], (nuint) 16);

      if(psuCreationDate)
         NCopyHiLo16(psuCreationDate, &abuReply[16]);

      if(psuCreationTime)
         NCopyHiLo16(psuCreationTime, &abuReply[18]);

      if(pluOwnerTrusteeID)
         NCopyLoHi32(pluOwnerTrusteeID, &abuReply[20]);

      if(pbuAccessRightsMask)
         *pbuAccessRightsMask = abuReply[24];

      if(pbuReserved)
         *pbuReserved = abuReply[25];

      NCopyHiLo16(psuIterHnd, &abuReply[26]);
   }

   return((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s2.c,v 1.9 1994/09/28 06:26:10 rebekah Exp $
*/
