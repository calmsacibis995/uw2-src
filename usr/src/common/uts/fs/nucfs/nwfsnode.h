/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nwfsnode.h	1.18"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nwfsnode.h,v 2.58.2.9 1995/01/26 06:20:07 stevbam Exp $"

#ifndef _FS_NUCFS_NWFSNODE_H
#define _FS_NUCFS_NWFSNODE_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/nucfs/nwfidata.h>		/* REQUIRED */
#include <fs/nucfs/nwfschandle.h>	/* REQUIRED */
#include <net/nw/nwportable.h>		/* REQUIRED */
#include <net/nuc/nucerror.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/nwfidata.h>		/* REQUIRED */
#include <sys/nwfschandle.h>		/* REQUIRED */
#include <sys/nwportable.h>		/* REQUIRED */
#include <sys/nucerror.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
**  Netware Unix Client
**
**	MODULE:
**		nwfsnode.h -	Contains the NetWare Client File System layer
**				(NWfs) server node object definitions.
**
**	ABSTRACT:
**		The nwfsnode.h is included in the NetWare Client File System 
**		(NWfs) Layer and represents the server node object structure.
**	
**	[The following is currently stale, and will be updated when
**	UW2.0 development completes.]
**
**	OBJECT THEOREM RULES OF INFERENCE (PRODUCTION):
**          1) Server node operations can make SPIL operation requests.
**          2) Server node objects have no SPIL resouce directly attached.
**          3) For each UNIX user ID referencing a server node there is a 
**             subordinate client handle object (NWFS_CLIENT_HANDLE_T).  The 
**             client handle object contains the maximal access modes of all 
**             UNIX Generic File System file structures (ie. struct file)
**             referenced by its processes file descriptors.
**          4) For each lock enforced on a server node object there is a
**             subordinate lock object (NUCFS_LOCK_T).
**          5) SPIL resources are indirectly attached to server nodes through
**             subordinate client handle objects (NWFS_CLIENT_HANDLE_T).
**          7) A parent server node object must exist while it is referenced
**             by the UNIX Generic File System or has active children leaf
**             (non directory) nodes.
**          8) Server node objects opened for exclusive access may have only 
**             one client handle (NWFS_CLIENT_HANDLE_T) attached (namely the
**             NWFS_CLIENT_HANDLE_T of UNIX client which opened the file).
**          9) Server node objects opened for exclusive access may have only
**             one UNIX Generic File System file structure (ie. struct file)
**             instance opened.
**
** +-------------------------------------------------------------+
** |                                                             |
** |             WARNING         WARNING         WARNING         |
** |                                                             |
** |                     !!! COMMENT IS STALE !!!                |
** |                                                             |
** |             WARNING         WARNING         WARNING         |
** |                                                             |
** +-------------------------------------------------------------+
**
**         10) Server node objects have a universe access mode represented as
**             an inclusive 'OR' of all access modes, active in their
**             NWFS_CLIENT_HANDLE_T sub-set (ie. the maximal access on the node 
**             of all opened UNIX client instantiations, and the associated
**             maximal access on the node of all UNIX Generic File System file
**             structures (ie. struct file)).
**
**	OBJECT PATHALOGICAL AXIOMS:
**          1) A server node (NWFS_SERVER_NODE_T) object is allocated and opened
**             when a UNIX process opens a NetWare file not currently referenced
**             by the local workstation.  A NWFS_CLIENT_HANDLE_T object will be
**             created and attached to the newly allocated NWFS_SERVER_NODE_T
**             object in behalf of the UNIX client.  A SPIL resource handle
**             will be opened with the access modes passed to NWfsOpenNode(3k),
**             and attached to the NWFS_CLIENT_HANDLE_T object.
**
**             NOTE:
**               All duplicated file descriptors (ie. dup, fork) for the UNIX
**               client opening the NWFS_SERVER_NODE_T object, share the same
**               NWFS_CLIENT_HANDLE_T object.
**
**          2) When a UNIX client process opens an existing NWFS_SERVER_NODE_T
**             object which is currently referenced by other process(es) of
**             the same UNIX client, and the access modes of the new open
**             are not present in the NWFS_CLIENT_HANDLE_T object maximal access
**             modes, a new SPIL resource handle will be opened representing
**             the new maximal modes.  The existing SPIL resource handle will be
**             detached from the NWFS_CLIENT_HANDLE_T object and closed.  The 
**             new SPIL resource handle will be attached to the
**             NWFS_CLIENT_HANDLE_T in order to support all of the possible
**             accesses currently opened.
**
**             NOTE:
**               The UNIX Generic File System will enforce accesses on file 
**               structures (ie. struct file) according to the access mode they
**               were opened with.  Thus there is no security breach.
**
**          3) When a UNIX client process opens an exsiting NWFS_SERVER_NODE_T
**             object which is currently not referenced by other process(es)
**             of the same UNIX client, a new NWFS_CLIENT_HANDLE_T object will
**             be created and attached to the NWFS_SERVER_NODE_T object in
**             behalf of the UNIX client.  A SPIL resource handle will be opened
**             with the access modes passed to NWfsOpenNode(3k), and attached to
**             the NWFS_CLIENT_HANDLE_T object.
**
**             NOTE:
**               The UNIX client must have its own SPIL resource handle to
**               access the NetWare resource.
**
**          4) When a new UNIX client process inherits file descriptor(s) of
**             another UNIX client (ie. setuid) which reference a 
**             NWFS_SERVER_NODE_T object which is not opened for exclusive
**             access, and the new UNIX client does not have a
**             NWFS_CLIENT_HANDLE_T object (ie. has not opened the file itself)
**             attached to the NWFS_SERVER_NODE_T object, a new
**             NWFS_CLIENT_HANDLE_T object will be created and attached to the
**             NWFS_SERVER_NODE_T object in behalf of the new UNIX client.  A
**             SPIL resource handle will be opened with the access mode
**             necessary to service the operation (ie. reads = NW_READ, get name
**             space = NW_READ, writes = NW_WRITE, truncates = NW_WRITE, set 
**             name space = NW_WRITE, etc), and attached to the 
**             NWFS_CLIENT_HANDLE_T object.  This is necessary, as it is unclear
**             which UNIX Generic File System file structure(s), the new UNIX
**             client inherited.  Additional operations will negotiate missing
**             access mode, if necessary, by opening a new SPIL resource handle
**             with the previous access mode plus the new needed access mode.
**             If the new SPIL resource handle is allocated, the previous SPIL
**             resource handle will be detached and closed and the new one will
**             be attached to the NWFS_CLIENT_HANDLE_T object.
**
**             NOTE:
**               Inherited file descriptors must have a SPIL resource handle
**               belonging to the new UNIX client.  
** 
**          5) When a new UNIX client process inherits file descriptor(s) of
**             another UNIX client (ie. setuid) which reference a 
**             NWFS_SERVER_NODE_T object, which is opened for exclusive access, 
**             the new UNIX client will attempt to clone a NWFS_CLIENT_HANDLE_T
**             object onto the existing and only NWFS_CLIENT_HANDLE_T object for
**             proxied access to the SPIL resource handle.  The access mode 
**             necessary to service the operation (ie. reads = NW_READ,
**             get name space = NW_READ, writes = NW_WRITE, truncates =
**             NW_WRITE, set name space = NW_WRITE, etc) is checked with 
**             the NetWare server and if access is granted, the clone
**             NWFS_CLIENT_HANDLE_T is proxy attached to the existing 
**             NWFS_CLIENT_HANDLE_T object.  Additional operations will 
**             negotiate missing access mode, if necessary, by checking access
**             with the server and if access is granted, the mode is attached
**             to the clone NWFS_CLIENT_HANDLE_T object. 
**
**             PROXY RATIONALE:
**               The only case which can cause this, is when a process opens
**               a NetWare file exclusively and then changes the effective
**               user ID.  Since it is impossible to have more than one SPIL
**               resource handle opened simultaneously for exclusive access,  a
**               new NWFS_CLIENT_HANDLE_T object can not be created for the new 
**               UNIX client.  The new UNIX client however was instantiated by
**               the holder of the exclusive access, and thus is implicitly
**               granted a proxy in this case.  The cloning NWFS_CLIENT_HANDLE_T
**               object access mode is checked with the server before proxy is
**               allowed, thus correctly implementing UNIX access semantics.
**               There will be a clone handle for each inheriting UNIX client.
**               It is guaranteed that the existing NWFS_CLIENT_HANDLE_T object
**               will have at least the access needed by any clone since there
**               can only be one UNIX Generic File System file structure (ie.
**               file struct) instance, and the access flags can not be changed
**               after open.
**
*/

