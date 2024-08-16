/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s117.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s117RestoreQServerRights**************
*********************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s117RestoreQServerRights
         (
            pNWAccess pAccess
         );

REMARKS: This call restores a queue server's identity after it has assumed its
         client's identity by using Change To Client Rights (function 116).  The
         queue server's login user ID and the associated security equivalence list
         are also restored to the queue server's values.

         This call does not change any queue server path mappings (directory bases)
         on the file server.  However, all access rights to those directories are
         recalculated to conform with the queue server's rights.  If the queue server
         changes some of its path mappings in order to service the queue job, the
         queue restores its own directory bases.  Any files that are opened before
         this call is made continue to be accessible with the rights of the queue
         client; any files opened after this call is made are accessible to any
         station with the rights of the queue server.

         Only a queue server that has previously changed its identity using Change
         To Client Rights can make this call.

         See Introduction to Queue NCPs for information on both the old and new
         job structures.

ARGS: <> pAccess

INCLUDE: ncpqms.h

RETURN:  0x0000  Successful
         0x8999  Directory Full Error
         0x89D0  Queue Error
         0x89D1  No Queue
         0x89D2  No Queue Server
         0x89D3  No Queue Rights
         0x89D4  Queue Full
         0x89D5  No Queue Job
         0x89D6  No Job Right
         0x89D7  Queue Servicing
         0x89D8  Queue Not Active
         0x89D9  Station Not Server
         0x89DA  Queue Halted
         0x89DB  Maximum Queue Servers
         0x89FF  Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 116  Change To Client Rights

NCP:     23 117  Restore Queue Server Rights

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s117RestoreQServerRights
(
   pNWAccess pAccess
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 117)
   #define NCP_STRUCT_LEN  ((nuint)    1)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16((pnuint16)&abuReq[0], &suNCPLen);
   abuReq[2] = (nuint8)NCP_SUBFUNCTION;

   return NWCRequestSingle(pAccess,
                           (nuint)NCP_FUNCTION,
                           abuReq,
                           (nuint)NCP_REQ_LEN,
                           NULL,
                           (nuint)0,
                           NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s117.c,v 1.7 1994/09/26 17:35:09 rebekah Exp $
*/
