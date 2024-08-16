/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/nwauditc.h	1.4"
 /**************************************************************************
 *
 * (C) Unpublished Copyright Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized or
 * modified in any manner or compiled, linked or uploaded or downloaded to or
 * from any computer system without the prior written consent of Novell, Inc.
 *
 *  $release$
 *  $modname: NWAUDITC.h$
 *  $version: 1.30$
 *  $date: Wed, Sep 4, 1991$
 *
 ************************************************************************** */

 /*
   FILE NWAUDITC.H
   System Auditing Library Calls Definitions
 */

#ifndef NWAUDITC_H
#define NWAUDITC_H

#ifndef NTYPES_H
#include "ntypes.h"
#endif

#ifndef NWCALDEF_H
#include "nwcaldef.h"
#endif

#include <npackon.h>

/*
   The header file NWAUDITH.H contains all the necessary structures and
definitions for using the interfaces.


   All API's except GetAuditCryptPassword and GetVolumeAuditStatus require
that the user has previously logged in as the Auditor for the specified volume,
using the NWLoginVolumeAuditor API. It is the users responsibility to call the
NWLogoutVolumeAuditor API, when the application terminates.

   volumeNumber is zero for all calls related to server, or the volumeNumber
of the volume containing the files if file related.

*/

typedef struct
{
   nuint32  volumeNumber;
   nuint16  connectionNumber;
   nuint8   auditFlags;
   nuint8   spare;
   nuint8   auditCryptPassword[16];
   nuint8   auditCryptPassword2[16];
}  AuditRequestPacket;
#define ARP AuditRequestPacket

typedef struct    /* this structure must be same as ARP above for */
{                 /* NWReadAuditFileRecord() */
   nuint32  containerID;
   nuint16  connectionNumber;
   nuint8   auditFlags;
   nuint8   spare;
   nuint8   auditCryptPassword[16];
   nuint8   auditCryptPassword2[16];
}  DSAuditRequestPacket;
#define DS_ARP DSAuditRequestPacket



NWCCODE NWGetAuditCryptPassword
(
   pnstr8   pbstrClearTextPassword,
   pnuint8  pbuAuditCryptPassword
);

NWCCODE NWGetVolumeAuditStatus
(
   NWCONN_HANDLE connectionNumber,
   nuint32  luVolNum,
   pnuint8  pbuBuffer,
   nuint16  suMaxSize
);

NWCCODE NWGetAuditFlags
(
   ARP N_FAR *arp
);

NWCCODE NWAddIsAuditedProperty
(
   ARP N_FAR *arp,
   nuint32  luUserID
);

NWCCODE NWLoginVolumeAuditor
(
   ARP N_FAR *arp
);

NWCCODE NWChangeAuditPassword
(
   ARP N_FAR *arp,
   pnuint8  pbuNewAuditCryptPassword,
   nuint8   buLevel
);

NWCCODE NWCheckAuditorVolumeAccess
(
   NWCONN_HANDLE connectionNumber,
   nuint32  luVolNum
);

NWCCODE NWCheckAuditingLevelTwoAccess
(
   ARP N_FAR *arp
);

NWCCODE NWCloseOldAuditFile
(
   ARP N_FAR *arp
);

NWCCODE NWDeleteOldAuditFile
(
   ARP N_FAR *arp
);

NWCCODE NWRemoveIsAuditedProperty
(
   ARP N_FAR *arp,
   nuint32  luUserID
);

NWCCODE NWDisableVolumeAuditing
(
   ARP N_FAR *arp
);

NWCCODE NWEnableVolumeAuditing
(
   ARP N_FAR *arp
);

NWCCODE NWIsUserAudited
(
   ARP N_FAR *arp,
   nuint32  luUserID
);

NWCCODE NWReadAuditBitMap
(
   ARP N_FAR *arp,
   pnuint8  pbuBuffer,
   nuint16  suMaxSize
);

NWCCODE NWReadAuditConfigHdr
(
   ARP N_FAR *arp,
   AuditConfigHdr N_FAR *buffer,
   nuint16  suMaxSize
);

