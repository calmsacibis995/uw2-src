/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpprint.h	1.3"
#if !defined( NCPPRINT_H )
#define NCPPRINT_H

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

typedef struct tagNWNCPQueueInfo
{
   nuint8   buJobNumber;
   nuint16  suReserved1;
   nuint8   abuFileName[14];
   nuint8   buVolumeNumber;
   nuint8   buPrintFlags;
   nuint8   buTabSize;
   nuint8   buTargetPrinter;
   nuint8   buCopies;
   nuint8   buFormType;
   nuint8   buOriginatingClient;
   nuint8   abuTimeSpooled[6];
   nuint8   abuReserved2[15];
   nuint8   abuBannerName[14];
   nuint8   abuReserved3[18];
   nuint32  luObjectID;
} NWNCPQueueInfo, N_FAR *pNWNCPQueueInfo;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( NWRCODE )
NWNCP17s0WriteToSpoolFile
(
   pNWAccess pAccess,
   nuint8   buDataLen,
   pnuint8  pbuDataB256
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP17s1CloseSpoolFile
(
   pNWAccess pAccess,
   nuint8   buAbortQueueFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP17s2SetSpoolFileFlags
(
   pNWAccess pAccess,
   nuint8   buPrintFlags,
   nuint8   buTabSize,
   nuint8   buTargetPrinter,
   nuint8   buCopies,
   nuint8   buFormType,
   nuint8   buReserved,
   pnstr8   pbstrBannerNameB14
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP17s3SpoolADiskFile
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buNameLen,
   pnstr8   pbstrNameB256
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP17s4ScanSpoolFileQueue
(
   pNWAccess pAccess,
   nuint8   buTargetPrinter,
   nuint8   buIterHnd,
   pNWNCPQueueInfo pQueueInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP17s5DeleteSpoolFile
(
   pNWAccess pAccess,
   nuint8   buTargetPrinter,
   nuint8   buNumberOfJobs,
   pnuint8  pbuJobListB199
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP17s6GetPrinterStatus
(
   pNWAccess pAccess,
   nuint8   buTargetPrinter,
   pnuint8  pbuHalted,
   pnuint8  pbuOffLine,
   pnuint8  pbuFormType,
   pnuint8  pbuRedirectedPrinter
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP17s9CreateSpoolFile
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buNameLen,
   pnstr8   pbstrNameB256
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP17s10GetPrinterQueue
(
   pNWAccess pAccess,
   nuint8   buPrinterNuber,
   pnuint32 pluObjID
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpprint.h,v 1.5 1994/09/26 17:11:30 rebekah Exp $
*/
