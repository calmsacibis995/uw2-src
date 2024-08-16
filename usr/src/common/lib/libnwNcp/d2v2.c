/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d2v2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

#define DSI_BASE_CLASS              0x00000800L
#define DSI_DEREFERENCE_BASE_CLASS  0x00020000L
/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS2v2ReadObjectInfo
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            nuint32  luEntryID,
            nuint32  luInfoFlags,
            pnuint32 pluEntryFlags,
            pnuint32 pluSubordinateCount,
            pnuint32 pluModificationTime,
            pnstr16  pwstrBaseClass,
            pnstr16  pwstrEntryName
         )

REMARKS:
/ *
DSV_READ_ENTRY_INFO                             2 (0x02)


Request
  nuint32 version;   //version 2
  nuint32 flags;
  nuint32 entryID;
  nuint32 infoFlags

Response

  nuint32   entryFlags;
  nuint32   subordinateCount;
  nuint32   modificationTime;
  unicode   baseClass;
  unicode   entryName;

Remarks

This verb allows you to return non-attribute information associated with an entry. This
information can be useful to Directory browse utilities. For example, various icons could be
associated with the entry based on the entry's base class and container status.

The entryID should be previously returned by this server.

The entryFlags in the response defines the following bits:

DS_ALIAS_ENTRY        Set if the entry is an alias entry

DS_PARTITION_ROOT     Set if the entry is the root entry of a partition

DS_CONTAINER_ENTRY    Set if the entry is allowed to have subordinates in the
                      Directory.The subordinateCount returns the number of entries which are immediately subordinate to this
                      entry in the Directory. If the subordinate count is unknown, 0xFFFFFFFF is returned. (The
                      subordinate count may not be known if the entry is an external entry.) The modificationTime
                      returns the time of the last modification to the entry. If the modification time is unknown, 0 is
                      returned. (The modification time may not be known if the entry is an external entry.)

The baseClass returns the name of the entry class which was used to create the entry.

The entryName returns the distinguished name of the entry.

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
NWNCPDS2v2ReadObjectInfo
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luEntryID,
   nuint32  luInfoFlags,
   pnuint32 pluEntryFlags,
   pnuint32 pluSubordinateCount,
   pnuint32 pluModificationTime,
   pnstr16  pwstrBaseClass,
   pnstr16  pwstrEntryName
)
{
   nuint8   buReq[19]; /* 4 * 4 + 3 */
   nuint8   buRep[576]; /* 5 * sizeof(nuint32) + MAX_DN_BYTES + 2 +
                        MAX_SCHEMA_NAME_BYTES + 2  + 3(pad) */
   NWCFrag  reqFrag[1];
   NWCFrag  repFrag[1];
   pnuint8  cur;
   nuint    szActRep;
   nuint32  outputFields,len1 = 0,len2 = 0;
   NWRCODE  err;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)2;
   NCopyLoHi32(&cur[4], &flFlags);   /* protocol Flags */
   NCopyLoHi32(&cur[8], &luInfoFlags);
   NCopyLoHi32(&cur[12], &luEntryID);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 16;

   cur = buRep;
   NAlign32(&cur);

   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 573;

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)2, (nuint)1,
            reqFrag, (nuint)1, repFrag, &szActRep);
   if (err < 0)
      return (err);

   NCopyLoHi32(&outputFields, &cur[0]);
   NCopyLoHi32(pluEntryFlags, &cur[4]);
   NCopyLoHi32(pluSubordinateCount, &cur[8]);
   NCopyLoHi32(pluModificationTime, &cur[12]);

   if (outputFields & DSI_BASE_CLASS)
   {
      NCopyLoHi32(&len1, &cur[16]);
      NWCMemMove(pwstrBaseClass, &cur[20], (nuint)len1);
   }

   cur += (20 + NPad32((nuint)len1));

   NCopyLoHi32(&len1, cur);
   NWCMemMove(pwstrEntryName, cur + 4, (nuint)len1);

   cur += 4 + NPad32((nuint)len1);

   if (outputFields & DSI_DEREFERENCE_BASE_CLASS)
   {
      NCopyLoHi32(&len2, cur);
      NWCMemMove(pwstrBaseClass, cur + 4, (nuint)len2);
   }

   return (err);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d2v2.c,v 1.7 1994/09/26 17:40:53 rebekah Exp $
*/
