/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s239.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s239GetLogicalRecsByConn**********************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP23s239GetLogicalRecsByConn
         (
            pNWAccess pAccess,
            nuint16  suTargetConnNum,
            pnuint16 psuIterHnd,
            pnuint16 psuNumRecs,
            pnuint8  pbuLogRecsB508,
         );

REMARKS: This call returns the logical records that a connection has logged with
         the file server.

         Target Connection Number indicates the logical connection that has the
         record locked exclusively.

         Last Record Seen specifies the last record for which information is being
         returned.  On the first call, Last Record Seen must be set to zero.
         Subsequent calls should set Last Record Seen to the value in Next
         Request Record returned in the previous reply message.  If the value in
         Next Request Record is zero, the file server has passed all information to
         the requesting client.

         Next Request Record is the value that Last Record Seen must be set to
         in the next call.

         The following information is returned as many times as Number Of
         Records indicates.

            Task Number indicates which task within the connection that has
            the record logged.

            Lock Status contains the bit flags indicating the file's lock status.

                  7 6 5 4 3 2 1 0
                  x x x x x x x 1  Locked exclusive
                  x x x x x x 1 x  Locked shareable
                  x x x x x 1 x x  Logged
                  x 1 x x x x x x  Lock is held by TTS

         Lock Name Length indicates the length of the Lock Name that follows.

         Lock Name is the name of the logical lock.

         The requesting client must have console operator rights to make this
         call.  This routine replaces the NetWare 286 v2.1 NCP Get Logical
         Records By Connection (0x2222  23 223).


ARGS: <> pAccess
      >  suTargetConnNum
      <> psuIterHnd
      <  psuNumRecs
      <  pLogicalRecsByConn

INCLUDE: ncpserve.h

RETURN:  0x0000 Successful
         0x8996 Server Out Of Memory
         0x89C6 No Console Rights
         0x89FD Bad Station Number

SERVER:  3.1 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 223  Get Logical Records By Connection (old)
         23 240  Get Logical Record Information

NCP:     23 223  Get Logical Records By Connection

CHANGES: 3 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s239GetLogicalRecsByConn
(
   pNWAccess pAccess,
   nuint16  suTargetConnNum,
   pnuint16 psuIterHnd,
   pnuint16 psuNumRecs,
   pnuint8  pbuLogRecsB508
)
{
   #define NCP_FUNCTION             ((nuint)          23)
   #define NCP_SUBFUNCTION          ((nuint8)        239)
   #define NCP_STRUCT_LEN           ((nuint16)         5)
   #define NCP_REQ_LEN              ((nuint)           7)
   #define NCP_REP_LEN              ((nuint)           4)
   #define NCP_REQ_FRAGS            ((nuint)           1)
   #define NCP_REP_FRAGS            ((nuint)           2)
   #define NCP_MAX_RECS_BY_CONN     ((nuint)         508)


   nint32   lCode;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuRep[4];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi16(&abuReq[3],&suTargetConnNum);
   NCopyHiLo16(&abuReq[5],psuIterHnd);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuRep;
   replyFrag[0].uLen  = NCP_REP_LEN;

   replyFrag[1].pAddr = pbuLogRecsB508;
   replyFrag[1].uLen  = NCP_MAX_RECS_BY_CONN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag, NCP_REP_FRAGS,
               replyFrag, NULL);
   if(lCode == 0)
   {
      NCopyLoHi16(psuIterHnd,&abuRep[0]);
      NCopyLoHi16(psuNumRecs,&abuRep[2]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s239.c,v 1.7 1994/09/26 17:36:53 rebekah Exp $
*/
