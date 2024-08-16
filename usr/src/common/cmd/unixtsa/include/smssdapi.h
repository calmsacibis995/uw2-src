/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/smssdapi.h	1.1"
/* **************************************************************************
	* Program Name:		Storage Management Services (SDAPI)
	*
	* Filename:			SMSSDAPI.H
	*
	* Date Created:		Feburary 17, 1992
	*
	* Version:			4.0
	*
	* Files used:		smstypes.h, smsdefns.h, and smstserr.h.
	*
	* Date Modified:	
	*
	* Modifications:	
	*
	* Comments:			
	*
	* (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
	*
	* No part of this file may be duplicated, revised, translated, localized or
	* modified in any manner or compiled, linked or uploaded or downloaded to or
	* from any computer system without the prior written consent of Novell, Inc.
	**************************************************************************/


// Constants
	// globally used constants

#if !defined(_SMSSDAPI_H_INCLUDED)
#define _SMSSDAPI_H_INCLUDED

#if !defined (_CLIB_HDRS_INCLUDED)
#define _CLIB_HDRS_INCLUDED
#include <string.h>
#include <stdlib.h>
#define NETWARE3X
#include <nwlocale.h>
#endif

#include <smstypes.h>
#include <smsdefns.h>
#include <smssderr.h>



#define NWSMSD_WAIT_PENDING				0xFFFFFFFF
#define NWSMSD_DONT_CARE					0xFFFFFFFF

#define NWSMSD_UNKNOWN						0xFFFFFFFF

	// Media number for NWSMSD_MEDIA_ID

#define NWSMSD_END_MEDIA					0xFFFF

	// modes for NSWMSDDismountMedia

#define NWSMSD_AUTO_EMANCIPATE_DEVICE		0x00000000

#define NWSMSD_DONT_EMANCIPATE_DEVICE		0x00000001

#define NWSMSD_UNCOND_EMANCIPATE_DEVICE	0x00000002

#define NWSMSD_AUTO_EMANCIPATE_MEDIA		0x00000000

#define NWSMSD_DONT_EMANCIPATE_MEDIA		0x00000004

#define NWSMSD_UNCOND_EMANCIPATE_MEDIA		0x00000008

#define NWSMSD_WRITE_TRAILER				0x00000010

#define NWSMSD_POSITION_INQUIRE			0x00000001

#define NWSMSD_POSITION_BEG_SESSION_REL	0x00000002

#define NWSMSD_POSITION_BEG_SESSION_ABS	0x00000003

#define NWSMSD_POSITION_END_SESSION_REL	0x00000004

#define NWSMSD_POSITION_END_SESSION_ABS	0x00000005

#define NWSMSD_POSITION_SECTOR_REL			0x00000006

#define NWSMSD_POSITION_SECTOR_ABS			0x00000007

#define NWSMSD_POSITION_PARTITION_ABS		0x00000008

#define NWSMSD_POSITION_MEDIA_INDEX		0x00000009

#define NWSMSD_REWIND_MEDIA				0x0000000A

#define NWSMSD_RE_TENSION_MEDIA			0x0000000B

#define NWSMSD_POSITION_END_OF_MEDIA		0x0000000C

#define NWSMSD_POSITION_MODE_LAST			0x0000000D


	// For moveMode parameter NWSMSDMoveMedia()

#define NWSMSD_MOVE_INQUIRE				0x00000001

#define NWSMSD_MOVE_MEDIA					0x00000002

#define NWSMSD_MOVE_MAGAZINE				0x00000003

#define NWSMSD_MOVE_EJECT					0x00000004

	// For transferBufferState in NWSMSD_CONTROL_BLOCK structure

#define NWSMSD_UNASSIGNED		 		  	0x00000000

#define NWSMSD_AVAILABLE		 		  	0x00000001

#define NWSMSD_READY_TO_TRANSFER	 	  	0x00000002

#define NWSMSD_TRANSFER_IN_PROGRESS		0x00000003

#define NWSMSD_TRANSFER_COMPLETE		  	0x00000004

