/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s5.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s5VolGetNum**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s5VolGetNum
         (
            pNWAccess pAccess,
            nuint8   buVolNameLen,
            pnstr8   pbstrVolName,
            pnuint8  pbuVolNum,
         );

REMARKS: This call allows a client to use a Volume Name to retrieve a Volume Number.
         Volume Numbers are required by several calls, including the calls to get a
         volume's usage statistics and the calls to get an object's trustee directory
         paths.

ARGS: <> pAccess
      >  buVolNameLen
      >  pbstrVolName
      <  pbuVolNum

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error

SERVER:

CLIENT:  DOS OS2 WIN NT

SEE:     22 06  Get Volume Name

NCP:     22 05  Get Volume Number

CHANGES: 15 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s5VolGetNum
(
   pNWAccess pAccess,
   nuint8   buVolNameLen,
   pnstr8   pbstrVolName,
   pnuint8  pbuVolNum
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 5)
   #define NCP_STRUCT_LEN  ((nuint16) (2 + buVolNameLen))
   #define NCP_REQ_LEN     ((nuint) (4+16))
   #define NCP_REPLY_LEN   ((nuint) 1)

   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN];
   nint    i;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNameLen;
   for (i = 0; i < (nint) buVolNameLen; i++)
      abuReq[4+i] = pbstrVolName[i];

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               pbuVolNum, NCP_REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s5.c,v 1.7 1994/09/26 17:34:36 rebekah Exp $
*/
