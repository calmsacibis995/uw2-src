/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spilcommon.h	1.13"
#ifndef _NET_NUC_SPIL_SPILCOMMON_H
#define _NET_NUC_SPIL_SPILCOMMON_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spilcommon.h,v 2.54.2.3 1995/02/12 18:58:49 mdash Exp $"

/*
 *  Netware Unix Client
 *
 *  MODULE:
 *    spilCommon.h -	The NUC Service Provider Interface layer common
 *						definitions.  Component of the NUC Core Services.
 *
 *  ABSTRACT:
 *    The vSpilCommon.h is included with NetWare Client File System functions,
 *    and the emulated SPIL virtual file system operations of the Core
 *    Services.  This provides a consistent semantic representation of
 *    interface information used passed between these layers.
 */

#ifdef _KERNEL_HEADERS
#include <net/xti.h>
#include <net/nuc/slstruct.h>
#else _KERNEL_HEADERS
#include <sys/xti.h>
#endif _KERNEL_HEADERS

/*
 * Manifest constants for defining NUC NLM modes of operation.
 */
#define NUC_NLM_NONE			0x00
#define NUC_NLM_NETWARE_MODE	0x01
#define NUC_NLM_UNIX_MODE		0x02

/*
 * NAME
 *	NWnameSpace -	The NetWare name space structure.
 * 
 * DESCRIPTION
 *	This data structure defines the NetWare name space interface used 
 *	between NetWare Client File System and the Service Provider 
 *	Interface Layer on NWsiGetNameSpaceInfo(3K) and 
 *	NWsiSetNameSpaceInfo(3K).
 *
 */

/*
 * Object types.
 */
#define	NS_FILE              0x01         /* A regular file           */ 
#define	NS_DIRECTORY         0x02         /* A directory              */ 
#define	NS_CHARACTER_SPECIAL 0x04         /* A charactor device       */ 
#define	NS_BLOCK_SPECIAL     0x08         /* A block device           */ 
#define	NS_FIFO              0x10         /* A named pipe             */ 
#define	NS_SYMBOLIC_LINK     0x20         /* A symbolic link          */
#define	NS_ROOT              0x40         /* A Root Directory         */
#define	NS_UNKNOWN           0xFF         /* Wildcard, match any      */

/*
 * Node object permissions.
 *
 * The bits masked by 0xffff should not be changed as they conform to
 * the NetWare NFS definition of owner, group, world, setuid, setgid and
 * sticky permission bits.
 *
 */
#define	NS_OTHER_EXECUTE_BIT	0x00001L	/* Other execute perm bit */
#define	NS_OTHER_WRITE_BIT		0x00002L	/* Other write perm bit	  */
#define	NS_OTHER_READ_BIT		0x00004L	/* Other read perm bit	  */
#define	NS_GROUP_EXECUTE_BIT	0x00008L	/* Group execute perm bit */
#define	NS_GROUP_WRITE_BIT		0x00010L	/* Group write perm bit	  */
#define	NS_GROUP_READ_BIT		0x00020L	/* Group read perm bit	  */
#define	NS_OWNER_EXECUTE_BIT	0x00040L	/* Owner execute perm bit */
#define	NS_OWNER_WRITE_BIT		0x00080L	/* Owner write perm bit	  */
#define	NS_OWNER_READ_BIT		0x00100L	/* Owner read perm bit	  */
#define	NS_STICKY_BIT			0x00200L	/* Is sticky bit set	  */
#define	NS_SET_GID_BIT			0x00400L	/* Is Set Group ID set	  */
#define	NS_SET_UID_BIT			0x00800L	/* Is Set User ID set	  */
#define	NS_FILE_EXECUTABLE_BIT	0x01000000L	/* Is File executable	  */
#define	NS_CD_ALLOWED_BIT		0x02000000L	/* Search directory perm  */
#define	NS_MANDATORY_LOCK_BIT	0x04000000L	/* Mandatory lock bit perm*/
#define	NS_HIDDEN_FILE_BIT		0x08000000L	/* Is Hidden from search  */	
#define	NS_PERMISSION_MASK		0x0FFFFL

/*
 * Define NWnameSpace structure mask.
 */
typedef struct NWnameSpace {
	int32	nodeNumber;			/* unique object identifier				*/
	int32	linkNodeNumber;		/* unique object identifier of the link	*/
								/* NOTE:								*/
								/*	regular file implies:				*/
								/* 		nodeNumber = linkNodeNumber		*/
	int32	nodeType;			/* Object type (see above)				*/
	uint32	nodePermissions;	/* Object permissions (see above)		*/
	uint32	nodeNumberOfLinks;	/* Object number of links				*/
	uint32	nodeSize;			/* Size in bytes of data space			*/
	uint32	userID;				/* User identifier of owner				*/
	uint32	groupID;			/* Group identifier of owner			*/
	uint32	accessTime;			/* Time of last access					*/
	uint32	modifyTime;			/* Time of last data modification		*/
	uint32	changeTime;			/* Time of last name space changed		*/
								/* Times measured in seconds since		*/
								/* 00:00:00 GMT, Jan 1, 1970			*/
	void_t	*openFileHandle;	/* if file is open, otherwise NULL		*/
	uint32  searchSequence;		/* Next directory search cookie			*/
} NWSI_NAME_SPACE_T;

