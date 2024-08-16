/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwmp.h	1.13"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwmp.h,v 2.53.2.1 1994/12/12 01:27:09 stevbam Exp $"

#ifndef _NET_NUC_NWMP_NWMP_H
#define _NET_NUC_NWMP_NWMP_H

/*
 *  Netware Unix Client
 *
 *	  MODULE: nwmp.h 
 *	ABSTRACT: Ioctl structure and gate information
 */

#ifdef _KERNEL_HEADERS
#include <net/xti.h>
#else _KERNEL_HEADERS
#include <sys/xti.h>
#endif _KERNEL_HEADERS

#define MAX_ADDRESS_SIZE 30
#define TEMP_DHTEntry	0x01
#define PERM_DHTEntry	0x02
#define MAX_DS_TREE_NAME_LEN	32
#define MAX_SERVER_NAME			48
#define NWMAX_USER_NAME_LENGTH	48
#define NWMAX_PASSWORD_LENGTH	128
#define MAX_TDS_SIZE			0xffff
#define REQUESTER_MAJOR_VERSION	1
#define REQUESTER_MINOR_VERSION	0
#define REQUESTER_REVISION		0

/*
 *	Manifest constants
 */
#define NWMP_MAJOR_VERSION 1
#define NWMP_MINOR_VERSION 0

/*
 *	Minor device states for the psudo-device
 */

#define NWMP_DEVICE_INUSE 0x100
#define NWMP_DEVICE_FREE  0

/*
 *	Define the call gates for calls through the management
 *	portal
 */

#define NWMP_SPI_INIT			(int)0x0005
#define NWMP_IPC_INIT			(int)0x0006
#define NWMP_SPI_DOWN			(int)0x0007
#define NWMP_IPC_DOWN			(int)0x0008
#define NWMP_HANDLE_IPX_HANGUP	(int)0x0009

#define NWMP_CREATE_SERVICE		(int)0x0010

#define NWMP_OPEN_TASK			(int)0x0012
#define NWMP_CLOSE_TASK			(int)0x0013
#define NWMP_AUTHENTICATE_TASK		(int)0x0014

#define NWMP_SCAN_SERVICE		(int)0x0016
#define NWMP_SCAN_TASK			(int)0x0017
#define NWMP_SCAN_SERVICE_BY_USER 	(int)0x0019
#define NWMP_SET_PRIMARY_SERVICE 	(int)0x001A
#define NWMP_GET_SERVICE_MESSAGE 	(int)0x001B
#define NWMP_CREATE_SERVICE_REQ		(int)0x001C
#define NWMP_CREATE_TASK_REQ		(int)0x001D
#define NWMP_NCP_PACKET_BURST_FLAG	(int)0x001F
#define NWMP_MAKE_SIGNATURE_DECISION	(int)0x0020
#define NWMP_SET_SIGNATURE_LEVEL	(int)0x0021
#define NWMP_GET_SECURITY_FLAGS		(int)0x0022

#define NWMP_GET_SERVER_CONTEXT		(int)0x0023
#define NWMP_LICENSE_CONN		(int)0x0024
#define NWMP_MAKE_CONNECTION_PERMANENT	(int)0x0025
#define NWMP_GET_CONN_INFO		(int)0x0026
#define NWMP_SET_CONN_INFO		(int)0x0027
#define NWMP_SCAN_CONN_INFO		(int)0x0028
#define	NWMP_GET_PRIMARY_SERVICE	(int)0x0029
#define	NWMP_GET_LIMITS			(int)0x002a
#define	NWMP_CLOSE_TASK_WITH_FORCE	(int)0x002b

#define NWMP_CREATE_DH_TABLE_REQ	(int)0x0041
#define NWMP_DELETE_DH_TABLE_REQ	(int)0x0042
#define NWMP_SET_DIR_HANDLE_REQ		(int)0x0043
#define NWMP_GET_DIR_HANDLE_REQ		(int)0x0044
#define NWMP_MODIFY_DIR_HANDLE_REQ	(int)0x0045
#define NWMP_REMOVE_DIR_HANDLE_REQ	(int)0x0046
#define NWMP_ALLOC_TDS_REQ		(int)0x0047
#define NWMP_FREE_TDS_REQ		(int)0x0048
#define NWMP_GET_TDS_REQ		(int)0x0049
#define NWMP_READ_TDS_REQ		(int)0x004a
#define NWMP_WRITE_TDS_REQ		(int)0x004b
#define NWMP_DN_READ_REQ		(int)0x004c
#define NWMP_DN_WRITE_REQ		(int)0x004d
#define NWMP_SET_CONFIG_PARMS_REQ	(int)0x004e
#define NWMP_GET_PREF_TREE_REQ		(int)0x004f
#define NWMP_SET_PREF_TREE_REQ		(int)0x0050
#define NWMP_READ_CLIENT_NLS_PATH_REQ	(int)0x0051
#define NWMP_GET_REQUESTER_VERSION_REQ	(int)0x0054
#define NWMP_GET_MONITORED_CONN_REQ	(int)0x0055
#define NWMP_SET_MONITORED_CONN_REQ	(int)0x0056
#define NWMP_DEALLOCATE_REQ_INIT	(int)0x0057
#define NWMP_SET_CHECKSUM_LEVEL		(int)0x0058
#define NWMP_CHECK_CONNECTION		(int)0x0059
#define NWMP_GET_CHECKSUM_FLAGS		(int)0x005A

