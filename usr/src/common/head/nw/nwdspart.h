/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwdspart.h	1.4"
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

#ifndef  _NWDSPART_HEADER_
#define  _NWDSPART_HEADER_

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

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else
#include <npackon.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

NWDSCCODE N_API NWDSAddPartition
(
   NWDSContextHandle context,
   pnstr8            server,
   pnstr8            partitionRoot,
   pnint32           iterationHandle,
   nbool8            more,
   pBuf_T            objectInfo
);

NWDSCCODE N_API NWDSAddReplica
(
   NWDSContextHandle context,
   pnstr8            server,
   pnstr8            partitionRoot,
   nuint32           replicaType
);

NWDSCCODE N_API NWDSChangeReplicaType
(
   NWDSContextHandle context,
   pnstr8            replicaName,
   pnstr8            server,
   nuint32           newReplicaType
);

NWDSCCODE N_API NWDSJoinPartitions
(
   NWDSContextHandle context,
   pnstr8            subordinatePartition,
   nflag32           flags
);

NWDSCCODE N_API NWDSListPartitions
(
   NWDSContextHandle context,
   pnint32           iterationHandle,
   pnstr8            server,
   pBuf_T            partitions
);

NWDSCCODE N_API NWDSRemovePartition
(
   NWDSContextHandle context,
   pnstr8            partitionRoot
);

NWDSCCODE N_API NWDSRemoveReplica
(
   NWDSContextHandle context,
   pnstr8            server,
   pnstr8            partitionRoot
);

NWDSCCODE N_API NWDSSplitPartition
(
   NWDSContextHandle context,
   pnstr8            subordinatePartition,
   nflag32           flags
);

NWDSCCODE N_API NWDSPartitionReceiveAllUpdates
(
   NWDSContextHandle context,
   pnstr8            partitionRoot,
   pnstr8            serverName
);

NWDSCCODE N_API NWDSPartitionSendAllUpdates
(
   NWDSContextHandle context,
   pnstr8            partitionRoot,
   pnstr8            serverName
);

NWDSCCODE N_API NWDSSyncPartition
(
   NWDSContextHandle context,
   pnstr8            server,
   pnstr8            partition,
   nuint32           seconds
);

NWDSCCODE N_API NWDSAbortPartitionOperation
(
   NWDSContextHandle context,
   pnstr8            partitionRoot
);

#ifdef __cplusplus
}
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else
#include <npackoff.h>
#endif

#endif  /* _NWDSPART_HEADER_ */

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwdspart.h,v 1.6 1994/06/08 23:32:53 rebekah Exp $
*/
