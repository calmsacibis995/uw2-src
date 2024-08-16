/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:api/nwapi.h	1.1"
#ident	"$Header: $"

/*
 * Copyright 1989, 1991 Novell, Inc. All Rights Reserved.
 *
 * THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND
 * TREATIES.  NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 *
 *    include/api/nwapi.h 1.25 (Novell) 7/30/91
 */

/*
 *
 *	This is a header file to be included by the application program 
 *
 */

#ifndef _api_h_
#define _api_h_



/*	Define general types. */
typedef	unsigned long		uint32;	/* 32-bit unsigned type */
typedef	unsigned short		uint16;	/* 16-bit unsigned type */
typedef	unsigned char		uint8;	/* 8-bit unsigned type */ 	

typedef	long				int32;	/* 32-bit signed type */
typedef	short				int16;	/* 16-bit signed type */
typedef char				int8;	/* 8-bit signed type */

typedef short				NWBoolean_ts;

#define 	SUCCESS		0

#ifndef 	TRUE
#define		TRUE		1
#define		FALSE		0
#endif

/*******************************************************************
 *
 * Information pertaining to the accounting API calls
 *
 *******************************************************************/
#define		NWMAX_OBJECT_NAME_LENGTH		48
#define		NWMAX_NUMBER_OF_HOLDS			32
#define		NWMAX_COMMENT_LENGTH			48

/* the following definitions are for use with NWSubmitAccountNote */
#define 	NWAN_CONNECT_TIME				1
#define		NWAN_DISK_STORAGE				2
#define		NWAN_LOG_IN						3
#define		NWAN_LOG_OUT					4
#define		NWAN_ACCOUNT_LOCKED				5
#define		NWAN_SERVER_TIME_MODIFIED		6

typedef struct {
	uint32	objectID;
	int32	holdAmount;
} NWHoldInfo_t;


/*******************************************************************
 *
 * Information pertaining to the bindery API calls
 *
 *******************************************************************/
 /* object and property states */
#define		NWBF_STATIC			0x00	
#define		NWBF_DYNAMIC		0x01	

/* property types */
#define		NWBF_ITEM			0x00
#define		NWBF_SET			0x02

#define		NWMAX_SEGMENT_DATA_LENGTH		128
#ifndef MAX_OBJECT_NAME_LENGTH
#define		MAX_OBJECT_NAME_LENGTH		NWMAX_OBJECT_NAME_LENGTH
#endif
#define		NWMAX_PROPERTY_NAME_LENGTH		16	
#define		NWMAX_MEMBER_NAME_LENGTH		48	
#define		NWMAX_PASSWORD_LENGTH			16
#define		NWMAX_PROPERTY_VALUE_LENGTH     128	

#define 	NWBS_ANY_READ					0x00
#define		NWBS_LOGGED_READ				0x01
#define		NWBS_OBJECT_READ				0x02
#define		NWBS_SUPER_READ					0x03
#define		NWBS_BINDERY_READ				0x04
#define		NWBS_ANY_WRITE					0x00
#define		NWBS_LOGGED_WRITE				0x10
#define		NWBS_OBJECT_WRITE				0x20
#define		NWBS_SUPER_WRITE				0x30
#define		NWBS_BINDERY_WRITE				0x40

#define		NWOT_WILD						0xFFFF
#define		NWOT_UNKNOWN					0x0000
#define		NWOT_USER						0x0001
#define		NWOT_USER_GROUP					0x0002
#define		NWOT_PRINT_QUEUE				0x0003
#define		NWOT_FILE_SERVER				0x0004
#define		NWOT_JOB_SERVER					0x0005
#define		NWOT_GATEWAY					0x0006
#define		NWOT_PRINT_SERVER				0x0007
#define		NWOT_ARCHIVE_QUEUE				0x0008
#define		NWOT_ARCHIVE_SERVER				0x0009
#define		NWOT_JOB_QUEUE					0x000A
#define		NWOT_ADMINISTRATION				0x000B
#define		NWOT_NAS_SNA_GATEWAY			0x0021
#define		NWOT_REMOTE_BRIDGE_SERVER		0x0024
#define		NWOT_TIME_SYNCHRONIZATION_SERVER 0x002D
#define		NWOT_ARCHIVE_SERVER_DYNAMIC_SAP	0x002E
#define		NWOT_ADVERTISING_PRINT_SERVER	0x0047
#define		NWOT_BTRIEVE_VAP				0x0050
#define		NWOT_PRINT_QUEUE_USER			0x0053
#define		NWOT_NVT_SERVER					0x009E

