/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/smstypes.h	1.2"

#if !defined(_SMSTYPES_H_INCLUDED)
#define _SMSTYPES_H_INCLUDED

#if !defined(TRUE)
#define TRUE  1
#define FALSE 0
#endif

#define  KILO  1024
#define  MEGA  (KILO * KILO)
#define  GIGA  (MEGA * KILO)
#define  TERA  (GIGA * KILO)

#if !defined(_nwtypes_) && !defined(_NWTYPES_H_INCLUDED_) && !defined(UNIX)
	typedef short            NWBOOLEAN;
	typedef unsigned long    CCODE;
	typedef char             INT8;
	typedef short            INT16;
	typedef long             INT32;
	typedef unsigned char    UINT8;
	typedef unsigned short   UINT16;
	typedef unsigned long    UINT32;

#define _nwtypes_
#define _NWTYPES_H_INCLUDED_
#endif

#if defined(__TURBOC__) || defined(MSC)
#define HUGE             huge
	typedef char             BUFFER;
	typedef char HUGE       *BUFFERPTR;
#if !defined(OS2_INCLUDED)
		typedef char             CHAR;
#endif
	typedef char HUGE       *LSTRING;
	typedef char            *PSTRING;
	typedef char HUGE       *STRING;
#endif

#if defined(NLM) || defined(__WATCOMC__) 
#define HUGE
	typedef unsigned char    BUFFER;
	typedef unsigned char   *BUFFERPTR;
	typedef unsigned char    CHAR;
	typedef unsigned char   *LSTRING;
	typedef unsigned char   *PSTRING;
	typedef unsigned char   *STRING;
#endif
#if defined(UNIX)
#define HUGE
	typedef short            NWBOOLEAN;
	typedef unsigned int    CCODE;
	typedef char             INT8;
	typedef short            INT16;
	typedef int              INT32;
	typedef unsigned char    UINT8;
	typedef unsigned short   UINT16;
	typedef unsigned int  	 UINT32;
	typedef char    BUFFER;
	typedef char   *BUFFERPTR;

	typedef char    CHAR;
	typedef char   *LSTRING;
	typedef char   *PSTRING;
	typedef char   *STRING;
	typedef UINT32           NWBOOLEAN32;
#endif

	typedef struct
	{
   	UINT32 v[2];
	} UINT64;

typedef struct
{
//   int     type:4;
//   int     timeZone:12;
    UINT16  typeAndTimeZone;
   INT16   year;
   UINT8   month;
   UINT8   day;
   UINT8   hour;
   UINT8   minute;
   UINT8   second;
   UINT8   centiSecond;
   UINT8   hundredsOfMicroseconds;
   UINT8   microSeconds;
    UINT32  reserved;
} ECMATime;

	typedef struct
	{
		UINT16
			size,
			buffer[1];
	} UINT16_BUFFER;

	typedef struct
	{
		UINT16
			size;

		char
			string[1];
	} STRING_BUFFER;

	typedef struct
	{
		UINT16	bufferSize;												// 0x00
		UINT16	dataSetNameSize;										// 0x02
		UINT8		nameSpaceCount;										// 0x04
		UINT8		keyInformationSize;									// 0x05
		UINT8		keyInformation[1];									// 0x06
	} NWSM_DATA_SET_NAME_LIST;
	// UINT32	nameSpaceType;
	// UINT32	reserved;
	// UINT8		count;
	// UINT16	namePositions[count];
	// UINT16	separatorPositions[count];
	// UINT16	nameLength;
	// UINT8		name[nameLength + 1];

	typedef struct
	{
		UINT16	bufferSize;												// 0x00
		UINT16	scanControlSize;										// 0x02
		UINT32	scanType;												// 0x04
		UINT32	firstAccessDateAndTime;									// 0x08
		UINT32	lastAccessDateAndTime;									// 0x0C
		UINT32	firstCreateDateAndTime;									// 0x10
		UINT32	lastCreateDateAndTime;									// 0x14
		UINT32	firstModifiedDateAndTime;								// 0x18
		UINT32	lastModifiedDateAndTime;								// 0x1C
		UINT32	firstArchivedDateAndTime;								// 0x20
		UINT32	lastArchivedDateAndTime;								// 0x24
		UINT8	returnChildTerminalNodeNameOnly;						// 0x28
		UINT8	parentsOnly;											// 0x29
		UINT8	childrenOnly;											// 0x2A
		UINT8	createSkippedDataSetsFile;								// 0x2B
		UINT8	generateCRC;											// 0x2C
		UINT8	returnNFSHardLinksInDataSetName;						// 0x2D
		UINT8	reserved[6];											// 0x2E
		UINT32	scanChildNameSpaceType;									// 0x34
		UINT32	returnNameSpaceType;									// 0x38
		UINT8	callScanFilter;											// 0x3C
		UINT16	otherInformationSize;									// 0x3D
		UINT8	otherInformation[1];									// 0x3F
	} NWSM_SCAN_CONTROL;

	typedef struct
	{
		UINT16	bufferSize;												// 0x00
		UINT16	scanInformationSize;									// 0x02
		UINT32	attributes;												// 0x04
		UINT32	creatorID;												// 0x08
		UINT32	creatorNameSpaceNumber;									// 0x0C
		UINT32	primaryDataStreamSize;									// 0x10
		UINT32	totalStreamsDataSize;									// 0x14
		UINT8	modifiedFlag;											// 0x18
		UINT8	deletedFlag;											// 0x19
		UINT8	parentFlag;												// 0x1A
		UINT8	reserved[5];											// 0x1B
		UINT32	accessDateAndTime;										// 0x20
		UINT32	createDateAndTime;										// 0x24
		UINT32	modifiedDateAndTime;									// 0x28
		UINT32	archivedDateAndTime;									// 0x2C
		UINT16	otherInformationSize;									// 0x30
		UINT8	otherInformation[1];									// 0x32
	} NWSM_SCAN_INFORMATION;

	typedef struct
	{
		UINT16	bufferSize;												// 0x00
		UINT16	selectionListSize;										// 0x02
		UINT8	selectionCount; 										// 0x04
		UINT8	keyInformationSize;										// 0x05
		UINT8	keyInformation[1];										// 0x06
	} NWSM_SELECTION_LIST;
	//	UINT32	selectionNameSpaceType;
	//	UINT32	selectionType;
	//	UINT8	count;
	//	UINT16	namePositions[count];
	//	UINT16	separatorPositions[count];
	//	UINT16	nameLength;
	//	UINT8	name[nameLength + 1];

	typedef struct
	{
		char	
			moduleFileName[256];

		UINT8
			moduleMajorVersion,
			moduleMinorVersion;

		UINT16
			moduleRevisionLevel;

		char
			baseOS[64];

		UINT8
			baseOSMajorVersion,
			baseOSMinorVersion;

		UINT16
			baseOSRevisionLevel;
	} 
	NWSM_MODULE_VERSION_INFO;

	typedef struct _NWSM_NAME_LIST
	{
		struct _NWSM_NAME_LIST *next;									// 0x00
		STRING	name;													// 0x04
		void   *other_info;
	} NWSM_NAME_LIST;



#define NWSM_NETWORK_CONNECTION     0
#define NWSM_NAME_PASSWORD_PAIR     1


typedef struct                         // this is the header of a generic,
{                                      // length-preceeded array of bytes
        UINT32     length;
        INT8       stream[4];

} NWSM_LongByteStream;




#endif

