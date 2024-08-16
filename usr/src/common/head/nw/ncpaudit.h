/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpaudit.h	1.4"
#ifndef NCPAUDIT_H
#define NCPAUDIT_H

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

typedef struct tagNWNCPAuditConfigFileHdr
{
   nuint16  suAuditFileVerDate;
   nuint8   buAuditFlag;
   nuint8   buErrMsgDelayInMinsFactor;
   nuint8   abuEncryptedPass[16];
   nuint32  luVolAuditFileMaxSize;
   nuint32  luVolAuditFileSizeThresh;
   nuint32  luAuditRecCount;
   nuint32  luHistRecCount;
   nuint32  aluReservedB7[7];
   nuint8   abuMapB64[64];
} NWNCPAuditConfigFileHdr, N_FAR *pNWNCPAuditConfigFileHdr;

typedef struct tagNWNCPTimeStamp
{
   nuint32 luSeconds;
   nuint16 suReplicaNum;
   nuint16 suEvent;
} NWNCPTimeStamp, N_FAR *pNWNCPTimeStamp;

typedef struct tagNWNCPContAuditConfigFileHdr
{
   nuint16   suAuditFileVerDate;
   nuint8    buAuditFlag;
   nuint8    buErrMsgDelayInMins;
   nuint32   luContainerID;
   nuint32   luSpareLong;
   NWNCPTimeStamp creationTS;
   nuint32   luBitMap;
   nuint32   luContAuditFileMaxSize;
   nuint32   luContAuditFileSizeThresh;
   nuint32   luAuditRecCount;
   nuint16   suReplicaNum;
   nuint8    buEnabledFlag;
   nuint8    abuSpareBytes[3];
   nuint16   suNumReplicaEntries;
   nuint32   aluSpareLongs[9];
   nuint32   luAuditDisabledCounter;
   nuint32   luAuditEnabledCounter;
   nuint8    abuCryptPassword1[16];
   nuint8    abuCryptPassword2[16];
   nuint32   luHdrModifiedCounter;
   nuint32   luFileResetCounter;
} NWNCPContAuditConfigFileHdr, N_FAR *pNWNCPContAuditConfigFileHdr;

typedef struct tagNWNCPVolAuditStatus
{
   nuint16  suAuditVerDate;
   nuint16  suAuditFileVerDate;
   nuint32  luAuditingEnabled;
   nuint32  luVolAuditSize;
   nuint32  luVolAuditFileCfgSize;
   nuint32  luVolAuditFileMaxSize;
   nuint32  luVolAuditFileSizeThresh;
   nuint32  luAuditRecCount;
   nuint32  luHistoryRecCount;
} NWNCPVolAuditStatus, N_FAR *pNWNCPVolAuditStatus;

typedef struct tagNWNCPContAuditStatus
{
   nuint16  suAuditVerDate;
   nuint16  suAuditFileVerDate;
   nuint32  luAuditingEnabled;
   nuint32  luContAuditSize;
   nuint32  luContAuditFileCfgSize;
   nuint32  luContAuditFileMaxSize;
   nuint32  luContAuditFileSizeThresh;
   nuint32  luAuditRecCount;
   nuint32  luHistoryRecCount;
} NWNCPContAuditStatus, N_FAR *pNWNCPContAuditStatus;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s1GetVolAuditStatus
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   pNWNCPVolAuditStatus pStatus
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s2AddUserAuditProp
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8,
   nuint32  luUserID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s3AddAuditorAccess
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s4ChgAuditorVolPass
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuOldKeyB8,
   pnuint8  pbuNewPassB16
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s5CheckAuditorAccess
(
   pNWAccess pAccess,
   nuint32  luVolNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s6DelUserAuditProp
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8,
   nuint32  luUserID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s7DisableVolAuditing
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s8EnableVolAuditing
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuPasswordB16
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s9IsUserAudited
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint32  luUserID,
   pnuint32 pluUserAuditFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s10ReadAuditBitMap
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   pnuint8  pbuMapB64
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s11ReadAuditFileCfgHdr
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   pNWNCPAuditConfigFileHdr pHeader
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s12ReadAuditingFiles
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditFileNum,
   nuint16  suAmtToRead,
   nuint32  luReadOffset,
   pnuint32 pluAmtRead,
   pnuint8  pbuBufferB512
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s13RemVolAuditAccess
(
   pNWAccess pAccess,
   nuint32  luVolNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s14ResetAuditFile
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s15ResetAuditHistFile
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s16WriteAuditBitMap
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8,
   pnuint8  pbuMapB64
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s17WriteAuditFilCfgHdr
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8,
   pNWNCPAuditConfigFileHdr pConfigHdr
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s18ChgAuditPass
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8,
   pnuint8  pbuCryptPassB16
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s19GetAuditFlags
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   pnuint8  pbuAuditFlags
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s20CloseOldAuditFile
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s21DelOldAuditFile
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP88s22ChkLvlTwoAccess
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8
);

/********************************** this is the start of the new  *****/
N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s200GetContAuditStatus
(
   pNWAccess pAccess,
   nuint32  luContID,
   pNWNCPContAuditStatus pStatus
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s202AddAuditorAccess
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s203ChgAuditorContPass
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint16  suAuditVerDate,
   pnuint8  pbuOldKeyB8,
   pnuint8  pbuNewPassB16
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s204CheckAuditorAccess
(
   pNWAccess pAccess,
   nuint32  luContID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s206DisableContAuditing
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s207EnableContAuditing
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint16  suAuditVerDate,
   pnuint8  pbuPasswordB16
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s209ReadAuditFileCfgHdr
(
   pNWAccess pAccess,
   nuint32  luContID,
   pNWNCPContAuditConfigFileHdr pHeader
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s210ReadAuditingFiles
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint16  suAuditFileNum,
   nuint16  suAmtToRead,
   nuint32  luReadOffset,
   pnuint32 pluAmtRead,
   pnuint8  pbuBufferB508
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s211RemContAuditAccess
(
   pNWAccess pAccess,
   nuint32  luContID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s212ResetAuditFile
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s213ResetAuditHistFile
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s214WriteAuditFilCfgHdr
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8,
   pNWNCPContAuditConfigFileHdr pConfigHdr
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s215ChgAuditPass
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8,
   pnuint8  pbuCryptPassB16
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s216GetAuditFlags
(
   pNWAccess pAccess,
   nuint32  luContID,
   pnuint8  pbuAuditFlags
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s217CloseOldAuditFile
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s218DelOldAuditFile
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s219ChkLvlTwoAccess
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint16  suAuditVerDate,
   pnuint8  pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s220IsObjectAudited
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint32  luObjID,
   pnuint32 pluContAuditFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP104s221ChgObjAuditStatus
(
   pNWAccess pAccess,
   nuint32  luContID,
   nuint32  luObjID,
   nuint32  luContAuditFlag
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpaudit.h,v 1.6 1994/09/26 17:11:08 rebekah Exp $
*/
