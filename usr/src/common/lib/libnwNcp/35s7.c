/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s7.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWNCP35s7AFPRename*******************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP35s7AFPRename
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            nuint32  luAFPEntryID,
            nuint32  luDestAFPEntryID,
            nuint8   buSourcePathLen,
            pnstr8   pbstrSourcePath,
            nuint8   buDestPathLen,
            pnstr8   pbstrDestPath
         )

REMARKS: Move or renames an AFP file or directory.  Source Path
         Stringmust be set to the old directory or filename, and Destination
         Path String must be set to the new name.

         If the file or directory is being renamed but not moved, Destination
         Base ID should be set to the file's current parent directory, and
         Destination Path String should be set to the new name of the file
         (or directory).

         If the file or directory is being moved but not renamed, Destination
         Base ID should be set to the ID of the target directory, and
         Destination Path Length should be set to 0.

         If the file or directory is being moved and renamed, the last item in
         Destination Path String is used as the new name of the file or
         directory.

ARGS: <> pAccess
      >  buVolNum
      >  luAFPEntryID
      >  luDestAFPEntryID
      >  buSourcePathLen
      >  pbstrSourcePath
      >  buDestPathLen
      >  pbstrDestPath

INCLUDE: ncpafp.h

RETURN:  0x0000  Successful
         0x8983  Hard I/O Error
         0x8984  No Create Privileges
         0x8988  Invalid File Handle
         0x898B  No Rename Privileges
         0x898E  All Files In Use
         0x8990  All Read Only
         0x8992  All Names Exist
         0x8993  No Read Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x8999  Directory Full Error
         0x899C  Invalid Path

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 07  AFP Rename

CHANGES: 19 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s7AFPRename
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint32  luDestAFPEntryID,
   nuint8   buSourcePathLen,
   pnstr8   pbstrSourcePath,
   nuint8   buDestPathLen,
   pnstr8   pbstrDestPath
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 7)
   #define NCP_STRUCT_LEN  ((nuint16) (12 + buSourcePathLen + buDestPathLen))
   #define REQ_LEN         ((nuint) 13)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 0)

   NWCFrag reqFrag[REQ_FRAGS];
   nuint16 suTemp;
   nuint8 abuReq[REQ_LEN];

   suTemp = NCP_STRUCT_LEN;
   NCopyHiLo16(abuReq, &suTemp);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;
   NCopyHiLo32(&abuReq[4], &luAFPEntryID);
   NCopyHiLo32(&abuReq[8], &luDestAFPEntryID);
   abuReq[12] = buSourcePathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrSourcePath;
   reqFrag[1].uLen  = (nuint) buSourcePathLen;

   reqFrag[2].pAddr = &buDestPathLen;
   reqFrag[2].uLen  = (nuint) 1;

   reqFrag[3].pAddr = pbstrDestPath;
   reqFrag[3].uLen  = (nuint) buDestPathLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s7.c,v 1.7 1994/09/26 17:38:28 rebekah Exp $
*/
