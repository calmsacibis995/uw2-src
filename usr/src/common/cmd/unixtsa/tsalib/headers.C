#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/headers.C	1.2"

#include                <stdio.h>
#include                <string.h>
#include                <smstypes.h>
#include                <smsutapi.h>
#include                <smsfids.h>
#include		"tsalib.h"
#ifndef UNIX
#include <dos.h>
#endif

STATIC CCODE PutFields(NWSM_FIELD_TABLE_DATA table[], BUFFERPTR *buffer,
		UINT32 *bufferSize, UINT32 *bufferData, UINT32 crcFlag);

STATIC CCODE GetHeader(BUFFERPTR *buffer, UINT32 *bufferSize,
		UINT32 *headFID, UINT32 *recordSize, UINT32 *archiveDateAndTime,
		UINT32 *headerSize);

#define LinkData(t, i, s, d)	(t[i].field.data = (d), t[i].dataSizeMap = t[i].sizeOfData = s)
#define Terminate(t, i)		if (t[i].found) *((BUFFERPTR)(t[i].data) + t[i].dataSize) = 0

#if defined(__TURBOC__) || defined(MSC)	
// from NLMSDK/INCLUDE/DOS.H
	struct _DOSTime
  	{
   	unsigned short bisecond : 5;          /* two second increments (0 - 29)*/
   	unsigned short minute   : 6;          /* 0 - 59                        */
   	unsigned short hour     : 5;          /* 0 - 23                        */
  	};

	struct _DOSDate
  	{
   	unsigned short day          : 5;      /* 1 - 31                        */
   	unsigned short month        : 4;      /* 1 - 12                        */
   	unsigned short yearsSince80 : 7;      /* years since 1980 (0 - 119)    */
  	};
#endif

#define DATE_TIME_INDEX			0
#define MEDIA_DATE_TIME_INDEX		1
#define LABEL_INDEX				2
#define NUMBER_INDEX				3
#define DOS_DATE_TIME_INDEX		4
#define DOS_MEDIA_DATE_TIME_INDEX	5
#define SET_MEDIA_HEADER_END	4
#define GET_MEDIA_HEADER_END	6

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMSetMediaHeaderInfo"
#endif
#ifdef UNIX
static void InitSetMediaHeaderInfo(
	NWSM_FIELD_TABLE_DATA *mediaHeaderTable,
	UINT8	tableSize)
{
	memset(mediaHeaderTable,'\0',sizeof(NWSM_FIELD_TABLE_DATA) * tableSize);
	if ( tableSize < SET_MEDIA_HEADER_END + 1 )
		return ;
	mediaHeaderTable[DATE_TIME_INDEX].field.fid = NWSM_MS_OPEN_DATE_TIME_CAL ;
	mediaHeaderTable[MEDIA_DATE_TIME_INDEX].field.fid = NWSM_MEDIA_OPEN_DATE_TIME_CAL ;
	mediaHeaderTable[LABEL_INDEX].field.fid = NWSM_MEDIA_SET_LABEL ;
	mediaHeaderTable[NUMBER_INDEX].field.fid = NWSM_MEDIA_NUMBER ;
	mediaHeaderTable[SET_MEDIA_HEADER_END].field.fid = NWSM_END ;
}
#endif

