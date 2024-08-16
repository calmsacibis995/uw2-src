/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d9v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS9v0ModifyObject
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            nuint32  luEntryID,
            nuint32  luAttrModLen,
            pnuint8  pAttrModData,
         )

REMARKS:
/ *
DSV_MODIFY_ENTRY                                9 (0x09)


Request

  nuint32   version;
  nuint32   flags;
  nuint32   entryID;
  AttrMod changes[];  //defined below

Response

  Returns only the NCP header.

Definitions of Parameter Types

  union AttrMod
  {
     AttributeDS_ADD_ATTRIBUTE     addAttribute; //defined below
     unicode DS_REMOVE_ATTRIBUTE   removeAttribute;
     AttributeDS_ADD_VALUE         addValues;
     AttributeDS_REMOVE_VALUE      removeValues;
     AttributeDS_ADDITIONAL_VALUE  additionalValues;
     AttributeDS_OVERWRITE_VALUE   overWrite;
     AttributeDS_CLEAR_ATTRIBUTE   ClearAttributes;
     AttributeDS_CLEAR_VALUE       ClearValue;
  };

  struct  Attribute
  {
     unicode attrName;
     any_t   attrValue[];
  };

Remarks

This verb allows you to perform one or more of the following series of modifications to the
attributes of an entry.

Add a new attribute
Remove an attribute
Add attribute valuesRemove attribute values
Replace attribute values
Modify an alias.

The following request parameters are defined:

The version ...

The flags ...

The entryID should be obtained as a result of a Resolve Name transaction. The entryID
identifies the entry to be modified.

The changes[] defines a sequence of modifications which are applied in the order specified. If
any of the individual modifications fail, an error is generated and the entry remains as it was
prior to the operation. That is, the operation is atomic. The end result of the sequence of
modifications shall not violate the Directory schema. However, it is possible, and sometimes
necessary, for the individual AttrMod changes to appear to do so.

The following types of modifications may occur:

The DS_ADD_ATTRIBUTE union tag identifies an attribute by name and specifies one or
more attribute values to be added to the attribute. An attempt to add an already existing
attribute results in an error.

The DS_REMOVE_ATTRIBUTE union tag identifies an attribute by name and specifies that
the attribute be removed from the entry. Any attempt to remove a non-existing attribute
results in an error. Note that this operation is not allowed if the attribute is present in the
RDN.

The DS_ADD_VALUE union tag identifies an attribute by name and specifies one or more
attribute values to be added to the attribute. An attempt to add an already existing value
results in an error.

The DS_REMOVE_VALUE union tag identifies an attribute name and specifies one or more
attribute values to be removed from the attribute. If the values are not present, this results in
an attribute error. If an attempt is made to modify the entry class attribute an error is
returned. Note that this operation is not allowed if the attribute is present in the RDN. Value
may be replaced by a combination of addValues and removeValues in a single Modify Entry
operation.

The DS_ADDITIONAL_VALUE union tag identifies an attribute by name and specifies one
or more additional values ... The DS_OVERWRITE_VALUE union tag identifies an attribute by the attribute name in the
argument, and specifies one or more attribute values to be overwritten.


The DS_CLEAR_ATTRIBUTE union tag identifies an attribute by name and specifies one or
more attribute names to be cleared. An attempt to clear a nonexisting attribute results in an
error.

The DS_CLEAR_VALUE union tag identifies an attribute by name and specifies one or more
attribute values to be cleared. An attempt to clear a nonexisting value results in an error.

If the operation succeeds, only the NCP header with a zero completion code is returned.

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
NWNCPDS9v0ModifyObject
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luEntryID,
   nuint32  luAttrModLen,
   pnuint8  pAttrModData
)
{
   nuint8   buReq[15];  /* 4 + 4 + 4 + 3 (padding) */
   NWCFrag  reqFrag[2];
   pnuint8  cur;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], &luEntryID);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 12;

   reqFrag[1].pAddr = pAttrModData;
   reqFrag[1].uLen = (nuint)luAttrModLen;

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)9, (nuint)2,
            reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d9v0.c,v 1.7 1994/09/26 17:41:30 rebekah Exp $
*/
