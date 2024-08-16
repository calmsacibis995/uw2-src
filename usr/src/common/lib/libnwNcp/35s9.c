/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s9.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWNCP35s9AFPSetFileInfo*******************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP35s9AFPSetFileInfo
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            nuint32  luAFPEntryID,
            pnuint16 psuReqBitMap,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            pNWNCPAFPFileInfo pMacFileInfo
         )

REMARKS: Set the information pertaining to the specified AFP file
         or directory.  The bits in the Request Bit Map field are interpreted
         as follows:

            0x0100 Set Attributes
            0x0400      Set Create Date
            0x0800      Set Access Date
            0x1000      Set Modify Date/Time
            0x2000      Set Backup Date/Time
            0x4000      Set Finder Info

         The Attributes field indicates the attributes of the directory
         or file.  The following bits are defined:

            0x0100      Read Only
            0x0200      Hidden
            0x0400      System
            0x0800      Execute Only
            0x1000      Subdirectory
            0x2000      Archive
            0x4000      Undefined
            0x8000      Shareable File
            0x0001      Search Mode
            0x0002      Search Mode
            0x0004      Search Mode
            0x0008      Undefined
            0x0010      Transaction
            0x0020      Index
            0x0040      Read Audit
            0x0080      Write Audit

         The Creation Date field sets the creation date (in AFP format) of
         the target directory or file.

         The Access Date field sets the date (in AFP format) that the target
         AFP file was last accessed.  This field is ignored for directories.

         The Modify Date and Modify Time fields set the date and time (in AFP
         format) that the target AFP file was last modified.  These fields are
         ignored for directories.

         The Backup Date and Backup Time fields set the date and time (in AFP
         format) that the target AFP file was last backed up.

         The Finder Info field sets the 32-bytes of finder information for the
         specified AFP directory or file.

ARGS: <> pAccess
      >  buVolNum
      >  luAFPEntryID
      >  pMacFileInfo
      >  psuReqBitMap
      >  pbstrPath

INCLUDE: ncpafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 09  AFP Set File Information

CHANGES: 19 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s9AFPSetFileInfo
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   pnuint16 psuReqBitMap,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pNWNCPAFPFileInfo pMacFileInfo
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 9)
   #define NCP_STRUCT_LEN  ((nuint16) (55 + buPathLen))
   #define REQ_LEN         ((nuint) 24)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 0)

   NWCFrag reqFrag[REQ_FRAGS];
   nuint16 suTemp;
   nuint8  abuReq[REQ_LEN];

   suTemp= NCP_STRUCT_LEN;
   NCopyHiLo16 (abuReq, &suTemp);
   abuReq[2]= NCP_SUBFUNCTION;
   abuReq[3]=buVolNum;
   NCopyHiLo32(&abuReq[4],  &luAFPEntryID);
   NCopyHiLo16(&abuReq[8],  psuReqBitMap);
   NCopyHiLo16(&abuReq[10], &pMacFileInfo->suAttr);
   NCopyHiLo16(&abuReq[12], &pMacFileInfo->suCreationDate);
   NCopyHiLo16(&abuReq[14], &pMacFileInfo->suAccessDate);
   NCopyHiLo16(&abuReq[16], &pMacFileInfo->suModifyDate);
   NCopyHiLo16(&abuReq[18], &pMacFileInfo->suModifyTime);
   NCopyHiLo16(&abuReq[20], &pMacFileInfo->suBackupDate);
   NCopyHiLo16(&abuReq[22], &pMacFileInfo->suBackupTime);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pMacFileInfo->abuFinderInfo;
   reqFrag[1].uLen  = FINDER_INFO_LEN;

   reqFrag[2].pAddr = &buPathLen;
   reqFrag[2].uLen  = (nuint) 1;

   reqFrag[3].pAddr = pbstrPath;
   reqFrag[3].uLen  = (nuint) buPathLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s9.c,v 1.7 1994/09/26 17:38:30 rebekah Exp $
*/
