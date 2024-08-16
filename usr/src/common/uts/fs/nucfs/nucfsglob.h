/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nucfsglob.h	1.8"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nucfsglob.h,v 2.52.2.3 1995/01/24 18:14:25 mdash Exp $"

#ifndef _FS_NUCFS_NUCFSGLOB_H
#define _FS_NUCFS_NUCFSGLOB_H

#ifdef _KERNEL_HEADERS
#include <util/ksynch.h>
#elif defined(_KERNEL) || defined(_KMEMUSER)
#include <sys/ksynch.h>
#endif /* _KERNEL_HEADERS */

/*
**  Netware Unix Client
**
**	MODULE:
**		nucfsglob.h -	Contains the NetWare UNIX Client File System
**				global variables used in the Virtual File System
**				Interface layer (NWfi) and the NetWare Client
**				File System layer (NWfs).
**
**	ABSTRACT:
**		The nucfsglob.h is included in the NetWare Client File System
**		layer (NWfs) and the Virtual File System Interface layer (NWfi).
**		It declares the static variables used within these layers.
**
**	NOTES:
**		Do not manipulate these variable if not needed.
**
*/

/* 
 * Reference the global variables used between the Virtual File System 
 * Interface layer (NWfi) and the NetWare Client File System layer (NWfs).
 *
 *    nucfsType                  - NetWare UNIX Client File System index into
 *                                 the UNIX Generic File System Switch table.
 *    nwfiInitialized            - If set to TRUE indicates that the Virtual
 *                                 File System Interface layer (NWfi) has 
 *                                 successfully been initialized.
 *    nwfsActiveNodeCount        - Total number of active server nodes on all
 *                                 of the mounted volumes.
 *    nwfsCacheNodeCount         - Total number of cached server nodes on all
 *                                 of the mounted volumes.
 *    nwfsStaleNodeCount         - Total number of stale server nodes on all
 *                                 of the mounted volumes.
 *    nwfsMaxNodes               - Upper limit on the number of server nodes. 
 *    nwfsMaxVolumes             - Upper limit on the number of 
 *                                 mounted volumes
 *    nwfsVolumesCount           - Number of active (mounted) volumes.
 *    nucfsDesperate             - Too many server nodes are allocated/cached.
 *    nucfsResourceShortage      - Immediate closing of cached resource 
 *                                 handles is desirable.
 */
extern	uint16		nucfsType;
extern	boolean_t	nwfiInitialized;
extern	uint32		nwfsActiveNodeCount;
extern	uint32		nwfsCacheNodeCount;
extern	uint32		nwfsStaleNodeCount;
extern	uint32		nwfsMaxNodes;
extern	uint32		nwfsMaxVolumes;
extern	uint32		nwfsVolumesCount;
extern	uint32		nucfsDesperate;
extern	uint32		nucfsResourceShortage;

#endif /* _FS_NUCFS_NUCFSGLOB_H */