/*
 *	Raw NCP requests used by api's and other people to want to
 *	piggyback on authentication
 *	GET_RAW		- Get a reply to a SEND_RAW request
 *	SEND_RAW	- Send a request to a service
 *	RAW			- Perform both send and get within driver without 
 *					returning to user land through system call this
 *					should provide better performance by not forcing
 *					a context switch, but, requires the caller to
 *					provide a buffer large enough to service the
 *					reply, during the request.
 *
 *	REGISTER_RAW - Allow raw requests via this file descriptor
 */

#define NWMP_GET_RAW			(int)0x0100
#define NWMP_SEND_RAW			(int)0x0101
#define NWMP_RAW			(int)0x0102
#define NWMP_REGISTER_RAW		(int)0x0103


/*
 *	Diagnostics/Statistics
 *		These have not been implemented.
 */
#define NWMP_SET_SERVICE_DIAGNOSTIC_LEVEL	(int)0x0030
#define NWMP_GET_SERVICE_DIAGNOSTIC_BLOCK	(int)0x0031
#define NWMP_SET_TASK_DIAGNOSTIC_LEVEL		(int)0x0032
#define NWMP_GET_TASK_DIAGNOSTIC_LEVEL		(int)0x0033
#define NWMP_GET_TRANSPORT_INFO			(int)0x0040

struct reqCreateTaskReq {
	struct netbuf address;
	int32	uid;					/* uid of the requestor */
	int32	gid;					/* gid of the requestor */
	int32	pid;					/* pid of the requestor - SLIME */
	int16	xautoFlags;				/* xauto flags for single login or not */
};

struct openServiceTaskReq {
	struct netbuf address;
	uint32	mode;
	uint32 flags;
};

struct authTaskReq {
	uint32	objID;
	uint32	authType;
	uint8	*authKey;
	int32	authKeyLength;
};

struct checksumLevel {
	uint32	level;
};

struct signatureLevel {
	uint32	level;
};

struct monConnReq {
	uint32	connReference;
};

struct DHTEntryReq {
	uint8	dirHandle;						/* server assigned dirHandle */
	uint8	srcDirHandle;					/* source dir handle */
	char	*path;							/* source path */
	uint16	pathLength;						/* length of source path */
	uint8	handleType;						/* temp or perm */
	uint16	PID;							/* used for temp handles */
};

struct TDSReq {
	uint32	tag;			/* tag ID of this TDS (must be unique for a uid) */ 
	uint32	maxSize;		/* max size of this TDS */
	uint32	dataSize;		/* size of data in this TDS or # of bytes to r/w */
	uint32	flags;			/* options associated with this TDS, not used. */
};

struct TDSRWReq {
	uint32	tag;			/* tag ID of this TDS (must be unique for a uid) */ 
	uint32	length;			/* # of bytes to read/write */
	uint32	offset;			/* offset into TDS data area */
	char	*buffer;		/* pointer to data area in user space */
};

struct initReq {
	uint32	securityLevel;	/* header signatures preference */
	uint32	checksumLevel;	/* ipx checksums preference */
	uint8	*preferredTree;	/* desired DS tree (optional) */
	uint32	prefTreeNameLen;  /* length of passed tree name */
	uint8	*NLSPath; 		/* path to NLS tables */
	uint32	NLSPathLen; 	/* length of path arg */
	uint8	*DNBuffer;		/* pointer to initial DNBuf value */
	uint32	DNBufferLength;	/* # chars in DNBuffer to copy */
};

struct gbufReq {
	uint32	bufLength;		/* # chars in buffer, # chars returned */
	uint8	*buffer;		/* pointer to data area */
};

struct authReq {
	uint8	*name;
	uint32	nameLength;
	uint8	*password;
	uint32	passwordLength;
};

struct reqVersReq {
	uint8	majorVersion;
	uint8	minorVersion;
	uint8	revision;
};


struct rawReq {
	char	*header;
	int32	sendHdrLen;
	int32	recvHdrLen;
	char	*data;
	int32	sendDataLen;
	int32	recvDataLen;
};