typedef struct {
	char		objectName[ NWMAX_OBJECT_NAME_LENGTH ];
	uint32		objectID;
	uint16		objectType;
	uint8		objectState;
	uint8		objectSecurity;
} NWObjectInfo_t;

typedef struct {
	char		propertyName[ NWMAX_PROPERTY_NAME_LENGTH ];
	uint8		propertyStateAndType;
	uint8		propertySecurity;
	uint8		propertyHasAValue;
} NWPropertyInfo_t;


/******************************************************************* 
* 
* Information pertaining to the connection API calls 
*
* *******************************************************************/ 
#ifndef NWMAX_OBJECT_NAME_LENGTH
#define 	NWMAX_OBJECT_NAME_LENGTH		48	
#endif
#define		NWMAX_LOGIN_TIME_LENGTH			7 
#define		NWMAX_INTERNET_ADDRESS_LENGTH	12 
#define		NWMAX_KEYED_PASSWORD_LENGTH		8



/********************************************************************
 *
 * Information pertaining to the file system API calls 
 *
 ********************************************************************/
#define		NWMAX_DIR_PATH_LENGTH			255
#define		NWMAX_FILE_HANDLE_SIZE			6
#define		NWMAX_DIR_NAME_LENGTH			16	
#define		NWMAX_FILE_NAME_LENGTH			16	
#define		NWMAX_VOLUME_NAME_LENGTH		16	
#define		NWMAX_NS_NAME					16
#define		NWMAX_DS_NAME					48
#define		NWMAX_NUM_NS					10 
#define		NWMAX_NUM_DS					10
#define		NWMAX_NS_COUNT					10 
#define		NWMAX_USER_RESTRICTIONS			12

/* file attributes */
#define		NWFA_NORMAL         0x00000000L
#define		NWFA_READ_ONLY      0x00000001L
#define		NWFA_HIDDEN         0x00000002L
#define		NWFA_SYSTEM         0x00000004L
#define		NWFA_EXECUTE_ONLY   0x00000008L
#define		NWFA_NEED_ARCHIVE   0x00000020L
#define		NWFA_SHARABLE       0x00000080L
#define		NWFA_TRANSACTIONAL  0x00001000L
#define		NWFA_INDEXED        0x00002000L
#define		NWFA_READ_AUDIT     0x00004000L
#define		NWFA_WRITE_AUDIT    0x00008000L
#define		NWFA_PURGE          0x00010000L
#define		NWFA_RENAME_INHIBIT 0x00020000L
#define		NWFA_DELETE_INHIBIT 0x00040000L
#define		NWFA_COPY_INHIBIT 	0x00080000L

/* Trustee Access Rights in a 286 Network directory */
#define		NWTA_NONE             ((uint8)0x00)
#define		NWTA_READ             ((uint8)0x01)
#define		NWTA_WRITE            ((uint8)0x02)
#define		NWTA_OPEN             ((uint8)0x04)
#define		NWTA_CREATE           ((uint8)0x08)
#define		NWTA_DELETE           ((uint8)0x10)
#define		NWTA_OWNERSHIP        ((uint8)0x20)
#define		NWTA_SEARCH           ((uint8)0x40)
#define		NWTA_MODIFY           ((uint8)0x80)
#define		NWTA_ALL              ((uint8)0xFF)

/* trustee rights and inherited rights for 386 */
#define		NWTR_NONE           0x0000
#define		NWTR_READ           0x0001
#define		NWTR_WRITE          0x0002
/*								0x0004 ignore this bit */
#define		NWTR_CREATE         0x0008
#define		NWTR_ERASE          0x0010
#define		NWTR_ACCESS         0x0020
#define		NWTR_FILE_SCAN      0x0040
#define		NWTR_MODIFY         0x0080
#define		NWTR_SUPERVISOR     0x0100
#define		NWTR_NORMAL         0x00FF 
#define		NWTR_ALL            0x01FF

/* search attributes */
#define		NWSA_NONE				0x00
#define		NWSA_HIDDEN				0x02
#define		NWSA_SYSTEM				0x04
#define		NWSA_BOTH				0x06
#define		NWSA_FILES_ONLY			0x20  /* NOT for 286 */
#define		NWSA_DIRECTORIES_ONLY	0x10  /* NOT for 286 */

/* desired open access rights */
#define		NWOR_READ           0x01
#define		NWOR_WRITE          0x02
#define		NWOR_DENY_READ      0x04
#define		NWOR_DENY_WRITE     0x08
#define		NWOR_COMPATIBILITY	0x10
#define		NWOR_SYNC_MODE		0x40

