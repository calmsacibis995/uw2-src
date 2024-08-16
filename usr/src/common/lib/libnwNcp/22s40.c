/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s40.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s40ScanDirDiskSpace********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s40ScanDirDiskSpace
         (
            pNWAccess          pAccess,
            nuint8            buDirHandle,
            nuint8            buSrchAttrs,
            nuint8            buSrchPatternLen,
            pnuint8           pbuSrchPattern,
            pnuint32          pluIterHnd,
            pNWNCPEntryUnion  pEntryInfo,
         )

REMARKS: This function scans a directory using the 8.3 wild card format.  To intialize
         a directory search, set the sequence number to -1 (0xFFFFFFFF).  This
         routine is the same as 0x2222 22 30 except that it also returns the data fork
         and resource fork size.  The file size is the actual file size rather than
         the logical file size (sparse files can be logically much larger than they
         actually are).

ARGS: <> pAccess
      >  buDirHandle
      >  buSrchAttrs
      >  buSrchPatternLen
      <  pbuSrchPattern
      <> pluIterHnd
      <  pEntryInfo

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8989  No Search Privileges
         0x899C  Invalid Path
         0x89FB  386 File Structure Not Supported On Server
         0x78FF  No More Matches In Directory

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 30  Scan a Directory
         22 35  Get Directory Disk Space Restrictions
         22 33  Add User Disk Space Restriction
         22 34  Remove User Disk Space Restriction
         22 41  Get Object Disk usage And Restrictions
         22 32  Scan Volume's User Disk Restrictions
         22 36  Set Directory Disk Space Restriction

NCP:     22 40  Scan Directory Disk Space

CHANGES: 9 Sep 1993 - written - rivie
----------------------------------------------------------------------------
    Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s40ScanDirDiskSpace
(
   pNWAccess          pAccess,
   nuint8            buDirHandle,
   nuint8            buSrchAttrs,
   nuint8            buSrchPatternLen,
   pnuint8            pbstrSrchPattern,
   pnuint32          pluIterHnd,
   pNWNCPEntryUnion  pEntryInfo,
   pnuint32          pluReservedB2
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 40)
   #define NCP_STRUCT_LEN  ((nuint16) (8 + buSrchPatternLen))
   #define NCP_REQ_LEN     ((nuint) 10)
   #define NCP_REPLY_LEN   ((nuint) 140)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 1)

   nint32   lCode;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nint i;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16 (&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   abuReq[4] = buSrchAttrs;
   NCopyHiLo32 (&abuReq[5], pluIterHnd);
   abuReq[9] = buSrchPatternLen;

   reqFrag[0].pAddr =  abuReq;
   reqFrag[0].uLen  =  NCP_REQ_LEN;

   reqFrag[1].pAddr =  pbstrSrchPattern;
   reqFrag[1].uLen  =  (nuint) buSrchPatternLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
          NCP_REPLY_FRAGS, replyFrag, NULL);
   if(lCode == 0)
   {
      NCopyHiLo32(pluIterHnd, &abuReply[0]);
      NWNCPUnpackEntryUnion(pEntryInfo, &abuReply[4], NCP_SUBFUNCTION);

      for (i = 0; i < 2; i++)
         NCopyLoHi32(&pluReservedB2[i], &abuReply[(i*4) + 132]);
   }
   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s40.c,v 1.7 1994/09/26 17:34:26 rebekah Exp $
*/
