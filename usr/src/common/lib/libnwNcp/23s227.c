/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s227.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s227GetLANDrvConfigInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s227GetLANDrvConfigInfo
         (
            pNWAccess pAccess,
            nuint8   buLANDriverNum,
            pNWNCPLANConfig pCfg,
         );

REMARKS: This call returns configuration information for the LAN drivers installed
         at the file server.

         Network Address contains the LAN board number to which the driver is
         attached.

         Host Address indicates the host address of the LAN board the driver is
         controlling.

         Board Installed indicates whether a driver is installed in the specified
         LAN board.  If Board Installed is zero, all fields returned are undefined.

         Option Number contains the option selected for the driver during
         configuration.  The selected option number specifies the hardware setup
         the driver uses when running the LAN board.

         Configuration Text is one or more null-terminated strings containing
         configuration information, such as the LAN board type ("NetWare
         Ethernet NP600," for example), and the LAN board hardware settings
         ("IRQ = 9, IOAddress = 300h, DMA = 7," for example).

ARGS: <> pAccess
      >  buLANDriverNum
      <  pCfg

INCLUDE: ncpserver.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89FF   Failure

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23 227  Get LAN Driver Configuration Information

CHANGES: 10 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s227GetLANDrvConfigInfo
(
   pNWAccess          pAccess,
   nuint8            buLANDriverNum,
   pNWNCPLANConfig   pCfg
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        227)
   #define NCP_STRUCT_LEN     ((nuint16)         4)
   #define NCP_REQ_LEN        ((nuint)           4)
   #define NCP_REP_LEN        ((nuint)         172)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = (nuint8) buLANDriverNum;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, abuReply,
         NCP_REP_LEN, NULL);
   if (lCode == 0)
   {
      NWCMemMove(pCfg->abuNetworkAddr, &abuReply[0], (nuint) 4);
      NWCMemMove(pCfg->abuHostAddr, &abuReply[4], (nuint) 6);
      pCfg->buBoardInstalled = abuReply[10];
      pCfg->buOptionNum = abuReply[11];
      NWCMemMove(pCfg->abuConfigText1, &abuReply[12], (nuint) 80);
      NWCMemMove(pCfg->abuConfigText2, &abuReply[92], (nuint) 80);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s227.c,v 1.7 1994/09/26 17:36:34 rebekah Exp $
*/
