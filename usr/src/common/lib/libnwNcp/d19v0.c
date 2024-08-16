/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d19v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS19v0GetEffRights
         (
            pNWAccess pNetAccess,
            nuint32  luEntryID,
            nuint32  luSubjectDNLen,
            pnstr16  pwstrSubjectDN,
            nuint32  luAttrNameLen,
            pnstr16  pwstrAttrName,
            pnuint32 pluRights,
         )

REMARKS:
/ *
DSV_GET_EFFECTIVE_RIGHTS                        19 (0x13)


Request

  version 0 (Client to Server)

     nuint32   version;
     nuint32   entryID;
     unicode   subjectDN;
     unicode   attrName;

  version 1 and version 2 (Server to Client)

     nuint32   version;
     nuint32   entryID;
     unicode   subjectDN;
     unicode   attrName;
     unicode   dn[]; //Both versions are the same with the exception of the array of
                     //distinguished names.

Response

  nuint32 rights;

Remarks

This verb allows you to get the 32-bit effective rights of a subject with respect to a proposed
operation on a protected Directory entry or its attribute or the following special names
permitted as the subjectDN:

DS_MASK_NAME
DS_ROOT_NAME
DS_PUBLIC_NAME

The following attribute names are possible:

DS_ALL_ATTRIBUTE_NAME
DS_SMS_RIGHTS_NAME
DS_ENTRY_RIGHTS_NAME

Any Attribute Name

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
NWNCPDS19v0GetEffRights
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   nuint32  luSubjectDNLen,
   pnstr16  pwstrSubjectDN,
   nuint32  luAttrNameLen,
   pnstr16  pwstrAttrName,
   pnuint32 pluRights
)
{
   nuint8   buReq[11]; /* 4 + 4 + 3 (pad) */
   nuint8   buRep[7]; /* 4 + 3 (pad) */
   nuint8   buReq2[7]; /* 4 + 3 (pad) */
   nuint8   buReq3[7]; /* 4 + 3 (pad) */
   NWCFrag  reqFrag[5];
   NWCFrag  repFrag[1];
   nuint    szActualRep;
   pnuint8  cur;
   NWRCODE  err;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &luEntryID);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 8;

   cur = buReq2;
   NAlign32(&cur);

   NCopyLoHi32(&cur[0], &luSubjectDNLen);

   reqFrag[1].pAddr = cur;
   reqFrag[1].uLen = 4;


   reqFrag[2].pAddr = pwstrSubjectDN;
   reqFrag[2].uLen = NPad32((nuint)luSubjectDNLen);

   cur = buReq3;
   NAlign32(&cur);

   NCopyLoHi32(&cur[0], &luAttrNameLen);

   reqFrag[3].pAddr = cur;
   reqFrag[3].uLen = 4;


   reqFrag[4].pAddr = pwstrAttrName;
   reqFrag[4].uLen = (nuint) luAttrNameLen;

   cur = buRep;
   NAlign32(&cur);

   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 4;

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)19, (nuint)5,
            reqFrag, (nuint)1, repFrag, &szActualRep);
   if (err < 0)
      return (err);

   NCopyLoHi32(pluRights, &cur[0]);

   return (err);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d19v0.c,v 1.7 1994/09/26 17:40:40 rebekah Exp $
*/
