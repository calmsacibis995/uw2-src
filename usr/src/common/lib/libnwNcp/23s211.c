/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s211.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s211DownServer**************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s211DownServer
         (
            pNWAccess pAccess,
            nuint8   buForceFlag,
         )
REMARKS: Allows a supervisor to take down a file server from a remote
         console.

         If the Force flag is zero, the server determines whether any stations
         have file server files open.  If any files are in use or open, a Failure
         completion code (0xFF) is returned, and the server does not go down.  If
         no files are open or in use, then the server will shut down, and a
         Successful completion code is returned.

         If the Force flag is 0xFF, the server will shut down.  All open files will
         automatically be closed, and all unfinished transactions will
         automatically be backed out.

         If the calling station does not have supervisor privileges, the No Console
         Rights completion code is returned and the server does not go down.

ARGS: <> pAccess
      >  buForceFlag

INCLUDE: ncpserve.h

RETURN:   0x0000   Successful
          0x89C6   No Console Rights
          0x89FF   Failure


SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     Disable File Server Login (0x2222  23  203)
         Get File Server Login Status (0x2222  23  205)
         Enable File Server Login (0x2222  23  204)


NCP:     23  211  (Down File Server)

CHANGES: 18 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
*****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s211DownServer
(
   pNWAccess pAccess,
   nuint8   buForceFlag
)
{
   #define NCP_FUNCTION       ((nuint)         23)
   #define NCP_SUBFUNCTION    ((nuint8)       211)
   #define NCP_STRUCT_LEN     ((nuint16)        2)
   #define NCP_REQ_LEN        ((nuint)          4)
   #define NCP_REP_LEN        ((nuint)          0)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16 (abuReq, &suNCPLen);

   abuReq[2] = NCP_SUBFUNCTION;

   if(buForceFlag == 0)
      abuReq[3] = (nuint8) 0xff; /* shut down file server with open files */
   else
      abuReq[3] =(nuint) 0;    /* do not shut down if files are opened */

   return (NWCRequestSingle (pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               NULL, NCP_REP_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s211.c,v 1.7 1994/09/26 17:36:07 rebekah Exp $
*/
