/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s37.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s37SetEntryInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s37SetEntryInfo
         (
            pNWAccess          pAccess,
            nuint8            buDirHandle,
            nuint8            buSrchAttrs,
            nuint32           luIterHnd,
            nuint32           luChangeBits,
            pNWNCPEntryUnion  pEntry
         );

REMARKS: This function sets or changes the file or directory entry information to the
         values entered in the ChangeBits field.  The ChangeBits field is defined
         as follows:

         ModifyNameBit:                0x0001
         FileAttributesBit:            0x0002
         CreateDateBit:                0x0004
         CreateTimeBit:                0x0008
         OwnerIDBit:                   0x0010
         LastArchivedDateBit:          0x0020
         LastArchivedTimeBit:          0x0040
         LastArchivedIDBit:            0x0080
         LastUpdatedDateBit:           0x0100
         LastUpdatedTimeBit:           0x0200
         LastUpdatedIDBit:             0x0400
         LastAccessedDateBit:          0x0800
         MaxAccessMaskBit:             0x1000
         MaximumSpaceBit:              0x2000

         (This call also works for Macintosh name space directory entries.)
         The change bits are as follows:

         MacModifyNameBit    0x0001
         MacFinderInfoBit    0x0002

ARGS: <> pAccess
       > buDirHandle
       > buSrchAttrs
       > luIterHnd
       > luChangeBits
       > pEntry

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8901  Invalid Parameter
         0x898C  No Set Privileges
         0x89BF  Invalid Name Space

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     22 37  Set Directory Entry Information

CHANGES: 15 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s37SetEntryInfo
(
   pNWAccess          pAccess,
   nuint8            buDirHandle,
   nuint8            buSrchAttrs,
   nuint32           luIterHnd,
   nuint32           luChangeBits,
   pNWNCPEntryUnion  pEntry
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 37)
   #define NCP_STRUCT_LEN  ((nuint16) 139)
   #define NCP_ENTRY_LEN   ((nuint) 128)
   #define NCP_REQ_LEN0    ((nuint) 13)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 0)

   nuint8 abuReq[NCP_REQ_LEN0], abuEntry[NCP_ENTRY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   abuReq[4] = buSrchAttrs;
   NCopyLoHi32(&abuReq[5], &luChangeBits);
   NCopyLoHi32(&abuReq[9], &luIterHnd);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN0;

   reqFrag[1].pAddr = abuEntry;
   reqFrag[1].uLen  = NCP_ENTRY_LEN;

   NWNCPPackEntryUnion(abuEntry, pEntry, NCP_SUBFUNCTION);

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s37.c,v 1.7 1994/09/26 17:34:21 rebekah Exp $
*/
