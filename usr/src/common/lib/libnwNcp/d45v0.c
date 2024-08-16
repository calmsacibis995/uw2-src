/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d45v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS45v0BackupObject
         (
            pNWAccess pNetAccess,
            pnuint32 pluIterationHandle,
            nuint32  luEntryID,
            pnuint32 pluBackupInfoLen,
            pnuint8  pBackupInfo
         )

REMARKS:
/ *
DSV_BACKUP_ENTRY                                45 (0x2D)


Request

  nuint32 version;
  nuint32 iterationHandle;
  nuint32 entryID;

Response

  nuint32      iterationHandle;
  BackupInfo info;         //defined below

Definitions of Parameter Types

  struct  BackupInfo
  {
     nuint32   version;
     nuint32   flags;
     nuint32   ChunkNumber;
     Attribute Attributeinfo[];
  }

  struct  Attribute
  {
     unicode attrName;
     any_t   attrValue[];
  }

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
NWNCPDS45v0BackupObject
(
   pNWAccess pNetAccess,
   pnuint32 pluIterationHandle,
   nuint32  luEntryID,
   pnuint32 pluBackupInfoLen,
   pnuint8  pBackupInfo
)
{
   NWRCODE  rcode;
   nuint8   buReq[15];  /* 4 + 4 + 4 + 3 (padding) */
   nuint8   buRep[7];   /* 4 + 3 (padding) */
   NWCFrag  reqFrag[1];
   NWCFrag  repFrag[2];
   nuint    uReplySize;
   pnuint8  cur;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], pluIterationHandle);
   NCopyLoHi32(&cur[8], &luEntryID);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 12;

   cur = buRep;
   NAlign32(&cur);

   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 4;

   repFrag[1].pAddr = pBackupInfo;
   repFrag[1].uLen = (nuint)*pluBackupInfoLen;

   rcode = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)45, (nuint)1,
            reqFrag, (nuint)2, repFrag, &uReplySize);

   /* cur still points to the iteration return buffer */
   NCopyLoHi32(pluIterationHandle, cur);

   /* subtract off the length of the first reply frag to derive
      the length of the data being returned. */
   *pluBackupInfoLen = (nuint32)(uReplySize - 4);

   return(rcode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d45v0.c,v 1.7 1994/09/26 17:41:03 rebekah Exp $
*/
