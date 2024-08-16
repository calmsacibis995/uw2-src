/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:62.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP62ScanFirst*****************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP62ScanFirst
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            pnuint8  pbuVolNum,
            pnuint16 psuDirID,
            pnuint32 psuIterHnd,
            pnuint8  pbuAccessRights,
         )

REMARKS: This call initiates a search for files in a directory. It returns the
         Directory ID and Directory Access Rights of the directory specified by
         Directory Handle and Path. The Volume Number and Directory ID can then be
         used with File Search Continue (function 63) to locate files or
         subdirectories.

         Beginning with NetWare v2.0a, the workstation shell uses this NCP, in
         conjunction with File Search Continue (function 63), to perform file and
         directory searches. Prior to this, other NCPs were used for file and
         directory searches.

         This call and File Search Continue were added to accommodate a problem with
         the DOS operating system.  The older NCPs relied on the server to keep a
         search context associated with a directory handle. When an End Of Task
         message was received from the workstation, the directory handle was deleted,
         causing the loss of the search context. Since DOS sends repeated "pseudo"
         End Of Task messages in a loop of a command file, directory handles were
         deleted, causing the older NCPs to fail.

         With this call, the Search Context is the file identifier in the Novell
         server file system; the context is no longer based on directory handles.
         Consequently, Novell's server implementation does not maintain a context
         block for searches made with this call.

         This call is replaced by the NetWare 386 v3.11 call Initialize Search
         (0x2222  87  02)

ARGS:<>  pAccess
      >  buDirHandle
      >  buPathLen
      >  pbstrPath
      <  pbuVolNum
      <  psuDirID
      <  psuIterHnd
      <  pbuAccessRights (optional)

INCLUDE: ncpfile.h

RETURN:
         0x8900  Successful
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  No Files Found

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     File Search Continue (0x2222  63  --)
         Initialize Search (0x2222  87  02)

NCP:     62 --  File Search Initialize

CHANGES:   7 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP62ScanFirst
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuVolNum,
   pnuint16 psuDirID,
   pnuint16 psuIterHnd,
   pnuint8  pbuAccessRights
)
{
   #define NCP_FUNCTION    ((nuint) 62)
   #define NCP_REQ_LEN     ((nuint) 2)
   #define NCP_REPLY_LEN   ((nuint) 6)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 1)

   nint32   lCode;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];

   abuReq[0] = buDirHandle;
   abuReq[1] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = (nuint) buPathLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if(lCode == 0)
   {
      *pbuVolNum=abuReply[0];

      NCopyLoHi16(psuDirID, &abuReply[1]);
      NCopyHiLo16(psuIterHnd, &abuReply[3]);

      if (pbuAccessRights != NULL)
         *pbuAccessRights =  abuReply[5];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/62.c,v 1.7 1994/09/26 17:38:42 rebekah Exp $
*/