struct scanServiceReq {
	uint32	serviceProtocol;
	uint32	transportProtocol;
	uint32	serviceFlags;
/*
 *	Valid serviceFlags values are:
 *		BINDERY_SERVER		this server is used for bindery queries
 *			(specified by "-s <server>" cmd line flag to sapd)
 *
 *	If serviceFlags is nonzero, then all services returned by the Scan 
 *	Services request will have serviceFlags TRUE.
 */
#define BINDERY_SERVER		0x02
	struct netbuf address;
};

struct scanServicesByUserReq {
	struct scanServicesByUserStruct 
		*serviceBuffer;					/* populated by NWMP */
	int32	serviceBufferLength;		/* in octets, supplied by caller */
	int32	serviceBufferUsed;			/* in octets, supplied by NWMP */
};

struct scanTaskReq {
	struct netbuf address;
	int32	userID;
	int32	groupID;
	uint32	mode;
};

struct regRawReq {
	struct netbuf address;
	uint32 flags;
};

#ifdef LATER
typedef struct tagNWCTranAddr
{
	uint32		uType;
	uint32		uLen;
	uint8		*pbuBuffer;
} NWCTranAddr, *pNWCTranAddr;

typedef struct tagNWCConnInfo
{
	uint32		uInfoVersion;
	uint32		uAuthenticationState;
	uint32		uBroadcastState;
	uint32		luConnectionReference;
	uint8		*pstrTreeName;
	uint8		*pstrWorkGroupId;
	uint32		luSecurityState;
	uint32		uConnectionNumber;
	uint32		luUserId;
	uint8		*pstrServerName;
	pNWCTranAddr	pTranAddr;
	uint32		uNdsState;
	uint32		uMaxPacketSize;
	uint32		uLicenseState;
	uint32		uPublicState;
} NWCConnInfo, *pNWCConnInfo;
#endif

struct getServerContextReq {
	uint32	majorVersion;
	uint32	minorVersion;
};

struct getConnInfoReq {
	uint32 uInfoLevel;
	uint32 uInfoLen;
	uint8 *buffer;
};

struct setConnInfoReq {
	uint32 uInfoLevel;
	uint32 uInfoLen;
	uint8 *buffer;
};

struct scanConnInfoReq {
	struct netbuf address;
	uint32 uScanInfoLevel;
	uint8 *pScanConnInfo;
	uint32 uScanInfoLen;
	uint32 uScanFlags;
	uint32 uInfoLevel;
	uint32 uInfoLen;
	uint32 luConnectionReference;
	uint8 *buffer;
};

struct ncpPacketBurstReq {
	int		function;			/* 0=read 1=write 2=enable 3=disable */
	int		offset;				/* offset into file */
	int		byteCount;			/* bytes to read or write */
	char	*buffer;			/* pointer to user-space buffer */
	int		bytesActuallyProcessed;	/* count of bytes actually processed */
	char	fileHandle[6];		/* 6-byte NetWare file handle */
	int		taskNumber;			/* task number (2-255) */
	struct netbuf address;
};

struct makeSignatureDecisionReq {
	uint8	flag;
};

struct getSecurityFlagsReq {
	uint32	defaultLevel;
	uint32	baseLevel;
	uint32	userLevel;
};

/*
 *	Structure for returning messages from any task associated with the
 *	requested service protocol.
 *	
 *	Input from the user-space caller:
 *
 *		serviceProtocol - integer service protocol number (NCP is 0)
 *		spilServiceName - pointer (in user space) to a SPIL service name 
 *			buffer of at least SPI_MAX_SERVICE_NAME_LENGTH.
 *		sprotoServiceName - pointer (in user space) to a SPROTO service name 
 *			buffer of at least SPI_MAX_SPROTO_SERVICE_NAME_LENGTH.
 *		uid - UNIX UID of the service task receiving the message.
 *		gid - UNIX GID of the service task receiving the message.
 *		sprotoUserName - pointer (in user space) to a SPROTO user name  
 *			buffer of at least SPI_MAX_USER_NAME_LENGTH.
 *		message - pointer to a user-space message buffer of at least 
 *			SPI_MAX_MESSAGE_LENGTH in length.
 *
 *	Output to the user-space caller:
 *
 *		spilServiceName - SPIL service name sending the message
 *		sprotoServiceName - service protocol service name sending the message
 *		uid - UNIX UID of the service task receiving the message.
 *		gid - UNIX GID of the service task receiving the message.
 *		sprotoUserName - pointer (in user space) to a SPROTO user name  
 *			buffer of at least SPI_MAX_USER_NAME_LENGTH.
 *		messageText - message from the service
 *		messageLength - total length of the message
 *		diagnostic - SPIL return code
 *		
 *
 */
struct getServiceMessageReq {
	uint32	serviceProtocol;
	struct netbuf spilServiceAddress;
	char	*sprotoServiceName;
	char	*sprotoUserName;
	uint32	uid;
	uint32	gid;
	char	*messageText;
	int		messageLength;
	uint32	diagnostic;
};

