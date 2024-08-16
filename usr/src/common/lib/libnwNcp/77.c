/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:77.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP77FileCreate**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP77FileCreate
         (
            pNWAccess       pAccess,
            nuint8         buDirHandle,
            nuint8         buFileAttrs,
            nuint8         buFileNameLen,
            pnstr8         pbstrFileName,
            pnuint8        pbuNWHandleB6,
            pNWNCPFileInfo pFileInfo
         );

REMARKS: This call is the same as Create File (0x2222  67  --), except that this
         request will always fail if a file with the same name already exists.

ARGS: <> pAccess
      >  buDirHandle
      >  buFileAttrs
      >  buFileNameLen
      >  pbstrFileName
      <  pbuNWHandleB6
      <  pFileInfo

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8980  Lock Fail
         0x8981  Out Of Handles
         0x8984  No Create Privileges
         0x8985  No Create/Delete Privileges
         0x8987  Create Filename Error
         0x898D  Some Files In Use
         0x898F  Some Read Only
         0x8990  All Read Only
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x8999  Directory Full Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89FD  Bad Station Number
         0x89FF  Failure, No Files Found

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     67 --  Create File
         68 --  Erase File
         87 01  Open Create File or Subdirectory

NCP:     77 --  Create New File

CHANGES: 4 Oct 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP77FileCreate
(
   pNWAccess       pAccess,
   nuint8         buDirHandle,
   nuint8         buFileAttrs,
   nuint8         buFileNameLen,
   pnstr8         pbstrFileName,
   pnuint8        pbuNWHandleB6,
   pNWNCPFileInfo pFileInfo
)
{
   #define NCP_FUNCTION    ((nuint) 77)
   #define REQ_LEN         ((nuint) 3)
   #define REPLY_LEN       ((nuint) 36)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   nint32  lCode;
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   abuReq[0] = buDirHandle;
   abuReq[1] = buFileAttrs;
   abuReq[2] = buFileNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrFileName;
   reqFrag[1].uLen  = buFileNameLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      nint i;

      for (i = 0; i < (nint) 6; i++)
         pbuNWHandleB6[i] = abuReply[i];

      NWNCPUnpackFileInfo(pFileInfo, &abuReply[6]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/77.c,v 1.7 1994/09/26 17:39:00 rebekah Exp $
*/
