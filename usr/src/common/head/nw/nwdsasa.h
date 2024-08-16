/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwdsasa.h	1.4"
/****************************************************************************
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/
#ifndef  _NWDSASA_HEADER_
#define  _NWDSASA_HEADER_

#ifndef  _NWDSDC_HEADER_
#ifdef N_PLAT_UNIX
#include <nw/nwdsdc.h>     /* for NWDSContextHandle typedef */
#else /* !N_PLAT_UNIX */
#include "nwdsdc.h"     /* for NWDSContextHandle typedef */
#endif /* N_PLAT_UNIX */
#endif

#ifndef NWCONNECT_INC
#ifdef N_PLAT_UNIX
#include <nw/nwconnec.h>
#else /* !N_PLAT_UNIX */
#include "nwconnec.h"
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

#define SESSION_KEY_SIZE   16
typedef nuint8 NWDS_Session_Key_T[SESSION_KEY_SIZE];  /* Optional session key */
typedef NWDS_Session_Key_T N_FAR *  pNWDS_Session_Key_T;

#ifdef N_PLAT_MAC
#include "nwcaldef.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* NWClient DS prototypes */
NWDSCCODE N_API NWCDSAuthenticate
(
   NWCONN_HANDLE        conn,
   nflag32              optionsFlag,
   pNWDS_Session_Key_T  sessionKey
);

NWDSCCODE N_API NWCDSLogout
(
   NWDSContextHandle context
);


NWDSCCODE N_API NWDSAuthenticate
(
   NWCONN_HANDLE        conn,
   nflag32              optionsFlag,
   pNWDS_Session_Key_T  sessionKey
);

NWDSCCODE N_API NWDSChangeObjectPassword
(
   NWDSContextHandle context,
   nflag32           optionsFlag,
   pnstr8            objectName,
   pnstr8            oldPassword,
   pnstr8            newPassword
);

NWDSCCODE N_API NWDSGenerateObjectKeyPair
(
   NWDSContextHandle contextHandle,
   pnstr8            objectName,
   pnstr8            objectPassword,
   nflag32           optionsFlag
);

NWDSCCODE N_API NWDSLogin
(
   NWDSContextHandle context,
   nflag32           optionsFlag,
   pnstr8            objectName,
   pnstr8            password,
   nuint32           validityPeriod
);

NWDSCCODE N_API NWDSLogout
(
   NWDSContextHandle context
);

NWDSCCODE N_API NWDSVerifyObjectPassword
(
   NWDSContextHandle context,
   nflag32           optionsFlag,
   pnstr8            objectName,
   pnstr8            password
);

#ifdef __cplusplus
}
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwdsasa.h,v 1.5 1994/06/08 23:32:41 rebekah Exp $
*/