/*
 * +-------------------------------------------------------------+
 * |                                                             |
 * |             WARNING         WARNING         WARNING         |
 * |                                                             |
 * |                     !!! COMMENT IS STALE !!!                |
 * |                                                             |
 * |             WARNING         WARNING         WARNING         |
 * |                                                             |
 * +-------------------------------------------------------------+
 *
 * NAME
 *    NWfsServerNode - The NWfs server node object structure.
 * 
 * DESCRIPTION
 *    This data structure defines the server node object structure.  It is
 *    paired with the UNIX Generic  Inode (INODE, VNODE, etc) to form the
 *    focal point for a NetWare file or a directory.  This represents the 
 *    paradigm of an Inode (INODE, VNODE, etc) in a UNIX Generic File System 
 *    mapped to a file or directory node in a NetWare volume.
 *
 *    idHashChain	- Hash chain (doubly linked). Each server node is
 *			  hashed by nameSpaceInfo.nodeNumber.
 *    volumeChain	- Active/Inactive chain.
 *    nodeState		- Represents the inclusive 'OR' of the following
 *			  bits:
 *				SNODE_ACTIVE:	is on the active list
 *				SNODE_INACTIVE:	is on the inactive list
 *				SNODE_INVALID:	is being created or destroyed
 *				SNODE_MARKER:	is a marker node
 *    myNames		- Doubly linked list of NWFI_NAME_T structures
 *			  representing names in the parent directories.
 *    childNames	- Doubly linked list of NWFI_NAME_T structures
 *			  represeting names of children (can be non-NULL
 *			  only for directories).
 *    nodeVolume        - Pointer to server volume this server node is on.
 *
 *    r_statelock	- Spin lock to mutext the following fields (up through
 *			  r_sv).
 *    nodeFlags         - Set to an inclusive 'OR' of the following:
 *                        	SNODE_MODIFY: nameSpaceInfo.modifyTime is stale
 *                        	SNODE_EXTEND: nameSpaceInfo.changeTime is stale
 *    hardHoldCount
 *    softHoldCount
 *    r_mapcnt
 *    gfsNode           - Pointer back to the UNIX Generic File System node
 *                        (INODE, VNODE, etc), this server node is paired with.
 *                        node.
 *    clientHandleList  - Doubly linked list of subordinate client handle
 *                        objects (NWFS_CLIENT_HANDLE_T) associated with this
 *                        server node.
 *    r_sv		- synchronization variable to block on when
 *			  data mutexed by r_statelock is in transition
 *			  (specifically for transitions of the
 *			  clientHandleList).
 *
 *    r_rwlock		- RWSLEEP_LOCK to mutex the ramin
 *    nameSpaceInfo.nodeNumber - Used as the unique node identifier both
 *                               internally with in the NetWare Client File
 *                               System layer (NWfs) and advertised externally
 *                               as the Inode number in the UNIX Generic Inode
 *                               (INODE, VNODE, etc).
 *    nameSpaceInfo.nodeType   - Set to the following object type:
 *                               NS_FILE              - Node is a regular file.
 *                               NS_DIRECTORY         - Node is a directory.
 *                               NS_CHARACTER_SPECIAL - Node is a character
 *                                                      special file (Not
 *                                                      applicable to DOS name
 *                                                      space).
 *                               NS_BLOCK_SPECIAL     - Node is a block special 
 *                                                      file (Not applicable to 
 *                                                      DOS name space).
 *                               NS_FIFO              - Node is a named pipe
 *                                                      file (Not applicable to 
 *                                                      DOS name space).
 *                               NS_SYMBOLIC_LINK     - Node is a symbolic link
 *                                                      file (Not applicable to 
 *                                                      DOS name space).
 *                               NS_ROOT              - Root node of the mounted
 *                                                      volume.
 *    nameSpaceInfo.nodePermission - Set to an inclusive OR of the following
 *                                    object mask:
 *                               NS_OTHER_EXECUTE_BIT   (Not in DOS Name Space)
 *                               NS_OTHER_WRITE_BIT     (Not in DOS Name Space)
 *                               NS_OTHER_READ_BIT      (Not in DOS Name Space)
 *                               NS_GROUP_EXECUTE_BIT   (Not in DOS Name Space)
 *                               NS_GROUP_WRITE_BIT     (Not in DOS Name Space)
 *                               NS_GROUP_READ_BIT      (Not in DOS Name Space)
 *                               NS_OWNER_EXECUTE_BIT
 *                               NS_OWNER_WRITE_BIT
 *                               NS_OWNER_READ_BIT
 *                               NS_STICKY_BIT          (Not in DOS Name Space)
 *                               NS_SET_GID_BIT         (Not in DOS Name Space)
 *                               NS_SET_UID_BIT         (Not in DOS Name Space)
 *                               NS_FILE_EXECUTABLE_BIT (Not in DOS Name Space)
 *                               NS_MANDATORY_LOCK_BIT  (Not in DOS Name Space)
 *                               NS_HIDDEN_FILE_BIT
 *    nameSpaceInfo.nodeNumberOfLinks - Set to number of names linked to the
 *                                      data space.
 *    nameSpaceInfo.nodeSize        - Set to size in bytes of the data space.
 *    nameSpaceInfo.nodeMajorNumber - Set to the Block or Character Device 
 *                                    major number. (Set only when nodeType
 *                                    is NS_CHARACTER_SPECIAL or 
 *                                    NS_BLOCK_SPECIAL).
 *    nameSpaceInfo.nodeMinorNumber - Set to the Block or Character Device 
 *                                    minor number. (Set only when nodeType
 *                                    is NS_CHARACTER_SPECIAL or 
 *                                    NS_BLOCK_SPECIAL).
 *    nameSpaceInfo.userID     - Set to the user identifier of the object owner.
 *    nameSpaceInfo.groupID    - Set to the group identifier of the object
 *                               owner.
 *    nameSpaceInfo.accessTime - Set to the time of last access in seconds
 *                               since 00:00:00 GMT 1/1/70.
 *    nameSpaceInfo.modifyTime - Set to the time of last data space modification
 *                               in seconds since 00:00:00 GMT 1/1/70.
 *    nameSpaceInfo.changeTime - Set to the time of last name space modification
 *                               in seconds since 00:00:00 GMT 1/1/70.
 *    nFlocksCached	       - Number of file and record locks cached by
 *				 associated client handles.
 */

