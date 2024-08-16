/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpbind.h	1.4"
#if !defined( NCPBIND_H )
#define NCPBIND_H

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
NWNCP23s0LoginUser
(
   pNWAccess pAccess,
   nuint8   buUserNameLen,
   pnstr8   pbstrUserName,
   nuint8   buPasswordLen,
   pnstr8   pbstrPassword
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s1ChangeUserObjPwd
(
   pNWAccess pAccess,
   nuint8   buUserObjNameLen,
   pnstr8   pbstrUserObjName,
   nuint8   buOldPwdLen,
   pnstr8   pbstrOldPwd,
   nuint8   buNewPwdLen,
   pnstr8   pbstrNewPwd
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s3GetUserObjID
(
   pNWAccess pAccess,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   pnuint32 pluObjID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s4GetUserObjName
(
   pNWAccess pAccess,
   nuint32  luObjID,
   pnuint32 pluRetObjID,
   pnstr8   pbstrObjNameB16
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s7GetGroupObjID
(
   pNWAccess pAccess,
   nuint8   buGroupNameLen,
   pnstr8   pbstrGroupName,
   pnuint32 pluGroupObjID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s8GetGroupName
(
   pNWAccess pAccess,
   nuint32  luGroupObjID,
   pnuint32 pluRetGroupObjID,
   pnuint8  pbuGroupNameB16
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s9GetMemberSetOfGroup
(
   pNWAccess pAccess,
   nuint32  luObjID,
   nuint16  suSetNum,
   pnuint32 pluObjIDB50
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s20LoginObject
(
   pNWAccess pAccess,
   nuint16  suObjectType,
   nuint8   buClientNameLen,
   pnstr8   pbstrClientName,
   nuint8   buPasswordLen,
   pnstr8   pbstrPassword
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s23GetLoginKey
(
   pNWAccess pAccess,
   pnuint8  pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s24KeyedObjLogin
(
   pNWAccess pAccess,
   pnuint8  pbuKeyB8,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s50CreateObj
(
   pNWAccess pAccess,
   nuint8   buStatusFlags,
   nuint8   buSecurityLevel,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s51DeleteObj
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s52RenameObj
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buNewObjNameLen,
   pnstr8   pbstrNewObjName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s53GetObjID
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   pnuint32 pluObjID,
   pnuint16 psuRetObjType,
   pnstr8   pbstrRetObjNameB48
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s54GetObjName
(
   pNWAccess pAccess,
   nuint32  luObjID,
   pnuint32 pluRetObjID,
   pnuint16 psuObjType,
   pnstr8   pbstrObjName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s55ScanObj
(
   pNWAccess pAccess,
   nuint16  suSrchObjType,
   nuint8   buSrchObjNameLen,
   pnstr8   pbstrSrchObjName,
   pnuint32 pluObjID,
   pnuint16 psuObjType,
   pnstr8   pbstrObjName,
   pnuint8  pbuObjFlags,
   pnuint8  pbuObjSecurity,
   pnuint8  pbuObjHasProperties
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s56ChangeObjSecurity
(
   pNWAccess pAccess,
   nuint8   buNewSecurityLevel,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   pnuint8  pbuConnStatus
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s57CreateProperty
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buPropertyFlags,
   nuint8   buPropertySecurity,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s58DeleteProperty
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s59ChgPropertySecurity
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buNewPropertySecurity,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName,
   pnuint8  pbuConnStatus
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s60ScanProperty
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   pnuint32 pluIterHnd,
   nuint8   buSrchPropertyNameLen,
   pnstr8   pbstrSrchPropertyName,
   pnstr8   pbstrPropertyName,
   pnuint8  pbuPropertyFlags,
   pnuint8  pbuPropertySecurity,
   pnuint8  pbuValueAvailable,
   pnuint8  pbuMoreFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s61ReadPropertyValue
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buSegNum,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName,
   pnuint8  pbuPropertyValueB128,
   pnuint8  pbuMoreFlag,
   pnuint8  pbuPropertyFlags
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s62WritePropertyValue
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buSegmentNum,
   nuint8   buMoreFlag,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName,
   pnuint8  pbuPropertyValueB128
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s63VerifyObjPwd
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buPwdLen,
   pnstr8   pbstrPwd
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s64ChangeObjPwd
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buOldPwdLen,
   pnstr8   pbstrOldPwd,
   nuint8   buNewPwdLen,
   pnstr8   pbstrNewPwd
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s65AddObjToSet
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName,
   nuint16  suMemberType,
   nuint8   buMemberNameLen,
   pnstr8   pbstrMemberName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s66DeleteObjFromSet
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName,
   nuint16  suMemberType,
   nuint8   buMemberNameLen,
   pnstr8   pbstrMemberName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s67IsObjInSet
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName,
   nuint16  suMemberType,
   nuint8   buMemberNameLen,
   pnstr8   pbstrMemberName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s68CloseBindery
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s69OpenBindery
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s70GetClntAccessMask
(
   pNWAccess pAccess,
   pnuint8  pbuSecurityLevel,
   pnuint32 pluObjID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s71ScanObjTrustPaths
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   pnuint16 psuIterHnd,
   nuint32  luObjID,
   pnuint32 pluRetObjID,
   pnuint8  pbuTrusAccessMask,
   pnuint8  pbuPathNameLen,
   pnstr8   pbstrPathName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s72GetObjAccessLevel
(
   pNWAccess pAccess,
   nuint32  luObjID,
   pnuint8  pbuAccessLevel
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s73IsObjManager
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s74KeyedVerifyPwd
(
   pNWAccess pAccess,
   pnuint8  pbuKeyB8,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s75KeyedChangePwd
(
   pNWAccess pAccess,
   pnuint8  pbuKeyB8,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buNewPwdLen,
   pnstr8   pbstrNewPwd
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s76ScanRelationsOfObj
(
   pNWAccess pAccess,
   nuint32  luLastSeen,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName,
   pnuint16 psuReturnCount,
   pnuint32 pluRelationsB32
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s50GetObjEffectRights
(
   pNWAccess pAccess,
   nuint32  luObjID,
   nuint8   buDirHand,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint16 psuAccessRights
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s230GetObjFreeDiskSpace
(
   pNWAccess pAccess,
   nuint32  luObjID,
   pnuint32 pluSysIntervalMarker,
   pnuint32 pluRetObjID,
   pnuint32 pluUnusedDiskBlocks,
   pnuint8  pbuRestrictionsEnforced
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpbind.h,v 1.6 1994/09/26 17:11:10 rebekah Exp $
*/