/*
 * Directory search entry requests, and formats
 * used on NWsiGetDirectoryEntries(3K)
 *
 * Directory Search Requests
 */
#define	GET_FIRST	0x00		/* Start from beginning			*/
#define GET_MORE	0x01		/* Continue from search index	*/
#define	GET_DONE	0x02		/* Close out search on dir		*/

/*
 * NAME
 *	NWdirectoryEntry - The NetWare Directory Entry Format structure.
 * 
 * DESCRIPTION
 *	This data structure defines the NetWare Directory Entry format used
 *	between NetWare Client File System and the Service Provider Interface
 *	Layer on NWsiGetDirectoryEntries(3K) operations.
 *
 *	Where:
 *	nodeAttributes	- Set one or more of the following:
 *				NS_HIDDEN_FILE_BIT
 *	structLength	- Size of entry padded to a 32 bit boundary.
 *	nodeID		- Unique node identifier of the entry.  Set by NetWare
 *                Client File System for DOS name space, and SPIL
 *                for UNIX name space.
 *	name		- Null terminated string of node name.
 *
 * NOTES
 *	Each NWSI_DIRECTORY_T structure formatted into a return buffer must
 *	be padded to a 32bit boundary in order to align the next formatted
 *	etnry ( ie. nodeAttributes, structLength, especially important on
 *	RISC processors).
 */
typedef	struct NWdirectoryEntry	{
	uint32				structLength;
	NWSI_NAME_SPACE_T		nameSpaceInfo;
	opaque_t			*netwareNode;
	char				name[1];
} NWSI_DIRECTORY_T;

/*
 * Define the maxium 32bit aligned (padded) entry
 */
#define	MAX_FORMATTED_ENTRY	((sizeof(NWSI_DIRECTORY_T) + DIR_SIZE + \
				sizeof(uint32)) & ~(sizeof(uint32) - 1))

/*
 * File Open Flags (Inclusive OR of the following)
 * used on NWsiCreateFile(3K) & NWsiOpenFileNode(3K)
 *
 * The following match NCP Open flags (defined in ncpconst.h):
 *		NW_READ			AR_READ
 *		NW_WRITE		AR_WRITE
 *		NW_DENY_READ	AR_DENY_READ
 *		NW_DENY_WRITE	AR_DENY_WRITE
 *		NW_EXCLUSIVE	AR_EXCLUSIVE
 */
#define	NW_READ					0x0001		/* Open for Reading				*/
#define NW_WRITE				0x0002		/* Open for Writing				*/
#define NW_DENY_READ			0x0004		/* Open with Deny Read			*/
#define NW_DENY_WRITE			0x0008		/* Open with Deny Write			*/
#define	NW_EXCLUSIVE			0x0010		/* Open for Exclusive Access	*/
#define NW_FAIL_IF_NODE_EXISTS	0x1000		/* Fail Create if Node exists	*/
#define	NW_CHECK_PARENT_SETGID	0x4000		/* If the parent directory has	*/
											/* the SETGID on, set this		*/
											/* entries GID to that of the	*/
											/* parent.  This bit only used	*/
											/* when creating a new object	*/
											/* in the UNIX Name Space		*/
#define	NW_INHERIT_PARENT_GID	0x8000		/* Set this entries GID to that	*/
											/* of the parent.  This bit is	*/
											/* used only when creating a	*/
											/* new object in the UNIX Name	*/
											/* Space.						*/


/*
 * Lock request structure, modes & types
 * used on NWsiRemoveFileLock(3K) & NWsiSetFileLock(3K)
 *
 * Lock Modes & Types 
 */
#define	READ_LOCK		0x00		/* Lock for reading 				*/
#define WRITE_LOCK		0x01		/* Lock for writing 				*/
#define	PHYSICAL_LOCK	0x00		/* A physical (mandatory) lock		*/
#define	LOGICAL_LOCK	0x01		/* A logical (advisory) lock		*/
									/* NOTE: Advisory locks are			*/
									/*       locally arbitrated,		*/
									/*       thus they are never		*/
									/*       placed on the wire.		*/

