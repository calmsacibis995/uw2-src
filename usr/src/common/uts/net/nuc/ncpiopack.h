/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ncpiopack.h	1.16"
#ifndef _NET_NUC_NCP_NCPIOPACK_H
#define _NET_NUC_NCP_NCPIOPACK_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ncpiopack.h,v 2.55.2.2 1995/02/12 23:37:00 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: iopacket.h
 *	ABSTRACT: definitions of dispatch packet layouts.
 */

#ifdef _KERNEL_HEADERS
#include <net/nuc/ncpconst.h>
#include <net/nuc/spfilepb.h>
#include <util/cmn_err.h>
#else _KERNEL_HEADERS
#include <sys/ncpconst.h>
#include <sys/spfilepb.h>
#include <sys/cmn_err.h>
#endif _KERNEL_HEADERS

#ifdef NUC_DEBUG
#define NCP_CMN_ERR cmn_err
#else NUC_DEBUG
#define NCP_CMN_ERR 
#endif NUC_DEBUG

/* 
 *	doPacketBurst is set by nwmp and is TRUE if packet bursting is permitted,
 *	FALSE otherwise.
 */
extern uint32 doPacketBurst;

/*
 *	Packet status flags
 */
#define IOP_FREE 		1	/* not in use			*/
#define IOP_INUSE		2	/* being messed with	*/
#define IOP_ONWIRE 		3	/* pending reply		*/

/*
 *	header structure of the NetWare core protocol request
 */
struct ncpHeader {
	uint16		type;	
	uint8		sequenceNumber;
	uint8		lowByteConnectionNumber;
	uint8		currentTask;
	uint8		highByteConnectionNumber;
	uint8		data[NCP_HEADER_DATA_SIZE];
};


typedef struct {
	uint8	sessionKey[8];
	uint32	currentMessageDigest[4];
	uint32	previousMessageDigest[4];
} md4_t;

/*
 *	I/O packet utilized in the pool of free request packets
 *	allocated per connection
 */
typedef struct pack {
	int32			pSemaphore;		/* semaphore for synching requests	*/
	int32			status;
	int32			ncpHeaderSize;
	NUC_IOBUF_T		*ncpDataBuffer;
	union {
		struct ncpHeader 	ncpHdr;
		char				ncpHdrData[NCP_HEADER_SIZE];	
	} ncpU;
	struct pack		*next;			/* next buffer in the list			*/
} iopacket_t; 

/*
 *	Packet pool structure allocated one per credential set.
 */
typedef struct {
	int32		pSemaphore;	/* Semaphore used for list access	*/
	int32		numEntry;	/* Number of packets in the pool	*/
	iopacket_t	*head;		/* First one						*/
	iopacket_t	*tail;		/* last one							*/
} ppool_t;


/* 
 *	Possible Request Types 
 */
#define	CREATE_A_SERVICE_CONNECTION			0x1111
#define	FILE_SERVICE_REQUEST				0x2222
#define	FILE_SERVICE_RESPONSE				0x3333
#define	DESTROY_A_SERVICE_CONNECTION		0x5555
#define	BIG_FILE_SERVICE_REQUEST			0x7777
#define	PREV_REQUEST_BEING_PROCESSED		0x9999

/*
 *	Special cases of connectio number
 */
#define NEW_CONNECTION			(uint8)0xFF

/*	
 *	Length of Response NCP header 
 */
#define	NCP_RESPONSE_HEADER_LENGTH			8

/*	
 *	Documented and client reference structure for NCP packets 
 */
typedef struct ClientNCPStructure {
	uint16		type;	
	uint8		sequenceNumber;
	uint8		lowByteConnectionNumber;
	uint8		currentTask;
	uint8		highByteConnectionNumber;
	/*	not needed data space */
} ncp_packet_t;

/*	
 *	Internal reference structure for NCP requests 
 */
#pragma pack(1)
typedef struct NCP_Request_Structure {
	uint16		requestType;	
	uint16		serviceConnection;
	uint8		sequenceNumber;
	uint8		currentTask;
	uint8		function;
	uint8		*data; 
} ncp_request_t;
#pragma pack()

/*	
 *	Internal reference structure for NCP responses 
 */
#pragma pack(1)
typedef struct NCP_Response_Structure {
	uint16		responseType;
	uint8		sequenceNumber;
    uint8		connectionNumberLowOrder;
	uint8		taskNumber;
    uint8		connectionNumberHighOrder;
    uint8		completionCode;
    uint8		connectionStatus;
	uint8		*data; 
} ncp_reply_t;
#pragma pack()

/*
 *	Macros for moving integers into NCP request structures.
 *
 *	MOVELE_* moves an integer value to its destination in a Little-Endian form
 *	or least significant digit first as on the Intel i80x86 processors.
 *
 *	MOVEBE_* moves an integer value to its destination in a Big-Endian form
 *	or most significant digit first as on most every other processor.
 *
 *	These macros assume the destination address TO is a pointer to a character
 *	array and that INT is an integer expression.
 */
