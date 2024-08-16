/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:68.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP68FileErase**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP68FileErase
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buSrchAttrs,
            nuint8   buFileNameLen,
            pnstr8   pbstrInFileName,
         );

REMARKS: This call allows a client to delete files from the target directory.  The
         client must have file deletion privileges in the target directory or this
         request will fail. This request will also fail if another client is using the
         targeted file(s).

         This call supports wildcards.

ARGS: <> pAccess
      >  buDirHandle
      >  buSrchAttrs
      >  buFileNameLen
      >  pbstrInFileName

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x898A  No Delete Privileges
         0x898D  Some Files In Use
         0x898E  All Files In Use
         0x898F  Some Read Only
         0x8990  All Read Only
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Failure, No Files Found

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     67 --  Create File

NCP:     68 --  Erase File

CHANGES: 3 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP68FileErase
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buSrchAttrs,
   nuint8   buFileNameLen,
   pnstr8   pbstrInFileName
)
{
   #define NCP_FUNCTION    ((nuint) 68)
   #define NCP_REQ_LEN     ((nuint) 3)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 0)

   nuint8 abuReq[NCP_REQ_LEN];
   NWCFrag reqFrag[NCP_REQ_FRAGS];

   abuReq[0] = buDirHandle;
   abuReq[1] = buSrchAttrs;
   abuReq[2] = buFileNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrInFileName;
   reqFrag[1].uLen  = buFileNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/68.c,v 1.7 1994/09/26 17:38:48 rebekah Exp $
*/
