/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:66.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP66FileClose******************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP66FileClose
         (
            pNWAccess pAccess,
            nuint8   buReserved,
            pnuint8  pbuNWHandleB6,
         )

REMARKS: This call closes a file.  FileHandle must be the handle returned to the
         client by an Open File or Create File request.  After a file is closed,
         the file handle is no longer valid.  File access requests to an invalid
         file handle will be disregarded by the server.

         A file handle must be closed before a file can be made available to other
         clients.


ARGS: <> pAccess,
       > buReserved,
       > pbuNWHandleB6

INCLUDE: "ncpfile.h"

RETURN:

SERVER:  2.0 2.2 3.11 4.0

CLIENT:  DOS WIN OS2

SEE:

NCP:     66 Close File

CHANGES: 27 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP66FileClose
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuNWHandleB6
)
{
   #define NCP_FUNCTION    ((nuint) 66)
   #define NCP_REQ_LEN     ((nuint) 7)

   nuint8 abuReq[NCP_REQ_LEN];

   abuReq[0]= buReserved;
   NWCMemMove(&abuReq[1],pbuNWHandleB6,(nuint) 6);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               NULL, (nuint) 0, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/66.c,v 1.7 1994/09/26 17:38:45 rebekah Exp $
*/
