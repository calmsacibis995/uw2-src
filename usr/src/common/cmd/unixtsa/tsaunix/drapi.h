/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/drapi.h	1.5"
#ifndef DRAPI_H_INCLUDED
#define DRAPI_H_INCLUDED
//****************************************************************************
// drapi.h
//****************************************************************************
//****************************************************************************
//  Function Prototypes
//****************************************************************************

void *AllocateStringBuffer(STRING_BUFFER **buffer, UINT16 length);

void *AllocateUINT16Buffer(UINT16_BUFFER **buffer, UINT16 length);

CCODE BuildResourceList(UINT32 connection);

CCODE BeginRestoreSession(UINT32 connection);

CCODE CatDataSetName(UINT32 connection, UINT32 nameSpaceType,
	STRING dataSetName, STRING terminalName,
	NWBOOLEAN terminalNameIsParent, char **newDataSetName);

CCODE ChangeToClientRightsOnTS(UINT32 *connection,
	STRING targetServiceName,
	UINT16 reserved,
	UINT32 serverConnection,
	UINT8 *internetAddress);

UINT8 CheckDate(DATA_SET_HANDLE *openDataSet);

CCODE CheckSelectionList(UINT32 connection, DATA_SET_HANDLE *openDataSet);

CCODE CloseDataSet(UINT32 connection, UINT32 *dataSetHandle);

CCODE ConnectToTargetService(UINT32 *connection,
  STRING targetServiceName,
  STRING targetUserName,
  STRING password);

CCODE ConvertError(UINT32 connection, UINT32 error, STRING message);

CCODE CreateDataCharacteristics(DATA_SET_HANDLE *openDataSet);
		
CCODE CreateDataSetHeader(DATA_SET_HANDLE *openDataSet);
		
CCODE CreateDataSetTrailer(DATA_SET_HANDLE *openDataSet);
		
CCODE CreateDataStreamHeader(DATA_SET_HANDLE *openDataSet);
		
CCODE CreateDataStreamTrailer(DATA_SET_HANDLE *openDataSet);
		
CCODE CreateExtendedAttributes(DATA_SET_HANDLE *openDataSet);
		
CCODE CreateFullPaths(DATA_SET_HANDLE *openDataSet);
		
CCODE DeleteDataSet(UINT32 connection, UINT32 sequence);

CCODE DirectoryIsIncluded(UINT8 *directory, NWSM_SELECTION_LIST *selectionList);

CCODE DirectoryIsExcluded(UINT8 *directory, NWSM_SELECTION_LIST *selectionList);

//void DisplayMessage(char *message, BOOL freeMemory);

CCODE DriveIsIncluded(UINT8 drive, NWSM_SELECTION_LIST *selectionList);

CCODE DriveIsExcluded(UINT8 drive, NWSM_SELECTION_LIST *selectionList);

CCODE FileIsIncluded(UINT8 *fullPath, UINT8 *fileName,
	 NWSM_SELECTION_LIST *selectionList);

CCODE FileIsExcluded(UINT8 *fullPath, UINT8 *fileName,
	NWSM_SELECTION_LIST *selectionList);

void FreeSequence(DATA_SET_SEQUENCE *sequence);

UINT32 GetNameSpaceType(UINT8 *nameString);

CCODE GetNameSpaceTypeInfo(UINT32 connection,
	UINT32 nameSpaceType,
	NWBOOLEAN *reverseOrder,
	STRING_BUFFER **firstSeparator,
	STRING_BUFFER **secondSeparator);

CCODE GetNextDataSet(DATA_SET_SEQUENCE *sequence);

CCODE GetOpenModeOptionString(UINT32 connection,
	UINT8  optionNumber,
	STRING optionString);

CCODE GetTargetResourceInfo(UINT32 connection,
	STRING resourceName,
	UINT16 *blockSize,
	UINT32 *totalBlocks,
	UINT32 *freeBlocks,
	NWBOOLEAN *resourceIsRemovable,
	UINT32 *purgeableBlocks,
	UINT32 *notYetPurgeableBlocks,
	UINT32 *migratedSectors,
	UINT32 *preCompressedSectors,
	UINT32 *compressedSectors);

CCODE GetTargetScanTypeString(UINT32 connection,
	UINT8 scanTypeBitPosition,
	STRING scanTypeString,
	UINT32 *required,
	UINT32 *disallowed);

CCODE GetTargetSelectionTypeStr(UINT32 connection,
	UINT8 selectionTypeBitPosition,
	STRING selectionTypeString1,
	STRING selectionTypeString2);

CCODE GetTargetServiceType(UINT32 connectionID,
	STRING targetServiceName,
	STRING targetServiceType,
	STRING targetServiceVersion);

CCODE IsDataSetExcluded(UINT32 connection,
	NWBOOLEAN isParent,
	NWSM_DATA_SET_NAME_LIST *dataSetName);

