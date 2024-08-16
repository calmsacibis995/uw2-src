/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:70.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP70FileSetAttrs***********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP70FileSetAttrs
         (
            pNWAccess pAccess,
            nuint8   buNewFileAttr,
            nuint8   buDirHandle,
            nuint8   buSrchAttr,
            nuint8   buFileNameLen,
            pnstr8   pbstrFileName,
         )

REMARKS: This call allows a client to modify a file's attributes. The client must
         have file modification privileges in the target directory. The target file
         must not be in use by other clients.  The target file's attributes are set to
         the attribute value specified by the client.

         Once set, the "execute-only" attribute on a file cannot be reset.

         This call supports wildcard attribute setting.

ARGS: <> pAccess,
      >  buNewFileAttr,
      >  buDirHandle,
      >  buSrchAttr,
      >  buFileNameLen,
      >  pbstrFileName,

INCLUDE: ncpfile.h

RETURN:  0x00  Successful
         0x8C  No Set Privileges
         0x8D  Some Files In Use
         0x8E  All Files In Use
         0x96  Server Out Of Memory
         0x98  Disk Map Error
         0x9B  Bad Directory Handle
         0x9C  Invalid Path
         0xA1  Directory I/O Error
         0xFD  Bad Station Number
         0xFF  Failure, No Files Found

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     23 15  Scan File Information

NCP:     70 --  Set File Attributes

CHANGES: 30 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP70FileSetAttrs
(
   pNWAccess pAccess,
   nuint8   buNewFileAttr,
   nuint8   buDirHandle,
   nuint8   buSrchAttr,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName
)
{
   #define NCP_FUNCTION    ((nuint) 70)
   #define NCP_REQ_LEN     ((nuint) 4)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 0)

   NWCFrag reqFrag[NCP_REQ_FRAGS];
   nuint8 abuReq[NCP_REQ_LEN];

   abuReq[0] = buNewFileAttr;
   abuReq[1] = buDirHandle;
   abuReq[2] = buSrchAttr;
   abuReq[3] = buFileNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrFileName;
   reqFrag[1].uLen  = buFileNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/70.c,v 1.7 1994/09/26 17:38:52 rebekah Exp $
*/
