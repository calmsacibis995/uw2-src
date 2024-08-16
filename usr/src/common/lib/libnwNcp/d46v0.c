/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d46v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS46v0RestoreObject
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            pnuint32 pluIterationHandle,
            nuint32  luParentID,
            nuint32  luRDNLen,
            pnstr16  pwstrRDN,
            nuint32  luEntryInfoLen,
            pnuint8  pEntryInfo,
            nuint32  luDNLen,
            pnuint8  pDN
         )

REMARKS:
/ *
DSV_RESTORE_ENTRY                               46 (0x2E)


Request

  nuint32      version
  nuint32      flags;
  nuint32      iterationHandle;
  nuint32      parentID;
  unicode      rdn;
  BackupInfo   entryInfo;    //defined below

Response

  nuint32   iterationHandle;
  unicode   dn;           //if flags & DS_RESTORE_MOVING)

Definitions of Parameter Types

  struct  BackupInfo
  {
     nuint32      version;
     nuint32      flags;
     nuint32      chunkNumbers;
     Attribute    attributes[]; //defined below
  }

  struct  Attribute
  {
     unicode attrName;
     any_t   attrValue[];
  }

Remarks

* /

ARGS:

INCLUDE:

RETURN:

SERVER:

CLIENT:

SEE:

NCP:

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

N_GLOBAL_LIBRARY(NWRCODE)
NWNCPDS46v0RestoreObject
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   pnuint32 pluIterationHandle,
   nuint32  luParentID,
   nuint32  luRDNLen,
   pnstr16  pwstrRDN,
   nuint32  luEntryInfoLen,
   pnuint8  pEntryInfo,
   nuint32  luDNLen,
   pnuint8  pDN
)
{
   nuint8   buReq[287];    /* 4 + 4 + 4 + 4 + 4 + 258 + 2 + 3 (padding) + 4 */
   nuint8   buRep1[7];     /* 4 + 3 (padding) */
   nuint8   buRep2[523];   /* 4 + 514 + 2 + 3 (padding) */
   NWCFrag  reqFrag[2];
   NWCFrag  repFrag[2];
   pnuint8  cur;
   nuint    uReplySize;
   nuint    uCurPos;
   NWRCODE  err;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], pluIterationHandle);
   NCopyLoHi32(&cur[12], &luParentID);
   NCopyLoHi32(&cur[16], &luRDNLen);
   NWCMemMove(&cur[20], pwstrRDN, (nuint)luRDNLen);
   uCurPos = 20 + NPad32((nuint)luRDNLen);
   NCopyLoHi32(&cur[uCurPos], &luEntryInfoLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = uCurPos + 4;

   reqFrag[1].pAddr = pEntryInfo;
   reqFrag[1].uLen = (nuint)luEntryInfoLen;

   cur = buRep1;
   NAlign32(&cur);

   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 4;

   cur = buRep2;
   NAlign32(&cur);

   repFrag[1].pAddr = cur;
   repFrag[1].uLen = (nuint)luDNLen;

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)46, (nuint)2,
            reqFrag, (nuint)2, repFrag, &uReplySize);

   if (err < 0)
      return err;

   cur = repFrag[0].pAddr;
   NCopyLoHi32(pluIterationHandle, &cur[0]);

   if (pDN != NULL)
   {
      cur = repFrag[1].pAddr;
      NCopyLoHi32(&luDNLen, &cur[0]);
      NWCMemMove((pnuint32)pDN, &cur[4], (nuint)luDNLen);
   }

   return err;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d46v0.c,v 1.7 1994/09/26 17:41:04 rebekah Exp $
*/
