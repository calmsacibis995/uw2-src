/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwtts.h	1.4"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWTTS_INC
#define NWTTS_INC

#ifndef NWCALDEF_INC
#ifdef N_PLAT_UNIX
# include <nw/nwcaldef.h>
#else /* !N_PLAT_UNIX */
# include <nwcaldef.h>
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

typedef struct
{
  nuint32 systemElapsedTime;
  nuint8  TTS_Supported;
  nuint8  TTS_Enabled;
  nuint16 TTS_VolumeNumber;
  nuint16 TTS_MaxOpenTransactions;
  nuint16 TTS_MaxTransactionsOpened;
  nuint16 TTS_CurrTransactionsOpen;
  nuint32 TTS_TotalTransactions;
  nuint32 TTS_TotalWrites;
  nuint32 TTS_TotalBackouts;
  nuint16 TTS_UnfilledBackouts;
  nuint16 TTS_DiskBlocksInUse;
  nuint32 TTS_FATAllocations;
  nuint32 TTS_FileSizeChanges;
  nuint32 TTS_FilesTruncated;
  nuint8  numberOfTransactions;
  struct
  {
    nuint8 connNumber;
    nuint8 taskNumber;
  } connTask[235];
} TTS_STATS;

NWCCODE N_API NWTTSAbortTransaction
(
   NWCONN_HANDLE  conn
);

NWCCODE N_API NWTTSBeginTransaction
(
   NWCONN_HANDLE  conn
);

NWCCODE N_API NWTTSIsAvailable
(
   NWCONN_HANDLE  conn
);

NWCCODE N_API NWTTSGetControlFlags
(
   NWCONN_HANDLE  conn,
   pnuint8        controlFlags
);

NWCCODE N_API NWTTSSetControlFlags
(
   NWCONN_HANDLE  conn,
   nuint8         controlFlags
);

NWCCODE N_API NWTTSEndTransaction
(
   NWCONN_HANDLE  conn,
   pnuint32       transactionNum
);

NWCCODE N_API NWTTSTransactionStatus
(
   NWCONN_HANDLE  conn,
   nuint32        transactionNum
);

NWCCODE N_API NWTTSGetProcessThresholds
(
   NWCONN_HANDLE  conn,
   pnuint8        logicalLockLevel,
   pnuint8        physicalLockLevel
);

NWCCODE N_API NWTTSSetProcessThresholds
(
   NWCONN_HANDLE  conn,
   nuint8         logicalLockLevel,
   nuint8         physicalLockLevel
);

NWCCODE N_API NWTTSGetConnectionThresholds
(
   NWCONN_HANDLE  conn,
   pnuint8        logicalLockLevel,
   pnuint8        physicalLockLevel
);

NWCCODE N_API NWTTSSetConnectionThresholds
(
   NWCONN_HANDLE  conn,
   nuint8         logicalLockLevel,
   nuint8         physicalLockLevel
);

NWCCODE N_API NWEnableTTS
(
   NWCONN_HANDLE  conn
);

NWCCODE N_API NWDisableTTS
(
   NWCONN_HANDLE  conn
);

NWCCODE N_API NWGetTTSStats
(
   NWCONN_HANDLE  conn,
   TTS_STATS N_FAR * ttsStats
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwtts.h,v 1.6 1994/06/08 23:33:29 rebekah Exp $
*/