/*
   This routine accepts DOS date/time format in the mediaDateAndTime field,
	and converts it to calender format before placing it in the buffer.
*/

	CCODE NWSMSetMediaHeaderInfo(
		NWSM_MEDIA_INFO		 *mediaInfo,
		BUFFERPTR				  buffer,
		UINT32					  bufferSize,
		UINT32					 *headerSize)
{
	CCODE ccode = 0;
	UINT32 calenderDateTime, mediaCalenderDateTime;
	UINT16 mediaNumber ;
#ifndef UNIX
	NWSM_FIELD_TABLE_DATA mediaHeaderTable[] =
	{
		{ { NWSM_MS_OPEN_DATE_TIME_CAL, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_MEDIA_OPEN_DATE_TIME_CAL, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_MEDIA_SET_LABEL, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_MEDIA_NUMBER, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_END } }
	};
#else
	NWSM_FIELD_TABLE_DATA mediaHeaderTable[SET_MEDIA_HEADER_END + 1] ;

	InitSetMediaHeaderInfo(mediaHeaderTable, SET_MEDIA_HEADER_END + 1) ;
#endif

#ifdef UNIX
	calenderDateTime = mediaInfo->mediaSetDateAndTime;
#else
	calenderDateTime = _ConvertDOSTimeToCalendar(mediaInfo->mediaSetDateAndTime);
#endif
	calenderDateTime = SwapUINT32(calenderDateTime);
	LinkData(mediaHeaderTable, DATE_TIME_INDEX, sizeof(UINT32),
			&calenderDateTime);
#ifdef UNIX
	mediaCalenderDateTime = mediaInfo->mediaDateAndTime;
#else
	mediaCalenderDateTime = _ConvertDOSTimeToCalendar(mediaInfo->mediaDateAndTime);
#endif
	mediaCalenderDateTime = SwapUINT32(mediaCalenderDateTime);
	LinkData(mediaHeaderTable, MEDIA_DATE_TIME_INDEX, sizeof(UINT32),
			&mediaCalenderDateTime);
	LinkData(mediaHeaderTable, LABEL_INDEX, strlen(mediaInfo->mediaSetLabel),
			mediaInfo->mediaSetLabel);

	mediaNumber = mediaInfo->mediaNumber ;
	mediaNumber = SwapUINT16(mediaNumber);
	LinkData(mediaHeaderTable, NUMBER_INDEX, sizeof(UINT16),
			&mediaNumber);

	*headerSize = 0;
	if ((ccode = PutFields(mediaHeaderTable, &buffer, &bufferSize, headerSize, CRC_NO))
			isnt 0)
		goto Return;

Return:
	return (ccode);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMGetMediaHeaderInfo"
#endif
#ifdef UNIX
static void InitGetMediaHeaderInfo(
	NWSM_GET_FIELDS_TABLE *mediaHeaderTable,
	UINT8	tableSize)
{  
	
	memset(mediaHeaderTable,'\0',sizeof(NWSM_GET_FIELDS_TABLE) * tableSize);
	if (tableSize < GET_MEDIA_HEADER_END + 1)
		return ;
	mediaHeaderTable[DATE_TIME_INDEX].fid = NWSM_MS_OPEN_DATE_TIME_CAL ;
	mediaHeaderTable[DATE_TIME_INDEX].dataSize = sizeof(UINT32) ;
	mediaHeaderTable[DATE_TIME_INDEX].found = FALSE ;

	mediaHeaderTable[MEDIA_DATE_TIME_INDEX].fid = NWSM_MEDIA_OPEN_DATE_TIME_CAL ;
	mediaHeaderTable[MEDIA_DATE_TIME_INDEX].dataSize = sizeof(UINT32) ;
	mediaHeaderTable[MEDIA_DATE_TIME_INDEX].found = FALSE ;

	mediaHeaderTable[LABEL_INDEX].fid = NWSM_MEDIA_SET_LABEL;
	mediaHeaderTable[LABEL_INDEX].dataSize = NWSM_MAX_MEDIA_LABEL_LEN - 1 ;
	mediaHeaderTable[LABEL_INDEX].found = FALSE ;

	mediaHeaderTable[NUMBER_INDEX].fid = NWSM_MEDIA_NUMBER ;
	mediaHeaderTable[NUMBER_INDEX].dataSize = sizeof(UINT16) ;
	mediaHeaderTable[NUMBER_INDEX].found = FALSE ;

	mediaHeaderTable[DOS_DATE_TIME_INDEX].fid = NWSM_MS_OPEN_DATE_TIME ;
	mediaHeaderTable[DOS_DATE_TIME_INDEX].dataSize = sizeof(UINT32) ;
	mediaHeaderTable[DOS_DATE_TIME_INDEX].found = FALSE ;

	mediaHeaderTable[DOS_MEDIA_DATE_TIME_INDEX].fid = NWSM_MEDIA_OPEN_DATE_TIME ;
	mediaHeaderTable[DOS_MEDIA_DATE_TIME_INDEX].dataSize = sizeof(UINT32) ;
	mediaHeaderTable[DOS_MEDIA_DATE_TIME_INDEX].found = FALSE ;

	mediaHeaderTable[GET_MEDIA_HEADER_END].fid = NWSM_END ;
}
#endif

/*
   This routine may read calender date/time format from media, but the
   mediaSetDateAndTime in mediaInfo will always be DOS date/time format.
*/

CCODE NWSMGetMediaHeaderInfo(
		BUFFERPTR			  headerBuffer,
		UINT32				  headerBufferSize,
		NWSM_MEDIA_INFO	 *mediaInfo)
{

	CCODE ccode = 0;
	UINT32 calenderDateTime, mediaCalenderDateTime;
	UINT16 mediaNumber ;
#ifndef UNIX
	NWSM_GET_FIELDS_TABLE mediaHeaderTable[] =
	{
		{NWSM_MS_OPEN_DATE_TIME_CAL, NULL, 4, FALSE },
		{NWSM_MEDIA_OPEN_DATE_TIME_CAL, NULL, 4, FALSE },
		{NWSM_MEDIA_SET_LABEL, NULL, NWSM_MAX_MEDIA_LABEL_LEN - 1, FALSE },
		{NWSM_MEDIA_NUMBER, NULL, 2, FALSE },
		{NWSM_MS_OPEN_DATE_TIME, NULL, 4, FALSE },
		{NWSM_MEDIA_OPEN_DATE_TIME, NULL, 4, FALSE },
		{NWSM_END }
	};
#else
	NWSM_GET_FIELDS_TABLE mediaHeaderTable[GET_MEDIA_HEADER_END + 1] ;
	
	InitGetMediaHeaderInfo(mediaHeaderTable, GET_MEDIA_HEADER_END + 1);
#endif

	mediaHeaderTable[DATE_TIME_INDEX].data = 	&calenderDateTime;
	mediaHeaderTable[MEDIA_DATE_TIME_INDEX].data = 	&mediaCalenderDateTime;
	mediaHeaderTable[LABEL_INDEX].data =		 mediaInfo->mediaSetLabel;
	mediaNumber = 0;			// this is a UINT32, GetFields will only copy a UINT16	
	mediaHeaderTable[NUMBER_INDEX].data = 		&mediaNumber;
	mediaHeaderTable[DOS_DATE_TIME_INDEX].data = 	&mediaInfo->mediaSetDateAndTime;
	mediaHeaderTable[DOS_MEDIA_DATE_TIME_INDEX].data = 	&mediaInfo->mediaDateAndTime;

	if ((ccode = SMDFGetFields(NWSM_MEDIA_HEADER, mediaHeaderTable,
			&headerBuffer, &headerBufferSize)) isnt 0)
		goto Return;

	Terminate(mediaHeaderTable, LABEL_INDEX);
	if (mediaHeaderTable[DATE_TIME_INDEX].found)
	{
		// unportable, may change RBC
		calenderDateTime = SwapUINT32(calenderDateTime);
#ifdef UNIX
		mediaInfo->mediaSetDateAndTime = calenderDateTime;
#else
		_ConvertTimeToDOS(calenderDateTime, (struct _DOSDate *)(((UINT16 *)&mediaInfo->mediaSetDateAndTime) + 1),
			(struct _DOSTime *)&mediaInfo->mediaSetDateAndTime);
#endif
	}

	if (mediaHeaderTable[MEDIA_DATE_TIME_INDEX].found)
	{
		// unportable, may change RBC
		mediaCalenderDateTime = SwapUINT32(mediaCalenderDateTime);
#ifdef UNIX
		mediaInfo->mediaDateAndTime = mediaCalenderDateTime ;
#else
		_ConvertTimeToDOS(mediaCalenderDateTime, (struct _DOSDate *)(((UINT16 *)&mediaInfo->mediaDateAndTime) + 1),
			(struct _DOSTime *)&mediaInfo->mediaDateAndTime);
#endif
	}

	mediaInfo->mediaNumber = 0 ;
	mediaInfo->mediaNumber = (UINT32)SwapUINT16(mediaNumber);

Return:
	return (ccode);

}
#undef DATE_TIME_INDEX
#undef MEDIA_DATE_TIME_INDEX
#undef LABEL_INDEX
#undef NUMBER_INDEX
#undef DOS_DATE_TIME_INDEX
#undef DOS_MEDIA_DATE_TIME_INDEX
#undef SET_MEDIA_HEADER_END	
#undef GET_MEDIA_HEADER_END

#define DATE_TIME_INDEX		 0
#define DESCRIPTION_INDEX	 1
#define SW_NAME_INDEX		 2
#define SW_TYPE_INDEX		 3
#define SW_VERSION_INDEX	 4
#define NAME_INDEX			 5
#define TYPE_INDEX			 6
#define VERSION_INDEX		 7
#define SESSION_HEADER_END		 8

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMSetSessionHeaderInfo"
#endif
#ifdef UNIX
static void InitSetSessionHeaderInfo(
	NWSM_FIELD_TABLE_DATA *sessionHeaderTable,
	UINT8	tableSize)
{
	memset(sessionHeaderTable,'\0',sizeof(NWSM_FIELD_TABLE_DATA) * tableSize);
	if ( tableSize < SESSION_HEADER_END + 1 )
		return ;
	sessionHeaderTable[DATE_TIME_INDEX].field.fid = NWSM_SESSION_DATE_TIME ;
	sessionHeaderTable[DESCRIPTION_INDEX].field.fid = NWSM_SESSION_DESCRIPTION ;
	sessionHeaderTable[SW_NAME_INDEX].field.fid = NWSM_SOFTWARE_NAME ;
	sessionHeaderTable[SW_TYPE_INDEX].field.fid = NWSM_SOFTWARE_TYPE ;
	sessionHeaderTable[SW_VERSION_INDEX].field.fid = NWSM_SOFTWARE_VERSION ;
	sessionHeaderTable[NAME_INDEX].field.fid = NWSM_SOURCE_NAME ;
	sessionHeaderTable[TYPE_INDEX].field.fid = NWSM_SOURCE_TYPE ;
	sessionHeaderTable[VERSION_INDEX].field.fid = NWSM_SOURCE_VERSION ;
	sessionHeaderTable[SESSION_HEADER_END].field.fid = NWSM_END ;
}
#endif

CCODE NWSMSetSessionHeaderInfo(
		NWSM_SESSION_INFO	 *sessionInfo,
		BUFFERPTR			  buffer,
		UINT32				  bufferSize,
		UINT32				 *headerSize)
{
	CCODE ccode = 0;
	UINT32 sessionDateTime ;
#ifndef UNIX
	NWSM_FIELD_TABLE_DATA sessionHeaderTable[] =
	{
		{ { NWSM_SESSION_DATE_TIME, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_SESSION_DESCRIPTION, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_SOFTWARE_NAME, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_SOFTWARE_TYPE, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_SOFTWARE_VERSION, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_SOURCE_NAME, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_SOURCE_TYPE, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_SOURCE_VERSION, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_END } }
	};
#else
	NWSM_FIELD_TABLE_DATA sessionHeaderTable[SESSION_HEADER_END + 1] ;

	InitSetSessionHeaderInfo(sessionHeaderTable,SESSION_HEADER_END + 1) ;
#endif

	sessionDateTime = SwapUINT32(sessionInfo->sessionDateAndTime);
	LinkData(sessionHeaderTable, DATE_TIME_INDEX, sizeof(UINT32),
			&sessionDateTime);
	LinkData(sessionHeaderTable, DESCRIPTION_INDEX, strlen(
			sessionInfo->sessionDescription), sessionInfo->sessionDescription);
	LinkData(sessionHeaderTable, SW_NAME_INDEX,
			strlen(sessionInfo->softwareName), sessionInfo->softwareName);
	LinkData(sessionHeaderTable, SW_TYPE_INDEX,
			strlen(sessionInfo->softwareType), sessionInfo->softwareType);
	LinkData(sessionHeaderTable, SW_VERSION_INDEX, strlen(sessionInfo->
			softwareVersion), sessionInfo->softwareVersion);
	LinkData(sessionHeaderTable, NAME_INDEX, strlen(sessionInfo->sourceName),
			sessionInfo->sourceName);
	LinkData(sessionHeaderTable, TYPE_INDEX, strlen(sessionInfo->sourceType),
			sessionInfo->sourceType);
	LinkData(sessionHeaderTable, VERSION_INDEX, strlen(
			sessionInfo->sourceVersion), sessionInfo->sourceVersion);

	*headerSize = 0;
	if ((ccode = PutFields(sessionHeaderTable, &buffer, &bufferSize, headerSize,
		CRC_NO)) isnt 0)
		goto Return;


Return:
	return (ccode);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMGetSessionHeaderInfo"
#endif
#ifdef UNIX
static void InitGetSessionHeaderInfo(
	NWSM_GET_FIELDS_TABLE *sessionHeaderTable,
	UINT8	tableSize)
{
	memset(sessionHeaderTable,'\0',sizeof(NWSM_GET_FIELDS_TABLE) * tableSize);
	if ( tableSize < SESSION_HEADER_END + 1	)
		return ;

	sessionHeaderTable[DATE_TIME_INDEX].fid = NWSM_SESSION_DATE_TIME ;
	sessionHeaderTable[DATE_TIME_INDEX].dataSize = sizeof(UINT32) ;
	sessionHeaderTable[DATE_TIME_INDEX].found = FALSE ;

	sessionHeaderTable[DESCRIPTION_INDEX].fid = NWSM_SESSION_DESCRIPTION ;
	sessionHeaderTable[DESCRIPTION_INDEX].dataSize = NWSM_MAX_DESCRIPTION_LEN - 1 ;
	sessionHeaderTable[DESCRIPTION_INDEX].found = FALSE ;

	sessionHeaderTable[SW_NAME_INDEX].fid = NWSM_SOFTWARE_NAME ;
	sessionHeaderTable[SW_NAME_INDEX].dataSize = NWSM_MAX_SOFTWARE_NAME_LEN - 1 ;
	sessionHeaderTable[SW_NAME_INDEX].found = FALSE ;

	sessionHeaderTable[SW_TYPE_INDEX].fid = NWSM_SOFTWARE_TYPE ;
	sessionHeaderTable[SW_TYPE_INDEX].dataSize = NWSM_MAX_SOFTWARE_TYPE_LEN - 1 ;
	sessionHeaderTable[SW_TYPE_INDEX].found = FALSE ;

	sessionHeaderTable[SW_VERSION_INDEX].fid = NWSM_SOFTWARE_VERSION ;
	sessionHeaderTable[SW_VERSION_INDEX].dataSize =  NWSM_MAX_SOFTWARE_VER_LEN - 1 ;
	sessionHeaderTable[SW_VERSION_INDEX].found = FALSE ;

	sessionHeaderTable[NAME_INDEX].fid = NWSM_SOURCE_NAME ;
	sessionHeaderTable[NAME_INDEX].dataSize = NWSM_MAX_TARGET_SRVC_NAME_LEN - 1 ;
	sessionHeaderTable[NAME_INDEX].found = FALSE ;

	sessionHeaderTable[TYPE_INDEX].fid = NWSM_SOURCE_TYPE ;
	sessionHeaderTable[TYPE_INDEX].dataSize = NWSM_MAX_TARGET_SRVC_TYPE_LEN - 1 ;
	sessionHeaderTable[TYPE_INDEX].found = FALSE ;

	sessionHeaderTable[VERSION_INDEX].fid = NWSM_SOURCE_VERSION ;
	sessionHeaderTable[VERSION_INDEX].dataSize = NWSM_MAX_TARGET_SRVC_VER_LEN - 1 ;
	sessionHeaderTable[VERSION_INDEX].found = FALSE ;

	sessionHeaderTable[SESSION_HEADER_END].fid = NWSM_END ;
}
#endif

CCODE NWSMGetSessionHeaderInfo(
		BUFFERPTR					  headerBuffer,
		UINT32						  headerBufferSize,
		NWSM_SESSION_INFO			 *sessionInfo)
{
	CCODE ccode = 0;
#ifndef UNIX
	NWSM_GET_FIELDS_TABLE sessionHeaderTable[] =
	{
		{ NWSM_SESSION_DATE_TIME, NULL, 4, FALSE },
		{ NWSM_SESSION_DESCRIPTION, NULL, NWSM_MAX_DESCRIPTION_LEN - 1, FALSE },
		{ NWSM_SOFTWARE_NAME, NULL, NWSM_MAX_SOFTWARE_NAME_LEN - 1, FALSE },
		{ NWSM_SOFTWARE_TYPE, NULL, NWSM_MAX_SOFTWARE_TYPE_LEN - 1, FALSE },
		{ NWSM_SOFTWARE_VERSION, NULL, NWSM_MAX_SOFTWARE_VER_LEN - 1, FALSE },
		{ NWSM_SOURCE_NAME, NULL, NWSM_MAX_TARGET_SRVC_NAME_LEN - 1, FALSE },
		{ NWSM_SOURCE_TYPE, NULL, NWSM_MAX_TARGET_SRVC_TYPE_LEN - 1, FALSE },
		{ NWSM_SOURCE_VERSION, NULL, NWSM_MAX_TARGET_SRVC_VER_LEN - 1, FALSE },
		{ NWSM_END }
	};
#else
	NWSM_GET_FIELDS_TABLE sessionHeaderTable[SESSION_HEADER_END + 1] ;

	InitGetSessionHeaderInfo(sessionHeaderTable, SESSION_HEADER_END + 1) ;
#endif

	sessionHeaderTable[DATE_TIME_INDEX].data = 	&sessionInfo->sessionDateAndTime;
	sessionHeaderTable[DESCRIPTION_INDEX].data = sessionInfo->sessionDescription;
	sessionHeaderTable[SW_NAME_INDEX].data = 	sessionInfo->softwareName;
	sessionHeaderTable[SW_TYPE_INDEX].data = 	sessionInfo->softwareType;
	sessionHeaderTable[SW_VERSION_INDEX].data = sessionInfo->softwareVersion;
	sessionHeaderTable[NAME_INDEX].data = 			sessionInfo->sourceName;
	sessionHeaderTable[TYPE_INDEX].data = 			sessionInfo->sourceType;
	sessionHeaderTable[VERSION_INDEX].data = 		sessionInfo->sourceVersion;

	if ((ccode = SMDFGetFields(NWSM_SESSION_HEADER, sessionHeaderTable,
			&headerBuffer, &headerBufferSize)) isnt 0)
		goto Return;

	sessionInfo->sessionDateAndTime = 
		SwapUINT32(sessionInfo->sessionDateAndTime);

	Terminate(sessionHeaderTable, DESCRIPTION_INDEX);
	Terminate(sessionHeaderTable, SW_NAME_INDEX);
	Terminate(sessionHeaderTable, SW_TYPE_INDEX);
	Terminate(sessionHeaderTable, SW_VERSION_INDEX);
	Terminate(sessionHeaderTable, NAME_INDEX);
	Terminate(sessionHeaderTable, TYPE_INDEX);
	Terminate(sessionHeaderTable, VERSION_INDEX);

Return:
	return (ccode);

}

#undef DATE_TIME_INDEX		 
#undef DESCRIPTION_INDEX	 
#undef SW_NAME_INDEX		 
#undef SW_TYPE_INDEX		 
#undef SW_VERSION_INDEX	 
#undef NAME_INDEX			 
#undef TYPE_INDEX			 
#undef VERSION_INDEX		 
#undef SESSION_HEADER_END		 

#define RECORD_HEADER_INDEX	0
#define FID_INDEX			1
#define DATE_TIME_INDEX	2
#define SIZE_INDEX			3
#define RECORD_END_INDEX	4

/* directoryInformation */
#define BEGIN_INDEX				0
#define DIRECTORY_HEADER_INDEX	1
#define OFFSET_TO_END_INDEX		2
#define ATTRIBUTES_INDEX		3
#define CREATOR_ID_INDEX		4 
#define CREATOR_NS_INDEX		5 
#define PRIM_DS_SIZE_INDEX		6 
#define TOTAL_DS_SIZE_INDEX	7 
#define MODIFY_FLAG_INDEX		8 
#define DELETE_FLAG_INDEX		9 
#define PARENT_FLAG_INDEX		10 
#define ACC_DATE_TIME_INDEX	11
#define CREAT_DATE_TIME_INDEX  12
#define MOD_DATE_TIME_INDEX	13
#define ARC_DATE_TIME_INDEX	14
#define OTHER_INFO_INDEX	    15
#define DS_NAME_INDEX		    16
#define DIRECTORY_END_INDEX				17

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMSetRecordHeader"
#endif
#ifdef UNIX
static void InitRecordHeaderTable(
	NWSM_FIELD_TABLE_DATA *recordHeaderTable,
	UINT8	tableSize)
{
	memset(recordHeaderTable,'\0',sizeof(NWSM_FIELD_TABLE_DATA) * tableSize);
	if ( tableSize < RECORD_END_INDEX + 1 )
		return ;

	recordHeaderTable[RECORD_HEADER_INDEX].field.fid = NWSM_BEGIN ;
	recordHeaderTable[FID_INDEX].field.fid = NWSM_DATA_RECORD_HEADER ;
	recordHeaderTable[DATE_TIME_INDEX].field.fid = NWSM_ARCHIVE_DATE_TIME, 

	recordHeaderTable[SIZE_INDEX].field.fid = NWSM_RECORD_SIZE ;
	recordHeaderTable[SIZE_INDEX].sizeOfData = sizeof(UINT32) ;
	recordHeaderTable[SIZE_INDEX].addressOfData = GET_ADDRESS ;
	recordHeaderTable[SIZE_INDEX].dataSizeMap = sizeof(UINT32) ;

	recordHeaderTable[RECORD_END_INDEX].field.fid = NWSM_END ;
}

static void InitDirectoryInformation(
	NWSM_FIELD_TABLE_DATA *directoryInformation,
	UINT8	tableSize)
{
	memset(directoryInformation,'\0',sizeof(NWSM_FIELD_TABLE_DATA) * tableSize);
	if ( tableSize < DIRECTORY_END_INDEX + 1 )
		return ;

	directoryInformation[BEGIN_INDEX].field.fid = NWSM_BEGIN ;
	directoryInformation[DIRECTORY_HEADER_INDEX].field.fid = NWSM_DIRECTORY_INFORMATION ;
	directoryInformation[OFFSET_TO_END_INDEX].field.fid = NWSM_OFFSET_TO_END ;
	directoryInformation[OFFSET_TO_END_INDEX].sizeOfData = 8 ;
	directoryInformation[OFFSET_TO_END_INDEX].addressOfData = GET_ADDRESS ;
	directoryInformation[OFFSET_TO_END_INDEX].dataSizeMap = 8 ;
	directoryInformation[ATTRIBUTES_INDEX].field.fid = NWSM_SCAN_INFO_ATTRIBUTES ;
	directoryInformation[CREATOR_ID_INDEX].field.fid = NWSM_SCAN_INFO_CREATOR_ID ;
	directoryInformation[CREATOR_NS_INDEX].field.fid = NWSM_SCAN_INFO_CREATOR_NS ;
	directoryInformation[PRIM_DS_SIZE_INDEX].field.fid = NWSM_SCAN_INFO_PRIM_DS_SIZE ;
	directoryInformation[TOTAL_DS_SIZE_INDEX].field.fid = NWSM_SCAN_INFO_TOTAL_DS_SIZE ;
	directoryInformation[MODIFY_FLAG_INDEX].field.fid = NWSM_SCAN_INFO_MODIFY_FLAG ;
	directoryInformation[DELETE_FLAG_INDEX].field.fid = NWSM_SCAN_INFO_DELETE_FLAG ;
	directoryInformation[PARENT_FLAG_INDEX].field.fid = NWSM_SCAN_INFO_PARENT_FLAG ;
	directoryInformation[ACC_DATE_TIME_INDEX].field.fid = NWSM_SCAN_INFO_ACC_DATE_TIME ;
	directoryInformation[CREAT_DATE_TIME_INDEX].field.fid = NWSM_SCAN_INFO_CREAT_DATE_TIME ;
	directoryInformation[MOD_DATE_TIME_INDEX].field.fid = NWSM_SCAN_INFO_MOD_DATE_TIME ;
	directoryInformation[ARC_DATE_TIME_INDEX].field.fid = NWSM_SCAN_INFO_ARC_DATE_TIME ;
	directoryInformation[OTHER_INFO_INDEX].field.fid = NWSM_SCAN_INFO_OTHER_INFO ;
	directoryInformation[DS_NAME_INDEX].field.fid = NWSM_DATA_SET_NAME_FID ;
	directoryInformation[DIRECTORY_END_INDEX].field.fid = NWSM_END ;
}
#endif

CCODE NWSMSetRecordHeader(
		BUFFERPTR					 *buffer,
		UINT32						 *bufferSize,
		UINT32						 *bufferData,
		NWBOOLEAN					  setCRC,
		NWSM_RECORD_HEADER_INFO	 *recordHeaderInfo)
{
	CCODE ccode = 0;
	BUFFERPTR _buffer;
	UINT32 _bufferData, _bufferSize, __bufferSize;
	NWSM_SCAN_INFORMATION *scanInformation;
	UINT32 headerArchiveDateAndTime ;

#ifndef UNIX
	UINT64 recordSize = {0} ;
	NWSM_FIELD_TABLE_DATA recordHeaderTable[] =
	{
		{ { NWSM_BEGIN } },
		{ { NWSM_DATA_RECORD_HEADER, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_ARCHIVE_DATE_TIME, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_RECORD_SIZE, UINT64_ZERO, NULL, 0, UINT64_ZERO }, sizeof(UINT32), GET_ADDRESS, sizeof(UINT32) },
		{ { NWSM_END } }
	}, directoryInformation[] =
	{
		{ { NWSM_BEGIN } },
		{ { NWSM_DIRECTORY_INFORMATION, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_OFFSET_TO_END, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 8, GET_ADDRESS, 8 },
		{ {     NWSM_SCAN_INFO_ATTRIBUTES, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_SCAN_INFO_CREATOR_ID, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ {     NWSM_SCAN_INFO_CREATOR_NS, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ {     NWSM_SCAN_INFO_PRIM_DS_SIZE, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ {     NWSM_SCAN_INFO_TOTAL_DS_SIZE, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ {     NWSM_SCAN_INFO_MODIFY_FLAG, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ {     NWSM_SCAN_INFO_DELETE_FLAG, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ {     NWSM_SCAN_INFO_PARENT_FLAG, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ {     NWSM_SCAN_INFO_ACC_DATE_TIME, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ {     NWSM_SCAN_INFO_CREAT_DATE_TIME, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ {     NWSM_SCAN_INFO_MOD_DATE_TIME, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ {     NWSM_SCAN_INFO_ARC_DATE_TIME, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ {     NWSM_SCAN_INFO_OTHER_INFO, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_DATA_SET_NAME_FID, UINT64_ZERO, NULL, 0, UINT64_ZERO }, 0, NULL, 0 },
		{ { NWSM_END } }
	};
#else
	UINT64 recordSize ;
	NWSM_FIELD_TABLE_DATA recordHeaderTable[RECORD_END_INDEX + 1], directoryInformation[DIRECTORY_END_INDEX + 1] ;

	InitRecordHeaderTable(recordHeaderTable, RECORD_END_INDEX + 1) ;

	InitDirectoryInformation(directoryInformation, 
			DIRECTORY_END_INDEX + 1) ;

	SMDFZeroUINT64(&recordSize);
#endif
	_buffer = *buffer;
	_bufferSize = *bufferSize;
	_bufferData = *bufferData;

	recordHeaderInfo->recordSize = 0;

	if (recordHeaderInfo->isSubRecord)
	{
		recordHeaderTable[FID_INDEX].field.fid = NWSM_DATA_SUBRECORD_HEADER;
		recordHeaderTable[DATE_TIME_INDEX].field.fid = NWSM_SKIP_FIELD;
	}

	else 
	{
		recordHeaderInfo->archiveDateAndTime = NWSMGetCurrentDateAndTime();
		headerArchiveDateAndTime = 
			SwapUINT32(recordHeaderInfo->archiveDateAndTime);
		LinkData(recordHeaderTable, DATE_TIME_INDEX, sizeof(UINT32),
				&headerArchiveDateAndTime);
	}

	SetUINT64Value(&recordSize, sizeof(UINT32));
	SwapUINT64buf(&recordSize);
	LinkData(recordHeaderTable, SIZE_INDEX, sizeof(UINT32),
				&recordSize);

	__bufferSize = *bufferSize;
	ccode = PutFields(recordHeaderTable, buffer, bufferSize, bufferData,
			setCRC ? CRC_LATER : CRC_NO);
	if (!ccode)
	{
		recordHeaderInfo->addressOfRecordSize = (UINT32 *)recordHeaderTable
				[SIZE_INDEX].addressOfData;
		if (setCRC)
		{
			recordHeaderInfo->crcBegin = _buffer;
			recordHeaderInfo->crcLength = (UINT32)(_buffer - *buffer -
					SMDFSizeOfFID(recordHeaderTable[FID_INDEX].field.fid) -
					1 - sizeof(UINT32));
			recordHeaderInfo->addressForCRC = (UINT32 *)(_buffer - 4);
		}

		else
			recordHeaderInfo->addressForCRC = NULL;

		if (!recordHeaderInfo->isSubRecord)
		{
			UINT32	attributes, creatorID, creatorNameSpaceNumber,
				primaryDataStreamSize, totalStreamsDataSize,
				accessDateAndTime, createDateAndTime,
				modifiedDateAndTime, archivedDateAndTime ;

			scanInformation = recordHeaderInfo->scanInformation;

			attributes = SwapUINT32(scanInformation->attributes);
			LinkData(directoryInformation, ATTRIBUTES_INDEX, 
				sizeof(UINT32), &attributes);

			creatorID = SwapUINT32(scanInformation->creatorID);
			LinkData(directoryInformation, CREATOR_ID_INDEX, 
				sizeof(UINT32), &creatorID);

			creatorNameSpaceNumber = 
			    SwapUINT32(scanInformation->creatorNameSpaceNumber);
			LinkData(directoryInformation, CREATOR_NS_INDEX, 
				sizeof(UINT32), &creatorNameSpaceNumber);

			primaryDataStreamSize = 
			    SwapUINT32(scanInformation->primaryDataStreamSize);
			LinkData(directoryInformation, PRIM_DS_SIZE_INDEX, 
				sizeof(UINT32), &primaryDataStreamSize);

			totalStreamsDataSize = 
			    SwapUINT32(scanInformation->totalStreamsDataSize);
			LinkData(directoryInformation, TOTAL_DS_SIZE_INDEX, 
				sizeof(UINT32), &totalStreamsDataSize);

			LinkData(directoryInformation, MODIFY_FLAG_INDEX,  
				sizeof(UINT8), &scanInformation->modifiedFlag);

			LinkData(directoryInformation, DELETE_FLAG_INDEX,  
				sizeof(UINT8), &scanInformation->deletedFlag);

			LinkData(directoryInformation, PARENT_FLAG_INDEX,  
				sizeof(UINT8), &scanInformation->parentFlag);

			accessDateAndTime = 
				SwapUINT32(scanInformation->accessDateAndTime);
			LinkData(directoryInformation, ACC_DATE_TIME_INDEX, 
				sizeof(UINT32), &accessDateAndTime);

			createDateAndTime = 
				SwapUINT32(scanInformation->createDateAndTime);
			LinkData(directoryInformation, CREAT_DATE_TIME_INDEX, 
				sizeof(UINT32), &createDateAndTime);

			modifiedDateAndTime = 
				SwapUINT32(scanInformation->modifiedDateAndTime);
			LinkData(directoryInformation, MOD_DATE_TIME_INDEX, 
				sizeof(UINT32), &modifiedDateAndTime);

			archivedDateAndTime = 
				SwapUINT32(scanInformation->archivedDateAndTime);
			LinkData(directoryInformation, ARC_DATE_TIME_INDEX, 
				sizeof(UINT32), &archivedDateAndTime);

			if (scanInformation->otherInformationSize > 0x7F)
			{
				UINT32 size = scanInformation->otherInformationSize;
				directoryInformation[OTHER_INFO_INDEX].field.data = &scanInformation->otherInformation;
				directoryInformation[OTHER_INFO_INDEX].dataSizeMap = NWSM_VARIABLE_SIZE;
				SMDFPutUINT64(&directoryInformation[OTHER_INFO_INDEX].field.dataSize, size);
				directoryInformation[OTHER_INFO_INDEX].sizeOfData = size;
			}

			else
				LinkData(directoryInformation, OTHER_INFO_INDEX, scanInformation->otherInformationSize,
						&scanInformation->otherInformation);

			/* Here it is assumed that whoever called this function
			   has already used the data set name functions to put
			   the datasetname in the required format with all the
			   byte ordering taken care of. - (6/29/93)
			*/
			if (recordHeaderInfo->dataSetName->dataSetNameSize > 0x7F)
			{
				UINT32 size = SwapUINT16(recordHeaderInfo->dataSetName->dataSetNameSize);
				
				directoryInformation[DS_NAME_INDEX].field.data = &recordHeaderInfo->dataSetName->dataSetNameSize;
				directoryInformation[DS_NAME_INDEX].dataSizeMap = NWSM_VARIABLE_SIZE;
				SMDFPutUINT64(&directoryInformation[DS_NAME_INDEX].field.dataSize, size);
				directoryInformation[DS_NAME_INDEX].sizeOfData = size;
			}

			else
				LinkData(directoryInformation, DS_NAME_INDEX, 
					SwapUINT16(recordHeaderInfo->dataSetName->dataSetNameSize),
						&recordHeaderInfo->dataSetName->dataSetNameSize);

			__bufferSize = *bufferSize;

			if ((ccode = PutFields(directoryInformation, buffer, bufferSize,
					bufferData, setCRC ? CRC_YES : CRC_NO)) isnt 0)
				goto Return;

			recordHeaderInfo->recordSize += (__bufferSize - *bufferSize);
		}
	}


Return:
	if (ccode)
	{
		*buffer = _buffer;
		*bufferSize = _bufferSize;
		*bufferData = _bufferData;
	}

	else
		recordHeaderInfo->headerSize = _bufferSize - *bufferSize;

	return (ccode);
}

#undef RECORD_HEADER_INDEX	
#undef FID_INDEX			
#undef DATE_TIME_INDEX	
#undef SIZE_INDEX	
#undef RECORD_END_INDEX	

#undef BEGIN_INDEX				
#undef DIRECTORY_HEADER_INDEX	
#undef OFFSET_TO_END_INDEX		
#undef ATTRIBUTES_INDEX		
#undef CREATOR_ID_INDEX		
#undef CREATOR_NS_INDEX		
#undef PRIM_DS_SIZE_INDEX		
#undef TOTAL_DS_SIZE_INDEX	
#undef MODIFY_FLAG_INDEX		
#undef DELETE_FLAG_INDEX		
#undef PARENT_FLAG_INDEX		
#undef ACC_DATE_TIME_INDEX	
#undef CREAT_DATE_TIME_INDEX  
#undef MOD_DATE_TIME_INDEX	
#undef ARC_DATE_TIME_INDEX	
#undef OTHER_INFO_INDEX	    
#undef DS_NAME_INDEX		    
#undef DIRECTORY_END_INDEX				

#undef LinkData

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMUpdateRecordHeader"
#endif
CCODE NWSMUpdateRecordHeader(
		NWSM_RECORD_HEADER_INFO	 *recordHeaderInfo)
{
	CCODE ccode = 0;
	UINT32 crc;

	if (recordHeaderInfo->addressOfRecordSize)
		*recordHeaderInfo->addressOfRecordSize = recordHeaderInfo->recordSize;

	if (recordHeaderInfo->addressForCRC)
	{
		crc = 0xffffffff;
		crc = NWSMGenerateCRC(recordHeaderInfo->crcLength, crc,
				recordHeaderInfo->crcBegin);
		*recordHeaderInfo->addressForCRC = crc;
	}

	return (ccode);
}

/* Note: the dataSetName pointer in recordHeaderInfo 
	may have invalid bufferSize fields in NWSM_DATA_SET_NAME_LIST
	and must not be freed , the scanInformation pointer must be NULL or
	point to a valid NWSM_SCAN_INFORMATION structures with a valid bufferSize,
	and will need to be freed */

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMGetRecordHeader"
#endif
CCODE NWSMGetRecordHeader(
		BUFFERPTR					 *buffer,
		UINT32						 *bufferSize,
		NWSM_RECORD_HEADER_INFO	 *recordHeaderInfo)
{
	CCODE ccode = 0;
	UINT32 _bufferSize, __bufferSize, headFID, 
		otherInformationSize = 0;
	BUFFERPTR _buffer, otherInfoBuffer = NULL;
	UINT16 allocSize, nameBufferSize, extraSpace;
	NWBOOLEAN setRealloc = FALSE;
	SMDF_FIELD_DATA field;
	NWSM_DATA_SET_NAME_LIST *localNameList = NULL ;
#ifndef UNIX
	NWSM_SCAN_INFORMATION localScanInformation = {0};
#else
	NWSM_SCAN_INFORMATION localScanInformation ;

	memset(&localScanInformation,'\0',sizeof(NWSM_SCAN_INFORMATION));
#endif
	_buffer = *buffer;
	_bufferSize = *bufferSize;

	recordHeaderInfo->dataSetName = NULL;
	if (recordHeaderInfo->scanInformation)
		recordHeaderInfo->scanInformation->scanInformationSize = 0;

	if ((ccode = GetHeader(buffer, bufferSize, &headFID, &recordHeaderInfo->
			recordSize, &recordHeaderInfo->archiveDateAndTime,
			&recordHeaderInfo->headerSize)) isnt 0)
		goto Return;

	if (headFID is NWSM_DATA_RECORD_HEADER)
	{
		recordHeaderInfo->isSubRecord = FALSE;

		__bufferSize = *bufferSize;

		do
		{
			if ((ccode = SMDFGetNextField(*buffer, *bufferSize, &field)) isnt 0)
				goto Return;

			switch (field.fid)
			{
			case NWSM_DATA_RECORD_HEADER:
			case NWSM_DATA_SUBRECORD_HEADER:
			case NWSM_VOLUME_HEADER:
			case NWSM_DIRECTORY_HEADER:
			case NWSM_FILE_HEADER:
			case NWSM_BINDERY_HEADER:
			case NWSM_286_BINDERY_HEADER:
				if (recordHeaderInfo->dataSetName)
					recordHeaderInfo->dataSetName->dataSetNameSize = 0;

				if (recordHeaderInfo->scanInformation)
					recordHeaderInfo->scanInformation->scanInformationSize = 0;
				goto Return;
			}

			*buffer += field.bytesTransfered;
			*bufferSize -= field.bytesTransfered;
		} while (field.fid isnt NWSM_DIRECTORY_INFORMATION);


		do
		{
			if ((ccode = SMDFGetNextField(*buffer, *bufferSize, &field)) isnt 0)
				goto Return;


			*buffer += field.bytesTransfered;
			*bufferSize -= field.bytesTransfered;

			switch (field.fid)
			{
			case NWSM_SCAN_INFO_PARENT_FLAG:
				localScanInformation.parentFlag = *((UINT8 *)field.data);
				break;

			case NWSM_SCAN_INFO_CREATOR_NS:
				localScanInformation.creatorNameSpaceNumber = 
					SwapUINT32p((char *)field.data);
				break;

			case NWSM_SCAN_INFO_ATTRIBUTES:
				localScanInformation.attributes = 
					SwapUINT32p((char *)field.data);
				break;

			case NWSM_SCAN_INFO_CREATOR_ID:
				localScanInformation.creatorID = 
					SwapUINT32p((char *)field.data);
				break;

			case NWSM_SCAN_INFO_PRIM_DS_SIZE:
				localScanInformation.primaryDataStreamSize = 
					SwapUINT32p((char *)field.data);
				break;

			case NWSM_SCAN_INFO_TOTAL_DS_SIZE:
				localScanInformation.totalStreamsDataSize = 
					SwapUINT32p((char *)field.data);
				break;

			case NWSM_SCAN_INFO_MODIFY_FLAG:
				localScanInformation.modifiedFlag = *((UINT8 *)field.data);
				break;

			case NWSM_SCAN_INFO_DELETE_FLAG:
				localScanInformation.deletedFlag = *((UINT8 *)field.data);
				break;

			case NWSM_SCAN_INFO_ACC_DATE_TIME:
				localScanInformation.accessDateAndTime = 
					SwapUINT32p((char *)field.data);
				break;

			case NWSM_SCAN_INFO_CREAT_DATE_TIME:
				localScanInformation.createDateAndTime = 
					SwapUINT32p((char *)field.data);
				break;

			case NWSM_SCAN_INFO_MOD_DATE_TIME:
				localScanInformation.modifiedDateAndTime =
					SwapUINT32p((char *)field.data);
				break;

			case NWSM_SCAN_INFO_ARC_DATE_TIME:
				localScanInformation.archivedDateAndTime = 
					SwapUINT32p((char *)field.data);
				break;

			case NWSM_SCAN_INFO_OTHER_INFO:
				SMDFGetUINT64(&field.dataSize, &otherInformationSize);
				otherInformationSize = SwapUINT32(otherInformationSize);
				if (otherInformationSize)
				{
					if ((otherInfoBuffer =  (char *) malloc((size_t)otherInformationSize)) is NULL)
					{
						ccode = NWSMUT_OUT_OF_MEMORY;
						goto Return;
					}

					memcpy(otherInfoBuffer, (BUFFERPTR) field.data, (size_t)otherInformationSize);
				}
				break;

			case NWSM_DATA_SET_NAME_FID:
				/* the dataSetName cannot be just assigned to 
				   an area in the field data because that area
				   may be at a non integer boundary. So just
				   allocate enough space to the datasetnamelist.
				   But the functions which use this dataSetName
				   should doctor up the fields in this list for
				   machine byte order etc, using the data set
				   name functions like GetFirstName etc.
				   - murthy (6/29/93)
				*/
				nameBufferSize = 
					SwapUINT16p((BUFFERPTR)field.data) + 2 ;

				if ((localNameList = 
						(NWSM_DATA_SET_NAME_LIST *)
					malloc(nameBufferSize)) == NULL ) {
					ccode = NWSMUT_OUT_OF_MEMORY;
					goto Return;
				}
				localNameList->bufferSize = nameBufferSize ;
				memcpy((char *)&(localNameList->dataSetNameSize)
					,(char *)field.data,nameBufferSize - 2);
				recordHeaderInfo->dataSetName = localNameList ;
				break;
			}
		} while (field.fid isnt NWSM_DIRECTORY_INFORMATION);

		recordHeaderInfo->recordSize -= (__bufferSize - *bufferSize);
		recordHeaderInfo->headerSize += (__bufferSize - *bufferSize);

		if (recordHeaderInfo->scanInformation)
		{
			allocSize = recordHeaderInfo->scanInformation->bufferSize;
			extraSpace = allocSize - (sizeof(NWSM_SCAN_INFORMATION) -
					sizeof(UINT8)); 

			if (otherInformationSize > extraSpace)
				setRealloc = TRUE;
		}
		
		else
			setRealloc = TRUE;

		if (setRealloc)
		{
			allocSize = sizeof(NWSM_SCAN_INFORMATION) + otherInformationSize;
			if (recordHeaderInfo->scanInformation)
			{
				if ((recordHeaderInfo->scanInformation = 
					(NWSM_SCAN_INFORMATION *) realloc(
						(void *)recordHeaderInfo->scanInformation, allocSize)) == NULL )
				{
					ccode = NWSMUT_OUT_OF_MEMORY;
					goto Return;
				}
			}

			else
			{
				if ((recordHeaderInfo->scanInformation = 
					(NWSM_SCAN_INFORMATION *)
						malloc(allocSize)) is 0)
				{
					if ( localNameList != NULL ) {
						free((char *)localNameList);
					}
					ccode = NWSMUT_OUT_OF_MEMORY;
					goto Return;
				}
			}
		}
		
		*(recordHeaderInfo->scanInformation) = localScanInformation;
		
		recordHeaderInfo->scanInformation->bufferSize = allocSize;
		recordHeaderInfo->scanInformation->scanInformationSize =
				sizeof(NWSM_SCAN_INFORMATION) -	sizeof(UINT16) - sizeof(UINT8) +
				otherInformationSize;
		
		recordHeaderInfo->scanInformation->otherInformationSize = otherInformationSize;
		if (otherInformationSize)
			memcpy(recordHeaderInfo->scanInformation->otherInformation, otherInfoBuffer, (size_t)otherInformationSize);

		goto Return;
	}
	else
		recordHeaderInfo->isSubRecord = TRUE;

Return:
	if (otherInfoBuffer)
		free(otherInfoBuffer);

	if (ccode)
	{
		*buffer = _buffer;
		*bufferSize = _bufferSize;
	}

	return (ccode);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "GetHeader"
#endif
STATIC CCODE GetHeader(
		BUFFERPTR					 *buffer,
		UINT32						 *bufferSize,
		UINT32						 *headFID,
		UINT32						 *recordSize,
		UINT32						 *archiveDateAndTime,
		UINT32						 *headerSize)
{
	BUFFERPTR __buffer;
 	CCODE ccode;
	UINT32 _bufferSize, __bufferSize, size, offsetToEnd;
	SMDF_FIELD_DATA field;

	__buffer = *buffer;
	__bufferSize = *bufferSize;

	*recordSize = 0;

NextSection:
	_bufferSize = *bufferSize;
	if ((ccode = SMDFGetNextField(*buffer, *bufferSize, &field)) isnt 0)
		goto Return;

	if (field.data and SwapUINT16p((char *)field.data) is NWSM_SYNC_DATA)
	{
		*headFID = field.fid;
		*buffer += field.bytesTransfered;
		*bufferSize -= field.bytesTransfered;
	}

	else
	{
		ccode = NWSMUT_INVALID_PARAMETER;	/* Change this */
		goto Return;
	}

	loop
	{
		if ((ccode = SMDFGetNextField(*buffer, *bufferSize, &field)) isnt 0)
			goto Return;

		switch (*headFID)
		{
		case NWSM_DATA_RECORD_HEADER:
		case NWSM_DATA_SUBRECORD_HEADER:
			if (field.fid is *headFID)
			{
				*buffer += field.bytesTransfered;
				*bufferSize -= field.bytesTransfered;
				goto Return;
			}

			else if (field.fid is NWSM_RECORD_SIZE)
				*recordSize = SwapUINT32p((char *)field.data);

			else if (field.fid is NWSM_ARCHIVE_DATE_AND_TIME)
				*archiveDateAndTime = 
					SwapUINT32p((char *)field.data);
			break;

		default:
			if (field.fid is *headFID)
			{
				*buffer += field.bytesTransfered;
				*bufferSize -= field.bytesTransfered;
				goto NextSection;
			}

			else if (field.fid is NWSM_OFFSET_TO_END)
			{
				SMDFGetUINT64(&field.dataSize, &size);
				offsetToEnd = 0;
				memcpy(&offsetToEnd, field.data, (size_t)_min(size, sizeof(offsetToEnd)));
				offsetToEnd = SwapUINT32(offsetToEnd);
				*buffer += field.bytesTransfered + offsetToEnd;
				*bufferSize -= field.bytesTransfered + offsetToEnd;
			}
			break;
		}
		
		*buffer += field.bytesTransfered;
		*bufferSize -= field.bytesTransfered;
	}

Return:
	if (ccode)
	{
		*buffer = __buffer;
		*bufferSize = __bufferSize;
	}

	else
		*headerSize = _bufferSize - *bufferSize;

	return (ccode);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "PutFields"
#endif
STATIC CCODE PutFields(
		NWSM_FIELD_TABLE_DATA		  table[],
		BUFFERPTR					 *buffer,
		UINT32						 *bufferSize,
		UINT32						 *bufferData,
		UINT32						  crcFlag)
{
	CCODE ccode = 0;
	int index = 0, offsetIndex = -1;
	UINT32 crc, dataOverflow;
	BUFFERPTR _buffer;
	UINT16 sync = NWSM_SYNC_DATA;
#ifndef UNIX
	UINT64 offsetToEnd = UINT64_ZERO;
	SMDF_FIELD_DATA endField = { 0, UINT64_ZERO, NULL, 0, UINT64_ZERO };
#else
	UINT64 offsetToEnd ;
	SMDF_FIELD_DATA endField ;

	memset(&endField,'\0',sizeof(SMDF_FIELD_DATA) );
	SMDFZeroUINT64(&offsetToEnd);
#endif
	_buffer = *buffer;

	if (table[index].field.fid is NWSM_BEGIN)
	{
		index++;
		table[index].dataSizeMap = table[index].sizeOfData = sizeof(sync);
		sync = SwapUINT16(sync);
		table[index].field.data = &sync;
		endField.fid = table[index].field.fid;
	}

	for (; table[index].field.fid isnt NWSM_END; ++index)
	{
		if (table[index].field.fid is NWSM_SKIP_FIELD)
			continue;

		if (table[index].field.fid is NWSM_OFFSET_TO_END)
		{
			table[index].field.data = &offsetToEnd;
			SetUINT64Value(&offsetToEnd, table[index].dataSizeMap);
			SwapUINT64buf(&offsetToEnd);
			table[index].addressOfData = GET_ADDRESS;
			offsetIndex = index;
		}

		if ((ccode = SMDFPutNextField(*buffer, *bufferSize, &table[index].field,
				table[index].dataSizeMap, table[index].sizeOfData)) isnt 0)
			goto Return;

		SMDFGetUINT64(&table[index].field.dataOverflow, &dataOverflow);
		if (dataOverflow)
		{
			ccode = NWSMUT_BUFFER_OVERFLOW;
			goto Return;
		}

		if (table[index].addressOfData)
			table[index].addressOfData = (void *)(*buffer +
					(table[index].field.bytesTransfered -
					(table[index].sizeOfData)));

		*buffer += table[index].field.bytesTransfered;
		*bufferSize -= table[index].field.bytesTransfered;
		if (bufferData)
			*bufferData += table[index].field.bytesTransfered;
	}

	if (offsetIndex isnt -1 and table[offsetIndex].addressOfData)
	{
		/* calculate offset value */
		SMDFPutUINT64(&offsetToEnd, *buffer - ((BUFFERPTR)table[offsetIndex].addressOfData + table[offsetIndex].dataSizeMap));

		if ((UINT8)SMDFSizeOfUINT64Data(offsetToEnd) > 
					(UINT8)table[offsetIndex].dataSizeMap)
		{
			ccode = NWSMUT_BUFFER_UNDERFLOW;
			goto Return;
		}

		/* see comment label SET addressOfData to see where it was set */
		memcpy(table[offsetIndex].addressOfData, &offsetToEnd, table[offsetIndex].dataSizeMap);
	}

	if (endField.fid)
	{
		if (crcFlag is CRC_NO)
			ccode = SMDFPutNextField(*buffer, *bufferSize, &endField,
					(UINT8)0, (UINT32)0);

		else
		{
			crc = 0xffffffff;
			if (crcFlag is CRC_YES)
				crc = NWSMGenerateCRC(*buffer - _buffer, crc, _buffer);

			crc = SwapUINT32(crc);
			endField.data = &crc;
			ccode = SMDFPutNextField(*buffer, *bufferSize, &endField,
					(UINT8)sizeof(crc), (UINT32)sizeof(crc));
		}

		if (ccode)
			goto Return;

		SMDFGetUINT64(&table[index].field.dataOverflow, &dataOverflow);
		if (dataOverflow)
		{
			ccode = NWSMUT_BUFFER_OVERFLOW;
			goto Return;
		}

		*buffer += endField.bytesTransfered;
		*bufferSize -= endField.bytesTransfered;
		if (bufferData)
			*bufferData += endField.bytesTransfered;
	}

Return:
	return (ccode);
}

void NWSMPadBlankSpace(BUFFERPTR bufferPtr, UINT32 unusedSize)
{

#if defined(DEBUG_CODE)
	CCODE _ccode;
#endif
	UINT16 offsetToEnd, sync = NWSM_SYNC_DATA;
#ifndef UNIX
	SMDF_FIELD_DATA field = {0};

#else
	SMDF_FIELD_DATA field ;

	memset(&field,'\0',sizeof(SMDF_FIELD_DATA) );
#endif
	if (unusedSize)
	{
	 	if (unusedSize < MIN_BLANK_SPACE_SECTION_SIZE)
			memset(bufferPtr, '\0', (size_t)unusedSize);

		else
		{							   
			sync = SwapUINT16(sync);
			field.fid = NWSM_BLANK_SPACE;
			field.data = &sync;
#if defined(DEBUG_CODE)
		    _ccode =
#endif
				SMDFPutNextField(bufferPtr, unusedSize, &field, sizeof(sync), sizeof(sync));
#if defined(DEBUG_CODE)
			if (_ccode)
				printf("%s: SMDFPutNextField error:%X (%s:%u)\n", RNAME, _ccode, __FILE__, __LINE__);
			if (SMDF_GT(&field.dataOverflow, 0))
				printf("%s: SMDFPutNextField dataOverflow %X (%s:%u)\n", RNAME, field.dataOverflow, __FILE__, __LINE__);
#endif
			offsetToEnd = unusedSize - MIN_BLANK_SPACE_SECTION_SIZE;
			bufferPtr += field.bytesTransfered;
			unusedSize -= field.bytesTransfered;
			field.fid = NWSM_OFFSET_TO_END;
			offsetToEnd = SwapUINT16(offsetToEnd);
			field.data = &offsetToEnd;
#if defined(DEBUG_CODE)
			    _ccode =
#endif
				SMDFPutNextField(bufferPtr, unusedSize, &field, sizeof(offsetToEnd), sizeof(offsetToEnd));
#if defined(DEBUG_CODE)
				if (_ccode)
					printf("%s: SMDFPutNextField error:%X (%s:%u)\n", RNAME, _ccode, __FILE__, __LINE__);
				if (SMDF_GT(&field.dataOverflow, 0))
					printf("%s: SMDFPutNextField dataOverflow %X (%s:%u)\n", RNAME, field.dataOverflow, __FILE__, __LINE__);
#endif
			offsetToEnd = unusedSize - MIN_BLANK_SPACE_SECTION_SIZE;
			bufferPtr += field.bytesTransfered;
			unusedSize -= field.bytesTransfered;
			memset(bufferPtr, '\0', (size_t)offsetToEnd);
			bufferPtr += offsetToEnd;
			unusedSize -= offsetToEnd;
			field.fid = NWSM_BLANK_SPACE;
#if defined(DEBUG_CODE)
		    _ccode =
#endif
				SMDFPutNextField(bufferPtr, unusedSize, &field, 0, 0);
#if defined(DEBUG_CODE)
			if (_ccode)
				printf("%s: SMDFPutNextField error:%X (%s:%u)\n", RNAME, _ccode, __FILE__, __LINE__);
			if (SMDF_GT(&field.dataOverflow, 0))
				printf("%s: SMDFPutNextField dataOverflow %X (%s:%u)\n", RNAME, field.dataOverflow, __FILE__, __LINE__);
#endif
		}
	}
}
