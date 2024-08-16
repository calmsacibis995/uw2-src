/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpconn.h	1.3"
#if !defined( NCPCONN_H )
#define NCPCONN_H

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

/***************************************************************************
   Constant declarations
****************************************************************************/

/***************************************************************************
   Structure declarations
****************************************************************************/

typedef struct tagNWNCPNetAddr
{
   nuint8   abuNetAddr[4];
   nuint8   abuNetNodeAddr[6];
   nuint16  suNetSocket;
} NWNCPNetAddr, N_FAR *pNWNCPNetAddr;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( NWRCODE )
NWNCP24EndOfJob
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s19GetInternetAddr
(
   pNWAccess pAccess,
   nuint8   buTargetConn,
   pnuint8  pbuNetAddrB12
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s26GetInternetAddr
(
   pNWAccess       pAccess,
   nuint32        luTargetConn,
   pNWNCPNetAddr  pNetAddr,
   pnuint8        pbuConnType
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s29ChangeConnectState
(
   pNWAccess    pAccess,
   nuint8      buReqCode
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s30SetWatchdogDelay
(
   pNWAccess    pAccess,
   nuint32     luDelayMins
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s21GetObjConnList
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   pnuint8  pbuListLen,
   pnuint8  pbuConnNumList
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s27GetObjConnList
(
   pNWAccess pAccess,
   nuint32  luSrchConnNum,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   pnuint8  pbuListLen,
   pnuint32 pluConnNumList
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP19GetStationNum
(
   pNWAccess pAccess,
   pnuint8  pbuStationNumB3
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s5GetStationLoggedInfo
(
   pNWAccess pAccess,
   nuint8   buTargetConnNum,
   pnuint8  pbuUserNameB16,
   pnuint8  pbuLoginTimeB7,
   pnuint8  pbuFullNameB39,
   pnuint32 pluUserID,
   pnuint32 pluSecEquivListB32,
   pnuint8  pbuReservedB64
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s22GetStationLoggedInfo
(
   pNWAccess pAccess,
   nuint8   buTargetConnNum,
   pnuint32 pluUserID,
   pnuint16 psuUserType,
   pnuint8  pbuUserNameB48,
   pnuint8  pbuLoginTimeB7,
   pnuint8  pbuReserved
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s28GetStationLoggedInfo
(
   pNWAccess pAccess,
   nuint32  luTargetConnNum,
   pnuint32 pluUserID,
   pnuint16 psuUserType,
   pnuint8  pbuUserNameB48,
   pnuint8  pbuLoginTimeB7,
   pnuint8  pbuReserved
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s2GetUserConnList
(
   pNWAccess pAccess,
   nuint8   buUserNameLen,
   pnstr8   pbstrUserName,
   pnuint8  pbuListLen,
   pnuint8  pbuConnNumList
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP25Logout
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP33NegotiateBufSize
(
   pNWAccess pAccess,
   nuint16  suPropBufSize,
   pnuint16 psuAccBufSize
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s31GetConnListFromObj
(
   pNWAccess pAccess,
   nuint32  luObjID,
   nuint32  luSrchConnNum,
   pnuint16 psuConnListLen,
   pnuint32 pluConnList
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP97GetBigPktNCPMaxPktSize
(
   pNWAccess pAccess,
   nuint16  suProposedMax,
   nuint8   buSecurityFlag,
   pnuint16 psuAcceptedMax,
   pnuint16 psuEchoSocket,
   pnuint8  pbuSecurityFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP101PktBurstConnReq
(
   pNWAccess pAccess,
   nuint32  luLocalConn,
   nuint32  luLocalMaxPktSize,
   nuint16  suLocalTargetSocket,
   nuint32  luLocalMaxSendSize,
   nuint32  luLocalMaxRecvSize,
   pnuint32 pluRemoteTargID,
   pnuint32 pluRemoteMaxPktSize
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpconn.h,v 1.5 1994/09/26 17:11:11 rebekah Exp $
*/