/* change attributes used in conjunction with set dir/file info */
#define		NWCA_NAME							0x0001	
#define		NWCA_ATTRIBUTES						0x0002
#define		NWCA_CREATE_DATE_AND_TIME			0x000C
#define		NWCA_OWNER_ID						0x0010
#define		NWCA_LAST_ARCHIVED_DATE_AND_TIME	0x0060
#define		NWCA_LAST_ARCHIVED_ID				0x0080
#define		NWCA_LAST_MODIFY_DATE_AND_TIME		0x0300
#define		NWCA_LAST_MODIFY_ID					0x0400
#define		NWCA_LAST_ACCESSED_DATE				0x0800
#define		NWCA_INHERITED_RIGHTS_MASK			0x1000
#define		NWCA_DIR_RESTRICTION				0x2000


typedef		uint8	NWDirHandle_ts;
typedef 	uint8	NWFileHandle_ta[ NWMAX_FILE_HANDLE_SIZE ];

typedef struct {
	NWDirHandle_ts	dirHandle;
	uint16			serverConnID;
	char			*pathName;
} NWPath_t;

typedef struct {
	uint32	totalBlocks;
	uint32	availableBlocks;
	uint32	purgableBlocks;
	uint32	notYetPurgableBlocks;
	uint32	totalDirEntries;
	uint32	availDirEntries;
	uint32	maxDirEntriesUsed;
	uint16	volNum;
	uint16	sectorsPerBlock;
	uint8	isHashed;
	uint8	isCached;
	uint8	isRemovable;
	uint8	isMounted;
	char	volName[ NWMAX_VOLUME_NAME_LENGTH ];
} NWVolUsage_t;

typedef struct {
	uint32	attributes;
	uint32	creationDateAndTime;
	uint32	ownerID;
	uint32	archiveDateAndTime;
	uint32	archiverID;
	uint32	lastModifyDateAndTime;
	uint32	dirRestriction;
	uint16	inheritedRightsMask;
	uint8	nameSpaceID;
	char	entryName[ NWMAX_DIR_NAME_LENGTH ];
} NWDirEntryInfo_t;

typedef struct {
	uint32	attributes;
	uint32	creationDateAndTime;
	uint32	ownerID;
	uint32	archiveDateAndTime;
	uint32	archiverID;
	uint32	updateDateAndTime;
	uint32	updatorID;
	uint32	fileSize;
	uint32	lastAccessDateAndTime;
	uint16	inheritedRightsMask;
	uint8	nameSpaceID;
	char	entryName[ NWMAX_FILE_NAME_LENGTH ];
} NWFileEntryInfo_t;

typedef struct {
	uint32	trusteeID;
	uint16	trusteeRights;
} NWTrusteeRights_t;

typedef struct {
	uint16	level;
	uint32	maxBlocks;
	uint32	availableBlocks;
} NWDirRestriction_t;

typedef struct {
	uint32	objectID;
	uint32	restriction;
} NWUserRestriction_t;

typedef struct
{
   uint32   deletedFileTime;
   uint32   deletedDateAndTime;
   uint32   deleterID;
   uint32   attributes;
   uint32   creationDateAndTime;
   uint32   ownerID;
   uint32   archiveDateAndTime;
   uint32   archiverID;
   uint32   updateDateAndTime;
   uint32   updatorID;
   uint32   fileSize;
   uint32   inheritedRightsMask;
   uint32   lastAccessDateAndTime;
   uint8    nameSpaceID;
   char     fileName[NWMAX_NS_NAME];
} NWSalvageableInfo_t;

typedef struct {
	uint8				associatedNameSpace;
	char				dataStreamName[NWMAX_DS_NAME];
} NWDataStreamInfo_t;
	
typedef struct {
	uint8				definedNameSpaces;
	char				nameSpaceName[NWMAX_NUM_NS] [NWMAX_NS_NAME];
	uint8				definedDataStreams;
	NWDataStreamInfo_t	dataStream[NWMAX_NUM_DS];
	uint8				loadedNSCount;
	uint8				loadedNS[NWMAX_NS_COUNT];
	uint8				volumesNSCount;
	uint8				volumesNS[NWMAX_NS_COUNT];
	uint8				volumesDSCount;
	uint8				volumesDS[NWMAX_NS_COUNT];
} NWNameSpaceInfo_t;


/*******************************************************************
 *
 * Information pertaining to the path services API calls
 *
 *******************************************************************/