/*
 * Define NWfsServerNode structure.
 */

typedef struct NWfsServerNode {
	/*
	 * The following are mutexed by the NUCFS_LIST_LOCK.
	 *
	 * However, header.headerState (i.e. nodeState) is protected by
	 * both NUCFS_LIST_LOCK and SNODE_LOCK. Both are required to write;
	 * either sufficies to read.
	 */
	struct NWfsNodeHeader	header;		/* must come first */

	/*
	 * The following is mutexed by the SNODE_LOCK.
	 */
	NWFI_NODE_T		*gfsNode;

	/*
	 * The following are constant for the life of the server node object.
	 */
	struct NWfsServerVolume	*nodeVolume;
	int32			nodeType;
	uint32			nodeNumber;

	/*
	 * The following are mutexed by the SNODE_LOCK.
	 *
	 */
	NWFI_LOCK_T		snodeLock;
	NWFI_SV_T		snodeSync;
	uint16			nodeFlags;
	uint16			hardHoldCount;
	uint16			softHoldCount;
	uint16			nameCacheSoftHolds;
	uint16			openResourceCount;
	uint16			closeLockCount;
	uint16			clientHandleStamp;
	uint16			cloneVnodeCount;
	uint16			asyncError;
	uint32			nodeNumberOfLinks;
	uint32			accessTime;
	uint32			modifyTime;
	uint32			changeTime;
	int32			nodeSize;
	NWFS_CACHE_INFO_T	cacheInfo;

	/*
	 * The following is updated with both the RW lock and SNODE_LOCK held.
	 * It may be read while holding either.
	 */
	uint32			nFlocksCached;

	/*
	 * The following is mutexed according to the coventions of client
	 * handles.
	 */
	NWFS_CLIENT_HANDLE_T	clientHandle;

	/*
	 * The following is self mutexing.
	 */
	NWFI_RWLOCK_T		snodeRwLock;

	/*
	 * The following is not mutexed at all, as it is only used as a
	 * hint for detecting sequential behavior.
	 */
	uint32			seqFaultAddr;

	/*
	 * Notes on file size:
	 *	A network file system has special problems which require
	 *	three file sizes to be maintained under the SRV4.2MP
	 *	VM implementation:
	 *
	 *		vmSize		This the local file size. When
	 *				a file is being extended,
	 *				vmSize is bumped up before the
	 *				extension.
	 *
	 *		nodeSize	This is the latest size which we got
	 *				back from the server. This size
	 *				lags behind vmSize when we are
	 *				locally extending the file. Also,
	 *				we do not necessarily have the RW lock
	 *				when we get this information back
	 *				from the server. Hence, this size
	 *				can temporarily diverge from vmSize.
	 *
	 *		postExtSize	The ``post extension'' size.
	 *				When a file is being extended by
	 *				write(2), the last page contains
	 *				invalid data (zeros) for a period
	 *				of time. We do not wish for fsflush()
	 *				or msync(2) to write these zeros to the
	 *				server. So therefore, when a file
	 *				is being extended, postExtSize is
	 *				bumped up after the file is extended.
	 *
	 * The following are mutexed by both the SNODE_RW_LOCK and the
	 * and the SNODE_LOCK. Both must be held (in writer mode) to
	 * modify. Either may be held (in reader mode) to access.
	 */
	NWFI_OFF_T		vmSize;
	NWFI_OFF_T		postExtSize;

	/*
	 * The following are protected by the RW lock
	 */
	uint32			r_mapcnt;

	/*
	 * The follow may only be accessed if:
	 *	A) RW lock is held in writer more and the node is active, or
	 *	B) The node is inactive (and held that way by the inactivating
	 *	   LWP).
	 * It holds the credentials for deleting a file which was renamed to a
	 * hidden name (instead of deleting).
	 */
	NWFS_CLIENT_HANDLE_T	*delayDeleteClientHandle;

	/*
	 * The following datum does not need protection under the
	 * assumption that a 32 bit quantity can be read/written atomically.
	 * Where such an assumption cannot be made, the snode lock should
	 * be used to ensure that it is read/written correctly.
	 * The datum is used to record the time when the server node is 
	 * placed either on the timedList or the cacheList volume 
	 * chains. Races in reading/writing this datum are otherwise benign.
	 */
	NWFI_CLOCK_T		snodeTimeStamp;

} NWFS_SERVER_NODE_T;

