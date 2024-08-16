#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/backup3.C	1.5"

#include <smsfids.h>
#include "tsaglobl.h"
#include "drapi.h"

/****************************************************************************/
#define BEGIN_INDEX	0
#define FULL_PATHS_INDEX	1
#define OFFSET_TO_END_INDEX	2
#define PATH_IS_FULLY_QUALIFIED_INDEX 3
#define NAME_SPACE_TYPE_INDEX 4
#define NAME_SPACE_POS_INDEX  5
#define NAME_SPACE_SEP_INDEX  6
#define NAME_SPACE_DATA_INDEX 7
#define END_INDEX	8

static void initFullPathTable(
NWSM_FIELD_TABLE_DATA *fullPathTable,
UINT8  tableSize)
{
	if ( tableSize < END_INDEX + 1 )
		return ;
	memset(fullPathTable, '\0', sizeof(NWSM_FIELD_TABLE_DATA) * tableSize);

	fullPathTable[BEGIN_INDEX].field.fid = NWSM_BEGIN ;
	fullPathTable[FULL_PATHS_INDEX].field.fid = NWSM_FULL_PATHS ;

	fullPathTable[OFFSET_TO_END_INDEX].field.fid = NWSM_OFFSET_TO_END ;
	fullPathTable[OFFSET_TO_END_INDEX].sizeOfData = sizeof(UINT64) ;
	fullPathTable[OFFSET_TO_END_INDEX].addressOfData = GET_ADDRESS ;
	fullPathTable[OFFSET_TO_END_INDEX].dataSizeMap = sizeof(UINT64) ;

	fullPathTable[PATH_IS_FULLY_QUALIFIED_INDEX].field.fid = 
						NWSM_PATH_IS_FULLY_QUALIFIED ;
	fullPathTable[PATH_IS_FULLY_QUALIFIED_INDEX].dataSizeMap =
	fullPathTable[PATH_IS_FULLY_QUALIFIED_INDEX].sizeOfData =
	    sizeof(UINT8);
	fullPathTable[NAME_SPACE_TYPE_INDEX].field.fid = NWSM_NAME_SPACE_TYPE ;
	fullPathTable[NAME_SPACE_POS_INDEX].field.fid = NWSM_NAME_POSITIONS ;
	fullPathTable[NAME_SPACE_SEP_INDEX].field.fid = NWSM_SEPARATOR_POSITIONS ;
	fullPathTable[NAME_SPACE_DATA_INDEX].field.fid = NWSM_PATH_NAME ;
	fullPathTable[END_INDEX].field.fid = NWSM_END ;
}