/*******************************************************************
 *
 * Information pertaining to the queue management API calls
 *
 *******************************************************************/
#define		NWMAX_QUEUE_NAME_LENGTH					48
#define		NWMAX_JOB_STRUCT_SIZE					256
#define		NWMAX_QUEUE_SUBDIR_LENGTH				119
#define		NWMAX_NUMBER_OF_JOB_NUMBERS				250
#define		NWMAX_NUMBER_OF_SERVER_CONN_NUMBERS		25
#define		NWMAX_NUMBER_OF_SERVER_OBJECT_IDS		25
#define		NWMAX_SERVER_STATUS_RECORD_LENGTH		64
#define		NWMAX_QUEUE_JOB_TIME_SIZE				6
#define		NWMAX_JOB_FILE_NAME_LENGTH				14
#define		NWMAX_JOB_DESCRIPTION_LENGTH			50
#define		NWMAX_CLIENT_RECORD_LENGTH				152
#define		NWMAX_FORM_NAME_LENGTH					16
#define		NWMAX_BANNER_NAME_FIELD_LENGTH			13
#define		NWMAX_BANNER_FILE_FIELD_LENGTH			13
#define		NWMAX_HEADER_FILE_NAME_LENGTH			14
#define		NWMAX_JOB_DIR_PATH_LENGTH				80	

/* the following are job control flags */
#define		NWCF_OPERATOR_HOLD			0x80
#define		NWCF_USER_HOLD				0x40
#define		NWCF_ENTRY_OPEN				0x20
#define		NWCF_SERVICE_RESTART		0x10
#define		NWCF_SERVICE_AUTO_START		0x08

/* the following are control flags used with NWPrintRecord_t */
#define		NWPCF_SUPPRESS_FF			0x0008
#define		NWPCF_NOTIFY_USER			0x0010
#define		NWPCF_TEXT_MODE				0x0040
#define		NWPCF_PRINT_BANNER			0x0080

/* the queue status flags are used with NWSetQueueCurrentStatus */
#define		NWQS_NO_SERVER_RESTRICTIONS				0x00
#define		NWQS_NO_MORE_JOBS						0x01
#define		NWQS_NO_MORE_SERVER_ATTACHMENTS			0x02
#define		NWQS_SERVERS_DISABLED					0x04

typedef unsigned char NWClientRecord_ta[ NWMAX_CLIENT_RECORD_LENGTH ];

typedef struct {
	uint8	versionNumber;
	uint8	tabSize;
	uint16	numCopies;
	uint16	controlFlags;
	uint16	linesPerPage;
	uint16	charsPerLine;
	char	formName[ NWMAX_FORM_NAME_LENGTH ];
	char	bannerNameField[ NWMAX_BANNER_NAME_FIELD_LENGTH ];
	char	bannerFileField[ NWMAX_BANNER_FILE_FIELD_LENGTH ];
	char	headerFileName[ NWMAX_HEADER_FILE_NAME_LENGTH ];
	char	directoryPath[ NWMAX_JOB_DIR_PATH_LENGTH ];
} NWPrintRecord_t;

typedef struct {
	uint8				clientStation;
	uint8				clientTask;
	uint32				clientID;
	uint32				targetServerID;
	uint8				targetExecutionTime[ NWMAX_QUEUE_JOB_TIME_SIZE];
	uint8				jobEntryTime[ NWMAX_QUEUE_JOB_TIME_SIZE ];
	uint16				jobNumber;
	uint16				jobType;
	uint8				jobPosition;
	uint8				jobControlFlags;
	uint8				jobFileName[ NWMAX_JOB_FILE_NAME_LENGTH ];
	NWFileHandle_ta		jobFileHandle;
	uint8				servicingServerStation;
	uint8				servicingServerTaskNumber;
	uint32				servicingServerIDNumber;
	uint8				jobDescription[ NWMAX_JOB_DESCRIPTION_LENGTH ];
	NWClientRecord_ta	queueRecord;
} NWQueueJobStruct_t;


/*******************************************************************
 *
 * Information pertaining to the server platform API calls
 *
 *******************************************************************/
#ifndef NWMAX_OBJECT_NAME_LENGTH
#define		NWMAX_OBJECT_NAME_LENGTH		48	
#endif
#define		NWMAX_COMPANY_NAME_LENGTH		80 
#define		NWMAX_DESCRIPTION_LENGTH		80
#define		NWMAX_DATE_LENGTH				24
#define		NWMAX_COPYRIGHT_NOTICE_LENGTH	80
#define		NWMAX_SERVER_NAME_LENGTH		48
#define		NWMAX_CONNECTION_LIST_LENGTH	50