#define NWSMSD_TRANSFER_STATUS_LAST		0x00000005


	// For sessionDataType in NWSMSD_CONTROL_BLOCK structure

#define	NWSMSD_TSA_DATA					0x00000001

#define	NWSMSD_END_OF_TSA_DATA			0x00000002

#define	NWSMSD_SESSION_TRAILER			0x00000003

#define	NWSMSD_SESSION_INDEX			0x00000004

#define	NWSMSD_MEDIA_INDEX				0x00000005

#define	NWSMSD_END_OF_SESSION			0x00000006


// For operationType in NWSMSDFormatMedia()

#define	NWSMSD_FORMAT_MEDIA					0x00000001

#define	NWSMSD_PARTITION_MEDIA				0x00000002

	// For alertType in NWSMSDRegisterAlertRoutine()

#define	NWSMSD_NEW_MEDIA					0x00000001

#define	NWSMSD_NEW_MEDIA_NEEDED				0x00000002

#define	NWSMSD_NEW_MEDIA_NOT_BLANK			0x00000004

#define	NWSMSD_NEW_MEDIA_INCORRECT			0x00000008

#define	NWSMSD_MEDIA_DELETED				0x00000010

#define	NWSMSD_MEDIA_RES_CHANGE				0x00000020

#define	NWSMSD_MEDIA_ADDED					0x00000040

// #define	RESERVED_FOR_FUTURE					0x00000080

#define	NWSMSD_DEVICE_RES_CHANGE			0x00000100

#define	NWSMSD_DEVICE_ADDED					0x00000200

#define	NWSMSD_DEVICE_DELETED				0x00000400


#define	NWSMSD_MAX_NUM_ALERTS				32

	//For alertResponseValue in NWSMSDAlertResponse:

#define	NWSMSD_NEW_MEDIA_CONTINUE			0x00000000

#define	NWSMSD_NEW_MEDIA_ABORT				0x00000001


	// Defines That Have SDI Strings Associated With Them

	// For valueType in NWSMSDConvertValueToMessage()

#define NWSMSD_VALUE_TYPE_MEDIA			0x00000001

#define NWSMSD_VALUE_TYPE_DEVICE			0x00000002

#define NWSMSD_VALUE_TYPE_OBJECT			0x00000003

#define NWSMSD_VALUE_TYPE_RELATION			0x00000004

#define NWSMSD_VALUE_TYPE_RESERVED			0x00000005

#define NWSMSD_VALUE_TYPE_MODE				0x00000006

#define NWSMSD_VALUE_TYPE_MOUNTED			0x00000007

#define NWSMSD_VALUE_TYPE_OWNER			0x00000008

#define NWSMSD_VALUE_TYPE_CAPACITY			0x00000009

#define NWSMSD_VALUE_TYPE_OPERATION		0x0000000A

#define NWSMSD_VALUE_TYPE_LAST				0x0000000B


	// Defines for NWSMSD_DEVICE_ID.deviceType

#define NWSMSD_DEVICE_DISK					0x00000000

#define NWSMSD_DEVICE_TAPE					0x00000001

#define NWSMSD_DEVICE_PRINTER	 			0x00000002

#define NWSMSD_DEVICE_RESERVED0 			0x00000003

#define NWSMSD_DEVICE_WORM					0x00000004

#define NWSMSD_DEVICE_CDROM	 			0x00000005

#define NWSMSD_DEVICE_RESERVED1 			0x00000006

#define NWSMSD_DEVICE_MO		 			0x00000007

#define NWSMSD_DEVICE_SINGLE_CHANGER		0x00000008

#define NWSMSD_DEVICE_MULTIPLE				0x00000009

	// Defines for  NWSMSD_MEDIA_ID.mediaType

#define NWSMSD_MEDIA_FIXED					0x00000000

#define NWSMSD_MEDIA_FLOPPY_5_25			0x00000001

#define NWSMSD_MEDIA_FLOPPY_3_50			0x00000002

