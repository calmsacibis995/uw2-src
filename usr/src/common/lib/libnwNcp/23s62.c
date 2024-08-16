/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s62.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s62WritePropertyValue****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s62WritePropertyValue
         (
            pNWAccess pAccess,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
            nuint8   buSegmentNum,
            nuint8   buMoreFlag,
            nuint8   buPropertyNameLen,
            pnstr8   pbstrPropertyName,
            pnuint8  pbuPropertyValueB128,
         );

REMARKS: Allow a client to write a value to an item property.  Values of
         set properties are manipulated with Add Bindery Object To Set (function
         2, subfunction 65) and Delete Bindery Object From Set (function 23,
         subfunction 66).  Property values must be kept within a single 128-byte
         segment.

         This call writes out the 128-byte value segment to the Segment Number
         specified by the client.  The first segment of a value is segment 1.
         Before segment N can be written, segments 1 to N -1 must be
         written.  Once a property value has been established, segments of it can
         be read or written in a random order.

         The More flag provided by the client informs the bindery whether the written
         segment is the last segment of the value.  If the More flag is zero and
         the current value has segments beyond the segment being written, the bindery
         will truncate the Property Value and discard the extra segments.

         Any client that has write privileges to the target property (Property Name)
         can make this call.

ARGS: <> pAccess
      >  suObjType
      >  buObjNameLen
      >  pbstrObjName
      >  buSegmentNum
      >  buMoreFlag
      >  buPropertyNameLen
      >  pbstrPropertyName
      >  pbuPropertyValueB128

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89E8  Write To Group
         0x89EC  No Such Set
         0x89F0  Illegal Wildcard
         0x89F8  No Property Write
         0x89FB  No Such Property
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 61  Read Property Value

NCP:     23 62  Write Property Value

CHANGES: 23 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s62WritePropertyValue
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buSegmentNum,
   nuint8   buMoreFlag,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName,
   pnuint8  pbuPropertyValueB128
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 62)
   #define NCP_STRUCT_LEN  ((nuint16) (135 + buObjNameLen + buPropertyNameLen))
   #define REQ1_LEN        ((nuint) 6)
   #define REQ2_LEN        ((nuint) 3)
   #define PROPERTY_LEN    ((nuint) 128)
   #define REQ_FRAGS       ((nuint) 5)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 abuReqA[REQ1_LEN], abuReqB[REQ2_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS];

   suNCPLen   = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReqA[0], &suNCPLen);
   abuReqA[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReqA[3], &suObjType);
   abuReqA[5] = buObjNameLen;

   abuReqB[0] = buSegmentNum;
   abuReqB[1] = buMoreFlag;
   abuReqB[2] = buPropertyNameLen;

   reqFrag[0].pAddr = abuReqA;
   reqFrag[0].uLen  = REQ1_LEN;

   reqFrag[1].pAddr = pbstrObjName;
   reqFrag[1].uLen  = buObjNameLen;

   reqFrag[2].pAddr = abuReqB;
   reqFrag[2].uLen  = REQ2_LEN;

   reqFrag[3].pAddr = pbstrPropertyName;
   reqFrag[3].uLen  = buPropertyNameLen;

   reqFrag[4].pAddr = pbuPropertyValueB128;
   reqFrag[4].uLen  = PROPERTY_LEN;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s62.c,v 1.7 1994/09/26 17:37:25 rebekah Exp $
*/