#ifdef LO_HI_MACH_TYPE

#define MOVEBE_INT16( TO, INT ) \
		(TO)[0] = ((unsigned int)(INT) & 0x0000ff00) >> 8,	\
		(TO)[1] = ((unsigned int)(INT) & 0x000000ff)

#define MOVEBE_INT32( TO, INT ) \
		(TO)[0] = ((unsigned int)(INT) & 0xff000000) >> 24,	\
		(TO)[1] = ((unsigned int)(INT) & 0x00ff0000) >> 16,	\
		(TO)[2] = ((unsigned int)(INT) & 0x0000ff00) >> 8,	\
		(TO)[3] = ((unsigned int)(INT) & 0x000000ff)

#define MOVELE_INT16( TO, INT ) (*(uint16 *)TO) = (INT)

#define MOVELE_INT32( TO, INT ) (*(uint32 *)TO) = (INT)

#endif /* LO_HI_MACH_TYPE */

#ifdef HI_LO_MACH_TYPE

#define MOVEBE_INT16( TO, INT ) (*(uint16 *)TO) = (INT)

#define MOVEBE_INT32( TO, INT ) (*(uint32 *)TO) = (INT)

#define MOVELE_INT16( TO, INT ) (TO)[0] = ((INT) & 0x00ff),	\
								(TO)[1] = ((INT) & 0xff00)>>8

#define MOVELE_INT32( TO, INT ) (TO)[0] = ((INT) & 0x000000ff),		\
								(TO)[1] = ((INT) & 0x0000ff00)>>8,	\
								(TO)[2] = ((INT) & 0x00ff0000)>>16,	\
								(TO)[3] = ((INT) & 0xff000000)>>24

#endif /* HI_LO_MACH_TYPE */

/*
 *	ncp_volume_t
 *
 *	This structure is passed as an opaque pointer to the file system
 *	when the root directory handle is allocated.
 */
typedef struct {
	int32	number;
	void_t	*ops;		/* Name space operations	*/
	uint8	nameSpace;	/* type of namespace		*/
#if ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))
	uint8	tag[2];
#endif ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))
} ncp_volume_t;

/*
 *	NCP_DIRHANDLE_T
 *
 *	An opaque pointer to this structure is returned from an Open Directory
 *	NCP request and passed by the File System as a parent directory handle.
 *
 */
typedef struct {
	uint32  DirectoryBase;			/* 386-style handle for the ns entry	*/
} NCP_DIRHANDLE_T;

/*
 *	NCP_FILEHANDLE_T
 *
 *	An opaque pointer to this structure is returned from an Open File
 *	NCP request and passed by the File System to Read/Write File
 *	NetWare 286-style file handles are six bytes in length and consume
 *	the entire structure.  NetWare 386-style file handles are four bytes
 *	long and begin at FileHandle[2] with the unused bytes set to zero.
 *
 */
typedef struct {
	uint8	FileHandle[6];		/* file handle */
} NCP_FILEHANDLE_T;

/*
 *	ncp_channel_t
 *	
 *	Bogarted from dpchannel.h, ncp_channel_t describes the channel handle 
 *	passed to all NCP functions that need to send an NCP.  It describes the 
 *	interprocess communication channel used.
 */
typedef struct {
	void_t			*transportHandle;
	void_t			*packetPool;
	void_t			*currRequest;
	void_t			*currReply;
	int32			wireSemaphore;
	int32			freeBufferSemaphore;
	struct free_rtn	freeBufferStruct;
	uint16			connectionNumber;
	flag8_t			connectionStatus;			/* state of the connection	*/
	uint32			negotiatedBufferSize;
	uint8			taskNumber;
	uint8			sequenceNumber;
	uint8			taskMask[NCP_MAX_TASKS/8];	/* bit mask of tasks		*/
#ifdef FS_ONLY
	int16			timeZoneOffset;				/* 0-23 hours from GMT		*/
#endif /* FS_ONLY */
	void_t			*taskPtr;					/* ncp_task_t				*/
	struct packetBurstInfoStruct *burstInfo;	/* packet burst info		*/
	uint16			packetBurstSocket;
	uint32			securityFlags;
	uint8			sizeOfDHT;			
	int32			referenceCount;
	int32			licenseCount;
	uint32			authenticationState;
	uint32			broadcastState;
	uint32			connectionReference;
	uint32			securityState;
	uint32			ndsState;
	uint32			licenseState;
	uint32			uPublicState;
	sleep_t			*connSleepLock;
	uint32			flags;
	uint32			channelHold;
	void_t			*spiTaskPtr;
#if ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))
	uint8			tag[2];
#endif ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))
} ncp_channel_t;

/*	values for 'flags'
 */
#define	PACKET_BURST_TRANSACTION		0x00000001