#define NWSMSD_MEDIA_OPTICAL_5_25			0x00000003

#define NWSMSD_MEDIA_OPTICAL_3_50			0x00000004

#define NWSMSD_MEDIA_TAPE_0_50				0x00000005

#define NWSMSD_MEDIA_TAPE_0_25				0x00000006

#define NWSMSD_MEDIA_TAPE_8MM				0x00000007

#define NWSMSD_MEDIA_TAPE_4MM				0x00000008

#define NWSMSD_MEDIA_BERNOULLI_DISK		0x00000009


	// For NWSMSD_MEDIA_LOCATION.objectType

#define NWSMSD_DEVICE						0x00000001

#define NWSMSD_MAIL_BOX					0x00000002

#define NWSMSD_STORAGE_BAY					0x00000003

#define NWSMSD_OBJECT_TYPE_LAST			0x00000004

	// For deviceRelation field in NWSMSD_DEVICE_ID structure

#define NWSMSD_DEVICE_SINGLE_MEDIA			0x00000000

#define NWSMSD_DEVICE_MAGAZINE				0x00000001

#define NWSMSD_DEVICE_CHANGER				0x00000002


	// For  NWSMSD_DEVICE_ID.reservedStatus and
	//        NWSMSD_MEDIA_ID.reservedStatus

#define NWSMSD_RESERVED_TO_THIS_SDI		0x00000001

#define NWSMSD_RESERVED_TO_OTHER_APP		0x00000002

#define NWSMSD_UNRESERVED					0x00000003

#define NWSMSD_RESERVE_TYPE_LAST			0x00000004


	// Subjugate Modes

#define NWSMSD_NOT_SUBJUGATED				0x00000000

#define NWSMSD_READ_MODE 					0x00000001

#define NWSMSD_WRITE_MODE 					0x00000002


	// mediaMounted conditions for NWSMSD_MEDIA_STATUS structure

#define NWSMSD_MEDIA_IS_MOUNTED			0x00000001

#define NWSMSD_MEDIA_IS_DISMOUNTED			0x00000002
#define NWSMSD_MEDIA_MOUNT_PENDING			0x00000003

	//  NWSMSD_MEDIA_ID.mediaOwner

	// The following are media owners defined by Novell

#define	UNIDENTIFIABLE_MEDIA			0x00000001
#define	HIGH_SIERRA_CDROM_MEDIA	  		0x00000002
#define	ISO_CDROM_MEDIA			  		0x00000003
#define	MAC_CDROM_MEDIA 				0x00000004
#define	NETWARE_FILE_SYSTEM_MEDIA		0x00000005
#define	INTERNAL_IDENTIFY_TYPE			0x00000007
#define	SMS_MEDIA_TYPE					0x00000008

// The following are media owners designated for third parties
#define	THIRD_PARTY_MASK				0xF0000000

	// Used in CAPACITY.factor

#define	NWSMSD_CAPACITY_BYTE			0x00000000
#define	NWSMSD_CAPACITY_KILO			0x00000001
#define	NWSMSD_CAPACITY_MEGA			0x00000002
#define	NWSMSD_CAPACITY_GIGA			0x00000003
#define	NWSMSD_CAPACITY_TERA			0x00000004


	// For NWSMSD_OBJECT_STATUS.objectOperation

#define	NWSMSD_OPERATION_NONE			0x00000000
#define	NWSMSD_OPERATION_WRITING		0x00000001
#define	NWSMSD_OPERATION_READING		0x00000002
#define	NWSMSD_OPERATION_FORMATTING		0x00000003


// End of constant definitions


	// Data Structures

	// NWSMSD_MEDIA_LOCATION

typedef struct
{
	UINT32					  objectType;
	UINT32					  uniqueDeviceID;
	UINT32					  bay;
	UINT32					  slot;
} NWSMSD_MEDIA_LOCATION;


	// NWSMSD_OBJECT_STATUS

