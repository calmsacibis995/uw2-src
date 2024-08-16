/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d11v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS11v0DefineAttr
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            nuint32  luAttrNameLen,
            pnstr16  pwstrAttrName,
            nuint32  luSyntaxID,
            nuint32  luAttrLower,
            nuint32  luAttrUpper,
            nuint32  luASN1IDLen,
            pnuint8  pbaASN1ID
         )

REMARKS:
/ *

DSV_DEFINE_ATTR                                 11 (0x0B)


Request

  nuint32   version,
  nuint32   flags,
  pnstr16   pwstrAttrName,
  nuint32   attrSyntaxID,
  nuint32   attrLower,
  nuint32   attrUpper,
  int8      asn1ID

Response

  Returns only the NCP header.

Remarks

This verb allows you to add an attribute definition to the schema.

The following request parameters are defined:

The attrFlags defines the following bits:

DS_SINGLE_VALUED_ATTR   Set if the attribute is single-valued.

DS_SIZED_ATTR           Set if the attribute is sized, which indicates that a valid
                        value range is specified. This only applies to attributes
                        with string or integer syntaxes.

                        If this bit is set, and the attribute has a string syntax,
                        attrLower and attrUpper refer to the lower and upper
                        bounds on the number of characters; this range does not
                        include the NULL terminator.

                        If this bit is set and the attribute syntax is Integer,
                        attrLower and attrUpper describe a valid value range
                        rather than a size.

The attrName contains the name of the new attribute. Names can be from one to 32 characters
in length (excluding NULL termination).

The attrSyntaxID defines the syntax used for this attribute's values.The attrLower and attrUpper specify the lower and upper bounds for the attribute's size, or
the attribute's value, depending on the attribute syntax.

The asn1ID contains the ASN.1 ID assigned to this attribute. The ID is encoded according to
the ASN.1-BER rules. If no ASN.1 ID has been assigned, this parameter should be a
zero-length array, maximum 32 bytes (MAX_ASN1_NAME) or 0-filled array. The asn1ID
may be 32 bytes maximum (MAX_SCHEMA_NAME_CHARS). This is not interpreted as
character data.

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
NWNCPDS11v0DefineAttr
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luAttrNameLen,
   pnstr16  pwstrAttrName,
   nuint32  luSyntaxID,
   nuint32  luAttrLower,
   nuint32  luAttrUpper,
   nuint32  luASN1IDLen,
   pnuint8  pbaASN1ID
)
{
   nuint8   buReq1[83]; /* 4 + 4 + 4 + 66 + 2 + 3 (padding) */
   nuint8   buReq2[15]; /* 4 + 4 + 4 + 3 (padding) */
   nuint8   buReq3[7];  /* 4 + 3 (padding) */
   NWCFrag  reqFrag[4];
   pnuint8  cur;

   cur = buReq1;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], &luAttrNameLen);
   NWCMemMove(&cur[12], pwstrAttrName, (nuint)luAttrNameLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 12 + NPad32((nuint)luAttrNameLen);

   cur = buReq2;
   NAlign32(&cur);

   NCopyLoHi32(&cur[0], &luSyntaxID);
   NCopyLoHi32(&cur[4], &luAttrLower);
   NCopyLoHi32(&cur[8], &luAttrUpper);

   reqFrag[1].pAddr = cur;
   reqFrag[1].uLen = 12;

   cur = buReq3;
   NAlign32(&cur);

   NCopyLoHi32(&cur[0], &luASN1IDLen);

   reqFrag[2].pAddr = cur;
   reqFrag[2].uLen = 4;

   reqFrag[3].pAddr = pbaASN1ID;
   reqFrag[3].uLen = NPad32((nuint)luASN1IDLen);

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)11, (nuint)4,
            reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d11v0.c,v 1.7 1994/09/26 17:40:29 rebekah Exp $
*/
