/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpds.h	1.5"
#if ! defined( NCPDS_H )
#define NCPDS_H

#define NAlign32(s)  *(s) = (void N_FAR *) (((nuint32)*(s) + 3) & ~3);
#define NPad32(s)    ((s + 3) & ~3)

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

typedef struct  NWTIMESTAMP
{
   nuint32  luSeconds;
   nuint16  suReplicaNum;
   nuint16  suEventID;
} NWTimeStamp, N_FAR * pNWTimeStamp;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

/* NWCLIENT DS Declare */

N_EXTERN_LIBRARY(NWRCODE)
NWCNCPDS2v2ReadObjectInfo
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luEntryID,
   nuint32  luInfoFlags,
   pnuint32 pluEntryFlags,
   pnuint32 pluSubordinateCount,
   pnuint32 pluModificationTime,
   pnstr16  pwstrBaseClass,
   pnstr16  pwstrEntryName
);

N_EXTERN_LIBRARY(NWRCODE)
NWCNCPDS1v0ResolveName
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luScopeOfReferral,
   nuint32  luTargetEntryNameLen,
   pnstr16  pwstrTargetEntryName,
   nuint32  luTransportTypeCnt,
   pnuint32 pluTransportType,
   nuint32  luTreewalkerTypeCnt,
   pnuint32 pluTreewalkerType,
   pnuint32 pluResolveInfoLen,
   pnuint8  pResolveInfo
);

N_EXTERN_LIBRARY(NWRCODE)
NWCNCPDS59v0BeginAuthen
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   nuint32  luClientRandom,
   pnuint32 pluServerRandom,
   pnuint32 pluEncryptedClientRandomLen,
   pnuint8  pEncryptedClientRandom
);

N_EXTERN_LIBRARY(NWRCODE)
NWCNCPDS60v0FinishAuthen
(
   pNWAccess pNetAccess,
   nuint32  luEncryptedSessionKeyLen,
   pnuint8  pEncryptedSessionKey,
   nuint32  luCredentialLen,
   pnuint8  pCredential,
   nuint32  luProofLen,
   pnuint8  pProof
);

N_EXTERN_LIBRARY(NWRCODE)
NWCNCPDS61Logout
(
   pNWAccess pNetAccess
);

N_EXTERN_LIBRARY(NWRCODE)
NWCNCPDS3v1Read
(
   pNWAccess pNetAccess,
   nuint32  luProtocolFlags,
   pnuint32 pluIterationHandle,
   nuint32  luEntryID,
   nuint32  luInfoType,
   nuint32  blAllAttributes,
   nuint32  luAttrNamesLen,
   pnuint8  pAttrNames,
   nuint32  luSubjectLen,
   pnuint16 pSubjectName,
   pnuint32 pluEntryInfoLen,
   pnuint8  pEntryInfo
);

N_GLOBAL_LIBRARY(NWRCODE)
NWNCPDS3v2Read
(
   pNWAccess pNetAccess,
   nuint32  luProtocolFlags,
   pnuint32 pluIterationHandle,
   nuint32  luEntryID,
   nuint32  luInfoType,
   nuint32  blAllAttributes,
   nuint32  luAttrNamesLen,
   pnuint8  pAttrNames,
   nuint32  luSubjectLen,
   pnuint16 pSubjectName,
   nuint32  luWholeSeconds,
   nuint16  suReplicaNum,
   nuint16  suEventID,
   pnuint32 pluEntryInfoLen,
   pnuint8  pEntryInfo
);

N_EXTERN_LIBRARY(NWRCODE)
NWCNCPDS53v0GetServerAddr
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   pnuint32 pluServerInfoLen,
   pnuint8  pbuServerInfo
);

N_EXTERN_LIBRARY(NWRCODE)
NWCNCPDS58v2FinishLogin
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luEntryID,
   nuint32  luEncryptedDataLen,
   pnuint8  pEncryptedData,
   pnuint32 pBeginLoginTime,
   pnuint32 pLogoutTime,
   pnuint32 pluEncryptedResponseLen,
   pnuint8  pEncryptedResponse
);

N_EXTERN_LIBRARY(NWRCODE)
NWCNCPDS57v0BeginLogin
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   pnuint32 pluPseudoID,
   pnuint32 pluServerRandomSeed
);

