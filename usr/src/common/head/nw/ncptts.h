/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncptts.h	1.5"
#if !defined( NCPTTS_H )
#define NCPTTS_H

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

typedef struct tagNWNCPTTSStats
{
  nuint32 luSysElapsedTime;
  nuint8  buSupported;
  nuint8  buEnabled;
  nuint16 suVolumeNum;
  nuint16 suMaxOpenTrans;
  nuint16 suMaxTransOpened;
  nuint16 suCurrTransOpen;
  nuint32 luTotalTrans;
  nuint32 luTotalWrites;
  nuint32 luTotalBackouts;
  nuint16 suUnfilledBackouts;
  nuint16 suDiskBlocksInUse;
  nuint32 luFATAllocations;
  nuint32 luFileSizeChanges;
  nuint32 luFilesTruncated;
  nuint8  buNumTransactions;
  struct
  {
    nuint8 buConnNumber;
    nuint8 buTaskNumber;
  } connTask[235];
} NWNCPTTSStats, N_FAR *pNWNCPTTSStats;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( NWRCODE )
NWNCP34s3TTSAbortTrans
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP34s1TTSBeginTrans
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP34s2TTSEndTrans
(
   pNWAccess pAccess,
   pnuint32 pluTransNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP34s5TTSGetAppThresholds
(
   pNWAccess pAccess,
   pnuint8  pbuLogLockThreshold,
   pnuint8  pbuPhyLockThreshold
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP34s9TTSGetTransBits
(
   pNWAccess pAccess,
   pnuint8  buFlags
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP34s7TTSGetThresholds
(
   pNWAccess pAccess,
   pnuint8  pbuLogLockThreshold,
   pnuint8  pbuPhyLockThreshold
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP34s0TTSIsAvailable
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP34s6TTSSetAppThresholds
(
   pNWAccess pAccess,
   nuint8   buLogLockThreshold,
   nuint8   buPhyLockThreshold
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP34s10TTSSetTransBits
(
   pNWAccess pAccess,
   nuint8   buFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP34s8TTSSetThresholds
(
   pNWAccess pAccess,
   nuint8   buLogLockThreshold,
   nuint8   buPhyLockThreshold
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP34s4TTSTransStatus
(
   pNWAccess pAccess,
   nuint32  luTransNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s208EnableTransTracking
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s207DisableTracking
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s208EnableTracking
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s213GetTrackingStats
(
   pNWAccess  pAccess,
   pNWNCPTTSStats pStats
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncptts.h,v 1.8 1994/09/26 17:11:38 rebekah Exp $
*/