NWCCODE NWReadAuditFileRecord
(
   nuint32  luVolContID,
   nint16   sFileCode,
   AuditRecord N_FAR *bfr,
   pnuint16 suRcdSize,
   nuint16  suMaxSize,
   pnuint8  pbuEofFlag
);

NWCCODE NWReadAuditingFile
(
   ARP N_FAR *arp,
   nint16   sFileCode,
   pnuint8  pbuBfr,
   nuint16  suBfrSize,
   nuint32  luOffset
);

NWCCODE NWInitAuditFileReads
(
   NWCONN_HANDLE connectionNumber,
   nuint32  luVolContID,
   nint16   sFileCode,
   nint16   sDirSrvcFlag
);

NWCCODE NWLogoutVolumeAuditor
(
   ARP N_FAR *arp
);

NWCCODE NWResetAuditFile
(
   ARP N_FAR *arp
);

NWCCODE NWResetHistoryFile
(
   ARP N_FAR *arp
);

NWCCODE NWWriteAuditBitMap
(
   ARP N_FAR *arp,
   pnuint8  pbuBuffer
);

NWCCODE NWWriteAuditConfigHdr
(
   ARP N_FAR *arp,
   AuditConfigHdr N_FAR *buffer
);

NWCCODE NWDSCheckAuditorCNTAccess
(
   NWCONN_HANDLE connectionNumber,
   nuint32  luContID
);

NWCCODE NWDSChangeAuditPassword
(
   DS_ARP N_FAR *arp,
   pnuint8  pbuNewCryptPassword,
   nuint8   buLevel
);

NWCCODE NWDSCheckAuditingLevelTwoAccess
(
   DS_ARP N_FAR *arp
);

NWCCODE NWDSCheckAuditorCNTAccess
(
   NWCONN_HANDLE connectionNumber,
   nuint32  luContID
);

NWCCODE NWDSCheckObjectAudited
(
   DS_ARP N_FAR *arp,
   nuint32  luObjectID
);

NWCCODE NWDSChangeObjectAudited
(
   DS_ARP N_FAR *arp,
   nuint32  luObjectID,
   nuint8   buAuditFlag
);

NWCCODE NWDSCloseOldAuditFile
(
   DS_ARP N_FAR *arp
);

NWCCODE NWDSDeleteOldAuditFile
(
   DS_ARP N_FAR *arp
);

NWCCODE NWDSDisableCNTAuditing
(
   DS_ARP N_FAR *arp
);

NWCCODE NWDSEnableCNTAuditing
(
   DS_ARP N_FAR *arp
);

NWCCODE NWDSGetAuditFlags
(
   DS_ARP N_FAR *arp
);

NWCCODE NWDSGetCNTAuditStatus
(
   NWCONN_HANDLE connectionNumber,
   nuint32  luContID,
   pnuint8  pbuBfr,
   nuint16  suMaxSize
);

NWCCODE NWDSLoginCNTAuditor
(
   DS_ARP N_FAR *arp
);

NWCCODE NWDSLogoutCNTAuditor
(
   DS_ARP N_FAR *arp
);

NWCCODE NWDSReadAuditFileHdr
(
   DS_ARP N_FAR *arp,
   pnuint8  pbuBfr,
   nuint16  suMaxSize
);

NWCCODE NWDSReadAuditingFile
(
   DS_ARP N_FAR *arp,
   nint16   sFileCode,
   pnuint8  pbuBfr,
   nuint16  suBfrSize,
   nuint32  luOffset
);

NWCCODE NWDSResetAuditFile
(
   DS_ARP N_FAR *arp
);

NWCCODE NWDSResetHistoryFile
(
   DS_ARP N_FAR *arp
);

NWCCODE NWDSWriteAuditFileHdr
(
   DS_ARP N_FAR *arp,
   pnuint8  pbuBfr
);

#include <npackoff.h>

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/inc/nwauditc.h,v 1.6 1994/06/08 23:35:36 rebekah Exp $
*/