/*
 * NAME
 *	SpilLockRequest - The SPIL Lock Request Structure.
 * 
 * DESCRIPTION
 *	This data structure defines the SPIL Lock Request format used between
 *	NetWare Client File System and the Service Provider Interface Layer
 *	on NWsiRemoveFileLock(3K) & NWsiSetFileLock(3K) operations.
 *
 */
typedef	struct SpilLockRequest	{
	uint32	offset;		/* Byte offset into file where lock starts	*/
	uint32	size;		/* Number of bytes in lock					*/
	uint32	mode;		/* READ_LOCK or WRITE_LOCK					*/
	uint32	type;		/* PHYSICAL_LOCK or LOGICAL_LOCK			*/
	uint32	timeout;	/* Timeout value in milliseconds 			*/
}NWSI_LOCK_T;
 
/*
 * NAME
 *	NWvolumeStats - The NetWare Volume Statistics Struture.
 * 
 * DESCRIPTION
 *	This data structure defines the NetWare Volume Statistics used between
 *	NetWare Client File System and the Service Provider Interface Layer
 *	on NWsiVolumeStats(3K) operations.
 *
 */
typedef	struct NWvolumeStats {
	uint32	totalBlocks;		/* Total blocks on volume		*/
	uint32	totalFreeBlocks;	/* Free blocks on volume		*/
	uint32	totalNodes;			/* Total dir entries on volume	*/
	uint32	totalFreeNodes;		/* Free dir entries on volume	*/
}NWSI_VOLUME_STATS_T;
 
/*
 * SPIL Layer Diagnostics
 */
#ifndef FAILURE
#define	FAILURE	-1
#endif
#ifndef SUCCESS
#define	SUCCESS	0
#endif

#define SPROTO_NCP	0

/*
 *	Constants used by other modules (namely NWMP)
 */
#define SPI_MAX_USER_NAME_LENGTH 			48
#define SPI_MAX_PASSWORD_LENGTH  			32
#define SPI_MAX_SERVICE_NAME_LENGTH			48
#define SPI_MAX_SPROTO_SERVICE_NAME_LENGTH	48
#define SPI_MAX_ADDRESS_LENGTH				64
#define SPI_MAX_PATH_LENGTH					MAXPATHLEN
#define SPI_MAX_MESSAGE_LENGTH				128

/*
 *	Task modes used to determine the state of the task from SPIL
 */
#define SPI_TASK_CONNECTED			0x01
#define SPI_TASK_AUTHENTICATED		0x02
#define SPI_TASK_RAW				0x04
#define SPI_PRIMARY_SERVICE			0x08
#define SPI_TASK_DRAINING			0x10
#define SPI_TASK_DELETED			0x20
#define	SPI_TASK_PERMANENT			0x40
#define SPI_ASYNC_REGISTERED		0x80	/* registered w/ async handler */

/*
 *	Structure handle stamps used in top layer of SPIL
 */
#define SPI_FHANDLE_STAMP	0xDD4653	/* Ascii FS */
#define SPI_DHANDLE_STAMP	0xDD4453	/* Ascii DS */
#define SPI_VOLUME_STAMP	0xDD5653 	/* Ascii VS */

#define MAX_ADDRESS_SIZE 30

/*
 *	NWSI_MESSAGE_T - contains a message returned from the service
 *	protocol layer for a particular SPI_TASK_T.  
 */
typedef struct {
	uint32			serviceProtocol;
	void_t			*spilTaskPtr;
	uint32			uid;
	uint32			gid;
	struct	netbuf	spilServiceAddress;
	char			buffer[MAX_ADDRESS_SIZE];
	char			sprotoServiceName[SPI_MAX_SPROTO_SERVICE_NAME_LENGTH];
	char			sprotoUserName[SPI_MAX_USER_NAME_LENGTH];
	char			messageText[SPI_MAX_MESSAGE_LENGTH];
	uint32			messageLength;
} NWSI_MESSAGE_T;

#ifdef _KERNEL_HEADERS
/*
 * NWsi function prototypes
 */
