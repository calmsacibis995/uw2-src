/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpmsg.h	1.5"
#if !defined( NCPMSG_H )
#define NCPMSG_H

#if !defined( NTYPES_H )
#ifdef N_PLAT_UNIX
#include <nw/ntypes.h>
#else /* !N_PLAT_UNIX */
#include <ntypes.h>
#endif /* N_PLAT_UNIX */
#endif

#if !defined( NWACCESS_H )
#ifdef N_PLAT_UNIX
#include <nw/nwaccess.h>
#else /* !N_PLAT_UNIX */
#include <nwaccess.h>
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( NWRCODE )
NWNCP21s9BroadcastToConsole
(
   pNWAccess pAccess,
   nuint8   buMsgLen,
   pnuint8  pbuMsg
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP21s8CheckPipeStatus
(
   pNWAccess pAccess,
   nuint8   buNumClients,
   pnuint8  pbuClientBuf,
   pnuint8  pbuNumStatus,
   pnuint8  pbuPipeStatusBuf
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP21s7CloseMsgPipe
(
   pNWAccess pAccess,
   nuint8   buNumClients,
   pnuint8  pbuClientBuf,
   pnuint8  pbuNumStatus,
   pnuint8  pbuPipeStatusBuf
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s253SendConsoleBroadcast
(
   pNWAccess  pAccess,
   nuint8    buNumStations,
   pnuint32  aluStationList,
   nuint8    buMsgLen,
   pnstr8    pbstrMsg
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s209SendConsoleBroadcast
(
   pNWAccess pAccess,
   nuint8   buNumStations,
   pnuint8  pbuStationList,   /* array of nuint8's (buNumStations) */
   nuint8   buMsgLen,
   pnstr8   pbstrMsg
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP21s2DisableBroadcasts
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP21s3EnableBroadcasts
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP21s11MsgGetBroadcast
(
   pNWAccess pAccess,
   pnuint8  pbuMsgLen,
   pnuint8  pbuMsg
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP21s1MsgGetBroadcast
(
   pNWAccess pAccess,
   pnuint8  pbuMsgLen,
   pnuint8  pbuMsg
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP21s5MsgGetPersonal
(
   pNWAccess pAccess,
   pnuint8  pbuSendingConnNum,
   pnuint8  pbuMsgLen,
   pnuint8  pbuMsg
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s13MsgLogNet
(
   pNWAccess pAccess,
   nuint8   buMsgLen,
   pnstr8   pbstrMsg
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP21s6MsgOpenPipe
(
   pNWAccess pAccess,
   nuint8   buNumClients,
   pnuint8  pbuClientBuf,
   pnuint8  pbuNumStatus,
   pnuint8  pbuPipeStatusBuf
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP21s0MsgSendBroadcast
(
   pNWAccess pAccess,
   nuint8   buNumClients,
   pnuint8  pbuClientBuf,
   nuint8   buMsgLen,
   pnstr8   pbstrMsg,
   pnuint8  pbuNumStatus,
   pnuint8  pbuSendStatusBuf
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP21s10MsgSendBroadcast
(
   pNWAccess  pAccess,
   nuint16   suNumClients,
   pnuint32  pluClientB,
   nuint8    buMsgLen,
   pnstr8    pbstrMsg,
   pnuint16  psuNumStatus,
   pnuint32  pluSendStatusB
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP21s4MsgSendPersonal
(
   pNWAccess pAccess,
   nuint8   buNumClients,
   pnuint8  pbuClientBuf,
   nuint8   buMsgLen,
   pnstr8   pbstrMsg,
   pnuint8  pbuNumStatus,
   pnuint8  pbuSendStatusBuf
);

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#ifdef __cplusplus
}
#endif

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpmsg.h,v 1.7 1994/09/26 17:11:29 rebekah Exp $
*/