CCODE BackupFullPaths(BACKUP_DATA_SET_HANDLE *dataSetHandle,
UINT32 *bytesToRead, UINT32 *bytesRead, BUFFERPTR *buffer)
{
	CCODE ccode = 0;
	UINT32 nameSpace = NFSNameSpace, totalCount;
	STRING fullPathName;
	UINT16 count = 0, loopCount;
	//UINT16_BUFFER *positionBufferPtr = NULL, *separatorBufferPtr = NULL;
	NWSM_DATA_SET_NAME name;
	NWSM_FIELD_TABLE_DATA fullPathTable[END_INDEX + 1] ;
	UINT8	pathIsFullyQualified = 1 ;

	initFullPathTable(fullPathTable,END_INDEX + 1);
	InitBufferState(dataSetHandle);

	memset(&name,'\0',sizeof(NWSM_DATA_SET_NAME));
	if ((ccode = NWSMGetOneName(
		dataSetHandle->dataSetSequence->dataSetNames, &name)) != 0) {
		goto Return;
	}

	fullPathName = name.name;

	/*   Set Name Space Type */
	fullPathTable[NAME_SPACE_TYPE_INDEX].dataSizeMap =
	    fullPathTable[NAME_SPACE_TYPE_INDEX].sizeOfData =
	    SMDFSizeOfUINT32Data(nameSpace);
	nameSpace = SwapUINT32(nameSpace);
	fullPathTable[NAME_SPACE_TYPE_INDEX].field.data = &nameSpace;

	if ( dataSetHandle->dataSetSequence->dataSetType == TYPE_FILE  && 
		dataSetHandle->dataSetSequence->scanControl->returnChildTerminalNodeNameOnly ) {
		pathIsFullyQualified = 0 ;
	}

	/*   Set PATH_IS_FULLY_QUALIFIED FID */
	fullPathTable[PATH_IS_FULLY_QUALIFIED_INDEX].field.data = 
			&pathIsFullyQualified;

	/* Get the separator and count arrays for the full path name */
	if ( pathIsFullyQualified == 0 ) {
		count = 0 ;
	}
	else {
		count = name.count ;
	//	positionBufferPtr = name.namePositions ;
	//	separatorBufferPtr = name.separatorPositions ;
	}
	/*
	if ((ccode = ParseDataSetName((UINT32)dataSetHandle->connection, 
			NFSNameSpace, fullPathName, &count, &positionBufferPtr,
			&separatorBufferPtr, pathIsFullyQualified)) != 0)
		goto Return;
	*/

	/*Set up the size fields for separatorBuffer and positionsBuffer */
	totalCount = count * sizeof(UINT16);
	if (totalCount <= 0x7F) {
		fullPathTable[NAME_SPACE_POS_INDEX].dataSizeMap =
    	fullPathTable[NAME_SPACE_POS_INDEX].sizeOfData = totalCount;

		fullPathTable[NAME_SPACE_SEP_INDEX].dataSizeMap =
    	fullPathTable[NAME_SPACE_SEP_INDEX].sizeOfData = totalCount;
	}
	else {
		fullPathTable[NAME_SPACE_POS_INDEX].dataSizeMap = NWSM_VARIABLE_SIZE;
		fullPathTable[NAME_SPACE_POS_INDEX].sizeOfData = totalCount;
		SMDFSetUINT64(&fullPathTable[NAME_SPACE_POS_INDEX].field.dataSize, &totalCount, sizeof(UINT32));

		fullPathTable[NAME_SPACE_SEP_INDEX].dataSizeMap = NWSM_VARIABLE_SIZE;
		fullPathTable[NAME_SPACE_SEP_INDEX].sizeOfData = totalCount;
		SMDFSetUINT64(&fullPathTable[NAME_SPACE_SEP_INDEX].field.dataSize, &totalCount, sizeof(UINT32));
	}

	/* Set up the size fields for the full path */
	if (strlen(fullPathName) <= 0x7F)
	{
		fullPathTable[NAME_SPACE_DATA_INDEX].dataSizeMap =
		    fullPathTable[NAME_SPACE_DATA_INDEX].sizeOfData = strlen(fullPathName);
	}
	else
	{
		fullPathTable[NAME_SPACE_DATA_INDEX].dataSizeMap = NWSM_VARIABLE_SIZE;
		fullPathTable[NAME_SPACE_DATA_INDEX].sizeOfData = strlen(fullPathName);
		SMDFSetUINT64(&fullPathTable[NAME_SPACE_DATA_INDEX].field.dataSize,
		    &fullPathTable[NAME_SPACE_DATA_INDEX].sizeOfData, sizeof(UINT32));
	}

	if ( count ) {
		/* Set the data pointers for the positionBuffer, 
           separatorBUffer and the fullPath */
		/* First swap the bytes */
		for(loopCount = 0 ; loopCount < count ; loopCount++) {
			SwapUINT16buf((char *)(name.namePositions)
						+ loopCount * sizeof(UINT16));
			SwapUINT16buf((char *)(name.separatorPositions)
						+ loopCount * sizeof(UINT16));
		}
		fullPathTable[NAME_SPACE_POS_INDEX].field.data = 
								name.namePositions;
		fullPathTable[NAME_SPACE_SEP_INDEX].field.data = 
								name.separatorPositions;
	}

	fullPathTable[NAME_SPACE_DATA_INDEX].field.data = fullPathName;

	ccode = PutFields(buffer, bytesToRead, fullPathTable, bytesRead, TRUE,
	    NWSM_FULL_PATHS, dataSetHandle, NULL);

Return:
	/*
	if (separatorBufferPtr)
		free((char *)separatorBufferPtr);

	if (positionBufferPtr)
		free((char *)positionBufferPtr);
	*/

	NWSMFreeName(&name);
	return (ccode);
}