CCODE	IsDataSetMatch(UINT8 *name, UINT8 *pattern);

CCODE IsDataSetValid(DATA_SET_SEQUENCE *sequence);

NWBOOLEAN IsMatch(char *stringToCheck, char *scanPattern);

CCODE OpenDataSetForBackup(UINT32 connection,
	UINT32 sequence,
	UINT32 mode,
	UINT32 *dataSetHandle);

CCODE OpenDataSetForRestore(UINT32 connection,
	UINT32 parentHandle,
	NWSM_DATA_SET_NAME_LIST *newDataSetName,
	UINT32 mode,
	UINT32 *dataSetHandle);

CCODE ParseDataSetName(UINT32 connection,
	UINT32 nameSpaceType,
	STRING dataSetName,
	NWSM_DATA_SET_NAME  *name);

CCODE PutFields(DATA_SET_HANDLE *openDataSet,
	NWSM_FIELD_TABLE_DATA	*table);

CCODE PutEAFields(DATA_SET_HANDLE *openDataSet,
	NWSM_FIELD_TABLE_DATA	*table);

CCODE ReadDataSet(UINT32 connection,
	UINT32 dataSetHandle,
	UINT32 bytesToRead,
	UINT32 *bytesRead,
	BUFFERPTR data);

CCODE ReadDataStream(DATA_SET_HANDLE *openDataSet,
	BUFFERPTR data,
	UINT32 bytesToRead,
	UINT32 *bytesRead);

CCODE ReleaseTargetService(UINT32 *connection);

CCODE RenameDataSet(UINT32 connection,
	UINT32 sequence,
	UINT32 nameSpaceType,
	STRING newDataSetName);

CCODE ReturnToParent(UINT32 connection,
	UINT32 *sequence);

CCODE ScanDataSetBegin(UINT32 connection,
	NWSM_DATA_SET_NAME_LIST *objectName,
	NWSM_SCAN_CONTROL *scanControl,
	NWSM_SELECTION_LIST *selectionList,
	UINT32 *sequence,
	NWSM_SCAN_INFORMATION **scanInformation,
	NWSM_DATA_SET_NAME_LIST **dataSetNames);

CCODE ScanDataSetEnd(UINT32 connection,
	UINT32 *sequence, 
	NWSM_SCAN_INFORMATION	**scanInformation,
	NWSM_DATA_SET_NAME_LIST	**dataSetNames);

CCODE ScanNextDataSet(UINT32 connection,
	UINT32 *sequence,
	NWSM_SCAN_INFORMATION **scanInformation,
	NWSM_DATA_SET_NAME_LIST **dataSetNames);

CCODE ScanSupportedNameSpaces(UINT32 connection,
	UINT32 *sequence,
	STRING resourceName,
	UINT32 *nameSpaceType,
	STRING nameSpaceName);

CCODE ScanTargetServiceName(UINT32 connectionID,
	UINT32 *sequenceNumber,
	STRING scanPattern,
	STRING targetServiceName);

CCODE ScanTargetServiceResource(UINT32 connection,
	UINT32 *sequence,
	STRING resourceName);

CCODE SeparateDataSetName(UINT32 connection,
	UINT32 nameSpaceType,
	STRING dataSetName,
	STRING_BUFFER **parentDataSetName,
	STRING_BUFFER **childDataSetName);

CCODE FixDataSetName(UINT32 connectionHandle, STRING dataSetName,
	UINT32 nameSpaceType, NWBOOLEAN isParent,
	NWBOOLEAN wildAllowedOnTerminal, STRING_BUFFER **newDataSetName);

CCODE SetArchiveStatus(UINT32 connection,
	UINT32 dataSetHandle,
	UINT32 setFlag,
	UINT32 archivedDateAndTime);

CCODE SetRestoreOptions(UINT32 connection,
	NWBOOLEAN checkCRC,
	NWBOOLEAN dontCheckSelectionList,
	NWSM_SELECTION_LIST *selectionList);

void SetState(DATA_SET_HANDLE *openDataSet, UINT32 fid);

void SetUpDataSetInformation(DATA_SET_SEQUENCE *sequence);

void SetUpScanInformation(DATA_SET_SEQUENCE *sequence);

CCODE SwapDelimiter(STRING swapString,
	UINT8 *swapChar1,
	UINT8 *swapChar2);

CCODE WriteDataSet(UINT32 connection,
	UINT32 dataSetHandle,
	UINT32 bytesToWrite,
	BUFFERPTR data);

CCODE GetUnsupportedOptions(UINT32 connectionHandle,
	UINT32 *unsupportedBackupOptions,
	UINT32 *unsupportedRestoreOptions);

CCODE GetModuleVersionInfo(UINT32   connection,
NWSM_MODULE_VERSION_INFO **infoPtr) ;

//****************************************************************************
//****************************************************************************
#endif