/*
 * Convenient names
 */
#define idHashChain		header.chain[0]
#define volumeChain		header.chain[1]
#define nodeState		header.headerState

/*
 * Macros to Translate Between the Snode Structure and id/volume Chains
 */
#define idChainToSnode(c)	((NWFS_SERVER_NODE_T *)(c))
#define volumeChainToSnode(c)	((NWFS_SERVER_NODE_T *)((NWFI_LIST_T *)(c) - 1))
#define snodeToIdChain(s)	(&((s)->idHashChain))
#define snodeToVolumeChain(s)	(&((s)->volumeChain))

#if defined(DEBUG) || defined(DEBUG_TRACE)

/*
 * nodeState definitions
 */
#define SNODE_TIMED		0x534e4f00	/* on the timer list */
#define SNODE_ACTIVE		0x534e4f01 	/* on the volume active list */
#define SNODE_INACTIVE		0x534e4f02	/* on the inactive list */
#define SNODE_STALE		0x534e4f03	/* destroy in-progress */
#define SNODE_MARKER		0x534e4f04	/* is a marker node */
#define SNODE_CHANDLE		0x534e4f05	/* is a client handle */
#define SNODE_EHANDLE		0x534e4f06	/* is an empty client handle */
#define SNODE_DEAD		0xdeadbeef	/* freed! */

