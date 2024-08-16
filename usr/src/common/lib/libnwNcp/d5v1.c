/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d5v1.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS5v1List
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            pnuint32 pluIterationHandle,
            nuint32  luParentID,
            nuint32  luInfoFlags,
            nuint32  luNameFilterLen,
            pnstr16  pwstrNameFilter,
            nuint32  luClassFilterLen,
            pnstr16  pwstrClassFilter,
            pnuint32 pluListEntryInfoLen,
            pnuint8  pListEntryInfo
         )

REMARKS:
/ *
DSV_LIST                                        5 (0x05)


Request

  nuint32 version;   //default 0
  nuint32 flags;
  nuint32 iterationHandle;
  nuint32 parentID;

Response

  nuint32         iterationHandle;
  ListEntryInfo   subordinates[];//defined below

Definitions of Parameter Types

  struct  ListEntryInfo
  {
     nuint32   entryID;
     nuint32   entryFlags;
     nuint32   subordinateCount;
     nuint32   modificationTime;
     unicode baseClass;
     unicode rdn;
  }

  The entryID field of the ListEntryInfo structure contains the entry ID for the subordinate
  entry. (Note that the entryID is only valid on the server where this transaction was
  performed.)

  The entryFlags field of the ListEntryInfo structure defines the following bits:

  DS_ALIAS_ENTRY        Set if the entry is an alias entry

  DS_PARTITION_ROOT     Set if the entry is the root entry of a partition

  DS_CONTAINER_ENTRY    Set if the entry is allowed to have subordinates in the
                        Directory.

  The subordinateCount field contains the number of entries which are immediately
  subordinate to this entry in the Directory. If the subordinate count is unknown,
  0xFFFFFFFF is returned. (The subordinate count may not be known if the entry is an
  external entry.)  The modificationTime field contains the time of the last modification to the entry. If the
  modification time is unknown, 0 is returned. (The modification time may not be known if
  the entry is an external entry.)

  The baseClass field contains the name of the entry class which was used to create the
  entry. However, if aliases are not being dereferenced, the baseClass is also "alias."

  The rdn field of the ListEntryInfo structure contains the rdn of the subordinate entry.

(Version 1)

???????? do this one or what

Request

  nuint32   version;
  nuint32   flags;
  nuint32   iterationHandle;
  nuint32   parentID;
  nuint32   infoFlags;
  unicode nameFilter;
  unicode classFilter;
Response

  Depends on value of infoFlags as shown below:

Flag

DS1_ENTRY_ID                  nuint32     entryID;
DS1_ENTRY_FLAGS               nuint32     flags;
DS1_SUBORDINATE_COUNT         nuint32     subCount;
DS1_MODIFICATION_TIME         nuint32     modificationTime;
DS1_MODIFICATION_TIMESTAMP    TIMESTAMP   modificationTimeStamp;
DS1_CREATION_TIMESTAMP        TIMESTAMP   creationTimeStamp;
DS1_PARTITION_ROOT_ID         nuint32     partitionID;
DS1_PARENT_ID                 nuint32     parentID;
DS1_REVISION_COUNT            nuint32     revisionCount;
DS1_BASECLASS                 unicode     baseClass;
DS1_ENTRY_RDN                 unicode     rdn;
DS1_ENTRY_DN                  unicode     dn;
DS1_PARTITION_ROOT_DN         unicode     partitionDN;
DS1_PARENT_DN                 unicode     parentDN;

Definitions of Parameter Types

  struct  TIMESTAMP
  {
     nuint32seconds;
     int16replicaNumber;
     int16event;
  }

Remarks

This verb allows you to list the immediate subordinates of an explicitly defined entry.

The following request parameters are defined:

The parentID should be obtained as a result of a Resolve Name transaction.

The iterationHandle in the request should be 0xFFFFFFFF if this is the initial message in an
iteration. Subsequent requests should supply the iteration ID returned in the previous
response.

The request succeeds if the entry is located, regardless of whether there is any subordinate
information to return.The following response parameters are defined:

The iterationHandle is set to 0xFFFFFFFF if this message contains the remainder of the
response. Otherwise, this ID can be used in subsequent List requests to obtain further portions
of the response. The level of granularity for partial results is a ListEntryInfo structure.

The subordinates conveys the information on the immediate subordinates, if any, of the
specified entry. Should any of the subordinate entries be aliases, they will not be
dereferenced. If the request flags indicate dereference alias, the baseClass of subordinates
which are aliases will be that of the object ??. If the request flags indicate that aliases are not
being dereferenced, the base class of subordinates which are aliases will be "alias."

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
NWNCPDS5v1List
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   pnuint32 pluIterationHandle,
   nuint32  luParentID,
   nuint32  luInfoFlags,
   nuint32  luNameFilterLen,
   pnstr16  pwstrNameFilter,
   nuint32  luClassFilterLen,
   pnstr16  pwstrClassFilter,
   pnuint32 pluListEntryInfoLen,
   pnuint8  pListEntryInfo
)
{
   nuint8   buReq[615]; /* 4 + 4 + 4 + 4 + 4 + 4 + 514 + 2 + 4 + 66 + 2 + 3 (padding) */
   nuint8   buRep[7];   /* 4 + 3 (padding) */
   NWCFrag  reqFrag[1];
   NWCFrag  repFrag[2];
   nuint    uCurPos;
   nuint    uActualReplySize;
   pnuint8  cur;
   NWRCODE  err;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)1;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], pluIterationHandle);
   NCopyLoHi32(&cur[12], &luParentID);
   NCopyLoHi32(&cur[16], &luInfoFlags);
   NCopyLoHi32(&cur[20], &luNameFilterLen);

   NWCMemMove(&cur[24], pwstrNameFilter, (nuint)luNameFilterLen);
   uCurPos = 24 + NPad32((nuint)luNameFilterLen);
   NCopyLoHi32(&cur[uCurPos], &luClassFilterLen);
   uCurPos += 4;

   NWCMemMove(&cur[uCurPos], pwstrClassFilter, (nuint)luClassFilterLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = uCurPos + NPad32((nuint)luClassFilterLen);

   cur = buRep;
   NAlign32(&cur);

   /* iterationHandle */
   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 4;

   repFrag[1].pAddr = pListEntryInfo;
   repFrag[1].uLen = (nuint)*pluListEntryInfoLen;

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)5, (nuint)1,
            reqFrag, (nuint)2, repFrag, &uActualReplySize);

   if (err < 0)
      return err;

   cur = repFrag[0].pAddr;
   NCopyLoHi32(pluIterationHandle, cur);

   /* subtract 4 for the iteration handle */
   *pluListEntryInfoLen = uActualReplySize - 4;

   return err;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d5v1.c,v 1.7 1994/09/26 17:41:16 rebekah Exp $
*/
