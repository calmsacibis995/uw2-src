/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d3v1.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"
#include "nwdserr.h"
#include "nwdsdefs.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS3v1Read
         (
            pNWAccess pNetAccess,
            nuint32  luProtocolFlags,
            pnuint32 pluIterationHandle,
            nuint32  luEntryID,
            nuint32  luInfoType,
            nuint32  blAllAttributes,
            nuint32  luAttrNamesLen,
            pnuint8  pAttrNames,
            nuint32  luSubjectLen,
            pnuint16 pSubjectName,
            pnuint32 pluEntryInfoLen,
            pnuint8  pEntryInfo
         )

REMARKS:
/ *
DSV_READ                                        3 (0x03)


Request

  nuint32   version;
  nuint32   protocolFlags;
  nuint32   iterationHandle;
  nuint32   entryID;
  nuint32   infoType;
  boolean   allAttributes;
  unicode   attrNames[];

if (infoType == 2)

  unicode dn;

Response

  nuint32   iterationHandle;
  Attribute entryInfo;   //defined below

Definitions of Parameter Types

  union Attribute
  {
     unicode   attrNames[];
     Attribute attributes[];//defined below
     Attribute effectiveRights;
  };

  struct  Attribute
  {
     nuint32   attrSyntaxID;
     unicode   attrName;
     any_t     attrValue[];
  };

Remarks

This verb allows you to read attribute values from an explicitly identified entry. It also allows
you to verify a distinguished name. In the second case, the existence of the name was verified
first using the Resolve Name request.
The entryID identifies the entry for which information is requested.

The iterationHandle is used to continue a previous request where the response was too large
to fit in a single message. It should be set initially to 0xFFFFFFFF and thereafter to the value
returned by the previous response.

The infoType, allAttributes,and attrNames parameters indicate what information from the
entry is requested. If allAttributes is TRUE, information about all attributes of the entry is
requested. If allAttributes is FALSE, information about attributes named in the attrNames is
requested. If allAttributes is FALSE and attrNames is empty, no attribute information will be
returned.

As listed below, the infoType specifies a request for attribute name information only, for both
attribute name and attribute value information, or for effective privileges information:

ValueInfoType

0    DS_ATTRIBUTE_NAME
1    DS_ATTRIBUTE_NAME and DS_ATTRIBUTE_VALUE
2    DS_EFFECTIVE_PRIVILEGES

If the attributeSelection is such as to request no attributes, this component is not meaningful.

The type definition for attrValue uses the any_t base type. This type means that the encoding
for an attribute value is encoded based on the syntax of the attribute. Each syntax has its own
encoding. For example, any_t denotes one of the defined encoded syntax (See the section
Attribute Syntaxes: NCP Encoding Rules in this chapter). In this instance, the encoding of an
attribute value depends on the accompanying syntax.

The TDR decoding routine will have to interpret the syntax in order to decode the
accompanying attribute value array. The any_t encodes the value by treating it as an int8
array. If a client agent doesn't know how to decode a particular attribute, that attribute can be
skipped by treating the array count as the size of the undecoded value.

The iterationHandle in the response is set to 0xFFFFFFFF if this message contains the
remainder of the response. Otherwise, this ID can be used in subsequent Read requests to
obtain further portions of the response. The level of granularity for partial results is an
individual attribute value of a value set. If a value set is split across two or more response
messages, the attribute information (syntaxID, attrName, valueCount [count of values in
current response]) of the current value set is repeated in each response. Note that an
individual value cannot be split across multiple responses. This means that a value (plus
message overhead) cannot be bigger than the minimum-maximum message size supported by
the NCP protocol (63K).
The entryInfo in the response holds the requested information. This union either contains a
list of attribute names or an array of structures containing the attribute name, attribute syntax
ID, and value set. The type of information returned depends on the infoType in the request.

If infoType is DS_EFFECTIVE_PRIVILEGES, the response packet is the following:

syntax: SYN_INTEGER


The allAttributes is interpreted as all attributes possible for the baseClass of the object.

If dn is NULL, the rights calculated are those of the logged in client.

If dn is not NULL, the rights calculated are those of "dn."

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
NWNCPDS3v1Read
(
   pNWAccess pNetAccess,
   nuint32  luProtocolFlags,
   pnuint32 pluIterationHandle,
   nuint32  luEntryID,
   nuint32  luInfoType,
   nuint32  blAllAttributes,
   nuint32  luAttrNamesLen,
   pnuint8  pAttrNames,
   nuint32  luSubjectLen,
   pnuint16 pSubjectName,
   pnuint32 pluEntryInfoLen,
   pnuint8  pEntryInfo
)
{
   nuint8   buReq1[27]; /* 4 + 4 + 4 + 4 + 4 + 4 + 3 (padding) */
   nuint8   buReq2[7];  /* 4 + 3 (padding) */
   nuint8   buReq3[7];  /* 4 + 3 (padding) */
   nuint8   buRep[15];  /* 4 + 4 + 4 + 3 (padding) */
   NWCFrag  reqFrag[4];
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
   *(pnuint32)&cur[0] = (nuint32)1;
   NCopyLoHi32(&cur[4], &luProtocolFlags);
   NCopyLoHi32(&cur[8], pluIterationHandle);
   NCopyLoHi32(&cur[12], &luEntryID);
   NCopyLoHi32(&cur[16], &luInfoType);
   NCopyLoHi32(&cur[20], &blAllAttributes);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 24;

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
      cur = buReq2;
      NAlign32(&cur);

      /* put 4 bytes of 0 for no names buffer or All attrs is TRUE */
      *(pnuint32)&cur[0] = (nuint32)0;

      reqFrag[1].pAddr = cur;
      reqFrag[1].uLen = 4;

      numOfReq = 2;
   }

   if (luInfoType == DS_EFFECTIVE_PRIVILEGES)
   {
      cur = buReq3;
      NAlign32(&cur);

      NCopyLoHi32(&cur[0], &luSubjectLen);

      reqFrag[numOfReq].pAddr = cur;
      reqFrag[numOfReq].uLen = 4;

      reqFrag[numOfReq+1].pAddr = pSubjectName;
      reqFrag[numOfReq+1].uLen = NPad32((nuint)luSubjectLen);

      numOfReq += 2;
   }

   cur = buRep;
   NAlign32(&cur);

   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 8;

   if ((blAllAttributes == N_TRUE) ||
            (blAllAttributes == N_FALSE && pAttrNames != NULL))
   {
      repFrag[1].pAddr = pEntryInfo;
      repFrag[1].uLen = (nuint)*pluEntryInfoLen;
      numOfRep = 2;
   }
   else
   {
      numOfRep = 1;
   }

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)3, numOfReq,
            reqFrag, (nuint)numOfRep, repFrag, &szActRep);
   if (err < 0)
      return (err);

   if (szActRep < repFrag[0].uLen)
      return ((NWRCODE)ERR_SYSTEM_ERROR);

   NCopyLoHi32(pluIterationHandle, cur);
   NCopyLoHi32(&unionTag, &cur[4]);

   if (unionTag != luInfoType)
      return ((NWRCODE)ERR_INVALID_UNION_TAG);

   *pluEntryInfoLen = repFrag[1].uLen;

   return (err);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d3v1.c,v 1.7 1994/09/26 17:40:58 rebekah Exp $
*/
