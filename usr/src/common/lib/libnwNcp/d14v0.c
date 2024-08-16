/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d14v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS14v0DefineClass
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            nuint32  luClassNameLen,
            pnstr16  pwstrClassName,
            nuint32  luASN1IDLen,
            pnuint8  pbuASN1ID,
            nuint32  luClassDataLen,
            pnuint8  pbuClassData
         )

REMARKS:
/ *
DSV_DEFINE_CLASS                                14 (0x0E)


Request

  nuint32   version;
  nuint32   flags;
  unicode className;
  int8    asn1ID[];
  unicode superClasses[];
  unicode containmentClasses[];
  unicode namingAttrs[];
  unicode mandatoryAttrs[];
  tdr-str optionalAttrs[];

Response

  Returns only the NCP header.

Remarks

This verb allows you to create a new class definition for the schema.

The following request parameters are defined:

The Flags defines the following bits:

DS_CONTAINER_CLASS    Set if entries of this class can have subordinates in the
                      Directory tree.

DS_EFFECTIVE_CLASS    Set if this class can be specified as the base class of a newly
                      created entry.

The className contains the name of the new class. Names can be from one to 32 characters
in length (MAX_SCHEMA_NAME_CHARS), excluding NULL termination.

The asn1ID contains the ASN.1 ID assigned to this class. The ID is encoded according to the
ASN.1-BER rules. If no ASN.1 ID has been assigned, this parameter should be a zero-length
array, maximum 32 bytes (MAX_ASN1_NAME) or a 0-filled array.

The superClasses lists the classes which are designated as super-classes for the new class. The
new class inherits the super-classes' definitions.
The containmentClasses lists the classes which may appear as superiors to entries of this class
in the directory tree.

The namingAttrs lists the attributes which may be used for naming entries of this class.

The mandatoryAttrs lists the attributes which must be defined for any entry of this class.

The optionalAttrs lists the attributes which may optionally appear as a part of an entry of this
class.

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
NWNCPDS14v0DefineClass
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luClassNameLen,
   pnstr16  pwstrClassName,
   nuint32  luASN1IDLen,
   pnuint8  pbuASN1ID,
   nuint32  luClassDataLen,
   pnuint8  pbuClassData
)
{
   nuint8   buReq1[83]; /* 4 + 4 + 4 + 66 + 2 + 3(pad)*/
   nuint8   buReq2[7];  /* 4 + 3 (padding) */
   NWCFrag  reqFrag[4];
   pnuint8  cur;

   cur = buReq1;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], &luClassNameLen);
   NWCMemMove(&cur[12], pwstrClassName, (nuint)luClassNameLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 12 + NPad32((nuint)luClassNameLen);

   cur = buReq2;
   NAlign32(&cur);

   NCopyLoHi32(&cur[0], &luASN1IDLen);

   reqFrag[1].pAddr = cur;
   reqFrag[1].uLen = 4;

   reqFrag[2].pAddr = pbuASN1ID;
   reqFrag[2].uLen = NPad32((nuint)luASN1IDLen);

   reqFrag[3].pAddr = pbuClassData;
   reqFrag[3].uLen = (nuint)luClassDataLen;

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)14, (nuint)4,
            reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d14v0.c,v 1.7 1994/09/26 17:40:33 rebekah Exp $
*/
