/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nwficommon.h	1.8"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nwficommon.h,v 2.55.2.5 1995/01/24 18:14:46 mdash Exp $"

#ifndef _FS_NUCFS_NWFICOMMON_H
#define _FS_NUCFS_NWFICOMMON_H


#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <net/nw/nwportable.h>	/* REQUIRED */
#include <net/nuc/nucerror.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/nwportable.h>	/* REQUIRED */
#include <sys/nucerror.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */


/*
**  Netware Unix Client
**
**	MODULE:
**		nwficommon.h -	The NetWare UNIX Client File System definitions
**				used in both the NetWare Virtual File System
**				layer (NWfi) and the user application programs.
**
**	ABSTRACT:
**		The nwficommon.h is included in NetWare Virtual File System
**		Interface Layer (NWfi) of the NetWare UNIX Client File System
**		layer (NUCfs).  This provides a consistent semantic 
**		representation of interface information used between the NWfi
**		layer and the user level application programs.
**
*/

#define	MAX_SERVER_NAME_LENGTH	48	/* Max server name len		*/
#define	MAX_VOLUME_NAME_LENGTH	48	/* Max volume name len		*/

/*
 * Mount flags.
 */
#define	NWFI_VOLUME_READ_ONLY	0x01	/* Volume was mounted read only	     */
#define	NWFI_USE_UID		0x02	/* Use the Uid in the credStruct     */
#define NWFI_USE_GID		0x04	/* Use the Gid in the credStruct     */
#define	NWFI_INHERIT_PARENT_GID	0x08	/* Inherit the parent's GID          */
#define	NWFI_DISALLOW_SETUID	0x10	/* Disallow setuid programs          */
#define	NWFI_NO_NAME_TRUNCATION	0x20	/* Return ENAMETOOLONG on long names */

/*
 * Struct mounta's flag.
 */
#define MS_NUCAM		0x8000	/* mounta->dataptr allocated in kernel*/

/*
 * NAME
 *    NUCfsMountArgs - The NUC File System mount arguments structure.
 * 
 * DESCRIPTION
 *    This data structure defines the NetWare UNIX Client File System mount
 *    argument structure.  This is the interface structure for use with the 
 *    NetWare specific mount command and NWfsMountRootNode(3k).
 *
 *    serverName - Server name to mount a volume on.
 *    volumeName - File System voluem name.
 *    mountFlags - Mount flags.  Set to an inclusive OR of the following:
 *                 NWFI_VOLUME_READ_ONLY   - Mount the NUC File System read
 *                                           only.
 *                 NWFI_USE_UID            - Use the UID passed in the specified
 *                                           credStruct.   Use the UID of an
 *                                           already authenticated user to do
 *                                           the mount.
 *                 NWFI_USE_GID            - Use the GID passed in the specified
 *                                           credStruct.   Use the GID of an
 *                                           already authenticated user to do
 *                                           the mount.
 *                 NWFI_INHERIT_PARENT_GID - When creating new nodes, set the
 *                                           new node's GID to the effective
 *                                           GID of the parent directory.
 *    credStruct - Credential structure of the user mounting the NUCFS File
 *                 System.
 */

/*
 * Define NWcfsMountArgs structure mask.
 */
typedef struct NWfiMountArgs {
	struct netbuf address;
	char		volumeName[MAX_VOLUME_NAME_LENGTH]; 
	uint32		mountFlags;
	struct  cred	credStruct;
} NWFI_MOUNT_ARGS_T;

