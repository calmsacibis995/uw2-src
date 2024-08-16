/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwdsdsa.h	1.5"
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

#ifndef  _NWDSDSA_HEADER_
#define  _NWDSDSA_HEADER_

#ifndef __NWDSTYPE_H
#ifdef N_PLAT_UNIX
#include <nw/nwdstype.h>
#else
#include <nwdstype.h>
#endif
#endif

#ifndef _NWDSBUFT_HEADER_
#ifdef N_PLAT_UNIX
#include <nw/nwdsbuft.h>
#else
#include <nwdsbuft.h>
#endif
#endif

#ifndef  _NWDSDC_HEADER_
#ifdef N_PLAT_UNIX
#include <nw/nwdsdc.h>
#else
#include <nwdsdc.h>
#endif
#endif

#ifndef NWCONNECT_INC
#ifdef N_PLAT_UNIX
#include <nw/nwconnec.h>
#else
#include <nwconnec.h>
#endif
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else
#include <npackon.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* NWClient DS prototypes */
NWDSCCODE N_API NWCDSMapIDToName
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   nuint32           objectID,
   pnstr8            object
);

NWDSCCODE N_API NWDSAddObject
(
   NWDSContextHandle context,
   pnstr8            objectName,
   pnint32           iterationHandle,
   nbool8            more,
   pBuf_T            objectInfo
);

NWDSCCODE N_API NWDSBackupObject
(
   NWDSContextHandle context,
   pnstr8            objectName,
   pnint32           iterationHandle,
   pBuf_T            objectInfo
);

NWDSCCODE N_API NWDSCompare
(
   NWDSContextHandle context,
   pnstr8            object,
   pBuf_T            buf,
   pnbool8           matched
);

/*-----------------------------------------------------------------------------
 *    In the case that objectName is a partition root, the partitionRootObject
 *    is the same as the objectName
 */

NWDSCCODE N_API NWDSGetPartitionRoot
(
   NWDSContextHandle context,
   pnstr8            objectName,
   pnstr8            partitionRoot
);

NWDSCCODE N_API NWDSList
(
   NWDSContextHandle context,
   pnstr8            object,
   pnint32           iterationHandle,
   pBuf_T            subordinates
);

NWDSCCODE N_API NWDSListContainers
(
   NWDSContextHandle context,
   pnstr8            object,
   pnint32           iterationHandle,
   pBuf_T            subordinates
);

NWDSCCODE N_API NWDSListByClassAndName
(
   NWDSContextHandle context,
   pnstr8            objectName,
   pnstr8            className,
   pnstr8            subordinateName,
   pnint32           iterationHandle,
   pBuf_T            subordinates
);

NWDSCCODE N_API NWDSGetCountByClassAndName
(
   NWDSContextHandle context,
   pnstr8            objectName,
   pnstr8            className,
   pnstr8            subordinateName,
   pnint32           count
);

NWDSCCODE N_API NWDSMapIDToName
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   nuint32           objectID,
   pnstr8            object
);

NWDSCCODE N_API NWDSMapNameToID
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   pnstr8            object,
   pnuint32          objectID
);

NWDSCCODE N_API NWDSModifyObject
(
   NWDSContextHandle context,
   pnstr8            objectName,
   pnint32           iterationHandle,
   nbool8            more,
   pBuf_T            changes
);

NWDSCCODE N_API NWDSModifyDN
(
   NWDSContextHandle context,
   pnstr8            objectName,
   pnstr8            newDN,
   nbool8            deleteOldRDN
);

NWDSCCODE N_API NWDSModifyRDN
(
   NWDSContextHandle context,
   pnstr8            objectName,
   pnstr8            newDN,
   nbool8            deleteOldRDN
);

NWDSCCODE N_API NWDSMoveObject
(
   NWDSContextHandle context,
   pnstr8            objectName,
   pnstr8            destParentDN,
   pnstr8            destRDN
);

