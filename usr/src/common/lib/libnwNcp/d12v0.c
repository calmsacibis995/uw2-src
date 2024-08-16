/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d12v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"
#include "nwdserr.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS12v0ReadAttrDef
         (
            pNWAccess pNetAccess,
            pnuint32 pluIterationHandle,
            nuint32  luInfoType,
            nuint32  blAllAttributes,
            nuint32  luAttrNamesLen,
            pnuint8  pAttrNames,
            pnuint32 pluAttrDefInfoLen,
            pnuint8  pAttrDefInfo
         )

REMARKS:
/ *
DSV_READ_ATTR_DEF                               12 (0x0C)


Request

  nuint32   version;
  nuint32   iterationHandle;
  nuint32   infoType;
  boolean   allAttributes;
  unicode   attrNames[];

Response

  nuint32      iterationHandle;
  AttrDefInfo  attrInfo;     //defined below

Definitions of Parameter Types

  union   AttrDefInfo
  {
     unicode attrNames[];
     AttrDef attrDefs[];   //defined below
  };

  struct  AttrDef
  {
     unicode   Name;
     nuint32   Flags;
     nuint32   SyntaxID;
     nuint32   Lower;
     nuint32   Upper;
     int8      asn1ID[];
  };

Remarks

This verb allows you to list attribute definitions from the schema.

The iterationHandle in the request should be 0xFFFFFFFF if this is the initial message in an
iteration. Subsequent requests should supply the iteration ID returned in the previous
response.
The infoType, allAttributes, and attrNames parameters indicate what information from the
schema is requested.

If allAttributes is TRUE, information about all attributes in the schema is requested.

If allAttributes is FALSE, information about attributes named in the attrNames is requested.

If allAttributes is FALSE and attrNames is empty, no attribute information will be returned.
(This can be used to verify read access to the schema.) The NoSuchAttribute error is returned
only if allAttributes is FALSE and none of the attributes selected is present.

As listed below, the infoType specifies a request for attribute name information only or both
attribute name and attribute definition information:

ValueInfoType

0    DS_ATTR_DEF_NAMES
1    DS_ATTR_DEF_NAMES and DS_ATTR_DEFS

If the attrNames in the request is such as to request no attributes, this component is not
meaningful.

The iterationHandle in the response is set to 0xFFFFFFFF if this message contains the
remainder of the response. Otherwise, this ID can be used in subsequent Read Attribute
Definition requests to obtain further portions of the response. The level of granularity for
partial results is an AttrDefInfo structure.

The attrInfo conveys the information on the attributes defined in the schema. This union either
contains a list of attribute names or an array of attribute definition structures. The type of
information returned depends on the infoType in the request.

The Name field contains the name of an attribute. Names can be from one to 32 characters in
length (MAX_SCHEMA_CHARS), excluding NULL termination.
The Flags field defines the following bits:

DS_SINGLE_VALUED_ATTR   Set if the attribute is single-valued.

DS_SIZED_ATTR           Set if the attribute is sized. This only applies to attributes
                        with string or integer syntaxes. If this bit is set, and the
                        attribute has a string syntax, the Lower and Upper fields
                        refer to the lower and upper size boundaries of the number
                        of characters permitted in a value. This does not include
                        the NULL termination. If this bit is set and the attribute
                        syntax is Integer, the lower and upper bounds describe a
                        valid value range.

DS_NONREMOVABLE_ATTR    Set if the attribute may not be deleted from the schema.

DS_READ_ONLY_ATTR       Set if the attribute values are not writeable by a client
                        (only writeable by DS). These attributes may be made by
                        the client.

DS_HIDDEN_ATTR          Set if the attribute is not readable or writeable by a client.

DS_STRING_ATTR          Set if the attribute syntax is of a string type.

DS_SYNC_IMMEDIATE       Set if the attribute...

DS_PUBLIC_READ          Set if the attribute can be read publicly

DS_SERVER_READ          Set if the attribute...

DS_WRITE_MANAGED        Set if the attribute...

DS_SF_PER_REPLICA_ATTR  Set if attribute values are server centric and not
                        synchronized with other replicas.

The SyntaxID field defines the syntax of this attribute.

The Lower and Upper fields specify the lower and upper bounds for the attribute's size, or
the attribute's value, depending on the attribute syntax. This only has meaning if the
DS_SIZED_ATTR bit is set in the Flags field.

The asn1ID field contains the ASN.1 ID assigned to this attribute. The ID is encoded
according to the ASN.1-BER rules. If no ASN.1 ID has been assigned, this field will be a
zero-filled array of MAX_ASN1_NAME.

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
NWNCPDS12v0ReadAttrDef
(
   pNWAccess pNetAccess,
   pnuint32 pluIterationHandle,
   nuint32  luInfoType,
   nuint32  blAllAttributes,
   nuint32  luAttrNamesLen,
   pnuint8  pAttrNames,
   pnuint32 pluAttrDefInfoLen,
   pnuint8  pAttrDefInfo
)
{
   nuint8   buReq1[19]; /* 4 + 4 + 4 + 4 + 3 (padding) */
   nuint8   buReq2[7];  /* 4 + 3 (padding) */
   nuint8   buRep[15]; /* 4 + 4 + 4 + 3 (padding) */
   NWCFrag  reqFrag[3];
   NWCFrag  repFrag[3];
   pnuint8  cur;
   nuint32  unionTag;
   nuint    numOfRep,
            numOfReq,
            szActRep;
   NWRCODE  err;

   cur = buReq1;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], pluIterationHandle);
   NCopyLoHi32(&cur[8], &luInfoType);
   NCopyLoHi32(&cur[12], &blAllAttributes);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 16;

   cur = buReq2;
   NAlign32(&cur);

   if ((blAllAttributes == N_FALSE) && (pAttrNames != NULL))
   {
      reqFrag[1].pAddr = pAttrNames;
      reqFrag[1].uLen = NPad32((nuint)luAttrNamesLen);

      numOfReq = 2;
   }
   else
   {
      /* put 4 bytes of 0 for no names buffer or All attrs is TRUE */
      *(pnuint32)&cur[0] = (nuint32)0;

      reqFrag[1].pAddr = cur;
      reqFrag[1].uLen = 4;

      numOfReq = 2;
   }

   cur = buRep;
   NAlign32(&cur);

   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 8;

   if ((blAllAttributes == N_TRUE) ||
            (blAllAttributes == N_FALSE && pAttrNames != NULL))
   {

      repFrag[1].pAddr = pAttrDefInfo;
      repFrag[1].uLen = (nuint)*pluAttrDefInfoLen;

      numOfRep = 2;
   }
   else
   {
      numOfRep = 1;
   }

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)12, numOfReq,
            reqFrag, numOfRep, repFrag, &szActRep);
   if (err < 0)
      return (err);

   if (szActRep < repFrag[0].uLen)
      return ((NWRCODE)ERR_SYSTEM_ERROR);

   NCopyLoHi32(pluIterationHandle, &cur[0]);
   NCopyLoHi32(&unionTag, &cur[4]);

   if (unionTag != luInfoType)
      return ((NWRCODE)ERR_INVALID_UNION_TAG);

   *pluAttrDefInfoLen = szActRep - repFrag[0].uLen;

   return (err);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d12v0.c,v 1.7 1994/09/26 17:40:30 rebekah Exp $
*/
