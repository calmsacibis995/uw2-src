/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpafp.h	1.5"
#ifndef NCPAFP_H
#define NCPAFP_H

#ifndef NTYPES_H
#ifdef N_PLAT_UNIX
#include <nw/ntypes.h>
#else /* !N_PLAT_UNIX */
#include "ntypes.h"
#endif /* N_PLAT_UNIX */
#endif

#ifndef NWACCESS_H
#ifdef N_PLAT_UNIX
#include <nw/nwaccess.h>
#else /* !N_PLAT_UNIX */
#include "nwaccess.h"
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

/* APPLE */

typedef struct NWNCPAFPFileInfo
{
   nuint32  luEntryID;
   nuint32  luParentID;
   nuint16  suAttr;
   nuint32  luDataForkLen;
   nuint32  luResourceForkLen;
   nuint16  suTotalOffspring;
   nuint16  suCreationDate;
   nuint16  suAccessDate;
   nuint16  suModifyDate;
   nuint16  suModifyTime;
   nuint16  suBackupDate;
   nuint16  suBackupTime;
   nuint8   abuFinderInfo[32];
   nuint8   abuLongName[34];
   nuint32  luOwnerID;
   nuint8   abuShortName[14];
   nuint16  suAccessPrivileges;
   nuint8   abuProDOSInfo[6];
} NWNCPAFPFileInfo, N_FAR * pNWNCPAFPFileInfo;

#define AFP_FILE_INFO_LEN   120
#define FINDER_INFO_LEN       ((nuint) 32)
#define PRODOS_INFO_LEN       ((nuint) 6)

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( void )
NWCUnpackAFPPacket
(
   pNWNCPAFPFileInfo pMacFileInfo,
   pnuint8         pbuRecPackedB120
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s13AFPCreateDir
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint8   buReserved,
   pnuint8  pbuFinderInfoB32,
   pnuint8  pbuProDOSInfoB6,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint32 pluNewAFPEntryID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s14AFPCreateFile
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint8   buDelFileFlag,
   pnuint8  pbuFinderInfoB32,
   pnuint8  pbuProDOSInfoB6,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint32 pluNewAFPEntryID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s17AFPScanFileInfo
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPBaseID,
   nuint32  luLastSeenAFPEntryID,
   nuint16  suDesiredResponseCnt,
   nuint16  suSearchBitMap,
   nuint16  suReqBitMap,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint16 psuResponseCnt,
   pNWNCPAFPFileInfo pMacFileInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s15AFPGetDirEntryInfo
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint16  suReqBitMap,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pNWNCPAFPFileInfo pMacFileInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s16AFPSetFileInfo
(
   pNWAccess       pAccess,
   nuint8         buVolNum,
   nuint16        suReqBitMap,
   nuint8         buPathLen,
   pnstr8         pbstrPath,
   pNWNCPAFPFileInfo pMacFileInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s11AFPAllocTempDirHan
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuNWDirHandle,
   pnuint8  pbuAccessRights
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s1AFPCreateDir
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   pnuint8  pbuFinderInfoB32,
   nuint8   buReserved,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint32 pluNewAFPEntryID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s2AFPCreateFile
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint8   buDelExisting,
   pnuint8  pbuFinderInfoB32,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint32 pluAFPEntryID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s3AFPDelete
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint8   buPathLen,
   pnstr8   pbstrPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s18AFPGetDOSNameEntryID
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   pnuint8  pbuDOSPathLen,
   pnstr8   pbstrDOSPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s6AFPGEntryIDFrmNWHan
(
   pNWAccess pAccess,
   pnuint8  pbuNWDirHandleB6,
   pnuint8  pbuVolNum,
   pnuint32 pluAFPEntryID,
   pnuint8  pbuResFork
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s4AFPGetEntryIDFromName
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint32 pluAFPEntryID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s12AFPGEntryIDFrmPathNm
(
   pNWAccess pAccess,
   nuint8   buNWDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint32 pluAFPEntryID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s5AFPGetFileInfo
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint16  suReqBitMap,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pNWNCPAFPFileInfo pMacFileInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s19AFPGMacInfoDelFile
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   pnuint8  pbuFinderInfoB32,
   pnuint8  pbuProDOSInfoB6,
   pnuint32 pluResFork,
   pnuint8  pbuFileNameLen,
   pnstr8   pbstrFileName
);


N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s8AFPOpenFileFork
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint8   buResFork,
   nuint8   buAccessMode,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint32 pluAFPEntryID,
   pnuint32 pluForkLen,
   pnuint8  pbuNWDirHandleB6
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s7AFPRename
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   nuint32  luDestAFPEntryID,
   nuint8   buSourcePathLen,
   pnstr8   pbstrSourcePath,
   nuint8   buDestPathLen,
   pnstr8   pbstrDestPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s10AFPScanFileInfo
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luCurrIterHnd,
   nuint32  luLastIterHnd,
   nuint16  suDesiredRespCnt,
   nuint16  suSearchBitMap,
   nuint16  suReqBitMap,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint16 psuActualRespCnt,
   pNWNCPAFPFileInfo pMacFileInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP35s9AFPSetFileInfo
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPEntryID,
   pnuint16 psuReqBitMap,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pNWNCPAFPFileInfo pMacFileInfo
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpafp.h,v 1.7 1994/09/26 17:11:06 rebekah Exp $
*/

