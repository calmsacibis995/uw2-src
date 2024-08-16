/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwmigrat.h	1.4"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWMIGRAT_INC
#define NWMIGRAT_INC

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

#define MAX_NUM_OF_DATA_STREAMS       3
#define MAX_SIZE_OF_SM_STRING       128
#define MAX_SIZE_OF_SM_INFO         128
#define MAX_NUM_OF_SM                32

#define ERR_INVALID_SM_ID           240
#define ERR_SM_ALREADY_REGISTERED   241
#define ERR_SM_CREATE_FAILED        242
#define ERR_SM_CLOSE_FAILED         243
#define ERR_SM_WRITE_NO_SPACE       244
#define ERR_SM_WRITE_IO_ERROR       245
#define ERR_SM_READ_IO_ERROR        246
#define ERR_SM_OPEN_FAILED          247
#define ERR_SM_DELETE_FAILED        248

typedef struct
{
   nuint32 IOStatus;
   nuint32 InfoBlockSize;
   nuint32 AvailSpace;
   nuint32 UsedSpace;
   /* A length preceded string is followed by SMInfo data */
   nuint8 SMInfo[MAX_SIZE_OF_SM_STRING + MAX_SIZE_OF_SM_INFO];
}  SUPPORT_MODULE_INFO;

typedef struct
{
   nuint32 numberOfSMs;
   nuint32 SMIDs[MAX_NUM_OF_SM];
}  SUPPORT_MODULE_IDS;

NWCCODE N_API NWMoveFileToDM
(
   NWCONN_HANDLE   conn,
   NWDIR_HANDLE    dirHandle,
   pnstr8          path,
   nuint8          nameSpace,
   nuint32         supportModuleID,
   nuint32         saveKeyFlag
);

NWCCODE N_API NWMoveFileFromDM
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint8         nameSpace
);

NWCCODE N_API NWGetDMFileInfo
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint8         nameSpace,
   pnuint32       supportModuleID,
   pnuint32       restoreTime,
   pnuint32       dataStreams
);

NWCCODE N_API NWGetDMVolumeInfo
(
   NWCONN_HANDLE  conn,
   nuint16        volume,
   nuint32        supportModuleID,
   pnuint32       numberOfFilesMigrated,
   pnuint32       totalMigratedSize,
   pnuint32       spaceUsedOnDM,
   pnuint32       limboSpaceUsedOnDM,
   pnuint32       spaceMigrated,
   pnuint32       filesInLimbo
);

NWCCODE N_API NWGetSupportModuleInfo
(
   NWCONN_HANDLE  conn,
   nuint32        infomationLevel,
   nuint32        supportModuleID,
   pnuint8        returnInfo,
   pnuint32       returnInfoLen
);

NWCCODE N_API NWGetDataMigratorInfo
(
   NWCONN_HANDLE  conn,
   pnuint32       DMPresentFlag,
   pnuint32       majorVersion,
   pnuint32       minorVersion,
   pnuint32       DMSMRegistered
);

NWCCODE N_API NWGetDefaultSupportModule
(
   NWCONN_HANDLE  conn,
   pnuint32       supportModuleID
);

NWCCODE N_API NWSetDefaultSupportModule
(
   NWCONN_HANDLE  conn,
   pnuint32       supportModuleID
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwmigrat.h,v 1.6 1994/06/08 23:33:10 rebekah Exp $
*/