typedef struct
{
	UINT32					  objectStatus;			//0x00
	UINT32					  objectOperation;		//0x04
	UINT32					  objectMode;			//0x08
	UINT32					  numberOfSharedApps;	//0x0c
} NWSMSD_OBJECT_STATUS;


	// NWSMSD_MEDIA_STATUS

typedef struct
{
	UINT32	 				  mediaMounted;
	NWSMSD_OBJECT_STATUS	  status;
} NWSMSD_MEDIA_STATUS;


	// NWSMSD_DEVICE_STATUS

typedef struct
{
	UINT32					  numberOfSiblings;			//0x00
	UINT32					  reserved0;				//0x04
	UINT32					  reserved1;		//0x08
	UINT32					  reserved2;		//0x0c
	NWSMSD_OBJECT_STATUS	  status;					//0x10 - 0x1F
} NWSMSD_DEVICE_STATUS;


	// NWSMSD_SESSION_ID

typedef struct
{
	UINT32					  sessionDateAndTime;
	char					  sessionDescription[NWSM_MAX_DESCRIPTION_LEN];
	char					  sourceName[NWSM_MAX_TARGET_SRVC_NAME_LEN];
	char					  sourceType[NWSM_MAX_TARGET_SRVC_TYPE_LEN];
	char					  sourceVersion[NWSM_MAX_TARGET_SRVC_VER_LEN];
} NWSMSD_SESSION_ID;


	// CAPACITY

typedef struct
{
	UINT32					  factor;					//0x00
	UINT32					  value;					//0x04
} CAPACITY;


	// NWSMSD_DEVICE_ID

typedef struct
{
				// 0x00
	UINT32					  uniqueDeviceID;
	UINT32					  siblingUniqueDeviceID;
	UINT32					  deviceType;
	UINT32					  deviceRelation;
				// 0x10

	char					  deviceName[NWSM_MAX_DEVICE_LABEL_LEN];
				// 0x50
	NWSMSD_DEVICE_STATUS	  deviceStatus;
				// 0x70
	UINT32					  reservedStatus;
	NWBOOLEAN				  sequential;
	NWBOOLEAN				  removable;
	CAPACITY				  deviceCapacity;
				// 0x80
	UINT32					  unitSize;
	UINT32					  reserved0;
	UINT32					  reserved1;
	UINT32					  reserved2;
				// 0x90
	UINT32					  reserved3;
	UINT32					  reserved4;
				// 0x98
} NWSMSD_DEVICE_ID;


	// NWSMSD_MEDIA_ID

typedef struct
{
				// 0x00
	UINT32					  uniqueMediaID;
	UINT32					  mediaSetDateAndTime;
	UINT32					  mediaDateAndTime;
	UINT32					  mediaNumber;
				// 0x10
	BUFFER					  mediaLabel[NWSM_MAX_MEDIA_LABEL_LEN];
				// 0x50
	NWSMSD_MEDIA_STATUS		  mediaStatus;
	UINT32					  reserved0[3];
				// 0x70
	NWSMSD_MEDIA_LOCATION	  mediaLocation;
				// 0x80
	UINT32					  mediaOwner;
	UINT32					  reservedStatus;
	UINT32					  mediaType;
	NWBOOLEAN				  sequential;
	NWBOOLEAN				  removable;
				// 0x90
	UINT32					  unitSize;
	UINT32					  reserved1[3];
				// 0xA0
	CAPACITY				  totalCapacity;
	CAPACITY				  reserved2;
				// 0xB0
} NWSMSD_MEDIA_ID;


// Handles
	// NWSMSD_DEVICE_HANDLE

typedef	UINT32			NWSMSD_DEVICE_HANDLE;


	// NWSMSD_MEDIA_HANDLE

typedef	UINT32			NWSMSD_MEDIA_HANDLE;


	// NWSMSD_SESSION_HANDLE

typedef	UINT32			NWSMSD_SESSION_HANDLE;

	// NWSMSD_HEADER_BUFFER