#undef BEGIN_INDEX
#undef FULL_PATHS_INDEX	
#undef OFFSET_TO_END_INDEX	
#undef NAME_SPACE_TYPE_INDEX 
#undef NAME_SPACE_DATA_INDEX
#undef NAME_SPACE_POS_INDEX  
#undef NAME_SPACE_SEP_INDEX  
#undef END_INDEX	


/****************************************************************************/
#define   CHAR_INDEX   0

#define BEGIN_INDEX	0
#define CHARACTERISTICS_INDEX	1
#define OFFSET_TO_END_INDEX	2
#define END_INDEX	3

static void initPrimaryNSHeaderTable(
NWSM_FIELD_TABLE_DATA *primaryNSHeaderTable,
UINT8  tableSize)
{
	if ( tableSize < END_INDEX + 1 )
		return ;

	memset(primaryNSHeaderTable, '\0', 
			sizeof(NWSM_FIELD_TABLE_DATA) * tableSize);

	primaryNSHeaderTable[BEGIN_INDEX].field.fid = NWSM_BEGIN ;
	primaryNSHeaderTable[CHARACTERISTICS_INDEX].field.fid = NWSM_CHARACTERISTICS ;
	primaryNSHeaderTable[OFFSET_TO_END_INDEX].field.fid = NWSM_OFFSET_TO_END ;
	primaryNSHeaderTable[OFFSET_TO_END_INDEX].sizeOfData = 8 ;
	primaryNSHeaderTable[OFFSET_TO_END_INDEX].addressOfData = GET_ADDRESS ;
	primaryNSHeaderTable[OFFSET_TO_END_INDEX].dataSizeMap = 8 ;
	primaryNSHeaderTable[END_INDEX].field.fid = NWSM_END ;
}

