/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s116.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s116ChangeToClientRights***********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s116ChangeToClientRights
         (
            pNWAccess pAccess,
            nuint32  luQueueID,
            nuint16  suJobID
         );

REMARKS: This call allows a server to change its login identity to match that of the
         client.  This call replaces the server's login user ID and associated
         security equivalence list with the ID and security equivalence list of the
         object (user) that placed the job in the job queue.

         This call does not change any queue server path mappings (directory bases)
         on the file server.  However, all access rights to those directories are
         re-calculated to conform with the client's rights.  Any files opened before
         this call is made continue to be accessible with the server's rights; any
         files opened after this call is made are accessible only with the client's
         rights.  After this call is made, the server creates any path mappings it
         requires to carry out the client's requests.

         The Restore Server's Rights call can reverse the effects of the Change To
         Client Rights call.  In addition, the server's rights are automatically
         reset if the server calls Finish Servicing Queue Job or Abort Servicing
         Queue Job.

         Only a queue server that has previously accepted a job for service can make
         this call.

         See Introduction to Queue NCPs for information on both the old and new
         job structures.

ARGS: <> pAccess
       > luQueueID
       > suJobID

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

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     23 117  Restore Queue Server Rights

NCP:     23 116  Change To Client Rights

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s116ChangeToClientRights
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  suJobNumber
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 116)
   #define NCP_STRUCT_LEN  ((nuint)    7)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32(&abuReq[3], &luQueueID);
   NCopyHiLo16(&abuReq[7], &suJobNumber);

   return NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
      NULL, 0, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s116.c,v 1.7 1994/09/26 17:35:07 rebekah Exp $
*/