typedef struct
{
 	UINT32					  bufferSize;
	UINT32					  headerSize;
	NWBOOLEAN				  reallocateOk;
	UINT32					  overflowSize;
	BUFFER					  headerBuffer[1];
} NWSMSD_HEADER_BUFFER;


	// NWSMSD_TRANS_BUF_POSITION

typedef struct
{
	UINT16					  mediaNumber;
	UINT16					  partitionNumber;
	UINT32					  sectorAddress;
} NWSMSD_TRANS_BUF_POSITION;


	// NWSMSD_MEDIA_POSITION

typedef struct
{
	UINT32					  partitionNumber;
	union
	{
		int		  relative;
		UINT32	  absolute;
	}						  sectorAddress;
	union
	{
		int		  sessionRelative;
		UINT32	  sessionAbsolute;
		UINT32	  mediaIndex;
	}						  number;
	NWSMSD_SESSION_ID		  sessionDesc;
	NWSMSD_SESSION_HANDLE	  sessionHandle;
	UINT32					  mediaNumber;
} NWSMSD_MEDIA_POSITION;


	// NWSMSD_CONTROL_BLOCK

typedef struct
{
	UINT32					  transferBufferState;			// 0x00
	NWSMSD_SESSION_HANDLE	  sessionHandle;				// 0x04
	UINT32					  transferBufferSequence;		// 0x08
	NWBOOLEAN				  finalTransferBuffer;			// 0x0C
	UINT16					  reservedVariable;				// 0x0E
	BUFFERPTR				  transferBuffer;				// 0x10
	UINT32					  transferBufferSizeAllocated;	// 0x14
	UINT32					  transferBufferSizeData;		// 0x18
	UINT32					  sessionDataType;				// 0x1C
	UINT32					  transferBufferDataOffset;		// 0x20
	UINT32					  bytesNotTransfered;			// 0x24
	UINT32					  bytesSpanned;					// 0x28
	NWSMSD_TRANS_BUF_POSITION beginningPosition;			// 0x2C
	NWSMSD_TRANS_BUF_POSITION endingPosition;				// 0x34
	UINT32					  completionStatus;				// 0x3C
} NWSMSD_CONTROL_BLOCK;


	// NWSMSD_DEVICE_LIST

typedef struct
{
	UINT32					  deviceTotalCount;
	UINT32					  deviceMaxCount;
	UINT32					  deviceResponseCount;
	UINT32					  uniqueDeviceID;
	NWSMSD_DEVICE_ID		  deviceID[1];
} NWSMSD_DEVICE_LIST;


	// NWSMSD_MEDIA_LIST

typedef struct
{
	UINT32					  mediaTotalCount;
	UINT32					  mediaMaxCount;
	UINT32					  mediaResponseCount;
	UINT32					  uniqueMediaID;
	NWSMSD_MEDIA_ID		  	  mediaID[];
} NWSMSD_MEDIA_LIST;


typedef struct
{
	UINT32					  sectorSize;
	UINT32					  maxTransferBufferSize;
	UINT16					  applicationAreaSize;
	UINT16					  applicationAreaOffset;
	UINT16					  transferBufferDataOffset;
} NWSMSD_TRANSFER_BUF_INFO;


	// NWSMSD_TIMEOUTS

typedef struct
{
		UINT32				  NWSMSDListMedia;
		UINT32				  NWSMSDSubjugateDevice;
		UINT32				  NWSMSDSubjugateMedia;
		UINT32				  NWSMSDDismountMedia;
		UINT32				  NWSMSDOpenSessionForWriting;
		UINT32				  NWSMSDOpenSessionForReading;
		UINT32				  NWSMSDCloseSession;
		UINT32				  NWSMSDWriteSessionData;
		UINT32				  NWSMSDReadSessionData;
		UINT32				  NWSMSDLabelMedia;
		UINT32				  NWSMSDDeleteMedia;
		UINT32				  NWSMSDPositionMedia;
		UINT32				  NWSMSDMoveMedia;
		UINT32				  NWSDMSFormatMedia;
} NWSMSD_TIMEOUTS;


	// NWSMSD_SDI_DEFAULTS