#else /* !(DEBUG || DEBUG_TRACE) */

/*
 * nodeState definitions
 */
#define SNODE_TIMED		0	/* on the timer list */
#define SNODE_ACTIVE		1	/* on the volume active list */
#define SNODE_INACTIVE		2	/* on the inactive list */
#define SNODE_STALE		3	/* destroy in-progress */
#define SNODE_MARKER		4	/* is a marker node */
#define SNODE_CHANDLE		5	/* is a client handle */
#define SNODE_EHANDLE		6	/* is an empty client handle */
#define SNODE_DEAD		(-1)	/* freed! */

#endif /* DEBUG || DEBUG_TRACE */

/*
 * nodeNumber mask:
 *	The inode number is obtained by taking the nodeNumber and extracting
 *	the lower 24 bits. The high 8 bits of the nodeNumber field consist
 *	of the generation count.
 */
#define NWFS_NODE_MASK		((1 << 24) - 1)

/*
 * nodeFlags definitions.
 */
#define	SNODE_CL_MODIFY		(1 << 0)	/* File modified by local */
						/* client */
#define SNODE_CL_EXTEND		(1 << 1)	/* File extended by local */
						/* client */
#define SNODE_CL_FAULT		(1 << 2)	/* File faulted by local */
						/* client */
#define SNODE_AT_VALID		(1 << 3)	/* attributes valid */
#define SNODE_SV_SIZE_CHANGE	(1 << 4)	/* size changed on server */
#define SNODE_SV_MODIFY_CHANGE	(1 << 5)	/* mtime changed on server */
#define SNODE_REMOTE_CACHE	(1 << 6)	/* disallow caching */
#define SNODE_DESTROY		(1 << 8)	/* forceable destroy in */
						/* progress  */

