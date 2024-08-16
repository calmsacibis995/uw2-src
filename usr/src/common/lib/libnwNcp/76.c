/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:76.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP76FileOpen**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP76FileOpen
         (
            pNWAccess       pAccess,
            nuint8         buDirHandle,
            nuint8         buSrchAttrs,
            nuint8         buAccessRights,
            nuint8         buFileNameLen,
            pnstr8         pbstrFileName,
            pnuint8        pbuNWHandleB6,
            pNWNCPFileInfo pFileInfo,
         )

REMARKS: This call allows a client to open an existing file. The "system" and "hidden"
         bits in the client's Search Attributes flag are used to discover or ignore
         system and hidden files as described in the introduction. File Name is a
         valid file path and is used with the Directory Handle to indicate which file
         should be opened.

         If the client lacks file open privileges in the target directory, the request
         will fail.

         The Desired Access Rights flag is a bit field with the following bits defined:

               Bit   Definition

         0     Open the file forreading (read).

         1     Open the file for writing (write).

         2     Do not allow other clients to open this file
               for writing (deny write).

         3     Do not allow other clients to open this file
               for reading (deny read).

         4     This client assumes a single-user
               environment (exclusive).

         5-7   Not defined.

         The Desired Access Rights flag designates which access rights a client wants
         in a specified file.  The client's initial Desired Access Rights are modified
         to reflect the actual access rights the client is allowed to the specified
         directory and file. Modifications are determined as follows:

         If the client lacks file Read privileges in the
         specified directory, the read bit of the Desired
         Access Rights flag is cleared.

         If the client lacks file Write privileges in the
         specified directory, the write bit of the Desired
         Access Rights flag is cleared.

         If the specified file is marked Read Only, the
         write bit of the Desired Access Rights flag is
         cleared.

         If the "exclusive" bit is cleared in the Desired Access Rights flag, the
         access flag is in its final state and the server processes the open request.

         If the "exclusive" bit is set in the Desired Access Rights flag, the client
         normally expectsexclusive access to files.  This is roughly equivalent to the
         "compatibility" bit defined in MS-DOS 3.X.  When the "exclusive" bit is set,
         the file's attribute flags are examined and the "shareable" bit tested. The
         shareable attribute on a file controls the final access rights granted to a
         client when the client is opening files in an exclusive-access mode.

         If the file's "shareable" bit is set, the "deny read" and "deny write"
         bits are cleared in the Access Rights flag. (All clients with appropriate
         directory access rights can read from and write to shareable files.)

         If the file's "shareable" bit is cleared and the Desired Access Rights
         flag's "write" bit is cleared, the "deny read" bit is cleared and the
         "deny write" bit is set.  This allows multiple clients to read from the file
         with no one writing to it.

         If the file's "shareable" bit is cleared and the Desired Access Rights
         flag's "write" bit is set, the "deny read" bit is set and the "deny write"
         bit is set.  This ensures that the file is kept for the exclusive use of the
         requesting client.

         After the Desired Access Rights flag has been modified, the result is
         compared with the access rights of other clients currently using the same
         file. If the Desired Access Rights are not compatible with the access rights
         of other clients, the open request is refused.

         The File Handle returned by this call must be used by the client for all
         subsequent file access requests. The client must not modify any information
         within the file handle.

         The File Name field contains the name of the file opened; if File Name is
         shorter than 14 characters, this field will be null-padded.

         The File Attribute field contains the attributes of the opened file.

         The File Length field indicates the file's length at the instant it was
         opened. If multiple clients are using a file, they must coordinate the
         extending of the shared file among themselves. For a discussion of extending
         shared files, see Get Current Size Of File (function 71).

         Dates and Times are in MS-DOS format except that they are stored in high-low
         order.

         Clients can open a single file more than once.  However, the file must be
         closed as many times as it is opened before the server will release the file
         to other clients.

         This call is replaced by the NetWare 386 v3.11 call Open Create File or
         Subdirectory (0x2222  87  01).

ARGS: <> pAccess
      >  buDirHandle
      >  buSrchAttrs
      >  buAccessRights
      >  buFileNameLen
      >  pbstrFileName
      <  pbuNWHandleB6
      <  pFileInfo

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8980  Lock Fail
         0x8981  Out Of Handles
         0x8982  No Open Privileges
         0x8994  No Write Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Lock Error, Failure, No Files Found

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     65 --  Open File (old)
         66 --  Close File
         87 01  Open Create File or Subdirectory

NCP:     76 --  Open File

CHANGES: 4 Oct 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP76FileOpen
(
   pNWAccess       pAccess,
   nuint8         buDirHandle,
   nuint8         buSrchAttrs,
   nuint8         buAccessRights,
   nuint8         buFileNameLen,
   pnstr8         pbstrFileName,
   pnuint8        pbuNWHandleB6,
   pNWNCPFileInfo pFileInfo
)
{
   #define NCP_FUNCTION    ((nuint) 76)
   #define NCP_REQ_LEN     ((nuint) 4)
   #define NCP_REPLY_LEN   ((nuint) 36)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 1)

   nint32   lCode;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];

   abuReq[0] = buDirHandle;
   abuReq[1] = buSrchAttrs;
   abuReq[2] = buAccessRights;
   abuReq[3] = buFileNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrFileName;
   reqFrag[1].uLen  = buFileNameLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
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
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/76.c,v 1.7 1994/09/26 17:38:59 rebekah Exp $
*/