typedef struct
{
	NWSMSD_DEVICE_ID		  deviceDesc;
	NWSMSD_MEDIA_ID			  mediaDesc;
	NWSMSD_TIMEOUTS			  timeouts;
} NWSMSD_SDI_DEFAULTS;


// End of structure definitions

// Function prototypes

	// These first two are used in sdi.h, not down below

typedef
CCODE _NWSMSDConnectToSDI(
			STRING					  sdiName,
			STRING					  sdiUserName,
			void					 *reserved,
			UINT32					 *connectionID);

typedef
CCODE _NWSMSDReleaseSDI(
			UINT32					 *connectionID);



typedef
CCODE _NWSMSDListDevices(
			UINT32					  connection,
			NWSMSD_DEVICE_LIST		 *deviceList);



typedef
CCODE _NWSMSDListMedia(
			UINT32					  connection,
			NWSMSD_DEVICE_HANDLE	  deviceHandle,
			NWBOOLEAN				  reScan,
			void					 *reserved0,
			NWSMSD_MEDIA_LIST		 *mediaList,
			CCODE					 *completionStatus);



typedef
CCODE _NWSMSDSubjugateDevice(
			UINT32					  connection,
			NWSMSD_DEVICE_ID		 *deviceDesc,
			UINT32					  deviceReadWriteMode,
			NWSMSD_DEVICE_HANDLE	 *deviceHandle,
			CCODE					 *completionStatus);



typedef
CCODE _NWSMSDEmancipateDevice(
			UINT32					  connection,
			NWSMSD_DEVICE_HANDLE	 *deviceHandle);



typedef
CCODE _NWSMSDSubjugateMedia(
			UINT32					  connection,
			NWSMSD_MEDIA_ID			 *mediaDesc,
			UINT32					  mediaReadWriteMode,
			void					 *reserved0,

			NWSMSD_MEDIA_HANDLE		 *mediaHandle,
			CCODE					 *completionStatus);



typedef
CCODE _NWSMSDEmancipateMedia(
			UINT32					  connection,
			NWSMSD_MEDIA_HANDLE		 *mediaHandle);



typedef
CCODE _NWSMSDMountMedia(
			UINT32					  connection,
			NWSMSD_DEVICE_ID		 *deviceDesc,
			UINT32					  deviceReadWriteMode,
			NWSMSD_MEDIA_ID			 *mediaDesc,
			UINT32					  mediaReadWriteMode,
			void					 *reserved0,
			NWSMSD_DEVICE_HANDLE	 *deviceHandle,
			NWSMSD_MEDIA_HANDLE		 *mediaHandle,
			CCODE					 *completionStatus);



typedef
CCODE _NWSMSDDismountMedia(
			UINT32					  connection,
			NWSMSD_DEVICE_HANDLE	 *deviceHandle,
			NWSMSD_MEDIA_HANDLE		 *mediaHandle,
			UINT32					  dismountMode,
			NWSMSD_HEADER_BUFFER	 *mediaTrailerInfo,
			CCODE					 *completionStatus);


typedef
CCODE _NWSMSDOpenSessionForWriting(
	UINT32					  connection,
	NWSMSD_MEDIA_HANDLE		  mediaHandle,
	NWSMSD_HEADER_BUFFER	 *sessionHeaderInfo,
	NWSMSD_TRANSFER_BUF_INFO *transferBufferInfo,
	NWSMSD_SESSION_HANDLE	 *sessionHandle,
	CCODE					 *completionStatus);


typedef
CCODE _NWSMSDOpenSessionForReading(
	UINT32					  connection,
	NWSMSD_MEDIA_HANDLE		  mediaHandle,
	NWSMSD_SESSION_ID		 *sessionDesc,
	void					 *reserved0,
	NWSMSD_HEADER_BUFFER	 *sessionHeader,
	UINT32					 *sectorSize,
	UINT32					 *transferBufferSize,
	NWSMSD_SESSION_HANDLE	 *sessionHandle,
	CCODE					 *completionStatus);


