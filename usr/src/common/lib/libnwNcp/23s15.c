/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s15.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s15ScanFiles
         (
            pNWAccess          pAccess,
            nuint8            buDirHandle,
            nuint8            buSrchAttrs,
            pnuint16          psuIterHnd,
            nuint8            buFileNameLen,
            pnstr8            pbstrInFileName,
            pnstr8            pbstrOutFileNameB14,
            pNWNCPFileInfo2   pFileInfo2,
            pnuint8           pbuReservedB56,
         )

REMARKS:
         This call allows a client to retrieve a file's extended status information.
         This information includes the trustee ID of the owner who created the file.
         The calling client must have directory search privileges in the target
         directory.

         This call is used the same way that Search For A File (0x2222  64  --) is
         used. A client should initially set the Last Search Index field to
         0xFFFF (-1).  If this call is used iteratively to scan entries in a directory,
         each subsequent call should set the Last Search Index to the Next Search
         Index returned by the server on the previous call.

         Directory Handle is an index number (1 to 255). A file server maintains a
         directory handle table for each logged in workstation. A directory handle
         points to a volume or a directory on the file server.

         A client must specify the Search Attributes for the type(s) of files to be
         scanned. (See the introduction for an explanation of the Search Attributes
         byte.)

         File Name Length is the length of the File Name that follows.

         The File Name can be up to 255 bytes long. File Name is a valid file path
         relative to the Directory Handle. A full file path appears in the following
         format:


         volume:directory/.../directory/filename

         A partial file path includes a filename and (optionally) one or more
         antecedent directory names.  A filename can be 1 to 8 characters long and
         can also include an extension of 1 to 3 characters. All letters must appear
         in upper case.

         A partial file path can be combined with a directory handle to identify a
         file. When a client passes a full file path, the client should also pass a
         value of 0x00 in the Directory Handle field.

         This call returns the file attributes of the specified file in the File
         Attributes field. (See the introduction for an explanation of File
         Attributes.)

         This call returns the extended attributes of the specified file in the
         Extended File Attributes field. (See the introduction for an explanation of
         File Attributes.)

         The File Size field indicates the size of the specified file in bytes.

         The Creation Date and Last Access Date fields indicate the creation date
         and last access date of the specified file (bytes 1 and 2 below). The Last
         Update Date And Time and the Last Archive Date And Time fields indicate the
         last time the file was updated or archived (bytes 1, 2, 3, and 4 on the
         next page). All dates are returned as illustrated in bytes 1 and 2. Times
         are returned as illustrated in bytes 3 and 4.

         Date Format

         Byte 1                        Byte 2
         *************************     *************************
         *15*14*13*12*11*10*9 *8 *     *7 *6 *5 *4 *3 *2 *1 *0 *
         *************************     *************************
         *******************************************************
         year                   month           day
         (0 to 119)             (1 to 12)       (1 to 31)
         (1980-2099)



         Time Format

         Byte 1                        Byte 2
         *************************     *************************
         *15*14*13*12*11*10*9 *8 *     *7 *6 *5 *4 *3 *2 *1 *0 *
         *************************     *************************
         *******************************************************
         hour                      minute           second
         (0 to 23)                 (0 to 59)        (0 to 29)
         (2 sec units)

         Owner Object ID is the bindery object ID of the user who created the file.

ARGS: <> pAccess
       > buDirHandle
       > buSrchAttrs
      <> psuIterHnd
       > buFileNameLen
       > pbstrInFileName
       < pbstrOutFileNameB14 (optional)
       < pFileInfo2
       < pbuReservedB56 (optional)

INCLUDE: ncpfile.h

RETURN:  Completion Code

         0x0000  Successful
         0x8988  Invalid File Handle
         0x8989  No Search Privileges
         0x8993  No Read Privileges
         0x8994  No Write Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  No Files Found


SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN UNIX

SEE:

NCP:     23 15  Scan File Information

CHANGES: 31 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s15ScanFiles
(
   pNWAccess          pAccess,
   nuint8            buDirHandle,
   nuint8            buSrchAttrs,
   pnuint16          psuIterHnd,
   nuint8            buFileNameLen,
   pnstr8            pbstrInFileName,
   pnstr8            pbstrOutFileNameB14,
   pNWNCPFileInfo2   pFileInfo2,
   pnuint8           pbuReservedB56
)
{
   #define NCP_FUNCTION       ((nuint) 23)
   #define NCP_SUBFUNCTION    ((nuint8) 15)
   #define NCP_STRUCT_LEN     ((nuint16) (6 + buFileNameLen))
   #define REQ_LEN            ((nuint) 8)
   #define REPLY_LEN          ((nuint) 38)
   #define RESERVED_LEN       ((nuint) 56)
   #define REQ_FRAGS          ((nuint) 2)
   #define REPLY_FRAGS        ((nuint) 2)

   nint32   lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN], abuBucket[RESERVED_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi16(&abuReq[3], psuIterHnd);
   abuReq[5] = buDirHandle;
   abuReq[6] = buSrchAttrs;
   abuReq[7] = buFileNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrInFileName;
   reqFrag[1].uLen  = (nuint) buFileNameLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pbuReservedB56 ? pbuReservedB56 : abuBucket;
   replyFrag[1].uLen  = RESERVED_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NCopyLoHi16(psuIterHnd, &abuReply[0]);

      if (pbstrOutFileNameB14)
         NWCMemMove(pbstrOutFileNameB14, &abuReply[2], (nuint) 14);

      pFileInfo2->buAttrs    = abuReply[16];
      pFileInfo2->buExtAttrs = abuReply[17];
      NCopyHiLo32(&pFileInfo2->luSize,          &abuReply[18]);
      NCopyHiLo16(&pFileInfo2->suCreationDate,  &abuReply[22]);
      NCopyHiLo16(&pFileInfo2->suAccessedDate,  &abuReply[24]);
      NCopyHiLo16(&pFileInfo2->suModifiedDate,  &abuReply[26]);
      NCopyHiLo16(&pFileInfo2->suModifiedTime,  &abuReply[28]);
      NCopyHiLo32(&pFileInfo2->luOwnerID,       &abuReply[30]);
      NCopyHiLo16(&pFileInfo2->suArchiveDate,   &abuReply[34]);
      NCopyHiLo16(&pFileInfo2->suArchiveTime,   &abuReply[36]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s15.c,v 1.7 1994/09/26 17:35:40 rebekah Exp $
*/
