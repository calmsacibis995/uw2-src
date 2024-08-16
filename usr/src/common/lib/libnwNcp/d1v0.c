/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d1v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS1v0ResolveName
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            nuint32  luScopeOfReferral,
            nuint32  luTargetEntryNameLen,
            pnstr16  pwstrTargetEntryName,
            nuint32  luTransportTypeCnt,
            pnuint32 pluTransportType,
            nuint32  luTreewalkerTypeCnt,
            pnuint32 pluTreewalkerType,
            pnuint32 pluResolveInfoLen,
            pnuint8  pResolveInfo
         )

REMARKS:
/ *
DSV_RESOLVE_NAME                                1 (0x01)


Request

  nuint32   version;
  nuint32   flags;
  nuint32   scopeOfReferral;
  unicode targetEntryName;
  nuint32   transportType[];
  nuint32   treewalkerType[];

Response

  ResolveInforesolvedName; //defined below

Definitions of Parameter Types

  union   ResolveInfo
  {
     NotFound         entryNotFound;     //defined below
     LocalEntryInfo   localEntryFound;   //defined below
     AliasEntry       aliasDereferenced; //defined below
     ReferralInfo     referrals[];       //defined below
     RemoteEntryInfo  remoteEntryFound;  //defined below
  }

  struct  NotFound
  {
     nuint32entryID;
     nuint32resolvedOffset;
  }

  struct  LocalEntryInfo
  {
     nuint32   entryID;
     Referralreferral[];
  }

  struct  AliasEntry
  {
     unicode newDN;
  }
  struct  Referral
  {
     nuint32type;
     int8 address[];
  }

  struct  ReferralInfo
  {
     nuint32   depthTag;
     nuint32   referralCount;
     Referralreferrals[];
  }

  struct  RemoteEntryInfo
  {
     nuint32entryID;
     nuint32reserved;
     nuint32referral[];
  }

Remarks

This verb allows you to resolve a portion of an NDS entry name and return a set of servers
which may be able to continue the name resolution process.

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
NWNCPDS1v0ResolveName
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luScopeOfReferral,
   nuint32  luTargetEntryNameLen,
   pnstr16  pwstrTargetEntryName,
   nuint32  luTransportTypeCnt,
   pnuint32 pluTransportType,
   nuint32  luTreewalkerTypeCnt,
   pnuint32 pluTreewalkerType,
   pnuint32 pluResolveInfoLen,
   pnuint8  pResolveInfo
)
{
   nuint8   buReq1[535],
            buReq2[21];
   pnuint8  cur;
   NWCFrag  reqFrag[2],
            replyFrag[1];

   cur = buReq1;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;

   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], &luScopeOfReferral);

   NCopyLoHi32(&cur[12], &luTargetEntryNameLen);
   NWCMemMove(&cur[16], pwstrTargetEntryName, (nuint)luTargetEntryNameLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 16 + NPad32((nuint)luTargetEntryNameLen);

   cur = buReq2;
   NAlign32(&cur);

   NCopyLoHi32(&cur[0], &luTransportTypeCnt);
   NCopyLoHi32(&cur[4], pluTransportType);
   NCopyLoHi32(&cur[8], &luTreewalkerTypeCnt);
   NCopyLoHi32(&cur[12],pluTreewalkerType);

   reqFrag[1].pAddr = cur;
   reqFrag[1].uLen = 16;


   replyFrag[0].pAddr = pResolveInfo;
   replyFrag[0].uLen = (nuint)*pluResolveInfoLen;

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)1, (nuint)2,
      reqFrag, (nuint)1, replyFrag, (pnuint)pluResolveInfoLen);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d1v0.c,v 1.7 1994/09/26 17:40:41 rebekah Exp $
*/