typedef
CCODE _NWSMSDCloseSession(
	UINT32					  connection,
	NWSMSD_SESSION_HANDLE	 *sessionHandle,
	CCODE					 *completionStatus);


typedef
CCODE _NWSMSDWriteSessionData(
	UINT32					  connection,
	NWSMSD_CONTROL_BLOCK	 *controlBlock);


typedef
CCODE _NWSMSDReadSessionData(
		UINT32				  connection,
		NWSMSD_CONTROL_BLOCK *controlBlock);



typedef
CCODE _NWSMSDCancelDataTransfer(
		UINT32				  connection,
		NWSMSD_CONTROL_BLOCK *controlBlock);



typedef
CCODE _NWSMSDLabelMedia(
	UINT32					  connection,
	NWSMSD_MEDIA_HANDLE		  mediaHandle,
	NWSMSD_HEADER_BUFFER	 *mediaHeaderInfo,
	CCODE					 *completionStatus);



typedef
CCODE _NWSMSDFormatMedia(
	UINT32					  connection,
	NWSMSD_MEDIA_HANDLE		  mediaHandle,
	UINT32					  operationType,
	UINT32					  numberOfPartitions,
	CAPACITY				 *partitionSizeArray,
	CCODE					 *completionStatus);



typedef
CCODE _NWSMSDDeleteMedia(
	UINT32					  connection,
	NWSMSD_MEDIA_HANDLE		  mediaHandle,
	UINT32					  deleteMode,
	CCODE					 *completionStatus);



typedef
CCODE _NWSMSDReturnMediaHeader(
	UINT32					  connection,
	NWSMSD_MEDIA_HANDLE		  mediaHandle,
	NWBOOLEAN				  verifyHeader,
	NWSMSD_HEADER_BUFFER	 *mediaHeader,
	CCODE	  				 *completionStatus);



typedef
CCODE _NWSMSDPositionMedia(
	UINT32					  connection,
	NWSMSD_MEDIA_HANDLE		  mediaHandle,
	NWSMSD_MEDIA_POSITION	 *mediaPosition,
	UINT32					  positionMode,
	void					 *reserved0,
	CCODE					 *completionStatus);



typedef
CCODE _NWSMSDMoveMedia(
	UINT32					  connection,
	NWSMSD_MEDIA_HANDLE		  mediaHandle,
	UINT32					  moveMode,
	NWSMSD_MEDIA_LOCATION	 *mediaLocation,
	CCODE					 *completionStatus);



typedef
CCODE _NWSMSDGetDeviceStatus(
	UINT32					  connection,
	NWSMSD_DEVICE_HANDLE	  deviceHandle,
	NWSMSD_DEVICE_ID		 *deviceDesc,
	NWSMSD_DEVICE_STATUS	 *deviceStatus);



typedef
CCODE _NWSMSDGetMediaStatus(
	UINT32					  connection,
	NWSMSD_MEDIA_HANDLE		  mediaHandle,
	NWSMSD_MEDIA_ID			 *mediaDesc,
	NWSMSD_MEDIA_STATUS		 *mediaStatus);



typedef
CCODE _NWSMSDGetDeviceCharacteristics(
	UINT32					  connection,
	NWSMSD_DEVICE_HANDLE	  deviceHandle,
	NWSMSD_DEVICE_ID		 *deviceCharacteristics);



typedef
CCODE _NWSMSDGetMediaCharacteristics(
	UINT32					  connection,
	NWSMSD_MEDIA_HANDLE		  mediaHandle,
	NWSMSD_MEDIA_ID			 *mediaCharacteristics);



typedef
CCODE _NWSMSDLabelDevice(
	UINT32					  connection,
	NWSMSD_DEVICE_HANDLE	  deviceHandle,
	BUFFERPTR				  deviceLabel);


typedef
CCODE _NWSMSDSetReadSDIDefaults(
	UINT32					  connection,
	NWSMSD_SDI_DEFAULTS		 *sdiDefaults,
	NWBOOLEAN				  setReadMode);



