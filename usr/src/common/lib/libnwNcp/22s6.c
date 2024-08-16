/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s6.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s6VolGetName**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s6VolGetName
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            pnuint8  pbuVolNameLen,
            pnstr8   pbstrVolName,
         );

REMARKS: This call allows a client to use a Volume Number to retrieve a Volume Name.
         A client can also use this call to scan volumes and determine the Volume
         (mount) Numbers and Volume Names of all volumes currently mounted on the
         file server.  If the client wants to scan the file server for such
         information, the client should start with Volume Number zero (0) and scan
         upward until a Failure error (0xFF) is returned.  If a volume that
         corresponds to the Volume Number has not been mounted, this call will
         still return Successful (0), but the Volume Name Length field returned by
         the server will be zero, indicating that no corresponding volume is currently
         mounted but that the volume mount slot is (potentially) valid.

ARGS: <> pAccess
      >  buVolNum
      <  pbuVolNameLen
      <  pbstrVolName

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x89FF  Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 05  Get Volume Number

NCP:     22 06  Get Volume Name

CHANGES: 15 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s6VolGetName
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   pnuint8  pbuVolNameLen,
   pnstr8   pbstrVolName
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 6)
   #define NCP_STRUCT_LEN  ((nuint16) 2)
   #define NCP_REQ_LEN     ((nuint) 4)
   #define NCP_REPLY_LEN   ((nuint) 1)
   #define NCP_REQ_FRAGS   ((nuint) 1)
   #define NCP_REPLY_FRAGS ((nuint) 2)
   #define NCP_MAX_VOL_LEN ((nuint) 16)

   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = pbuVolNameLen;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   replyFrag[1].pAddr = pbstrVolName;
   replyFrag[1].uLen  = NCP_MAX_VOL_LEN;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s6.c,v 1.7 1994/09/26 17:34:40 rebekah Exp $
*/
