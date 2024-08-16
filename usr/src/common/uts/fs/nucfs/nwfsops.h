/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nwfsops.h	1.16"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nwfsops.h,v 2.56.2.10 1995/02/03 03:31:12 stevbam Exp $"

#ifndef _FS_NUCFS_NWFSOPS_H
#define _FS_NUCFS_NWFSOPS_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/nucfs/nwfidata.h>	/* REQUIRED */
#include <fs/nucfs/nwfslock.h>	/* REQUIRED */
#include <fs/nucfs/flock_cache.h> /* REQUIRED */
#include <net/nw/nwportable.h>	/* REQUIRED */
#include <net/nuc/nucerror.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/nwfidata.h>	/* REQUIRED */
#include <sys/nwfslock.h>	/* REQUIRED */
#include <sys/flock_cache.h>	/* REQUIRED */
#include <sys/nwportable.h>	/* REQUIRED */
#include <sys/nucerror.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
**  Netware Unix Client
**
**	MODULE:
**		nwfsops.h -	The NetWare Client File System layer (NWfs)
**				operations definitions.
*/

/*
 * Reference NWfs volume object operations.
 */
struct NWfsCred;
struct NUCfsIoArgs;
struct NWfsClientHandle;
struct NWfsServerNode;
struct NWnameSpace;
struct NWfsServerVolume;
struct NUCfsVolumeStats;
struct NWfiVolumeFlushData;

ccode_t
NWfsMountVolume (  struct NWfsCred *credentials, 
		   struct netbuf *serverAddress,
		   char *volumeName,
		   uint32 volumeFlags,
		   struct NWfsServerVolume **serverVolume,
		   struct NWfsServerNode **rootNode,
		   struct  NUCfsVolumeStats *volumeStats,
		   struct  NWfiVolumeFlushData *volFlushData,
		   enum NUC_DIAG *diagnostic);
ccode_t
NWfsVolumeStatistics ( struct NWfsServerNode *serverNode, 
	struct  NUCfsVolumeStats *volumeStats, struct NWfsCred *credentials,
	enum NUC_DIAG *diagnostic);
ccode_t
NWfsPrepareToUnMountVolume (struct NWfsServerNode *rootNode,
	enum NUC_DIAG *diagnostic);
ccode_t
NWfsDoUnMountVolume(struct NWfsServerNode *rootNode);
void
NWfsDestroyVolumeAndRootNode(struct NWfsCred *, struct NWfsServerNode *);
void_t
NWfsGetRootNode(struct NWfsServerVolume *, struct NWfsServerNode **);
ccode_t
NWfsGetServerTime(
	struct NWfsCred		*credentials,
	struct NWfsServerVolume	*volume,
	int32			*timePtr,
	enum NUC_DIAG		*diagnostic);
void
NWfsVolumeSync(struct NWfsServerVolume *volume);

/*
 * Reference NWfs server node object operations.
 */
ccode_t
NWfsCreateFileNode (
        struct NWfsCred         *credentials,
        struct NWfsServerNode   *parentNode,
        char                    *fileName,
        struct NWnameSpace      *nameSpaceInfo,
        uint32                  accessFlags,
        struct NWfsServerNode   **newFileNode,
        enum    NUC_DIAG        *diagnostic);
ccode_t
NWfsFileLock(
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	uint32			accessFlags,
	NUCFS_LOCK_T		*lockStruct,
	enum NUC_DIAG		*diagnostic);
void
NWfsCleanFileLocks(
	NWFS_CLIENT_HANDLE_T	*chandle,
	uint32			accessFlags,
	NUCFS_LOCK_T		*lock);
ccode_t
NWfsUpgradeFileLock (
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	uint32			accessFlags,
	NUCFS_LOCK_T		*lockStruct,
	boolean_t		*upgraded,
	NUCFS_FLOCK_CACHE_T	**savedCache,
	enum NUC_DIAG		*diagnostic);
ccode_t
NWfsRestoreFileLock (
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	uint32			accessFlags,
	NUCFS_LOCK_T		*lockStruct,
	NUCFS_FLOCK_CACHE_T	*savedCache,
	enum NUC_DIAG		*diagnostic);
ccode_t
NWfsSetFileLock(
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	uint32			accessFlags,
	NUCFS_LOCK_T		*lockStruct,
	enum NUC_DIAG		*diagnostic);
ccode_t
NWfsRemoveFileLock(
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	uint32			accessFlags,
	NUCFS_LOCK_T		*lockStruct,
	enum NUC_DIAG		*diagnostic);
