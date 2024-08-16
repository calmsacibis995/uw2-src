/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s27.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s27DelScan**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s27DelScan
         (
            pNWAccess          pAccess,
            nuint8            buDirHandle,
            pnuint32          pluIterHnd,
            pNWNCPDelEntryInfo pInfo,
         );

REMARKS: This function gets the salvageable file information for a file in the
         current directory.  If Sequence has been set to FFFFFFFFh, it will start
         with a new search, otherwise it will return the next file.

ARGS: <> pAccess
      >  buDirHandle
      <> pluIterHnd
      <  pEntryInfo

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x89FB  Server Does Not Support 386 Salvage Functions
         0x89FF  No More Salvageable Files In Directory

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 29  Purge Salvageable File
         22 28  Recover Salvageable File

NCP:     22 27  Scan Salvageable Files

CHANGES: 16 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s27DelScan
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   pnuint32 pluIterHnd,
   pNWNCPDelEntryInfo pInfo
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 27)
   #define NCP_STRUCT_LEN  ((nuint16) 6)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 132)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   NCopyLoHi32(&abuReq[4], pluIterHnd);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuReply, NCP_REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NCopyLoHi32(pluIterHnd, &abuReply[0]);

      NCopyLoHi32(&pInfo->luSubdir, &abuReply[4]);
      NCopyLoHi32(&pInfo->luAttrs, &abuReply[8]);
      pInfo->buUniqueID = abuReply[12];
      pInfo->buFlags = abuReply[13];
      pInfo->buNamSpc = abuReply[14];
      pInfo->buFileNameLen = abuReply[15];
      NWCMemMove(pInfo->pbstrFileNameB256, &abuReply[16], (nuint) 12);
      NCopyLoHi32(&pInfo->luCreationDateTime, &abuReply[28]);
      NCopyHiLo32(&pInfo->luOwnerID, &abuReply[32]);
      NCopyLoHi32(&pInfo->luArchivedDateTime, &abuReply[36]);
      NCopyHiLo32(&pInfo->luArchiverID, &abuReply[40]);
      NCopyLoHi32(&pInfo->luUpdatedDateTime, &abuReply[44]);
      NCopyHiLo32(&pInfo->luUpdatorID, &abuReply[48]);
      NCopyLoHi32(&pInfo->luFileSize, &abuReply[52]);
      NWCMemMove(pInfo->abuReservedB44, &abuReply[56], (nuint) 44);
      NCopyLoHi16(&pInfo->suRightsMask, &abuReply[100]);
      NCopyLoHi16(&pInfo->suAccessDate, &abuReply[102]);
      NCopyLoHi32(&pInfo->luDelFileTime, &abuReply[104]);
      NCopyLoHi32(&pInfo->luDelDateTime, &abuReply[108]);
      NCopyHiLo32(&pInfo->luDelID, &abuReply[112]);
      NWCMemMove(pInfo->abuReservedB16, &abuReply[116], (nuint) 16);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s27.c,v 1.7 1994/09/26 17:34:08 rebekah Exp $
*/