/*
 * Volume flush data structure. 
 *
 * This is a  per mounted volume data structure, and is used to record
 * information and state variables for operating two daemon LWPs for
 * that volume. The two daemons are (a) the page push daemon, which
 * provides context for handling deferred (asynchronous) page pushes,
 * and (2) the attribute flush daemon, which performs periodic attribute
 * invalidations and resource releases.
 *
 * The contents of this structure are:
 *	struct NWfsServerVolume *serverVolume:
 *		Pointer back to the associated volume structure.
 *	uint_t	flags:
 *		This field is used to communicate with the daemons
 *		whether the volume is being unmounted.
 *	event_t attFlushEvent:
 *		Event variable for activating the attribute flush daemon.
 *	event_t pagePushEvent:
 *		Event variable for activating the page push daemon.
 *	k_lwpid_t attFlushLwpId:
 *		LWP id of the attribute flush daemon. When the volume
 *		is unmounted, the system needs to reap the LWPid. 
 *	k_lwpid_t pagePushLwpId:
 *		LWP id of the page push daemon. When the volume
 *		is unmounted, the system needs to reap the LWPid. 
 *	clock_t	lastStaleCheckTime:
 *		This field is used to record the time when the last
 *		global invalidation of attributes (and name cache)
 *		occurred.
 */
typedef struct NWfiVolumeFlushData {
	struct NWfsServerVolume *serverVolume;
	uint_t	flags;
	event_t attFlushEvent;
	event_t pagePushEvent;
	k_lwpid_t attFlushLwpId;
	k_lwpid_t pagePushLwpId;
	clock_t	lastStaleCheckTime;
	int volumeIndex;
} NWFI_VOLFLUSH_DATA_T;

/*
 * NWfiVolumeFlushData flags definitions:
 *
 *	VOL_UNMOUNTING: 	volume is being unmounted.
 *	VOL_FLUSH_DEINIT:	
 */
#define		VOL_UNMOUNTING		0x01
#define		VOL_FLUSH_DEINIT	0x02

#define	NWFI_VOLUME_UNMOUNTING(vfdP)	((vfdP)->flags & VOL_UNMOUNTING)
#define	NWFI_DONT_CACHE_NAME(cHandle)	\
	NWFI_VOLUME_UNMOUNTING((cHandle)->snode->nodeVolume->volFlushData)

#define		LWP_NAMESTRING_LEN	32

/*
 * extern functions
 */

struct NWfsServerNode;
struct NWfsServerVolume;
struct vfs;
struct NWfsClientHandle;
struct dirent;
struct uio;
struct NWfsCred;
struct NWfsServerNode;
struct page;
struct vnode;

struct vnode *NWfiBindVnodeToSnode (struct NWfsServerNode *snode,
			struct vfs *vfsp);
struct vnode *NWfiBindVnodeToClientHandle (struct NWfsClientHandle *);
ccode_t NWfiErrorMap (int32);
void NWfiGetNodePermissions (uint32, mode_t *);
void NWfiSetNodePermissions (mode_t, uint32 *);
ccode_t NWfiSaveDirEntry (struct dirent *, struct uio *, char *, int32, off_t);
ccode_t NWfiSetNodeType (enum vtype, int32 *, enum NUC_DIAG *);
ccode_t NWfiSyncWithServer(struct NWfsServerNode *, int, struct NWfsCred *,
			enum NUC_DIAG *);
ccode_t NWfiTruncateBytesOnNode (struct NWfsCred *, struct NWfsServerNode *,
			off_t, enum NUC_DIAG *);
void NWfiStaleNode(struct NWfsServerNode *, int);
void NWfiGetNodePermissions (uint32, mode_t *);
int nuc_doputpage(struct vnode *, struct page *, int, cred_t *);
int NWfiActiveNode(struct NWfsServerNode *, int);
void nucfs_timeout_start(void);
void nucfs_timeout_stop(void);
int NWfiFlushInit(struct NWfiVolumeFlushData **, int);
void NWfiFlushActivate(struct NWfsServerVolume *, struct NWfiVolumeFlushData *);
void NWfiFlushDeInit(struct NWfiVolumeFlushData *);
boolean_t NWfiDelay(long);
int NWfiIsOpenFile(struct NWfsClientHandle *);

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_NUCFS_NWFICOMMON_H */