NWDSCCODE N_API NWDSRead
(
   NWDSContextHandle context,
   pnstr8            object,
   nuint32           infoType,
   nbool8            allAttrs,
   pBuf_T            attrNames,
   pnint32           iterationHandle,
   pBuf_T            objectInfo
);

NWDSCCODE N_API NWDSReadObjectInfo
(
   NWDSContextHandle    context,
   pnstr8               object,
   pnstr8               distinguishedName,
   pObject_Info_T       objectInfo
);

NWDSCCODE N_API NWDSRemoveObject
(
   NWDSContextHandle context,
   pnstr8            object
);

NWDSCCODE N_API NWDSRestoreObject
(
   NWDSContextHandle context,
   pnstr8            objectName,
   pnint32           iterationHandle,
   nbool8            more,
   nuint32           size,
   pnuint8           objectInfo
);

NWDSCCODE N_API NWDSSearch
(
   NWDSContextHandle context,
   pnstr8            baseObjectName,
   nint              scope,
   nbool8            searchAliases,
   pBuf_T            filter,
   nuint32           infoType,
   nbool8            allAttrs,
   pBuf_T            attrNames,
   pnint32           iterationHandle,
   nint32            countObjectsToSearch,
   pnint32           countObjectsSearched,
   pBuf_T            objectInfo
);

NWDSCCODE N_API NWDSOpenStream
(
   NWDSContextHandle    context,
   pnstr8               objectName,
   pnstr8               attrName,
   nflag32              flags,
   NWFILE_HANDLE N_FAR  *fileHandle
);

NWDSCCODE N_API NWDSWhoAmI
(
   NWDSContextHandle context,
   pnstr8            objectName
);

NWDSCCODE N_API NWDSGetServerDN
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   pnstr8            serverDN
);

NWDSCCODE N_API NWDSGetServerAddresses
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   pnuint32          countNetAddress,
   pBuf_T            netAddresses
);

NWDSCCODE N_API NWDSInspectEntry
(
   NWDSContextHandle context,
   pnstr8            serverName,
   pnstr8            objectName,
   pBuf_T            errBuffer
);

NWDSCCODE N_API NWDSReadReferences
(
   NWDSContextHandle context,
   pnstr8            serverName,
   pnstr8            objectName,
   nuint32           infoType,
   nbool8            allAttrs,
   pBuf_T            attrNames,
   nuint32           timeFilter,
   pnint32           iterationHandle,
   pBuf_T            objectInfo
);


NWDSCCODE N_API NWDSExtSyncList
(
   NWDSContextHandle context,
   pnstr8            objectName,
   pnstr8            className,
   pnstr8            subordinateName,
   pnint32           iterationHandle,
   pTimeStamp_T      timeStamp,
   nbool             onlyContainers,
   pBuf_T            subordinates
);

NWDSCCODE N_API NWDSExtSyncRead
(
   NWDSContextHandle context,
   pnstr8            objectName,
   nuint32           infoType,
   nbool8            allAttrs,
   pBuf_T            attrNames,
   pnint32           iterationHandle,
   pTimeStamp_T      timeStamp,
   pBuf_T            objectInfo
);

NWDSCCODE N_API NWDSExtSyncSearch
(
   NWDSContextHandle context,
   pnstr8            baseObjectName,
   nint              scope,
   nbool8            searchAliases,
   pBuf_T            filter,
   pTimeStamp_T      timeStamp,
   nuint32           infoType,
   nbool8            allAttrs,
   pBuf_T            attrNames,
   pnint32           iterationHandle,
   nint32            countObjectsToSearch,
   pnint32           countObjectsSearched,
   pBuf_T            objectInfo
);

#ifdef __cplusplus
}
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else
#include <npackoff.h>
#endif

#endif                           /* #ifndef _NWDSDSA_HEADER_ */

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwdsdsa.h,v 1.7 1994/09/26 17:12:03 rebekah Exp $
*/