struct getLimitsReq {
	uint32	maxClients;
	uint32	maxClientTasks;
};

/*
 *	Argument structure union
 */
union NWMPArgs {
	struct openServiceTaskReq 	openArgs;
	struct authTaskReq		authArgs;
	struct regRawReq		reqRawArgs;
	struct rawReq			rawArgs;
	struct scanServiceReq		scanServiceArgs;
	struct scanTaskReq		scanTaskArgs;
	struct regRawReq		regRawArgs;
	struct scanServicesByUserReq	scanServicesByUserArgs;
	struct getServiceMessageReq	getMessageReqArgs;
	struct reqCreateTaskReq		reqCreateTaskArgs;
	struct ncpPacketBurstReq	reqPacketBurstArgs;
	struct makeSignatureDecisionReq	makeSignatureDecisionArgs;
	struct getSecurityFlagsReq	getSecurityFlagsArgs;
	struct getServerContextReq	getServerContextArgs;
	struct getConnInfoReq		getConnInfoArgs;
	struct setConnInfoReq		setConnInfoArgs;
	struct scanConnInfoReq		scanConnInfoArgs;
	struct DHTEntryReq			DHTEntryArgs;	/* set, mod, del, get entry */
	struct TDSReq				TDSArgs;		/* alloc, free, get */ 
	struct TDSRWReq				TDSRWArgs;		/* read, write */ 
	struct gbufReq				DNArgs;			/* read, write */
	struct initReq				initArgs;
	struct gbufReq				prefTreeArgs;	
	struct gbufReq				NLSPathArgs;
	struct authReq				defaultAuthArgs; /* default authentication */
	struct reqVersReq			reqVersArgs;	/* requester version info */
	struct monConnReq			monConnArgs;	/* get or set */
	struct signatureLevel		sigLevelArgs;
	struct checksumLevel		chksumLevelArgs;
	struct getLimitsReq			getLimitsArgs;
};

/*
 *	Structure returned by scan service
 */
struct serviceInfoStruct {
	uint32	serviceProtocol;
	uint32	transportProtocol;
	struct netbuf *address;
	uint32	serviceFlags;
};

/*
 *  Structure array returned by ScanServicesByUser
 */
struct scanServicesByUserStruct {
	struct netbuf address;
	char buffer[MAX_ADDRESS_SIZE];
	char userName[64];
	uint32 taskMode;			/* task modes are defined in spilcommon.h */
	uint32	resourceCount;		/* zero if no resources allocated for task */
								/* nonzero means the task cannot be deleted */
};
	
/*
 *	NUCMESSAGED is a string containing the path to nucmessaged
 */
#define NUCMESSAGED "/usr/sbin/nucmessaged"

/*
 *	AUTOAUTHENTICATOR is a string containing the path to nwlogind
 */
#define AUTOAUTHENTICATOR  "/usr/sbin/nwlogind"

/*
 *	TDSListEntry	defines how a tagged data store list entry is composed.
 */
typedef struct {
	uint32	uid;		/* uid of Unix user who owns this TDS */
	uint32	tag;		/* tag ID of this TDS (must be unique for a uid) */ 
	uint32	maxSize;	/* size of this TDS */
	uint32	dataSize;	/* size of data in this TDS */
	uint32	flags;		/* options associated with this TDS */
	uint8	*bufPtr;	/* pointer to data buffer */
	void	*TDSForwardPtr;	 /* ptr to next entry */
} TDSListEntry_t;

/*
 *  DNListEntry    defines how a distinguished name list entry is composed.
 */
typedef struct {
    uint32  uid;        /* uid of Unix user who owns this DN */
    uint8   *bufPtr;    /* pointer to data buffer */
	uint32	bufLength;	/* indicates number of characters in the bufPtr */
    void	*DNForwardPtr;   /* ptr to next entry */
} DNListEntry_t;

/*
 *  initListEntry    defines how a initialization list entry is composed.
 *
 *	Note: the preferred entities may be moved elsewhere if we decide
 *	that they should be on a finer granularity than per uid.
 */
typedef struct {
    uint32  uid;        	/* uid of Unix user who owns this entry */
    void  *initForwardPtr;   /* ptr to next entry */
	uint32	securityFlags;		/* signature, encryption, etc. preferences */
	uint32	refCount;			/* # of active "connections" by this uid */
	uint8	*preferredTree;		/* ptr to tree name */
	uint32	prefTreeNameLen;	/* # of characters store (no null) */
	uint8	*NLSPath;			/* ptr to NLS path (local) */
	uint32	NLSPathLength; 		/* number of characters stored (no null) */
	uint32	monitoredConn;		/* reference # of monitored DS connection */
} initListEntry_t;
#endif	/* _NET_NUC_NWMP_NWMP_H */
