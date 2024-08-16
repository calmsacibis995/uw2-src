/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:69.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP69FileRename***************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP69FileRename
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buSrchAttrs,
            nuint8   buFileNameLen,
            pnstr8   pbstrFileName,
            nuint8   buDstDirHandle,
            nuint8   buNewFileNameLen,
            pnstr8   pbstrNewFileName,
         )

REMARKS: This call allows a client to rename a file.  The source directory (where the
         file resides) and the target directory (where the renamed file will be
         deposited) do not need to be the same directory.  This call can be used to
         move a file from one directory to another.  However, the two directories
         must reside on the same server volume. This call will not move a file from
         one volume to another.

         The client must have file modification privileges in both the source and the
         target directories.  The rename attempt will fail if the file is being used
         by other clients or if the target name already exists in the target directory.
         Wildcard renaming is supported.

         This call is replaced by the NetWare 386 v3.11 call Rename or Move A File or
         Subdirectory (0x2222  87 04).

ARGS: <> pAccess,
      >  buDirHandle,
      >  buSrchAttrs,
      >  buFileNameLen,
      >  pbstrFileName,
      >  buDstDirHandle,
      >  buNewFileNameLen,
      >  pbstrNewFileName,

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8987  Create Filename Error
         0x898B  No Rename Privileges
         0x898D  Some Files In Use
         0x898E  All Files In Use
         0x898F  Some Read Only
         0x8990  All Read Only
         0x8991  Some Names Exist
         0x8992  All Names Exist
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899A  Rename Across Volume
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Failure, No Files Found

SERVER:  2.2

CLIENT:  DOS WIN OS2 NT

SEE:     87 04  Rename or Move A File or SubDirectory

NCP:     69 --  Rename File

CHANGES: 1 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP69FileRename
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buSrchAttrs,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName,
   nuint8   buDstDirHandle,
   nuint8   buNewFileNameLen,
   pnstr8   pbstrNewFileName
)
{
   #define NCP_FUNCTION    ((nuint) 69)
   #define NCP_REQ_LEN0    ((nuint) 3)
   #define NCP_REQ_LEN2    ((nuint) 2)
   #define NCP_REQ_FRAGS   ((nuint) 4)
   #define NCP_REPLY_FRAGS ((nuint) 0)

   NWCFrag reqFrag[NCP_REQ_FRAGS];
   nuint8 abuReq[NCP_REQ_LEN0], abuReq2[NCP_REQ_LEN2];

   abuReq[0] = buDirHandle;
   abuReq[1] = buSrchAttrs;
   abuReq[2] = buFileNameLen;

   abuReq2[0] = buDstDirHandle;
   abuReq2[1] = buNewFileNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN0;

   reqFrag[1].pAddr = pbstrFileName;
   reqFrag[1].uLen  = buFileNameLen;

   reqFrag[2].pAddr = abuReq2;
   reqFrag[2].uLen  = NCP_REQ_LEN2;

   reqFrag[3].pAddr = pbstrNewFileName;
   reqFrag[3].uLen  = buNewFileNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/69.c,v 1.7 1994/09/26 17:38:49 rebekah Exp $
*/
