/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s100.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s100CreateQ************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s100CreateQ
         (
            pNWAccess pAccess,
            nuint16  suQueueType,
            nuint8   buQueueNameLength,
            pnstr8   pbstrQueueName,
            nuint8   buPathBase,
            nuint8   buPathLength,
            pnstr8   pbstrPathName,
            pnuint32 pluQueueID
         );

REMARKS: This call creates the following:

         - A queue of type Queue Type with the name Queue Name in the file
         server's bindery.

         - The property Q_DIRECTORY.  (A path is set to the directory
         specified by Path Base as modified by the Path Name.)

         - A new subdirectory whose name is the 8-character ASCII
         hexadecimal representation of the new queue's bindery object ID
         number.  (The property Q_DIRECTORY is initialized with the path
         name of the newly created directory.)

         - The group properties Q_SERVERS, Q_OPERATORS, and Q_USERS.
         (No members are added to these groups.)

         Queue Name can be no longer than 47 characters.  The path name of the
         directory, specified by the Path Base and Path Name fields, cannot be
         longer than 118 characters.  A Path Base of zero, a Path Length of 10, and
         a Path Name string of "SYS:SYSTEM" are conventional.

         Like other Queue Services calls, Create Queue is actually accomplished by
         Bindery Services calls.  The main advantage of using this call is that the
         entire operation is backed out if any step of the creation process fails.
         Only a station with supervisor privileges can make this call.

         See Introduction to Queue NCPs for information on both the old and new job
         structures.


ARGS: <> pAccess
       > suQueueType
       > buQueueNameLength
       > pbstrQueueName
       > buPathBase
       > buPathLength
       > pbstrPathName
      <  pluQueueID (optional)

INCLUDE: ncpqms.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
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

SEE:     23 101  Destroy Queue

NCP:     23 100  Create Queue

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s100CreateQ
(
   pNWAccess pAccess,
   nuint16  suQueueType,
   nuint8   buQueueNameLength,
   pnstr8   pbstrQueueName,
   nuint8   buPathBase,
   nuint8   buPathLength,
   pnstr8   pbstrPathName,
   pnuint32 pluQueueID
)
{
   #define NCP_FUNCTION     ((nuint)   23)
   #define NCP_SUBFUNCTION  ((nuint8) 100)
   #define NCP_STRUCT_LEN   ((nuint) (6 + buQueueNameLength + buPathLength))
   #define NCP_REQ_LEN0     ((nuint)    6)
   #define NCP_REQ_LEN2     ((nuint)    2)
   #define NCP_REQ_FRAGS    ((nuint)    4)
   #define NCP_REP_FRAGS    ((nuint)    1)

   nint32  lCode;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];
   nuint8  abuReq0[NCP_REQ_LEN0], abuReq2[NCP_REQ_LEN2];
   nuint16 suNCPLen;
   nuint32 luTmpQueueID;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq0[0], &suNCPLen);
   abuReq0[2] = NCP_SUBFUNCTION;

   NCopyHiLo16(&abuReq0[3], &suQueueType);
   abuReq0[5] = buQueueNameLength;

   abuReq2[0] = buPathBase;
   abuReq2[1] = buPathLength;

   reqFrag[0].pAddr = abuReq0;
   reqFrag[0].uLen  = NCP_REQ_LEN0;

   reqFrag[1].pAddr = pbstrQueueName;
   reqFrag[1].uLen  = buQueueNameLength;

   reqFrag[2].pAddr = abuReq2;
   reqFrag[2].uLen  = NCP_REQ_LEN2;

   reqFrag[3].pAddr = pbstrPathName;
   reqFrag[3].uLen  = buPathLength;

   replyFrag[0].pAddr = &luTmpQueueID;
   replyFrag[0].uLen  = 4;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
            NCP_REP_FRAGS, replyFrag, NULL);

   if (lCode == 0)
   {
      if (pluQueueID)
         *pluQueueID = NSwapHiLo32(luTmpQueueID);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s100.c,v 1.7 1994/09/26 17:34:42 rebekah Exp $
*/