/*
 * Macros for Holding/Releasing an Snode
 */

#define SNODE_HOLD_L(sp) {				\
	NVLT_ASSERT(SNODE_LOCK_OWNED(sp));		\
	++(sp)->hardHoldCount;				\
}

#define SNODE_HOLD(sp) {				\
	SNODE_LOCK(sp);					\
	SNODE_HOLD_L(sp);				\
	SNODE_UNLOCK(sp);				\
}

#define SNODE_SOFT_HOLD_L(sp) {				\
	NVLT_ASSERT(SNODE_LOCK_OWNED(sp));		\
	++(sp)->softHoldCount;				\
}

#define SNODE_SOFT_HOLD(sp) {				\
	SNODE_LOCK(sp);					\
	SNODE_SOFT_HOLD_L(sp);				\
	SNODE_UNLOCK(sp);				\
}

/*
 * SNODE_CHILDNAME_SOFT_HOLD and SNODE_CHILDNAME_SOFT_RELEASE
 * macro interfaces are for purposes of adding/removing soft holds
 * on a childnode from the name cache. These interfaces maintain
 * accounting of soft holds that accrue from name cache alone.
 * The reason for tracking the contribution of the name cache in
 * soft holds placed on a server node, is that it helps identify
 * the case when the name cache does not hold such references --
 * which in turn makes it possible to bypass the name cache purge 
 * when a server node is being freed.
 */
#define SNODE_CHILDNAME_SOFT_HOLD_L(sp) {		\
	SNODE_SOFT_HOLD_L(sp);				\
	++(sp)->nameCacheSoftHolds;			\
}

#define SNODE_CHILDNAME_SOFT_HOLD(sp) {			\
	SNODE_LOCK(sp);					\
	SNODE_CHILDNAME_SOFT_HOLD_L(sp);		\
	SNODE_UNLOCK(sp);				\
}

#define SNODE_RELEASE(sp) {				\
	SNODE_LOCK(sp);					\
	if ((sp)->hardHoldCount == 1) {			\
		SNODE_UNLOCK(sp);			\
		NWfsInactiveServerNode(sp);		\
	} else {					\
		--(sp)->hardHoldCount;			\
		SNODE_UNLOCK(sp);			\
	}						\
}

#define SNODE_SOFT_RELEASE(sp) {				 \
	SNODE_LOCK(sp);			 		 	 \
	if((--(sp)->softHoldCount + (sp)->hardHoldCount) == 0) { \
		SNODE_UNLOCK(sp);		 		 \
		NWfsFreeServerNode(sp);				 \
	} else {						 \
		SNODE_UNLOCK(sp);				 \
	}							 \
}

#define SNODE_CHILDNAME_SOFT_RELEASE(sp) {			 \
	SNODE_LOCK(sp);			 		 	 \
	--(sp)->nameCacheSoftHolds;				 \
	if((--(sp)->softHoldCount + (sp)->hardHoldCount) == 0) { \
		SNODE_UNLOCK(sp);		 		 \
		NWfsFreeServerNode(sp);				 \
	} else {						 \
		SNODE_UNLOCK(sp);				 \
	}							 \
}

/*
 * Destroy the identity of server node ``sp'' on volume ``volp''.
 * Returns ``sp'' soft held.
 */
#define SNODE_DESTROY_IDENTITY(sp, volp, countName) {		\
	NVLT_ASSERT(NUCFS_LIST_LOCK_OWNED());			\
	NVLT_ASSERT(SNODE_REASONABLE(sp));			\
	NVLT_ASSERT((sp)->nodeState != SNODE_STALE);		\
	NWfiRemque(snodeToIdChain(sp));				\
	NWfiRemque(snodeToVolumeChain(sp));			\
	--(volp)->countName;					\
	++(volp)->staleCount;					\
	++nwfsStaleNodeCount;					\
	SNODE_LOCK(sp);						\
	(sp)->nodeState = SNODE_STALE;				\
	SNODE_UNLOCK(sp);					\
}

/* 
 * Move a server node from "timed" list to the back of "active" list. 
 * Caller must provide any necessary lock cover.
 */