ccode_t
NWfsDoFileLock(
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	NUCFS_LOCK_T		*lockStruct,
	enum NUC_DIAG		*diagnostic);
ccode_t
NWfsLinkFile (
        struct NWfsCred         *credentials,
        struct NWfsServerNode   *existingFile,
        struct NWfsServerNode   *newLinkParentNode,
        char                    *newLinkFileName,
        enum    NUC_DIAG        *diagnostic);
ccode_t
NWfsLookUpNode (
        struct NWfsCred         *credentials,
        struct NWfsServerNode   *parentNode,
        char                    *nodeName,
        struct NWfsServerNode   **foundNode,
        enum    NUC_DIAG        *diagnostic);
ccode_t
NWfsMakeDirNode (
        struct NWfsCred         *credentials,
        struct NWfsServerNode   *parentNode,
        char                    *dirName,
        struct NWnameSpace      *nameSpaceInfo,
        struct NWfsServerNode   **newDirNode,
        enum    NUC_DIAG        *diagnostic);
ccode_t
NWfsSetNameSpaceInfo (
        struct NWfsClientHandle *clientHandle,
        struct NWnameSpace      *nameSpaceInfo,
        enum    NUC_DIAG        *diagnostic);
ccode_t
NWfsMoveNode (
        struct NWfsCred         *credentials,
        struct NWfsServerNode   *oldParentNode,
        char                    *oldName,
        struct NWfsServerNode   *sourceNode,
        struct NWfsServerNode   *newParentNode,
        char                    *newName,
        enum    NUC_DIAG        *diagnostic);
ccode_t
NWfsOpenNode (
        struct NWfsCred         *nwfscred,
        struct NWfsServerNode   *serverNode,
        struct NWfsClientHandle **retClientHandle,
        uint32                  accessFlags,
	int			*handleIsNew,
        enum    NUC_DIAG        *diagnostic);
ccode_t
NWfsReadBytesOnNode (
        struct NWfsCred         *credentials,
        struct NWfsServerNode   *fileNode,
        struct NUCfsIoArgs      *ioArgs,
        enum    NUC_DIAG        *diagnostic);
ccode_t
NWfsReadDirNodeEntries (
        struct NWfsClientHandle *clientHandle,
        NUCFS_DIR_IO_ARGS_T     *dirIoArgs,
        enum    NUC_DIAG        *diagnostic);
ccode_t
NWfsDeleteNode (
        struct NWfsCred         *credentials,
        struct NWfsServerNode   *parentNode,
        char                    *childName,
        struct NWfsServerNode   *childNode,
        enum    NUC_DIAG        *diagnostic);
ccode_t
NWfsSymbolicLink (
        struct NWfsCred         *credentials,
        struct NWfsServerNode   *newLinkParentNode,
        char                    *newLinkFileName,
        struct NWnameSpace      *newLinkNameSpaceInfo,
        char                    *relativeExistingNodePath,
        enum    NUC_DIAG        *diagnostic);
ccode_t
NWfsReadSymbolicLink(
	struct NWfsServerNode	*netwareNode, 
	NUCFS_IO_ARGS_T		*netwareIoArgs,	
	int			nmaxBytesToRead,
	int			*bytesRead,
	struct NWfsCred		*nwfsCred, 
	enum NUC_DIAG		*diagnostic);
ccode_t
NWfsTruncateBytesOnNode (
        struct NWfsCred         *credentials,
        struct NWfsServerNode   *fileNode,
        uint32                  truncateOffset,
        enum    NUC_DIAG        *diagnostic);
ccode_t
NWfsWriteBytesOnNode (
        struct NWfsCred         *credentials,
        struct NWfsServerNode   *fileNode,
        NUCFS_IO_ARGS_T         *ioArgs,
        enum    NUC_DIAG        *diagnostic);

/*
 * other exported functions.
 */
void	
nucfs_timeout(void *);

int
do_nuc_bio(struct buf *);

void
NWfsVolumeCacheRecycle(struct NWfsServerVolume *, int);

void
NWfsReleaseStaleCachingHolds(struct NWfsServerNode *,
	struct NWfsServerNode *);
void
NWfsNameCacheRelease(struct NWfsServerNode *, 
	struct NWfsServerNode *);

/*
 * Other exported data.
 */
extern NWFI_LIST_T		NWfsMountedVolumesList;

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_NUCFS_NWFSOPS_H */
