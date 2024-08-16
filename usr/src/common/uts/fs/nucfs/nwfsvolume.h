/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nwfsvolume.h	1.11"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nwfsvolume.h,v 2.53.2.6 1995/01/29 22:48:49 stevbam Exp $"

#ifndef _FS_NUCFS_NWFSVOLUME_H
#define _FS_NUCFS_NWFSVOLUME_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/nucfs/nwfidata.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/nwfidata.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
**  Netware Unix Client
**
**	MODULE:
**		nwfsvolume.h -	Contains the NetWare Client File System layer
**				(NWfs) server volume structure definitions.
**
**	ABSTRACT:
**		The nwfsvolume.h is included in the NetWare Clinet File System 
**		layer (NWfs) and represents the	server volume object structure.
**
**	OBJECT THEOREM RULES OF INFERENCE (PRODUCTION)
**          1) Server volume operations make SPIL operation requests.
**          2) Server volume objects have SPIL volume resources attached.
*/

/*
 * NAME
 *    NWfsVolume - The NetWare Client File System server volume structure.
 *
 * DESCRIPTION
 *    This data structure defines the NWfs server volume structure.
 *    The data structure has a pointer to a list of server node objects
 *    associated with this volume.  It is paired with the UNIX mount structure
 *    and associated Generic inode (INODE, VNODE, etc) that the NetWare 
 *    Server:Volume is being mounted on to form the focal point for a NetWare
 *    file system.  This represents the paradigm of a dependent file system in
 *    a UNIX Generic File System mapped to a NetWare Volume.
 * 
 *    logicalBlockSize    - Maximum number of bytes allowed in a logical block 
 *                          on this file system volume.
 *    serverNodeList      - Doubly linked list of active server node objects
 *			    (NWFS_SERVER_NODE_T) associated with this volume.
 *    cacheServerNodeList - Doubly linked list of inactive server node objects
 *                          (NWFS_SERVER_NODE_T) associated with this server
 *			    volume.
 *                          NOTE:
 *                            Whenever a server node is released it is put on 
 *                            this list for a period of time. If the server
 *                            node is used before the time has expired it is
 *                            put back on the serverNodeList.  Otherwise it is
 *                            released upon the expiration of the time.
 *    idHashTable	  - Hash table of all server node (NWFS_SERVER_NODE_T)
 *			    objects in this volume using
 *			    nameSpaceInfo.nodeNumber as the hash key.
 *    serverName          - Name of the NetWare server this volume is on.
 *    volumeName          - Name of the volume.
 *    volumeFlags         - Is set to an inclusive 'OR' of the following:
 *                          NUCFS_VOLUME_READ_ONLY   - Volume was mounted for
 *                                                     read access only.
 *                          NWFS_UNIX_NETWARE_NAME_SPACE - Volume uses UNIX name
 *                                                     space in NetWare mode.
 *                          NWFS_UNIX_NFS_NAME_SPACE - Volume uses UNIX name
 *                                                     space in NFS mode.
 *                          NWFS_DOS_NAME_SPACE      - Volume uses DOS name
 *                                                     space.
 *                          NWFS_UNMOUNTING_VOLUME   - Volume unmounting. 
 *                          NUCFS_INHERIT_PARENT_GID - When creating new nodes,
 *                                                     set the new node's GID to
 *                                                     that of the parent.
 *                          NWFS_RESTRICT_CHOWN      - The owner of a node is
 *                                                     not permitted to change
 *                                                     the ownership of the node
 *                                                     and can change the group
 *                                                     ID only to a group of 
 *                                                     which he/she is a member
 *                                                     of. Must be super-user.
 *                          NOTE:
 *                          A server volume is associated with only one of the 
 *                          supported name spaces. 
 *    mountPoint          - Pointer back to the UNIX mount structure, this
 *                          server volume is paired with.
 *    rootNode            - Pointer to the server root node of this server 
 *                          volume.
 *    spilVolumeHandle    - Pointer to the SPIL volume handle associated with 
 *                          this server volume.
 */

/*
 * Define NWfsServerVolume structure mask.
 */
typedef struct NWfsServerVolume {
	/*
	 * The following is mutexed by the NWfiMountVolumeLock.
	 */
	NWFI_LIST_T	serverVolumeList;

	/*
	 * The following are mutexed by the NUCFS_LIST_LOCK.
	 */
	NWFI_LIST_T	timedNodeList;
	NWFI_LIST_T	activeNodeList;
	NWFI_LIST_T	cacheNodeList;
	uint32		activeOrTimedCount;
	uint32		cacheCount;
	uint32		staleCount;
	NWFI_CLOCK_T	activeStamp;
	NWFI_BUF_T	asyncIoList;
	NWFI_LIST_T	idHashTable[NUCFS_NODE_LIST_BUCKETS];
	uint32		idHashStamp;

	/*
	 * The following fields are constant for the life of this data
	 * structure.
	 */
	uint32		logicalBlockSize;
	struct netbuf	*address;
	char		*volumeName;
	uint32		volumeFlags;
	opaque_t	*mountPoint;
	struct NWfsServerNode
			*rootNode;
	struct NWslHandle
			*spilVolumeHandle;
	nwcred_t	privateCredentials;
	uint8		nucNlmMode;

	/*
	 * The data structure referenced by the following pointer
	 * is protected either by NUCFS_LIST_LOCK or by virtue of
	 * being accessed from a single thread of activity. The pointer
	 * itself is constant for the life of this data structure.
	 */
	struct NWfiVolumeFlushData
			*volFlushData;

} NWFS_SERVER_VOLUME_T;

/*
 * Macros to Translate Between the server volume and embedded chain.
 */
#define chainToServerVolume(ch)	((NWFS_SERVER_VOLUME_T *)(ch))

#define	NWFS_VOLUME_FLAGS(vol)	((NWFS_SERVER_VOLUME_T *)(vol))->volumeFlags

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_NUCFS_NWFSVOLUME_H */