#define	SNODE_TIMED_TO_ACTIVE(sp) {				\
	NVLT_ASSERT((sp)->nodeState == SNODE_TIMED);		\
	(sp)->nodeState = SNODE_ACTIVE;				\
	NWfiRemque(snodeToVolumeChain(sp));			\
	NWfiInsque(snodeToVolumeChain(sp), NWFI_PREV_ELEMENT(	\
		&((sp)->nodeVolume->activeNodeList)));		\
}

/* 
 * Move a server node from "active" list to the back of "timed" list.
 * Caller must provide any necessary lock cover.
 */
#define	SNODE_ACTIVE_TO_TIMED(sp) {				\
	NVLT_ASSERT((sp)->nodeState == SNODE_ACTIVE);		\
	(sp)->nodeState = SNODE_TIMED;				\
	NWfiRemque(snodeToVolumeChain(sp));			\
	NWfiInsque(snodeToVolumeChain(sp), NWFI_PREV_ELEMENT(	\
		&((sp)->nodeVolume->timedNodeList)));		\
	NWFI_GET_CLOCK((sp)->snodeTimeStamp);			\
}

/*
 * ASSERTs
 */
#define SNODE_HELD_L(sp)	((sp)->softHoldCount + (sp)->hardHoldCount != 0)
#define SNODE_REASONABLE_L(sp)	(SNODE_HELD_L(sp) && 			\
				 ((sp)->nodeState == SNODE_ACTIVE ||	\
				  (sp)->nodeState == SNODE_INACTIVE ||	\
				  (sp)->nodeState == SNODE_TIMED || 	\
				  (sp)->nodeState == SNODE_STALE))
#define SNODE_REASONABLE(sp)	((sp) != NULL &&			\
				 (SNODE_LOCK(sp),			\
				  SNODE_REASONABLE_L(sp) ?		\
					(SNODE_UNLOCK(sp), TRUE)	\
					: (SNODE_UNLOCK(sp), FALSE)))	\

/*
 * Low persistence locks.
 */
#define SNODE_LOCK(sp)		NWFI_LOCK(&(sp)->snodeLock)
#define SNODE_UNLOCK(sp)	NWFI_UNLOCK(&(sp)->snodeLock)
#define SNODE_LOCK_OWNED(sp)	NWFI_LOCK_OWNED(&(sp)->snodeLock)
#define SNODE_WAIT(sp)		NWFI_WAIT(&(sp)->snodeLock,&(sp)->snodeSync)
#define SNODE_WAKEUP(sp)	NWFI_WAKEUP(&((sp)->snodeSync))

/*
 * High persistence locks.
 */
#define SNODE_RD_LOCK(sp)	NWFI_RD_LOCK(&(sp)->snodeRwLock)
#define SNODE_WR_LOCK(sp)	NWFI_WR_LOCK(&(sp)->snodeRwLock)
#define SNODE_RW_UNLOCK(sp)	NWFI_RW_UNLOCK(&(sp)->snodeRwLock)

/*
 * Hash function for server nodes.
 */
#define SNODE_HASH(nodeNumber)	((nodeNumber) & (NUCFS_NODE_LIST_BUCKETS - 1))

/*
 * Mark a file as modified/extended by the client.
 */
#define SNODE_SET_MODIFIED_L(snode) {			\
	(snode)->nodeFlags |= SNODE_CL_MODIFY;		\
}
#define SNODE_SET_MODIFIED(snode) {			\
	SNODE_LOCK(snode);				\
	SNODE_SET_MODIFIED_L(snode);			\
	SNODE_UNLOCK(snode);				\
}
#define SNODE_SET_EXTENDED_L(snode) {			\
	(snode)->nodeFlags |= SNODE_CL_EXTEND;		\
}
#define SNODE_SET_EXTENDED(snode) {			\
	SNODE_LOCK(snode);				\
	SNODE_SET_EXTENDED_L(snode);			\
	SNODE_UNLOCK(snode);				\
}

#define SNODE_HAS_FLOCK(snode)	((snode)->nFlocksCached)

/*
 * Exported Functions.
 */
struct NWfsCred;
struct NWfsName;
struct NWnameSpace;
struct NWslHandle;
extern void NWfsInactiveServerNode(NWFS_SERVER_NODE_T *);
extern void NWfsFreeServerNode(NWFS_SERVER_NODE_T *);
extern ccode_t
NWfsCreateOrUpdateNode (
        NWFS_CLIENT_HANDLE_T    *parentClientHandle,
        struct NWfsName         *hashName,
        NWFS_CACHE_INFO_T	*cacheInfo,
        struct NWnameSpace      *nameSpaceInfo,
        struct NWslHandle	*resourceHandle,
        uint32                  accessFlags, 
        NWFS_SERVER_NODE_T      **foundNode,
        enum    NUC_DIAG        *diagnostic);