/* Directory Services */
N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS7v0AddObject
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luParentID,
   nuint32  luRDNLen,
   pnstr16  pwstrRDN,
   nuint32  luAddObjectLen,
   pnuint8  pAddObjectData
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS20v0AddPartition
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luIterationHandle,
   nuint32  luParentID,
   nuint32  luPartitionRDNLen,
   pnstr16  pwstrPartitionRDN,
   nuint32  luTargetServerDNLen,
   pnstr16  pwstrTargetServerDN,
   nuint32  luAddPartitionLen,
   pnuint8  pAddPartitionData
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS25v0AddReplica
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luPartitionRootID,
   nuint32  luReplicaType,
   nuint32  luTargetServerNameLen,
   pnstr16  pwstrTargetServerName
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS45v0BackupObject
(
   pNWAccess pNetAccess,
   pnuint32 pluIterationHandle,
   nuint32  luEntryID,
   pnuint32 pluBackupInfoLen,
   pnuint8  pBackupInfo
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS42v0BeginMoveEntry
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luDestinationParentID,
   nuint32  luNewRDNLen,
   pnstr16  pwstrNewRDN,
   nuint32  luSourceServerDNLen,
   pnstr16  pwstrSourceServerDN
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS31v0ChangeReplicaType
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luPartitionRootID,
   nuint32  luReplicaType,
   nuint32  luTargetServerNameLen,
   pnstr16  pwstrTargetServerName
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS50v0CloseIteration
(
   pNWAccess pNetAccess,
   nuint32  luIterationHandle,
   nuint32  luIterationVerb
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS4v0Compare
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   nuint32  luCompDataLen,
   pnuint8  pCompData,
   pnbool8  pMatched
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS11v0DefineAttr
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luAttrNameLen,
   pnstr16  pwstrAttrName,
   nuint32  luSyntaxID,
   nuint32  luAttrLower,
   nuint32  luAttrUpper,
   nuint32  luASN1IDLen,
   pnuint8  pbaASN1ID
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS14v0DefineClass
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luClassNameLen,
   pnstr16  pwstrClassName,
   nuint32  luASN1IDLen,
   pnuint8  pbaASN1ID,
   nuint32  luClassDataLen,
   pnuint8  pbuClassData
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS43v0FinishMoveEntry
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luSourceID,
   nuint32  luDestinationParentID,
   nuint32  luNewRDNLen,
   pnstr16  pwstrNewRDN,
   nuint32  luDestinationServerDNLen,
   pnstr16  pwstrDestinationServerDN
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS19v0GetEffRights
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   nuint32  luSubjectDNLen,
   pnstr16  pwstrSubjectDN,
   nuint32  luAttrNameLen,
   pnstr16  pwstrAttrName,
   pnuint32 pluRights
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS41v0GetReplicaRootID
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   pnuint32 pluPartitionID
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS53v0GetServerAddr
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   pnuint32 pluServerInfoLen,
   pnuint8  pbuServerInfo
);

N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS80v0InspectEntry
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luEntryID,
   pnuint32 pluErrInfoLen,
   pnuint8  pErrInfo
);

