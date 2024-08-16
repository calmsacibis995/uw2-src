/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:11.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP11SyncClrLogRec**********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP11SyncClrLogRec
         (
            pNWAccess pAccess,
            nuint8   buRecNameLen,
            pnuint8  pbuRecName,
         );

REMARKS: This call releases the specified synchronization string (Synch Name) and
         removes it from the client's synchronization string table.  The
         synchronization string cannot be used until it is re-logged by the client.

ARGS: <> pAccess
      >  buRecNameLen
      >  pbuRecName

INCLUDE: ncpsync.h

RETURN:  0x0000  Successful
         0x89FF  Unlock Error

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     09 --  Log Logical Record
         10 --  Lock Logical Record Set
         14 --  Clear Logical Record Set
         12 --  Release Logical Record
         13 --  Release Logical Record Set

NCP:     11 --  Clear Logical Record

CHANGES: 27 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP11SyncClrLogRec
(
   pNWAccess pAccess,
   nuint8   buRecNameLen,
   pnstr8  pbuRecName
)
{
   #define NCP_FUNCTION    ((nuint) 11)
   #define REQ_LEN         ((nuint) 1)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   NWCFrag reqFrag[REQ_FRAGS];

   reqFrag[0].pAddr = &buRecNameLen;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuRecName;
   reqFrag[1].uLen  = buRecNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag, REPLY_FRAGS,
               NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/11.c,v 1.7 1994/09/26 17:31:58 rebekah Exp $
*/
