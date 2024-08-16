/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s209.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpmsg.h"

/*manpage*NWNCP23s209SendConsoleBroadcast********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s209SendConsoleBroadcast
         (
            pNWAccess pAccess,
            nuint8   buNumStations,
            pnuint8  pbuStationList,
            nuint8   buMsgLen,
            pnstr8   pbstrMsg,
         )

REMARKS: This call sends a message to a list of logical connections.  A
         message will not reach a station that has disabled broadcasts or is
         not logged in. The requesting client must have console operator
         rights.

ARGS: <> pAccess
      >  buNumStations
      >  pbuStationList
      >  buMsgLen
      >  pbstrMsg
      >  flOption

INCLUDE: ncpmsg.h

RETURN:  0x89C6  No Console Rights
         0x89Fd  Bad Station Number

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 209  Send Console Broadcast

CHANGES:  25 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s209SendConsoleBroadcast
(
   pNWAccess pAccess,
   nuint8   buNumStations,
   pnuint8  pbuStationList,
   nuint8   buMsgLen,
   pnstr8   pbstrMsg
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        209)
   #define NCP_STRUCT_LEN     ((nuint16) (3 + buNumStations+ buMsgLen))
   #define NCP_REQ_LEN        ((nuint)           4)
   #define NCP_REQ_FRAGS      ((nuint)           4)
   #define NCP_REP_FRAGS      ((nuint)           0)

   NWCFrag reqFrag[NCP_REQ_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN];

   suNCPLen= NCP_STRUCT_LEN;
   NCopyHiLo16(abuReq, &suNCPLen);
   abuReq[2]= NCP_SUBFUNCTION;
   abuReq[3]= buNumStations;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbuStationList;
   reqFrag[1].uLen  = (nuint) buNumStations;

   reqFrag[2].pAddr = &buMsgLen;
   reqFrag[2].uLen  = (nuint) 1;

   reqFrag[3].pAddr = pbstrMsg;
   reqFrag[3].uLen  = (nuint) buMsgLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
      NCP_REP_FRAGS, NULL, NULL));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s209.c,v 1.7 1994/09/26 17:36:04 rebekah Exp $
*/
