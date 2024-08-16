/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s135.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpqms.h"

/*manpage*NWNCP23s135GetQJobFileSize****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s135GetQJobFileSize
         (
            pNWAccess pAccess,
            nuint32  luInQueueID,
            nuint32  luInJobNumber,
            pnuint32 pluOutQueueID,
            pnuint32 pluOutJobNumber,
            pnuint32 pluFileSize
         );

REMARKS: This is a new NetWare 386 v3.11 call that replaces the earlier call Get
         Queue Job File Size (0x2222  23  120).  This new NCP allows the use of the
         high connection byte in the Request/Reply header of the packet.  A new job
         structure has also been defined for this new NCP.  See Introduction to Queue
         NCPs for information on both the old and new job structures.

ARGS: <> pAccess
       > luInQueueID
       > luInJobNumber
      <  pluOutQueueID (optional)
      <  pluOutJobNumber (optional)
      <  pluFileSize

INCLUDE: ncpqms.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 120  Get Queue Job File Size

NCP:     23 135  Get Queue Job File Size

CHANGES: 2 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s135GetQJobFileSize
(
   pNWAccess pAccess,
   nuint32  luInQueueID,
   nuint32  luInJobNumber,
   pnuint32 pluOutQueueID,
   pnuint32 pluOutJobNumber,
   pnuint32 pluFileSize
)
{
   #define NCP_FUNCTION    ((nuint)   23)
   #define NCP_SUBFUNCTION ((nuint8) 135)
   #define NCP_STRUCT_LEN  ((nuint)    9)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint)   12)

   nint32   lCode;
   nuint8  abuReq[NCP_STRUCT_LEN+2];
   nuint8  abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16((pnuint16)&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo32((pnuint32)&abuReq[3], &luInQueueID);
   NCopyHiLo32((pnuint32)&abuReq[7], &luInJobNumber);

   lCode = NWCRequestSingle(pAccess,
                            (nuint)NCP_FUNCTION,
                            abuReq,
                            (nuint)NCP_REQ_LEN,
                            abuReply,
                            (nuint)NCP_REPLY_LEN,
                            NULL);

   if(lCode == 0)
   {
      if(pluOutQueueID != NULL)
        NCopyHiLo32(pluOutQueueID, (pnuint32)&abuReply[0]);

      if(pluOutJobNumber != NULL)
        NCopyLoHi32(pluOutJobNumber, (pnuint32)&abuReply[4]);

      NCopyHiLo32(pluFileSize, (pnuint32)&abuReply[8]);
   }

   return((NWRCODE)lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s135.c,v 1.10 1994/09/26 17:35:36 rebekah Exp $
*/