enum NUC_DIAG
NWsiGetDirectoryPath (  nwcred_t            *credPtr,
                        SPI_HANDLE_T        *volHandle,
                        SPI_HANDLE_T        *dirHandle,
                        char                *path,
                        enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiGetDirectoryEntries (   nwcred_t            *credPtr,
                            SPI_HANDLE_T        *volHandle,
                            SPI_HANDLE_T        *dirHandle,
                            uint8               requestType,
                            uint32              *offset,
                            NUC_IOBUF_T         *buffer,
                            enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiGetParentNameSpaceInfo (    nwcred_t            *credPtr,
								SPI_HANDLE_T        *volHandle,
								NWSI_NAME_SPACE_T   *nameSpaceInfo,
								enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiReadFile (  nwcred_t            *credPtr,
                SPI_HANDLE_T        *volHandle,
                SPI_HANDLE_T        *fileHandle,
                uint32              offset,
                NUC_IOBUF_T         *buffer,
                enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiWriteFile ( nwcred_t            *credPtr,
                SPI_HANDLE_T        *volHandle,
                SPI_HANDLE_T        *fileHandle,
                uint32              offset,
                NUC_IOBUF_T         *buffer,
                enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiTruncateFile (  nwcred_t            *credPtr,
                    SPI_HANDLE_T        *volHandle,
                    SPI_HANDLE_T        *fileHandle,
                    uint32              offset,
                    enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiSetFileLock (   nwcred_t            *credPtr,
                    SPI_HANDLE_T        *volHandle,
                    SPI_HANDLE_T        *fileHandle,
                    NWSI_LOCK_T         *lockStruct,
                    enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiRemoveFileLock (    nwcred_t            *credPtr,
                        SPI_HANDLE_T        *volHandle,
                        SPI_HANDLE_T        *fileHandle,
                        NWSI_LOCK_T         *lockStruct,
                        enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiLinkFile(   nwcred_t            *credPtr,
                SPI_HANDLE_T        *volHandle,
                SPI_HANDLE_T        *currParentHandle,
                char                *currName,
                int32               currUniqueID,
                SPI_HANDLE_T        *newParentHandle,
                char                *newName,
                enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiGetFileSize (   nwcred_t            *credPtr,
                    SPI_HANDLE_T        *volHandle,
                    SPI_HANDLE_T        *fileHandle,
                    uint32              *size,
                    enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiOpenNode (  nwcred_t            *credPtr,
                SPI_HANDLE_T        *volHandle,
                SPI_HANDLE_T        *parentHandle,
                char                *objectName,
                int32               objectType,
                int32               openFlags,
                SPI_HANDLE_T        **objectHandle,
                NWSI_NAME_SPACE_T   *nameSpaceInfo,
                enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiCloseNode ( nwcred_t            *credPtr,
                SPI_HANDLE_T        *volHandle,
                SPI_HANDLE_T        *objectHandle,
                int32               objectType,
                enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiCreateNode (    nwcred_t            *credPtr,
                    SPI_HANDLE_T        *volHandle,
                    SPI_HANDLE_T        *parentHandle,
                    char                *objectName,
                    int32               openFlags,
                    SPI_HANDLE_T        **objectHandle,
                    NWSI_NAME_SPACE_T   *nameSpaceInfo,
                    enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiDeleteNode (    nwcred_t            *credPtr,
                    SPI_HANDLE_T        *volHandle,
                    SPI_HANDLE_T        *parentHandle,
                    char                *objectName,
                    int32               objectType,
                    int32               objectID,
                    enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiRenameNode (    nwcred_t            *credentials,
                    SPI_HANDLE_T        *volumeHandle,
                    SPI_HANDLE_T        *oldParentHandle,
                    char                *oldName,
                    NWSI_NAME_SPACE_T   *nameSpaceInfo,
                    SPI_HANDLE_T        *newParentHandle,
                    char                *newName,
                    enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiGetNameSpaceInfo (  nwcred_t            *credPtr,
                        SPI_HANDLE_T        *volHandle,
                        SPI_HANDLE_T        *parentHandle,
                        char                *objectName,
                        NWSI_NAME_SPACE_T   *nameSpaceInfo,
                        enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiSetNameSpaceInfo (  nwcred_t            *credPtr,
                        SPI_HANDLE_T        *volHandle,
                        SPI_HANDLE_T        *parentHandle,
                        char                *objectName,
                        NWSI_NAME_SPACE_T   *nameSpaceInfo,
                        enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiAutoAuthenticate (   nwcred_t            *credPtr,
                        SPI_HANDLE_T        *volHandle,
                        int16               xautoFlags,
                        enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiGetServerTime ( nwcred_t            *credPtr,
					SPI_HANDLE_T        *volHandle,
					int32               *serverTime,
					enum    NUC_DIAG    *diagnostic );

ccode_t
NWsiOpenVolume (    nwcred_t            *credPtr,
					struct netbuf       *serviceAddress,
					char                *volumeName,
					SPI_HANDLE_T        **volHandle,
					uint8				*nucNlmMode,
					uint32              *logicalBlockSize,
					enum    NUC_DIAG    *diagnostic );

enum NUC_DIAG
NWsiVolumeStats (   nwcred_t            *credPtr,
					SPI_HANDLE_T        *volHandle,
					NWSI_VOLUME_STATS_T *volumeStats,
					enum NUC_DIAG       *diagnostic );

ccode_t
NWsiCloseVolume (   nwcred_t            *credPtr,
					SPI_HANDLE_T        *volHandle,
					enum    NUC_DIAG    *diagnostic );

#endif _KERNEL_HEADERS

#endif /* _NET_NUC_SPIL_SPILCOMMON_H */
