/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s26.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s26GetPathName**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s26GetPathName
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            nuint16  suDirBase,
            pnuint8  pbuPathLen,
            pnstr8   pbstrPath
         );

REMARKS: This call returns the Directory Path for a Volume Number/Directory Entry
         Number pair.  This call does not work under NetWare 386.

ARGS: <> pAccess
      >  buVolNum
      >  suDirBase
      <  pbuPathLen
      <  pbstrPath

INCLUDE: ncpfile.h

RETURN:  0x0000 Successful
         0x8998 Disk Map Error
         0x899C Invalid Path
         0x89A1 Directory I/O Error

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     22 26  Get Path Name Of A Volume-Directory Number Pair

CHANGES: 14 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s26GetPathName
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint16  suDirBase,
   pnuint8  pbuPathLen,
   pnstr8   pbstrPath
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 26)
   #define NCP_STRUCT_LEN  ((nuint16) 4)
   #define MAX_PATH_LEN    ((nuint) 256)
   #define NCP_REQ_LEN     ((nuint) 6)
   #define NCP_REPLY_LEN   ((nuint) 1)
   #define NCP_REQ_FRAGS   ((nuint) 1)
   #define NCP_REPLY_FRAGS ((nuint) 2)

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;
   NCopyLoHi16(&abuReq[4], &suDirBase);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = pbuPathLen;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   replyFrag[1].pAddr = pbstrPath;
   replyFrag[1].uLen  = *pbuPathLen;
/*   replyFrag[1].uLen  = MAX_PATH_LEN; if the buf len is passed we should use it */

   return(NWCRequest(pAccess,
                     NCP_FUNCTION,
                     NCP_REQ_FRAGS,
                     reqFrag,
                     NCP_REPLY_FRAGS,
                     replyFrag,
                     NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s26.c,v 1.7 1994/09/26 17:34:06 rebekah Exp $
*/
