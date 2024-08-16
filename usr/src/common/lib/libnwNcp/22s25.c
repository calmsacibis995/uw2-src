/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s25.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s25SetDirInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s25SetDirInfo
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint16  suCreationDate,
            nuint16  suCreationTime,
            nuint32  luOwnerID,
            nuint8   buMaxRightsMask,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
         );

REMARKS: This call allows a client to modify information about a directory.  Clients
         can use this call to restore a directory that has been destroyed and is
         being regenerated from some backup medium.

         The directory's Creation Date, Creation Time, and Maximum Access Rights can
         be set by any client that has access control and modify rights in the
         directory's parent directory.

         The Owner ID number can be changed only by a client that is a supervisor of
         the object.

ARGS: <> pAccess
      >  buDirHandle
      >  suCreationDate
      >  suCreationTime
      >  luOwnerID
      >  buMaxRightsMask
      >  buPathLen
      >  pbstrPath

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x898C  No Set Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FF  Failure, No Files Found

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     22 45  Get Directory Information

NCP:     22 25  Set Directory Information

CHANGES: 15 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s25SetDirInfo
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint16  suCreationDate,
   nuint16  suCreationTime,
   nuint32  luOwnerID,
   nuint8   buMaxRightsMask,
   nuint8   buPathLen,
   pnstr8   pbstrPath
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 25)
   #define NCP_STRUCT_LEN  ((nuint16) 12 + buPathLen)
   #define NCP_REQ_LEN     ((nuint) 14)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 0)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   NCopyHiLo16(&abuReq[4], &suCreationDate);
   NCopyHiLo16(&abuReq[6], &suCreationTime);
   NCopyHiLo32(&abuReq[8], &luOwnerID);
   abuReq[12] = buMaxRightsMask;
   abuReq[13] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = (nuint) buPathLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s25.c,v 1.7 1994/09/26 17:34:05 rebekah Exp $
*/