typedef struct {
	uint16	majorVersion;
	uint16	minorVersion;
	uint16	revision;
	uint16	SFTLevel;
	uint16	TTSLevel;
	uint16	accountingVersion;
	uint16	VAPVersion;
	uint16	queueingVersion;
	uint16	printServerVersion;
	uint16	virtualConsoleVersion;
	uint16	securityRestrictionLevel;
	uint16	internetBridgeSupport;
	uint16	maxClientConnSupported;
	uint16	clientConnInUse;
	uint16	peakClientConnUsed;
	uint16	maxVolumes;
	char	serverName[ NWMAX_SERVER_NAME_LENGTH ];
} NWServerPlatformInfo_t;

typedef struct {
	uint8	year;
	uint8	month;
	uint8	day;
	uint8	hour;
	uint8	minute;
	uint8	second;
	uint8	dayOfWeek;	/* 0 means sunday */
} NWServerPlatformDateAndTime_t;

typedef struct {    /* For 286 core printer use only */
	uint8	printerHalted;
	uint8	printerOffLine;
	uint8	currentFormType;
	uint8	redirectedPrinter;
} NWPrinterInfo_t;

typedef struct {
	char	companyName[ NWMAX_COMPANY_NAME_LENGTH ];
	char	revisionDescription[ NWMAX_DESCRIPTION_LENGTH ];
	char	revisionDate[ NWMAX_DATE_LENGTH ];
	char	copyrightNotice[ NWMAX_COPYRIGHT_NOTICE_LENGTH ];
} NWDescriptionStrings_t;

typedef struct {  /* For Portable NetWare only  */
	uint32		NWProcs;
	uint32		TotalPackets;								
	uint32		CreateConnectionRequests;
	uint32		CreateFileRequests;
	uint32		OpenRequests;
	uint32		ReadRequests;
	uint32		WriteRequests;
	uint32		NumOpenFiles;
	uint32		MaxSimultaneousOpens;
	uint32		LostPacketResends;
	uint32		CacheHits;
	uint32		CacheMisses;
	uint32		PrintRequests;
	uint32		MessageRequests;
	uint32		DirectoryRequests;
	uint32		BinderyAndMiscRequests;
	uint32		UnknownRequests;
	uint32		TTSRequests;
} NWServerPlatformStats_t;


typedef struct {  /* For Portable NetWare only  */
	uint32		TTSRequests;
	uint32		TotalWrites;
	uint32		TotalBackouts; 
	uint32		TransactionBackouts;
	uint32		UnfilledBackoutRequests;
	uint32		NumTransactions;
	uint32		MaxTransactionsOpened;
} NWServerPlatformTTS_t;

			
typedef struct {  /* For Portable NetWare only  */
	uint32		LogicalLockRequests;
	uint32		NumLogicalLocks;
	uint32		MaxSimultaneousLogLocks;
	uint32		FileLockRequests;
	uint32		NumFileLocks;
	uint32		MaxSimultaneousFileLocks;
	uint32		PhysLockRequests;
	uint32		NumPhysLocks;
	uint32		MaxSimultaneousPhysLocks;
	uint32		SemaphoreRequests;
	uint32		NumSemaphores;
	uint32		MaxSimultaneousSemaphores;
} NWServerPlatformSync_t;

			
typedef struct {  /* For Portable NetWare only  */
	uint32		SharedMemorySize;
	uint32		CurrentSharedMemoryUsage;
	uint32		MaxSharedMemoryUsed;	
} NWServerPlatformSharedMemoryInfo_t;
			
			
			

/*******************************************************************
 *
 * Information pertaining to the synchronization platform API calls
 *
 *******************************************************************/
#define		NWMAX_LOGICAL_RECORD_NAME_LENGTH 	80
#define		NWMAX_SEMAPHORE_NAME_LENGTH 		127

/* file lock log flags */
#define		NWFL_LOG_ONLY					0x00
#define		NWFL_LOG_AND_LOCK				0x01

/* physical and logical record log flags */
#define		NWPL_LOG_ONLY					0x00
#define		NWPL_LOG_AND_LOCK_EXCLUSIVE		0x01
#define		NWPL_LOG_AND_LOCK_SHAREABLE		0x03

/* logical and physical lock set flags */
#define		NWLS_EXCLUSIVE					0x00
#define		NWLS_SHAREABLE					0x02


/*******************************************************************
 *
 * Information pertaining to the transaction tracking platform API calls
 *
 *******************************************************************/
/* currently no data */




#endif