N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS24v0JoinPartitions
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luEntryID
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS5v1List
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   pnuint32 pluIterationHandle,
   nuint32  luParentID,
   nuint32  luInfoFlags,
   nuint32  luNameFilterLen,
   pnstr16  pwstrNameFilter,
   nuint32  luClassFilterLen,
   pnstr16  pwstrClassFilter,
   pnuint32 pluListEntryInfoLen,
   pnuint8  pListEntryInfo
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS18v0ListContClasses
(
   pNWAccess pNetAccess,
   pnuint32 pluIterationHandle,
   nuint32  luParentEntryID,
   pnuint32 pluContainableClassesLen,
   pnuint8  pContainableClasses
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS22v0ListPartitions
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   pnuint32 pluIterationHandle,
   pnuint32 pluPartitionInfoLen,
   pnstr8   pPartitionInfo
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS16v0ModifyClassDef
(
   pNWAccess pNetAccess,
   nuint32  luClassNameLen,
   pnstr16  pwstrClassName,
   nuint32  luOptionalAttrsLen,
   pnuint8  pOptionalAttrs
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS9v0ModifyObject
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luEntryID,
   nuint32  luAttrModLen,
   pnuint8  pAttrModData
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS10v0ModifyRDN
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   nuint32  blDelOldRDN,
   nuint32  luNewRDNLen,
   pnstr16  pwstrNewRDN
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS27v0OpenStream
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luEntryID,
   nuint32  luAttrNameLen,
   pnstr16  pwstrAttrName,
   pnuint32 pluFileID,
   pnuint32 pluFileSize
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS3v1Read
(
   pNWAccess pNetAccess,
   nuint32  luProtocolFlags,
   pnuint32 pluIterationHandle,
   nuint32  luEntryID,
   nuint32  luInfoType,
   nuint32  blAllAttributes,
   nuint32  luAttrNamesLen,
   pnuint8  pAttrNames,
   nuint32  luSubjectLen,
   pnuint16 pSubjectName,
   pnuint32 pluEntryInfoLen,
   pnuint8  pEntryInfo
);

N_EXTERN_LIBRARY(NWRCODE)
NWCNCPDS3v1Read
(
   pNWAccess pNetAccess,
   nuint32  luProtocolFlags,
   pnuint32 pluIterationHandle,
   nuint32  luEntryID,
   nuint32  luInfoType,
   nuint32  blAllAttributes,
   nuint32  luAttrNamesLen,
   pnuint8  pAttrNames,
   nuint32  luSubjectLen,
   pnuint16 pSubjectName,
   pnuint32 pluEntryInfoLen,
   pnuint8  pEntryInfo
);

N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS12v0ReadAttrDef
(
   pNWAccess pNetAccess,
   pnuint32 pluIterationHandle,
   nuint32  luInfoType,
   nuint32  blAllAttributes,
   nuint32  luAttrNamesLen,
   pnuint8  pAttrNames,
   pnuint32 pluAttrDefInfoLen,
   pnuint8  pAttrDefInfo
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS15v0ReadClassDef
(
   pNWAccess pNetAccess,
   pnuint32 pluIterationHandle,
   nuint32  luInfoType,
   nuint32  blAllClasses,
   nuint32  luClassNamesLen,
   pnuint8  pClassNames,
   pnuint32 pluClassInfoLen,
   pnuint8  pClassInfo
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS2v2ReadObjectInfo
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luEntryID,
   nuint32  luInfoFlags,
   pnuint32 pluEntryFlags,
   pnuint32 pluSubordinateCount,
   pnuint32 pluModificationTime,
   pnstr16  pwstrBaseClass,
   pnstr16  pwstrEntryName
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS79v0ReadReferences
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   pnuint32 pluIterHandle,
   nuint32  luEntryID,
   nuint32  luInfoType,
   nuint32  luInfoFlags,
   nuint32  luTimeFilter,
   nuint32  blAllAttributes,
   nuint32  luAttrNamesLen,
   pnuint8  pAttrNames,
   pnuint32 pluObjInfoLen,
   pnuint8  pObjInfo
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS13v0RemoveAttrDef
(
   pNWAccess pNetAccess,
   nuint32  luAttrNameLen,
   pnstr16  pwstrAttrName
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS17v0RemoveClassDef
(
   pNWAccess pNetAccess,
   nuint32  luClassNameLen,
   pnstr16  pwstrClassName
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS8v0RemoveObject
(
   pNWAccess pNetAccess,
   nuint32  luEntryID
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS21v0RemovePartition
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luPartitionRootID
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS26v0RemoveReplica
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luPartitionRootID,
   nuint32  luTargetServerDNLen,
   pnstr16  pwstrTargetServerDN
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS63v0RepairTimeStamp
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luPartitionRootID
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS1v0ResolveName
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luScopeOfReferral,
   nuint32  luTargetEntryNameLen,
   pnstr16  pwstrTargetEntryName,
   nuint32  luTransportTypeCnt,
   pnuint32 pluTransportType,
   nuint32  luTreewalkerTypeCnt,
   pnuint32 pluTreewalkerType,
   pnuint32 pluResolveInfoLen,
   pnuint8  pResolveInfo
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS46v0RestoreObject
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   pnuint32 pluIterationHandle,
   nuint32  luParentID,
   nuint32  luRDNLen,
   pnstr16  pwstrRDN,
   nuint32  luEntryInfoLen,
   pnuint8  pEntryInfo,
   nuint32  luDNLen,
   pnuint8  pDN
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS6v3Search
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   pnuint32 pluIterationHandle,
   nuint32  luBaseEntryID,
   nuint32  luScope,
   nuint32  luNodesToSearch,
   nuint32  luInfoType,
   nuint32  luInfoFlags,
   nuint32  blAllAttributes,
   nuint32  luAttrNamesLen,
   pnuint8  pAttrNames,
   nuint32  luFilterLen,
   pnuint8  pFilter,
   pnuint32 pluNumberOfNodesSearched,
   pnuint32 pluSearchInfoLen,
   pnuint8  pSearchInfo
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS6v4Search
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   pnuint32 pluIterationHandle,
   nuint32  luBaseEntryID,
   nuint32  luScope,
   nuint32  luNodesToSearch,
   nuint32  luInfoType,
   nuint32  luInfoFlags,
   nuint32  luTimeSeconds,
   nuint16  suTimeReplica,
   nuint16  suTimeEvent,
   nuint32  blAllAttributes,
   nuint32  luAttrNamesLen,
   pnuint8  pAttrNames,
   nuint32  luFilterLen,
   pnuint8  pFilter,
   pnuint32 pluNumberOfNodesSearched,
   pnuint32 pluSearchInfoLen,
   pnuint8  pSearchInfo
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS23v0SplitPartition
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luNewPartitionRootID
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS38v1SyncPartition
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luSeconds,
   nuint32  luPartitionDNLen,
   pnstr16  pwstrPartitionDN
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS39v0SyncSchema
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luSeconds
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS76v3AbortPartOperation
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luPartitionRootID
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS78v0s1PartFuncRcvUpd
(
   pNWAccess pNetAccess,
   nuint32  luReplicaRootID,
   nuint32  luTargetServerID
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS78v0s2PartFuncSndUpd
(
   pNWAccess pNetAccess,
   nuint32  luReplicaRootID
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS57v0BeginLogin
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   pnuint32 pluPseudoID,
   pnuint32 pluServerRandomSeed
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS58v2FinishLogin
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luEntryID,
   nuint32  luEncryptedDataLen,
   pnuint8  pEncryptedData,
   pnuint32 pBeginLoginTime,
   pnuint32 pLogoutTime,
   pnuint32 pluEncryptedResponseLen,
   pnuint8  pEncryptedResponse
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS59v0BeginAuthen
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   nuint32  luClientRandom,
   pnuint32 pluServerRandom,
   pnuint32 pluEncryptedClientRandomLen,
   pnuint8  pEncryptedClientRandom
);

N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS60v0FinishAuthen
(
   pNWAccess pNetAccess,
   nuint32  luEncryptedSessionKeyLen,
   pnuint8  pEncryptedSessionKey,
   nuint32  luCredentialLen,
   pnuint8  pCredential,
   nuint32  luProofLen,
   pnuint8  pProof
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS55v0ChangePassword
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   nuint32  luEncryptedRequestLen,
   pnuint8  pEncryptedRequest
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS61Logout
(
   pNWAccess pNetAccess
);

N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS54v0SetKeys
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   nuint32  luEncryptedRequestLen,
   pnuint8  pEncryptedRequest
);


N_EXTERN_LIBRARY(NWRCODE)
NWNCPDS56v1VerifyPassword
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   nuint32  luEncryptedRequestLen,
   pnuint8  pEncryptedRequest
);


N_EXTERN_LIBRARY( NWRCODE )
NWNCP114s1GetUTCTime
(
   pNWAccess pAccess,
   pnuint32 pluSeconds,
   pnuint32 pluSubSeconds,
   pnuint32 pluStatus,
   pnuint32 pluEventOffset1,
   pnuint32 pluEventOffset2,
   pnuint32 pluAdjustmentTime,
   pnuint32 pluEventType
);

N_EXTERN_LIBRARY(NWRCODE)
NWNCP104s1PingServer
(
   pNWAccess pAccess,
   pnuint   psuReplySize,
   pnuint8  pbuReplyData
);

N_EXTERN_LIBRARY(NWRCODE)
NWNCP104s4GetBindContext
(
   pNWAccess pAccess,
   nuint    suReplyDataLen,
   pnuint8  pbuReplyData
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpds.h,v 1.7 1994/09/26 17:11:13 rebekah Exp $
*/
