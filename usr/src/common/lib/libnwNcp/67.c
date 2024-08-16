/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:67.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP67FileCreate**************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP67FileCreate
         (
            pNWAccess       pAccess,
            nuint8         buDirHandle,
            nuint8         buFileAttrs,
            nuint8         buFileNameLen,
            pnstr8         pbstrFileName,
            pnuint8        pbuNWHandleB6,
            pNWNCPFileInfo pFileInfo,
         );

REMARKS: This call creates a new file for the calling client in the
         indicated directory.  The client must at least have file creation
         privileges in the directory or this request will be rejected.  If
         the client has file deletion privileges in the indicated directory
         and a file with the same name already exists in the directory, the
         existing file will be erased before the new file is created.  If the
         client does not have file deletion privileges in the indicated
         directory and a file with the same name already exists in the
         directory, this request will fail.

         The newly created file will be stamped with the date and time of
         its creation.  The file attributes byte will be set to the
         attributes specified by the client.  The newly created file can be
         opened for access with Open File (0x2222  76  --).  The file will
         be opened as an exclusive file with both read and write access
         requested.  The actual access rights granted will depend on the
         client's file access privileges in the specified directory.

         This call is replaced by the NetWare 386 v3.11 call Open Create
         File or Subdirectory (0x2222  87  01).

ARGS: <> pAccess
      >  buDirHandle
      >  buFileAttrs
      >  buFileNameLen
      >  pbstrFileName
      <  pbuNWHandleB6
      <  pFileInfo (optional)

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

SEE:     77 --  Create New File
         68 --  Erase File
         87 01  Open Create File or Subdirectory

NCP:     67 --  Create File

CHANGES: 4 Oct 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP67FileCreate
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
   #define NCP_FUNCTION    ((nuint) 67)
   #define REQ_LEN         ((nuint) 3)
   #define REPLY_LEN       ((nuint) 36)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   nint32   lCode;
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
      NWCMemMove(pbuNWHandleB6, &abuReply[0], (nuint) 6);
      NWNCPUnpackFileInfo(pFileInfo, &abuReply[6]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/67.c,v 1.7 1994/09/26 17:38:46 rebekah Exp $
*/