extern ccode_t
NWfsCreateRootNode (
        struct NWfsCred         *credentials,
        struct NWfsServerVolume	*serverVolume,
        NWFS_SERVER_NODE_T      **serverNode,
        enum    NUC_DIAG        *diagnostic);
void
NWfsUpdateNodeInfo (
        struct NWfsCred         *credentials,
        struct NWnameSpace      *nameSpaceInfo,
        NWFS_CACHE_INFO_T       *cacheInfo,
        NWFS_SERVER_NODE_T      *snode);
ccode_t
NWfsGetAttribsById(
        NWFS_SERVER_NODE_T      *serverNode,
        struct NWfsCred         *credentials,
        enum NUC_DIAG           *diagnostic);
ccode_t
NWfsGetAttribsByName(
        NWFS_CRED_T             *credentials,
        NWFS_SERVER_NODE_T      *parentNode,
        NWFS_SERVER_NODE_T      *serverNode,
        char                    *nodeName,
        NWSI_NAME_SPACE_T       *nameSpaceInfo,
        enum NUC_DIAG           *diagnostic);
void
NWfsDeleteById(NWFS_CLIENT_HANDLE_T *clientHandle);
ccode_t
NWfsDeleteByName(
        NWFS_CLIENT_HANDLE_T    *parentClientHandle,
        char                    *fileName,
        NWFS_SERVER_NODE_T      *fileNode,
        enum NUC_DIAG           *diagnostic);
void NWfsStaleNode(NWFS_SERVER_NODE_T *snode);
void NWfsInvalidateNode(NWFS_SERVER_NODE_T *snode);
void NWfsForEachSnode(NWFI_LIST_T *list, void (*func)(), void *arg);
void NWfsDestroyNode(NWFS_SERVER_NODE_T *snode);
void NWfsLockSnodes(NWFS_SERVER_NODE_T *snodeArray[]);
void NWfsUnLockSnodes(NWFS_SERVER_NODE_T *snodeArray[]);
void NWfsFlockStale(NWFS_SERVER_NODE_T *snode);

ccode_t
NWfsHideNode(
        NWFS_CRED_T             *credentials,
        NWFS_SERVER_NODE_T      *parentNode,
        NWFS_SERVER_NODE_T      *snode,
        enum NUC_DIAG           *diagnostic);
ccode_t
NWfsRenameNode(
        NWFS_CRED_T             *credentials,
        NWFS_SERVER_NODE_T      *oldParentNode,
        NWFS_SERVER_NODE_T      *newParentNode,
        NWFS_SERVER_NODE_T      *sourceNode,
        char                    *oldName,
        char                    *newName,
        enum    NUC_DIAG        *diagnostic);

#if defined(DEBUG) || defined(DEBUG_TRACE)
boolean_t
NWfsNodeIsHeld(NWFS_SERVER_NODE_T *sNode);
boolean_t
NWfsNodeIsSoftHeld(NWFS_SERVER_NODE_T *sNode);
#endif /* DEBUG || DEBUG_TRACE */

/*
 * Macros to create/destroy list markers.
 */
#define SNODE_CREATE_MARKER(snodep)	{				  \
	*(snodep) = kmem_alloc(sizeof(struct NWfsNodeHeader), KM_SLEEP);  \
	((struct NWfsNodeHeader *)*(snodep))->headerState = SNODE_MARKER; \
}

#if defined(DEBUG) || defined(DEBUG_TRACE)
#define SNODE_DESTROY_MARKER(snode)	{				\
	NVLT_ASSERT(((struct NWfsNodeHeader *)(snode))->headerState ==	\
		    SNODE_MARKER);					\
	((struct NWfsNodeHeader *)(snode))->headerState = SNODE_DEAD;	\
	kmem_free(snode, sizeof(struct NWfsNodeHeader));		\
}

extern int	NWfsFileLockTransparent;	/* Send locks transparently */
#else /* !(DEBUG || DEBUG_TRACE) */
#define SNODE_DESTROY_MARKER(snode)	{				\
	kmem_free(snode, sizeof(struct NWfsNodeHeader));		\
}

#endif /* DEBUG || DEBUG_TRACE */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_NUCFS_NWFSNODE_H */
