/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s12.c	1.7"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s12TrusteesScanDir**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s12TrusteesScanDir
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buTrusteeSetNum,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            pnstr8   pbstrDirNameB16,
            pnuint16 psuCreationDate,
            pnuint16 psuCreationTime,
            pnuint32 pluOwnerID,
            pnuint32 pluTrusteeIDSetB5,
            pnuint8  pbuTrusteeAccessMasksB5,
         )

REMARKS: This call allows a client to determine the objects that are trustees of the
         specified directory.

         This call must be used iteratively to retrieve all trustees of a given
         directory.  The first time this call is used, the Trustee Set Number must be
         set to 1.  On successive requests, the client must increment the Trustee Set
         Number by 1.  The server will return a Failure error when the client requests
         a Trustee Set Number that has no trustees.

         Directory Path will be null-padded.

         Creation Date and Creation Time are in DOS format, except that the values are
         stored in high-low order.

         A directory can have an arbitrary number of objects (usually users or user
         groups) listed as trustees.  Each trustee has a corresponding Trustee Access
         Mask.

         If a trustee is deleted, the deleted trustee ID number is replaced with a
         long zero, indicating that the trustee slot is unused.  (Zero is not a valid
         trustee ID number.) Within each set of five trustee ID numbers, all zero
         entries
         are collected at the end of the trustee ID vector.  Therefore, a client
         algorithm scanning a directory's trustees will stop scanning the current set
         of 5 trustees when it encounters a trustee ID of zero.  The algorithm must
         then retrieve the next trustee set from the server and scan it.  This process
         is repeated until the client receives a completion code other than the
         Successful code.

         Clients making this call must have access control rights to the target
         directory or to its parent directory.

ARGS: <> pAccess
       > buDirHandle
       > buTrusteeSetNum
       > buPathLen
       > pbstrPath
      <  pbstrDirNameB16 (optional)
      <  psuCreationDate (optional)
      <  psuCreationTime (optional)
      <  pluOwnerID (optional)
      <  pluTrusteeIDSetB5 (optional)
      <  pbuTrusteeAccessMasksB5 (optional)

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x898C  No Set Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Failure

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     22 38  Scan File Or Directory For Extended Trustees
         87 05  Scan File or Subdirectory For Trustees

NCP:     22 12  Scan Directory For Trustees

CHANGES: 14 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s12TrusteesScanDir
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buIterHnd,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnstr8   pbstrDirNameB16,
   pnuint16 psuCreationDate,
   pnuint16 psuCreationTime,
   pnuint32 pluOwnerID,
   pnuint32 pluTrusteeIDSetB5,
   pnuint8  pbuTrusteeAccessMasksB5
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 12)
   #define NCP_STRUCT_LEN  ((nuint16) 4 + buPathLen)
   #define NCP_REQ_LEN     ((nuint) 6)
   #define NCP_REPLY_LEN   ((nuint) 49)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 1)

   nint32   lCode;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN], abuRep[NCP_REPLY_LEN];
   nint  i;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   abuReq[4] = buIterHnd;
   abuReq[5] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = buPathLen;

   replyFrag[0].pAddr = abuRep;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);

   if(lCode == 0)
   {
      if(pbstrDirNameB16)
         NWCMemMove(pbstrDirNameB16, &abuRep[0], 16);

      if(psuCreationDate)
         NCopyHiLo16(psuCreationDate, &abuRep[16]);

      if(psuCreationTime)
         NCopyHiLo16(psuCreationTime, &abuRep[18]);

      if(pluOwnerID)
         NCopyLoHi32(pluOwnerID, &abuRep[20]);

      if(pluTrusteeIDSetB5)
      {
         for(i = 0; i < 5; i++)
            NCopyHiLo32(&pluTrusteeIDSetB5[i], &abuRep[24+(4*i)]);
      }

      if(pbuTrusteeAccessMasksB5)
         NWCMemMove(pbuTrusteeAccessMasksB5, &abuRep[44], 5);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s12.c,v 1.9 1994/09/28 06:26:12 rebekah Exp $
*/