CCODE BackupCharacteristics(BACKUP_DATA_SET_HANDLE *dataSetHandle,
UINT32 *bytesToRead, UINT32 *bytesRead, BUFFERPTR *buffer)
{
	CCODE ccode = 0;
	DATA_SET_SEQUENCE *dataSetSequence = dataSetHandle->dataSetSequence;
	NWSM_FIELD_TABLE_DATA primaryNSHeaderTable[END_INDEX + 1], 
			primaryNSEntryTable[2],
			primaryNSTrailerTable[1] ;
	struct stat *statbp ;
	char *ownerName ;
	ECMATime ecmaTime ;

	initPrimaryNSHeaderTable(primaryNSHeaderTable,END_INDEX + 1);

	memset(primaryNSEntryTable,'\0',
			sizeof(NWSM_FIELD_TABLE_DATA) * 2);
	primaryNSEntryTable[1].field.fid = NWSM_END ;

	memset(primaryNSTrailerTable,'\0',
			sizeof(NWSM_FIELD_TABLE_DATA) );
	primaryNSTrailerTable[0].field.fid = NWSM_END ;

	statbp = &dataSetSequence->directoryStack->dirEntry.statb ;

	InitBufferState(dataSetHandle);
	/*   Write header to output buffer */
	if ((ccode = PutFields(buffer, bytesToRead, primaryNSHeaderTable, 
			bytesRead, FALSE, 0, dataSetHandle, NULL)) != 0)
		goto Return;

	/*   Write access Date to output buffer */
	if (dataSetSequence->scanInformation->accessDateAndTime)
	{
		NWSMUnixTimeToECMA(
			dataSetSequence->scanInformation->accessDateAndTime , 
			&ecmaTime, 0);
		MapECMATime(ecmaTime);
		primaryNSEntryTable[CHAR_INDEX].field.fid = 
							NWSM_ACCESS_DATE_TIME_ECMA ;
		primaryNSEntryTable[CHAR_INDEX].dataSizeMap = 
		primaryNSEntryTable[CHAR_INDEX].sizeOfData = sizeof(ECMATime);
		primaryNSEntryTable[CHAR_INDEX].field.data = &ecmaTime ;
		if ((ccode = PutFields(buffer, bytesToRead, 
				primaryNSEntryTable, bytesRead, FALSE, 0, 
				dataSetHandle, NULL)) != 0)
			goto Return;
	}


	/*   Write modified Date And Time to output buffer */
	if (dataSetSequence->scanInformation->modifiedDateAndTime)
	{
		NWSMUnixTimeToECMA(
			dataSetSequence->scanInformation->modifiedDateAndTime , 
			&ecmaTime, 0);
		MapECMATime(ecmaTime);
		primaryNSEntryTable[CHAR_INDEX].field.fid = NWSM_MODIFY_DATE_TIME_ECMA;
		primaryNSEntryTable[CHAR_INDEX].dataSizeMap = 
		primaryNSEntryTable[CHAR_INDEX].sizeOfData = sizeof(ECMATime);
		primaryNSEntryTable[CHAR_INDEX].field.data = &ecmaTime;
		if ((ccode = PutFields(buffer, bytesToRead, 
				primaryNSEntryTable, bytesRead, FALSE, 0, 
				dataSetHandle, NULL)) != 0)
			goto Return;
	}

	/*   Write create Date And Time to output buffer */
	if (dataSetSequence->scanInformation->createDateAndTime)
	{
		NWSMUnixTimeToECMA(
			dataSetSequence->scanInformation->createDateAndTime , 
			&ecmaTime, 0);
		MapECMATime(ecmaTime);
		primaryNSEntryTable[CHAR_INDEX].field.fid = NWSM_CREATION_DATE_TIME_ECMA;
		primaryNSEntryTable[CHAR_INDEX].dataSizeMap = 
		primaryNSEntryTable[CHAR_INDEX].sizeOfData = sizeof(ECMATime);
		primaryNSEntryTable[CHAR_INDEX].field.data = &ecmaTime;
		if ((ccode = PutFields(buffer, bytesToRead, 
				primaryNSEntryTable, bytesRead, FALSE, 0, 
				dataSetHandle, NULL)) != 0)
			goto Return;
	}

	/*   Write Directory flag to output buffer */
	if (dataSetSequence->scanInformation->parentFlag)
	{
		primaryNSEntryTable[CHAR_INDEX].field.fid = NWSM_DIRECTORY;
		primaryNSEntryTable[CHAR_INDEX].dataSizeMap = SMDF_BIT_ONE;

		if ((ccode = PutFields(buffer, bytesToRead, 
				primaryNSEntryTable, bytesRead, FALSE, 0, 
				dataSetHandle, NULL)) != 0)
			goto Return;
	}

	/*   Write owner name to output buffer */
	if ((ownerName = GetNWNameByuid((int)statbp->st_uid)) != NULL ) {
		UINT32 fieldIDSize ;

		primaryNSEntryTable[CHAR_INDEX].field.fid = NWSM_OWNER_NAME ;
		fieldIDSize = strlen(ownerName) + 1;
		if ( fieldIDSize <= 0x7F ) {
			primaryNSEntryTable[CHAR_INDEX].dataSizeMap = 
			primaryNSEntryTable[CHAR_INDEX].sizeOfData = 
								fieldIDSize ;
		}
		else {
			primaryNSEntryTable[CHAR_INDEX].dataSizeMap = 
							NWSM_VARIABLE_SIZE;
			primaryNSEntryTable[CHAR_INDEX].sizeOfData = 
							fieldIDSize;
			SMDFSetUINT64(
				&primaryNSEntryTable[CHAR_INDEX].field.dataSize,
				&fieldIDSize, 4);
		}
		primaryNSEntryTable[CHAR_INDEX].field.data = ownerName ;

		if ((ccode = PutFields(buffer, bytesToRead, 
				primaryNSEntryTable, bytesRead, FALSE, 0, 
				dataSetHandle, NULL)) != 0)
			goto Return;
	}

	/*    Write trailer table */
	ccode = PutFields(buffer, bytesToRead, primaryNSTrailerTable, bytesRead, TRUE, NWSM_CHARACTERISTICS,
	    dataSetHandle, NULL);

Return:
	return (ccode);
}

#undef CHAR_INDEX

#undef BEGIN_INDEX
#undef CHARACTERISTICS_INDEX	
#undef OFFSET_TO_END_INDEX	
#undef END_INDEX	
/****************************************************************************/