typedef 
void _NWSMSDAlertRoutine(
				UINT32	  alertHandle,
				UINT32	  alertType,
				UINT32	  uniqueID,
				UINT32	  alertNumber,
				STRING	  alertString);

typedef
CCODE _NWSMSDRegisterAlertRoutine(
	UINT32					  connection,
	UINT32					  alertType,
	_NWSMSDAlertRoutine		 *alertRoutine);



typedef
CCODE _NWSMSDAlertResponse(
	UINT32					  connection,
	UINT32					  alertHandle,
	UINT32					  alertType,
	UINT32					  alertResponseValue);



typedef
CCODE _NWSMSDConvertValueToMessage(
	UINT32					  connection,
	UINT32					  valueType,
	UINT32					  value,
	UINT32					  stringSize,
	STRING					  string);



typedef
CCODE _NWSMSDConvertError(
	UINT32					  connection,
	CCODE					  errorNumber,
	STRING					  string);


// end of Function Prototypes


CCODE NWSMListSDIs(
		char						 *pattern,
		NWSM_NAME_LIST				**nameList);

CCODE GetSDIFunction(
		UINT32						  connectionID, 
		UINT16						  smspcode);

CCODE NWSMSDConnectToSDI(
		STRING						  sdiName,
		STRING						  sdiUserName,
		void						 *authentication,
		UINT32						 *connectionID);

CCODE NWSMSDReleaseSDI(
		UINT32						 *connectionID);

CCODE NWSMUnsupported(
		UINT32						  cp,
		...);

_NWSMSDListDevices 				NWSMSDListDevices;
_NWSMSDListMedia					NWSMSDListMedia;
_NWSMSDSubjugateDevice				NWSMSDSubjugateDevice;
_NWSMSDEmancipateDevice			NWSMSDEmancipateDevice;
_NWSMSDSubjugateMedia				NWSMSDSubjugateMedia;
_NWSMSDEmancipateMedia				NWSMSDEmancipateMedia;
_NWSMSDMountMedia					NWSMSDMountMedia;
_NWSMSDDismountMedia				NWSMSDDismountMedia;
_NWSMSDOpenSessionForWriting		NWSMSDOpenSessionForWriting;
_NWSMSDOpenSessionForReading		NWSMSDOpenSessionForReading;
_NWSMSDCloseSession				NWSMSDCloseSession;
_NWSMSDWriteSessionData			NWSMSDWriteSessionData;
_NWSMSDReadSessionData				NWSMSDReadSessionData;
_NWSMSDCancelDataTransfer			NWSMSDCancelDataTransfer;
_NWSMSDLabelMedia					NWSMSDLabelMedia;
_NWSMSDFormatMedia					NWSMSDFormatMedia;
_NWSMSDDeleteMedia					NWSMSDDeleteMedia;
_NWSMSDReturnMediaHeader			NWSMSDReturnMediaHeader;
_NWSMSDPositionMedia				NWSMSDPositionMedia;
_NWSMSDMoveMedia					NWSMSDMoveMedia;
_NWSMSDGetDeviceStatus				NWSMSDGetDeviceStatus;
_NWSMSDGetMediaStatus				NWSMSDGetMediaStatus;
_NWSMSDGetDeviceCharacteristics	NWSMSDGetDeviceCharacteristics;
_NWSMSDGetMediaCharacteristics		NWSMSDGetMediaCharacteristics;
_NWSMSDLabelDevice					NWSMSDLabelDevice;
_NWSMSDSetReadSDIDefaults			NWSMSDSetReadSDIDefaults;
_NWSMSDRegisterAlertRoutine		NWSMSDRegisterAlertRoutine;
_NWSMSDAlertResponse				NWSMSDAlertResponse;
_NWSMSDConvertValueToMessage		NWSMSDConvertValueToMessage;
_NWSMSDConvertError				NWSMSDConvertError;

#endif					   
/***************************************************************************/

 
 
