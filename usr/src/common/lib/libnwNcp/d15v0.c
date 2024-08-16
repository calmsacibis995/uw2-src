/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d15v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"
#include "nwdserr.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS15v0ReadClassDef
         (
            pNWAccess pNetAccess,
            pnuint32 pluIterationHandle,
            nuint32  luInfoType,
            nuint32  blAllClasses,
            nuint32  luClassNamesLen,
            pnuint8  pClassNames,
            pnuint32 pluClassInfoLen,
            pnuint8  pClassInfo
         )

REMARKS:
/ *
DSV_READ_CLASS_DEF                              15 (0x0F)


Request

  nuint32   version;
  nuint32   iterationHandle;
  nuint32   infoType;
  boolean   allClasses;
  unicode   classNames[];

Response

  nuint32   iterationHandle;
  ClassInfo classInfo;   //defined below

Definitions of Parameter Types

     union   ClassInfo
     {
     unicode   classNames[];
     ClassDef  InfoClassDefInfo;  //defined below
     ClassDef  classDefs[];    //defined below
     }

     struct  ClassDef
     {
        ClassDef  InfoclassDefInfo;  //defined below
        unicode   superClasses[];
        unicode   containmentClasses[];
        unicode   namingAttrs[];
        unicode   mandatoryAttrs[];
        unicode   optionalAttrs[];
     };

     struct  ClassDefInfo
     {
        unicode className;
        nuint32   classFlags;
        int8      asn1ID;
     }
Remarks

This verb allows you to read class definitions from the schema.

The iterationHandle in the request should be 0xFFFFFFFF if this is the initial message in an
iteration. Subsequent requests should supply the iteration ID returned in the previous
response.

The infoType, allClasses, and classNames parameters indicate what information from the
schema is requested.

If allClasses is TRUE, information about all classes in the schema is requested.

If allClasses is FALSE, information about classes named in the classNames is requested.

If allClasses is FALSE and classNames is empty, no class information will be returned. (This
can be used to verify read access to the schema.)

The NoSuchClass error is returned only allClasses is FALSE and none of the classes selected
is present.

As listed below, the infoType specifies a request for class name definition only or for both
class name definition and class definition, or for expanded class definition, or for class
definitions information:

ValueInfoType

0    DS_CLASS_DEF_NAMES
1    DS_CLASS_DEF_NAMES and DS_CLASS_DEFS
2    DS_EXPANDED_CLASS_DEFS
3    DS_INFO_CLASS_DEFS

If the classNames request is such as to request no classes, this component is not meaningful.

The iterationHandle in the response is set to 0xFFFFFFFF if this message contains the
remainder of the response. Otherwise, this ID can be used in subsequent Read Class
Definition requests to obtain further portions of the response. The level of granularity for
partial results is a ClassInfo structure.

The classInfo conveys information on the classes defined in the schema. This union either
contains a list of class names or an array of structures containing a complete class definition
or the ClassDefInfo. The type of information returned depends on infoType in the request.
The className field contains the name of the class. Names can be from one to 32 characters
in length (MAX_SCHEMA_NAME_CHARS), excluding termination.

The classFlags field defines the following bits:

DS_CONTAINER_CLASS         Set if entries of this class can have subordinates in the
                           Directory tree.

DS_EFFECTIVE_CLASS         Set if this class can be specified as the base class of a
                           newly created entry.

DS_NONREMOVABLE_CLASS      Set if the class may not be removed from the schema.
                           The classes defined by Novell are nonremovable.

DS_AMBIGUOUS_NAMING        Set if the class ...

DS_AMBIGUOUS_CONTAINMENT   Set if the class...

The asn1ID contains the ASN.1 ID assigned to this class. The ID is encoded according to the
ASN.1-BER rules. If no ASN.1 ID has been assigned, this will be a zero-filled array,
maximum 32 characters (MAX_ASN1_NAME). The asn1ID may be 32 characters maximum
(MAX_SCHEMA_NAME_CHARS). This is not interpreted as character data.

The superClasses field lists the classes which are designated as super classes for the class. The
class inherits the super-classes' definitions.

The containmentClasses field lists the classes which may appear as superiors to entries of this
class in the Directory tree.

The namingAttrs field lists the attributes which may be used for naming entries of this class.

The mandatoryAttrs field lists the attributes which must be defined for any entry of this class.

The optionalAttrs field lists the attributes which may optionally appear as a part of an entry of
this class.

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
NWNCPDS15v0ReadClassDef
(
   pNWAccess pNetAccess,
   pnuint32 pluIterationHandle,
   nuint32  luInfoType,
   nuint32  blAllClasses,
   nuint32  luClassNamesLen,
   pnuint8  pClassNames,
   pnuint32 pluClassInfoLen,
   pnuint8  pClassInfo
)
{
   nuint8   buReq[19]; /* 4 + 4 + 4 + 4 + 3(pad) */
   nuint8   buRep[15]; /* 4 + 4 + 4 + 3(pad) */
   NWCFrag  reqFrag[2];
   NWCFrag  repFrag[2];
   pnuint8  cur;
   nuint32  unionTag;
   nuint    numOfRep,
            szActRep;
   NWRCODE  err;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], pluIterationHandle);
   NCopyLoHi32(&cur[8], &luInfoType);
   NCopyLoHi32(&cur[12], &blAllClasses);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 16;

   if ((blAllClasses == N_FALSE) && (pClassNames != NULL))
   {
      reqFrag[1].pAddr = pClassNames;
      reqFrag[1].uLen = NPad32((nuint)luClassNamesLen);

   }
   else
   {
      nuint8   buReq2[7];  /* 4 + 3(pad) */

      cur = buReq2;
      NAlign32(&cur);

      /* put 4 bytes of 0 for no names buffer or All attrs is TRUE */
      *(pnuint32)&cur[0] = (nuint32)0;

      reqFrag[1].pAddr = cur;
      reqFrag[1].uLen = 4;
   }

   cur = buRep;
   NAlign32(&cur);

   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 8;

   if ((blAllClasses == N_TRUE) ||
            (blAllClasses == N_FALSE && pClassNames != NULL))
   {
      repFrag[1].pAddr = pClassInfo;
      repFrag[1].uLen = (nuint)*pluClassInfoLen;

      numOfRep = 2;

   }
   else
   {
      repFrag[0].uLen = 12;
      numOfRep = 1;
   }

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)15, (nuint)2,
            reqFrag, (nuint)numOfRep, repFrag, &szActRep);
   if (err < 0)
      return (err);

   if (szActRep < repFrag[0].uLen)
      return ((NWRCODE)ERR_SYSTEM_ERROR);

   NCopyLoHi32(pluIterationHandle, &cur[0]);
   NCopyLoHi32(&unionTag, &cur[4]);

   if (unionTag != luInfoType)
      return ((NWRCODE)ERR_INVALID_UNION_TAG);

   *pluClassInfoLen = szActRep;

   return (err);

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d15v0.c,v 1.7 1994/09/26 17:40:34 rebekah Exp $
*/