/*
 *	NUCAttributeStruct
 *
 *	This structure communicates the various attributes of a file or 
 *	subdirectory between a UNIX client and NetWare 386 3.11 NUC.NLM.
 *	Several of the fields are from the DOS name space and are noted below. 
 *
 *	UNIXnodeNumber - the unique directory entry number of the UNIX name space
 *		entry.  It is considered the INODE number of the entry.  If the entry
 *		is a HARD LINK, this field contains the entry number of the terminal
 *		node and the RealUNIXnodeNumber contains the unique directory entry
 *		number of the hard link.
 *
 *	DOSnodeNumber - the unique directory entry number of the DOS name space
 *		entry.  It is provided only for future manipulation of the DOS name
 *		space.  If the entry is a HARD LINK, this field contains the entry
 *		number of the terminal node and the RealDOSnodeNumber contains the
 *		unique directory entry number of the hard link.
 *
 *	realUNIXnodeNumber - If the entry is a HARD LINK, this field contains the
 *		true UNIX name space directory number of the entry, otherwise it is 
 *		equal to the UNIXnodeNumber field.
 *
 *	realDOSnodeNumber - If the entry is a HARD LINK, this field contains the
 *		true DOS name space directory number of the entry, otherwise it is 
 *		equal to the DOSnodeNumber field.
 *
 *	nodeType - A type field with the following definition:
 *
 *		VALUE	MEANING
 *
 *		1		Regular File
 *		2		Directory
 *		4		Character Special Device
 *		8		Block Special Device
 *		16		Named Pipe
 *		32		Symbolic Link
 *
 *	nodePermissions - access permissions allowed for the user.  The bits
 *	are defined as follows:
 *
 *		BIT		MEANING
 *
 *		0		Other execute
 *		1		Other write
 *		2		Other read
 *		3		Group execute
 *		4		Group write
 *		5		Group read
 *		6		Owner execute
 *		7		Owner write
 *		8		Owner read
 *		9		Sticky bit
 *		10		Set Group ID
 *		11		Set User ID
 *
 *	numberOfLinks - the number of HARD LINK entries associated with this
 *		entry.
 *	
 *	nodeSize - the size of the file in bytes.  Directories are always fixed in
 *		size and that size has no specific meaning other that it is non-zero.
 *
 *	majorNumber and minorNumber - two currently unsupported fields that would
 *		contain the major and minor device number if this entry were a special
 *		device.
 *
 *	userID - the UNIX userid of the owner.
 *	
 *	groupID - the UNIX groupid of the owner
 *
 *	The next three fields contain UNIX time, the number of seconds since 
 *		00:00 hours, 01 January 1970.
 *
 *	accessTime - UNIX time when the data space was last read or written.  It
 *		is converted from the DOS ModifiedDate/Time if the ModifiedDate is 
 *		more recent than the DOS LastAccessDate, otherwise it is the DOS 
 *		LastAccessDate with time set to 00:00 hours.
 *
 *	modifyTime - UNIX time when the data space was last written.  It is
 *		converted from the DOS ModifiedDate/Time.
 *
 *	changeTime - UNIX time when the name space or attribute information was
 *		last updated.  This field has no paradigm in NetWare and therefore
 *		always equals the modifyTime.
 *
 */
typedef struct {
    uint8	UNIXnodeNumber[4]; 		/* UNIX directory entry # w/unique ID	*/
	uint8	DOSnodeNumber[4];		/* DOS directory entry # w/unique ID	*/
	uint8	realUNIXnodeNumber[4];	/* if a hard link, the real UNIX node	*/
	uint8	realDOSnodeNumber[4];	/* if a hard link, the real DOS node	*/
    uint8	nodeType[4];			/* Object type (see above)				*/
    uint8	nodePermissions[4];		/* Object permissions (see above)		*/
    uint8	numberOfLinks[4];		/* Object number of links				*/
    uint8	nodeSize[4];			/* Size in bytes of data space			*/
    uint8	majorNumber[2];			/* Major number of special device		*/
    uint8	minorNumber[2];			/* Minor number of special device		*/
    uint8	userID[4];				/* User identifier of owner				*/
    uint8	groupID[4];				/* Group identifier of owner			*/
    uint8	accessTime[4];			/* Time of last access					*/
    uint8	modifyTime[4];			/* Time of last data modification		*/
    uint8	changeTime[4];			/* Time of last name space changed		*/
									/* Times measured in seconds since		*/
									/* 00:00:00 GMT, Jan 1, 1970			*/
} NUCAttributeStruct;

/*
 *	NCP_DATETIME_T
 *
 *	A structure for returning the server date and time to the caller.
 *
 */
typedef struct {
    uint32	year;				
    uint32	month;				
    uint32	day;				
    uint32	hour;				
    uint32	minute;				
    uint32	second;				
    uint32	dayOfWeek;				
} NCP_DATETIME_T;


#endif /* _NET_NUC_NCP_NCPIOPACK_H */
