#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/smsapi.C	1.7"

#include <tsad.h>
#include <ctype.h>
#include <smdr.h>
#include <smsdrerr.h>
#include <error.h>
#include <smspcode.h>
#include <tsapi.h>
#include <smsutapi.h>
#include "tsaglobl.h"
#include "respond.h"
#include "drapi.h"
#include "filesys.h"
#include <utime.h>

int IsInResourceList(char *targetResourceName,
RESOURCE_LIST **resourcePtr, 
TARGET_INFO *targetInfo)
{
	RESOURCE_LIST *resPtr = targetInfo->resource ;
	int resourceNumber = 0;

	while ( resPtr != NULL ) {
		if ( strcmp(resPtr->resourceName,targetResourceName) == 0 ) {
			*resourcePtr = resPtr ;
			return(resourceNumber);
		}
		resPtr = resPtr->next ;
		resourceNumber++ ;
	}
	return(-1);
}

int GetResourceNumber(char *dataSetName, 
RESOURCE_LIST **resourcePtr, 
TARGET_INFO *targetInfo)
{
	RESOURCE_LIST *resPtr = targetInfo->resource ;
	int resourceNumber = 1, resourceNameLength = 0, savedResNum = -1 ;
	/*
	char endChr, *endPtr = NULL, *separator, p1 ;
	*/

	/* Take off the last component of the path */
	/*
	endChr = NWSMStripEndSeparator(NFSNameSpace, dataSetName, &endPtr);
	separator = strrchr(dataSetName,'/') ;
	if ( separator ) {
		if ( separator == dataSetName ) {
			separator++ ;
		}
		p1 = *separator ;
		*separator = NULL ;
	}
	*/

	resPtr = resPtr->next ; /* Skip the first node i.e, workstation */
	while ( resPtr != NULL ) {
		/* Go through the linked list and get the max match */
		if ( strncmp(resPtr->resourceName,dataSetName,
			resPtr->resourceNameLength) == 0 ) {
			if ( resourceNameLength < resPtr->resourceNameLength ){
				savedResNum = resourceNumber ;
				resourceNameLength = 
					resPtr->resourceNameLength ;
				*resourcePtr = resPtr ;
			}
		}
		resPtr = resPtr->next ;
		resourceNumber++ ;
	}
	if ( savedResNum == -1 ) {
		*resourcePtr = NULL ;
	}

	/* Restore the last component of the path */
	/*
	if ( endPtr ) {
		*endPtr = endChr ;
	}
	if ( separator ) {
		*separator = p1 ;
	}
	*/

	return(savedResNum);
}

int IsLocalFS(dev_t devNo, TARGET_INFO *targetInfo)
{
	RESOURCE_LIST *resPtr = targetInfo->resource ;

	while ( resPtr != NULL ) {
		if ( resPtr->resourceDevice == devNo ) {
			return(TRUE);
		}
		resPtr = resPtr->next ;
	}
	return(FALSE);
}

int FindNextFS(int *resourceNumber,
RESOURCE_LIST **resourcePtr, 
TARGET_INFO *targetInfo)
{
	RESOURCE_LIST *resPtr = targetInfo->resource ;
	int resIndex = 0 ;

	(*resourceNumber)++ ;
	while ( resPtr != NULL ) {
		if ( resIndex == *resourceNumber ) {
			*resourcePtr = resPtr ;
			return(0);
		}
		resPtr = resPtr->next ;
		resIndex++ ;
	}
	return(-1);
}

/********************************************************************
Function:		CloseDataSet
********************************************************************/
CCODE CloseDataSet(UINT32 connectionHandle, UINT32 *dsHandle)
{
	CCODE ccode = 0;
	DATA_SET_HANDLE *dataSetHandle = (DATA_SET_HANDLE *)*dsHandle;
	NWSM_DATA_SET_NAME name;
	char *namePtr , path[MAXPATHLEN+1];

	memset(&name,'\0',sizeof(NWSM_DATA_SET_NAME));
	if (!connectionHandle ||
		((CONNECTION *)connectionHandle)->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		goto Return;
	}  

	if (dataSetHandle->back.valid != VALID_BACKUP_DATA_SET &&
		NotValidOrEvenClose(dataSetHandle->rest.valid)) {   
		ccode = NWSMTS_INVALID_DATA_SET_HANDLE;
		goto Return;
	}
      
	if (dataSetHandle->back.valid == VALID_BACKUP_DATA_SET) {
		struct utimbuf timebuf ;
		DATA_SET_SEQUENCE *sequence ;

		if (dataSetHandle->back.stateData) {
			free(dataSetHandle->back.stateData);
			dataSetHandle->back.stateData = NULL;
		}

		sequence = dataSetHandle->back.dataSetSequence ;
        	 
		/* Note: The following UINT32 cast works because we use 
		   dataSetSequence as both a flag (when it equals 
		   ERROR_LOG_SEQUENCE and SKIPPED_DATA_SETS_SEQUENCE) and as a 
		   pointer (default case)
		*/
 
		switch ((UINT32)sequence)
		{
		case ERROR_LOG_SEQUENCE:
			((CONNECTION *)connectionHandle)->ErrorLogFileHandle.position =
                	((CONNECTION *)connectionHandle)->ErrorLogFileHandle.size = 0;
			break;
 
		case SKIPPED_DATA_SETS_SEQUENCE:
			((CONNECTION *)connectionHandle)->SkippedDataSetsFileHandle.position =
			((CONNECTION *)connectionHandle)->SkippedDataSetsFileHandle.size = 0;
			break;
 
		default:
			if ( sequence->dataSetIsOpen == FALSE) {
				break ;
			}
			if (sequence->dataSetType != TYPE_FILE ||
				S_ISSPECIAL(sequence->scanInformation->attributes) ||
				S_ISLNK(sequence->scanInformation->attributes) ) {
				sequence->dataSetIsOpen = FALSE ;
				break ;
			}

			UnlockAndClose(
				   dataSetHandle->back.smFileHandle.FileHandle);
			sequence->dataSetIsOpen = FALSE ;
			if (sequence->resourcePtr->resourceFlags & FS_READONLY) {
				break ;
			}
			/* Reset the access date */
			/* following piece of code to get the file name
               can be trashed if we store the file name in
               the backup data set handle structure when we
               open the file */
			if ((ccode = NWSMGetOneName(
					(BUFFERPTR)sequence->dataSetNames, &name)) != 0){
				goto Return;
			}

			if(strncmp(name.name,TSAGetMessage(ROOT_OF_FS),
			    strlen(TSAGetMessage(ROOT_OF_FS))) == 0 ) {
				strcpy( path, name.name ) ;
			}
			else {
				strcpy(path,sequence->fullPath->string);
	                               strcat( path, name.name ) ;
			}
			namePtr = strchr(path,':');
			if ( namePtr == NULL ) {
				ccode = NWSMTS_INVALID_PATH;
				goto Return;
			}
				
			*namePtr = '/' ;
			timebuf.actime =
        		sequence->directoryStack->dirEntry.statb.st_atime ; 
			timebuf.modtime = 
        		sequence->directoryStack->dirEntry.statb.st_mtime ; 
        	if (utime(namePtr, &timebuf) == -1) {
				ccode = HandleUnixError(errno);
				goto Return;
			}
			break;
		}
	}
	else
	{
		if (dataSetHandle->rest.smFileHandle.valid == VALID_SMFILE) {
			UnlockAndClose(
				dataSetHandle->rest.smFileHandle.FileHandle) ;
		}
 
		if (dataSetHandle->rest.tempData) {
			free(dataSetHandle->rest.tempData);
			dataSetHandle->rest.tempData = 0;
		}
 
		if (dataSetHandle->rest.stateData) {
			free(dataSetHandle->rest.stateData);
			dataSetHandle->rest.stateData = NULL;
		}
 
		if (dataSetHandle->rest.dataSetName) {
			NWSMFreeString(&dataSetHandle->rest.dataSetName);
		}
	}
      
 
Return:
	if (ccode != NWSMTS_INVALID_CONNECTION_HANDL &&
			ccode != NWSMTS_INVALID_DATA_SET_HANDLE) {
		if (dataSetHandle->back.valid == VALID_BACKUP_DATA_SET)
			TrashValidFlag(dataSetHandle->back.valid);
		else
			TrashValidFlag(dataSetHandle->rest.valid);
		free((char *)dataSetHandle);
	}
      
	*dsHandle = 0;
 
	NWSMFreeName(&name);
	return (ccode);
}

/*********************************************************************

Function:		DeleteDataSet

Design Notes:	
*********************************************************************/
CCODE DeleteDataSet(UINT32 connectionHandle,
UINT32 sequence)
{
	CCODE ccode = 0;
	DATA_SET_SEQUENCE *dataSetSequence;
	char path[MAXPATHLEN+1], *pathPtr = path;
         
	if (!connectionHandle || 
		((CONNECTION *)connectionHandle)->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		goto Return;
	}
 
	dataSetSequence = (DATA_SET_SEQUENCE *)sequence;
	if (!dataSetSequence || dataSetSequence->valid != VALID_SEQUENCE) {
		ccode = NWSMTS_INVALID_SEQUENCE_NUMBER;
		goto Return;
	}

	strcpy( pathPtr, dataSetSequence->fullPath->string ) ;
	if ( strncmp(pathPtr,TSAGetMessage(ROOT_OF_FS),
		strlen(TSAGetMessage(ROOT_OF_FS))) == 0 ) {
		pathPtr = strchr(pathPtr,':');
		*pathPtr = '/' ;
	}

	if (dataSetSequence->dataSetType is TYPE_FILE) {
		strcat(pathPtr, dataSetSequence->singleFileName);
		if ( unlink(pathPtr) != 0 ) {
			ccode = NWSMTS_DELETE_ERR ;
		}
	}
 	else {
		if ( rmdir(pathPtr) != 0 ) {
			ccode = NWSMTS_DELETE_ERR ;
		}
	}
 
Return:
	return (ccode);
}

/*********************************************************************

Function:		FixDataSetName

Design Notes:	
*********************************************************************/
CCODE FixDataSetName(UINT32 connectionHandle, STRING dataSetName, 
	UINT32 nameSpaceType, NWBOOLEAN isParent,
	NWBOOLEAN wildAllowedOnTerminal, STRING_BUFFER **newDataSetName)
{
	CCODE ccode = 0;
	STRING ptr ;
	BUFFERPTR childPtr ;
	BUFFER child[MAXNAMELEN + 1] ;

	if (!connectionHandle || 
		((CONNECTION *)connectionHandle)->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		goto Return;
	}

	if (nameSpaceType > MaxNameSpaces) {
		ccode = NWSMTS_INVALID_NAME_SPACE_TYPE ;
		goto Return ;
	}
	if (!NWSMCopyString(newDataSetName, 
			ConvertEngineToTsa(dataSetName, NULL), -1)) {
		ccode = NWSMTS_OUT_OF_MEMORY;
		goto Return ;
	}

	switch (nameSpaceType) {
	case DOSNameSpace :
	case NFSNameSpace :
	case FTAMNameSpace :
	case OS2NameSpace :
		ptr = strchr((*newDataSetName)->string,':');
		if ( nameSpaceType == DOSNameSpace ) {
			strupr((*newDataSetName)->string);
		}
		else if ( ptr ) {
			*ptr = 0 ;
			strupr((*newDataSetName)->string);
			*ptr = ':' ;
		}
 
		if ( !ptr ) {
			if ( isParent ) {
				ccode = NWSMTS_INVALID_PATH ;
			}
			goto Return ;
		}

		for (ptr = strchr((*newDataSetName)->string,'\\'); ptr;
				ptr = strchr(ptr,'\\')) {
			*ptr = '/' ;
		}
		if ( (ptr = strstr((*newDataSetName)->string,":/")) != NULL ){
			memmove(ptr+1, ptr+2, strlen(ptr+2) + 1);
		}

		if (isParent && !EndChar((*newDataSetName)->string,":/")) {
			if (!NWSMCatString(newDataSetName, "/", -1)) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return ;
			}
		}
		if ( wildAllowedOnTerminal ) {
			childPtr = NWSMStripPathChild(nameSpaceType,
					(*newDataSetName)->string, child, MAXNAMELEN) ;
			if ( NWSMIsWild((*newDataSetName)->string) ) {
				ccode = NWSMTS_INVALID_PATH ;
			}
			if ( childPtr != NULL ) {
				*childPtr = *child ;
			}
		}
		else {
			if ( NWSMIsWild((*newDataSetName)->string) ) {
				ccode = NWSMTS_INVALID_PATH ;
			}
		}
		break ;
	
	case MACNameSpace :
		if ((ptr = strchr((*newDataSetName)->string,':')) != NULL) {
			*ptr = 0 ;
			strupr((*newDataSetName)->string);
			*ptr = ':' ;
		}
		else {
			if ( isParent ) {
				ccode = NWSMTS_INVALID_PATH ;
			}
			goto Return ;
		}
		if (isParent && !EndChar((*newDataSetName)->string,":")) {
			if (!NWSMCatString(newDataSetName, ":", -1)) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return ;
			}
		}
		break ;
	}
	
Return:
	return (ccode);
}


//****************************************************************************
//
//  Function:		GetOpenModeOptionString
//
//  Purpose:		
//						
//
//  Calls:			
//
//	 lib calls:		
//						
//
//  Parameters:	
//
//  Design Notes:	
//						
//****************************************************************************
CCODE GetOpenModeOptionString(UINT32 connection,
UINT8  optionNumber,
STRING optionString)
{
	if (!connection ||
			((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		return(NWSMTS_INVALID_CONNECTION_HANDL);
	}

	if (optionString == NULL)
		return NWSMTS_INVALID_PARAMETER;

	if (optionNumber > 23)
		return NWSMTS_INVALID_OPEN_MODE_TYPE;

	switch (optionNumber)
	{
		case 0:
			strcpy(optionString, TSAGetMessage(TSA_NO_DATA_STREAMS));
			return(0);

		case 1:

		default:

			return NWSMTS_OPEN_MODE_TYPE_NOT_USED;
	}
}


static UINT16	messages[32] =
{
	DONT_TRAVERS_SUBDIRS,	// bit 0 NWSM_DO_NOT_TRAVERSE
	0xFFFF,		// bit 1 NWSM_EXCLUDE_ARCHIVED_CHILDREN
	0xFFFF,                    	// bit 2 NWSM_EXCLUDE_HIDDEN_CHILDREN
	0xFFFF,                         // bit 3 NWSM_EXCLUDE_HIDDEN_PARENTS
	0xFFFF,                   	// bit 4 NWSM_EXCLUDE_SYSTEM_CHILDREN
	0xFFFF,                         // bit 5 NWSM_EXCLUDE_SYSTEM_PARENTS
	0xFFFF,		// bit 6	  NWSM_EXCLUDE_CHILD_TRUSTEES
	0xFFFF,		// bit 7	  NWSM_EXCLUDE_PARENT_TRUSTEES
	0xFFFF,		// bit 8	  NWSM_EXCLUDE_ACCESS_DATABASE
	0xFFFF,		// bit 9	  NWSM_EXCLUDE_VOLUME_RESTS
	0xFFFF,		// bit 10  NWSM_EXCLUDE_DISK_SPACE_RESTS
	0xFFFF,         // bit 11  NWSM_EXCLUDE_EXTENDED_ATTRIBUTS
	DONT_BACKUP_DATA_STREAMS, // bit 12  NWSM_EXCLUDE_DATA_STREAMS
	0xffff,		// bit 13  NWSM_EXCLUDE_MIGRATED_CHILD
	0xffff,		// bit 14  NWSM_EXPAND_COMPRESSED_DATA
	0xffff,		// bit 15
	0xffff,		// bit 16
	0xffff,		// bit 17
	0xffff,		// bit 18
	0xffff,		// bit 19
	0xffff,		// bit 20
	0xffff,		// bit 21
	0xffff,		// bit 22
	0xffff,		// bit 23
	0xffff,		// bit 24
	0xffff,		// bit 25
	0xffff,		// bit 26
	0xffff,		// bit 27
	0xffff,		// bit 28
	0xffff, 	// bit 29 
	INCLUDE_REMOVABLE_FS, 	// bit 30 NWSM_INCLUDE_REMOVABLE_FS
	EXCLUDE_DEVICE_SPECIAL_FILES // bit 31 NWSM_EXCLUDE_SPECIAL_FILES
};

//****************************************************************************
//
//  Function:		GetTargetScanTypeString
//
//  Purpose:		
//						
//
//  Calls:			
//
//	 lib calls:		
//						
//
//  Parameters:	
//
//  Design Notes:	
//						
//****************************************************************************
CCODE GetTargetScanTypeString(UINT32 connection,
UINT8 scanTypeBitPosition,
STRING scanTypeString,
UINT32 *required,
UINT32 *disallowed)
{
	if (!connection ||
			((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		return(NWSMTS_INVALID_CONNECTION_HANDL);
	}
	if (scanTypeString == NULL)
		return NWSMTS_INVALID_PARAMETER;

	*scanTypeString = 0;
	*required = 0L;
	*disallowed = 0L;

	if (scanTypeBitPosition > 31)
		return NWSMTS_INVALID_SCAN_TYPE;

	// Check for end of the list
	if (!messages[scanTypeBitPosition])
		// No need for the SMS engine to call us anymore
 		return NWSMTS_SCAN_TYPE_NOT_USED;

	if (messages[scanTypeBitPosition] != 0xFFFF)
		strcpy(scanTypeString, 
				TSAGetMessage(messages[scanTypeBitPosition]));

	return 0L;
}

UINT16	selectionMessages[32][2] =
{
	{ 0, 0 },						// bit 0
	{ EXCLUDE_RESOURCES, INCLUDE_RESOURCES },	// bit 1		NWSM_TSA_DEFINED_RESOURCE_EXC and NWSM_TSA_DEFINED_RESOURCE_INC
	{ EXCLUDE_DIRECTORIES, INCLUDE_DIRECTORIES },	// bit 2		NWSM_PARENT_TO_BE_EXCLUDED and NWSM_PARENT_TO_BE_INCLUDED
	{ EXCLUDE_FILES, INCLUDE_FILES },		// bit 3		NWSM_CHILD_TO_BE_EXCLUDED and NWSM_CHILD_TO_BE_INCLUDED
	{ EXCLUDE_PATH_FILES, INCLUDE_PATH_FILES },	// bit 4		NWSM_EXCLUDE_CHILD_BY_FULL_NAME and NWSM_INCLUDE_CHILD_BY_FULL_NAME
	{ 0, 0 },															// bit 5
	{ 0, 0 },															// bit 6
	{ 0, 0 },															// bit 7
	{ 0, 0 },															// bit 8
	{ 0, 0 },															// bit 9
	{ 0, 0 },															// bit 10
	{ 0, 0 },															// bit 11
	{ 0, 0 },															// bit 12
	{ 0, 0 },															// bit 13
	{ 0, 0 },															// bit 14
	{ 0, 0 },															// bit 15
	{ 0, 0 },															// bit 16
	{ 0, 0 },															// bit 17
	{ 0, 0 },															// bit 18
	{ 0, 0 },															// bit 19
	{ 0, 0 },															// bit 20
	{ 0, 0 },															// bit 21
	{ 0, 0 },															// bit 22
	{ 0, 0 },															// bit 23
	{ 0, 0 },															// bit 24
	{ 0, 0 },															// bit 25
	{ 0, 0 },															// bit 26
	{ 0, 0 },															// bit 27
	{ 0, 0 },															// bit 28
	{ 0, 0 },															// bit 29
	{ 0, 0 },															// bit 30
	{ 0, 0 }																// bit 31
};

//****************************************************************************
//
//  Function:		GetTargetSelectionTypeString
//
//  Purpose:		
//						
//
//  Calls:			
//
//	 lib calls:		
//						
//
//  Parameters:	
//
//  Design Notes:	
//						
//****************************************************************************
CCODE GetTargetSelectionTypeStr(UINT32 connection,
UINT8 selectionTypeBitPosition,
STRING selectionTypeString1,
STRING selectionTypeString2)
{
	if (!connection ||
			((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		return(NWSMTS_INVALID_CONNECTION_HANDL);
	}

	if ((selectionTypeString1 == NULL) || (selectionTypeString2 == NULL))
		return NWSMTS_INVALID_PARAMETER;

	if ((selectionTypeBitPosition > 31) || (selectionTypeBitPosition <= 0))
		return NWSMTS_INVALID_SELECTION_TYPE;

	*selectionTypeString1 = 0;
	*selectionTypeString2 = 0;

	// Check for end of the list
	if (selectionMessages[selectionTypeBitPosition][0] == 0 &&
		selectionMessages[selectionTypeBitPosition][1] == 0)
		// No need for the SMS engine to call us anymore
		return NWSMTS_SELECTION_TYPE_NOT_USED;

	if (selectionMessages[selectionTypeBitPosition][0])
		strcpy(selectionTypeString1,
		TSAGetMessage(selectionMessages[selectionTypeBitPosition][0]));

	if (selectionMessages[selectionTypeBitPosition][1])
		strcpy(selectionTypeString2,
		 TSAGetMessage(selectionMessages[selectionTypeBitPosition][1]));

	return 0L;
}

 
/**********************************************************************/
//    Function: IsDataSetExcluded
//
//    Purpose:  Determine if a data set has been excluded from a scan
// 
//    Parameters: connectionHandle       input
//                isParent               input
//                dataSetName            input
//
//    Returns:    ccode - 0 dataset not excluded
//                        NWSM_DATA_SET_EXCLUDED if dataset excluded
//                        NWSM_INVALID_CONNECTION_HANDLE if error
//
/**********************************************************************/
CCODE  IsDataSetExcluded(UINT32  connectionHandle,
NWBOOLEAN                isParent,
NWSM_DATA_SET_NAME_LIST *dataSetName)
{
	CCODE ccode = NWSMTS_DATA_SET_EXCLUDED;
	UINT32 nameSpaceType = NFSNameSpace;
	SELECTION *selectionLists;
	BUFFERPTR namePtr ;
	BUFFER fileName[MAXNAMLEN + 1] ;
	NWSM_DATA_SET_NAME   entry;
	RESOURCE_LIST   *resPtr;
      
	memset((char *)&entry,'\0',sizeof(NWSM_DATA_SET_NAME));

	if ( !connectionHandle ||
		((CONNECTION *)connectionHandle)->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		goto Return;
	}

	if ((selectionLists = ((CONNECTION *)connectionHandle)->selectionLists)
						== NULL) {
		ccode = 0;
		goto Return;
	}

#ifdef DEBUG
	logerror(PRIORITYWARN,"IsDataSetExcluded:isParent %d\n",isParent);
	FLUSHLOG ;
#endif
 
	if (NWSMGetDataSetName(dataSetName, nameSpaceType, &entry)) {
		goto Return;
	}

	namePtr = ConvertEngineToTsa(entry.name,NULL);

	if ( strncmp(namePtr,TSAGetMessage(ROOT_OF_FS),
			strlen(TSAGetMessage(ROOT_OF_FS))) == 0 ) {
		namePtr = strchr(namePtr, ':');
		*namePtr = '/' ;
	}

	if ( GetResourceNumber(namePtr, &resPtr, 
				((CONNECTION *)connectionHandle)->targetInfo) == -1 ) {
		goto Return;
	}


	if (isParent) {
		// check for excluded File Systems and directories
		ccode = IncludeOrExcludeSubDirectories(resPtr->resourceName, 
					namePtr, selectionLists, nameSpaceType) ;
		if ( ccode == DATA_SET_NOT_INCLUDED ) {
			ccode = 0 ;
		}
	}
	else {
		NWSMStripPathChild(nameSpaceType, namePtr, fileName, MAXNAMLEN);
		ccode = IncludeOrExcludeFilesAndFilesWithPaths(
					resPtr->resourceName, namePtr, fileName, 
					selectionLists, nameSpaceType) ;
	}
      
	if ( ccode ) {
		ccode = NWSMTS_DATA_SET_EXCLUDED ;
	}
Return:
	NWSMFreeName(&entry) ;
 
	return (ccode);
}
 
/****************************************************************************/
//    Function: InitBackupDataSetHandle
//
//    Purpose:   Initialize the Backup DataSet Handle structure
//
//    Parameters: dataSetSequence         input
//                connection            input
//                mode                  input
//               dataSetHandle         output
//
//    Returns:    ccode - 0 if successful non-zero if failure
//                dataSetHandle - the initialized structure
//
/****************************************************************************/
CCODE InitBackupDataSetHandle(DATA_SET_SEQUENCE *dataSetSequence,
CONNECTION *connection, UINT32 mode, BACKUP_DATA_SET_HANDLE **dataSetHandle)
{
	CCODE ccode = 0;
 
	if (!dataSetHandle) {
		ccode = NWSMTS_INVALID_PARAMETER;
		goto Return;
	}

	if ((*dataSetHandle = (BACKUP_DATA_SET_HANDLE *)calloc(1,
				sizeof(BACKUP_DATA_SET_HANDLE))) == NULL) {
		ccode = NWSMTS_OUT_OF_MEMORY;
		goto Return;
	}

	(*dataSetHandle)->connection = connection;
	(*dataSetHandle)->dataSetSequence = dataSetSequence;
	(*dataSetHandle)->state = STATE_HEADER;
	(*dataSetHandle)->valid = VALID_BACKUP_DATA_SET;
	(*dataSetHandle)->mode  = mode;
 
Return:
	return (ccode);
}
 
/****************************************************************************/
//    Function:  InitRestoreDataSetHandle
//
//    Purpose:   Initialize the Restore DataSet Handle structure
//
//    Parameters:connection            input
//               parentDataSetHandle      input
//               mode                  input
//              dataSetHandle           output
//
//    Returns: ccode - 0 if successful non-zero if failure
//             dataSetHandle - the initialized structure
//
/****************************************************************************/
CCODE InitRestoreDataSetHandle(CONNECTION *connection, 
RESTORE_DATA_SET_HANDLE *parentDataSetHandle, UINT16 mode, 
RESTORE_DATA_SET_HANDLE **dataSetHandle)
{
	CCODE ccode = 0;
 
	if (!dataSetHandle) {
		ccode = NWSMTS_INVALID_PARAMETER;
		goto Return;
	}

	if ((*dataSetHandle = (RESTORE_DATA_SET_HANDLE *)calloc(1,
				sizeof(RESTORE_DATA_SET_HANDLE))) == NULL) {
		ccode = NWSMTS_OUT_OF_MEMORY;
		goto Return;
	}

	(*dataSetHandle)->state              = STATE_GET_ENTIRE_SUBRECORD;
	(*dataSetHandle)->connection          = connection;
	(*dataSetHandle)->parentDataSetHandle = parentDataSetHandle;
	(*dataSetHandle)->mode                = mode;
	(*dataSetHandle)->valid              = VALID_RESTORE_DATA_SET_ALMOST;
	(*dataSetHandle)->restoreFlag       = RESTORE_DATA_SET_NOT_YET_CREATED;
	(*dataSetHandle)->unixChars.userID = connection->loginUserID;
	(*dataSetHandle)->unixChars.groupID = connection->loginGroupID;
	(*dataSetHandle)->currentDataStreamNumber = 0xFFFFFFFF ;

Return:
	return (ccode);
}
 

/****************************************************************************/
//    Function: OpenDataSetForBackup
//
//    Purpose:  Open the named DataSet and prepare to back it up
//              
//              Check for valid parameters
//              Initialize/Create a dataSetHandle
//              If dataSetNames was passed in then copy it into
//              the dataSet Handle
//
//    Parameters: connectionHandle      input
//                dsSequence            input
//                mode                  input
//                dsHandle              output
//
//    Returns:    ccode - 0 if successful
//                        non-zero if failed
//                dsHandle - Handle to the valid dataSet
//
/****************************************************************************/
CCODE  OpenDataSetForBackup(UINT32 connectionHandle,
UINT32 dsSequence,
UINT32 mode,
UINT32 *dsHandle)
{
	BACKUP_DATA_SET_HANDLE *dataSetHandle = NULL;
	DATA_SET_SEQUENCE *dataSetSequence = (DATA_SET_SEQUENCE *)dsSequence;
	CCODE ccode = 0;
	SMFILE_HANDLE  smFileHandle;
	NWSM_DATA_SET_NAME name;
	char *namePtr , path[MAXPATHLEN+1];

	if (!dsHandle) {
		ccode = NWSMTS_INVALID_PARAMETER;
		goto Return1;
	}  

	if ( !connectionHandle ||
		((CONNECTION *)connectionHandle)->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		goto Return1;
	}
      
	memset(&smFileHandle,'\0',sizeof(SMFILE_HANDLE));
	memset(&name,'\0',sizeof(NWSM_DATA_SET_NAME));
 
	// Determine if a valid sequence has been been passed in
	if (!dsSequence || (dsSequence != ERROR_LOG_SEQUENCE &&
		dsSequence != SKIPPED_DATA_SETS_SEQUENCE &&
		dataSetSequence->valid != VALID_SEQUENCE)) {
		ccode = NWSMTS_INVALID_SEQUENCE_NUMBER;
		goto Return1;
	}  
 
#ifdef DEBUG
	logerror(PRIORITYWARN,"OpenDataSetForBackup:1\n");
	FLUSHLOG ;
#endif
 
	if ((ccode = InitBackupDataSetHandle(dataSetSequence,
			(CONNECTION *)connectionHandle, mode, 
			&dataSetHandle)) != 0) {
		goto Return;
	}  
 
 
	/* The ErrorLogFIle is a special case - this is an ASCII file
 	* which contains the errors. The file is currently open,
 	* OpenDataSetForBackup resets the position to 0 so that the
 	* errors can be read from the file 
 	*/
	if (dsSequence == ERROR_LOG_SEQUENCE) {
		dataSetHandle->tempFileHandle = 
			&((CONNECTION *)connectionHandle)->ErrorLogFileHandle;
		dataSetHandle->tempFileHandle->position = 0;
		goto Return;
	}  
 
	/* the SKippedDataSEtFile is a special case - this is Binary file that
 	* is already open it has the dataSetNames of all items that have not
 	* been scanned/read OpenDataSetForBackup resets the position to 0 so
 	* the data in this file can be read
 	*/
	if (dsSequence == SKIPPED_DATA_SETS_SEQUENCE) {
		dataSetHandle->tempFileHandle = 
		    &((CONNECTION *)connectionHandle)->SkippedDataSetsFileHandle;
		dataSetHandle->tempFileHandle->position = 0;
		goto Return;
	}
#ifdef DEBUG
	logerror(PRIORITYWARN,"OpenDataSetForBackup:2\n");
	FLUSHLOG ;
#endif
 
	if (dataSetSequence->dataSetIsOpen == TRUE) {
		ccode = NWSMTS_DATA_SET_IS_OPEN;
		goto Return;
	}

	if ( dataSetSequence->validScanInformation == 0 ) {
		ccode = NWSMTS_OPEN_DATA_STREAM_ERR ;
		LogSkippedDataSets(dataSetSequence, ccode) ;
		goto Return;
	}

#ifdef DEBUG
	logerror(PRIORITYWARN,"OpenDataSetForBackup: name %s\n", 
				dataSetSequence->fullPath->string);
	FLUSHLOG ;
#endif
 
	switch (dataSetSequence->dataSetType) {
	case TYPE_FILE:
		/* Get the full path from dataSetNames
		* Then open the file and set up the file handle
		*/
		
		dataSetHandle->smFileHandle.FileHandle =
					smFileHandle.FileHandle = -1 ;
		if (S_ISSPECIAL(dataSetSequence->scanInformation->attributes)){
			break ;
		}

		if ( (mode & NWSM_NO_DATA_STREAMS) || 
				(dataSetSequence->scanControl->scanType & 
					NWSM_EXCLUDE_DATA_STREAMS) ) {
			break ;
		}

		if (S_ISLNK(dataSetSequence->scanInformation->attributes)){
			dataSetHandle->smFileHandle.size =
			dataSetSequence->scanInformation->primaryDataStreamSize;
			dataSetHandle->smFileHandle.outputDataStreamHeader = TRUE;
			dataSetHandle->smFileHandle.valid = VALID_SMFILE;
			break;
		}

		if ((ccode = NWSMGetOneName(
			(BUFFERPTR)dataSetSequence->dataSetNames, 
			&name)) != 0){
			goto Return;
		}

		if ( strncmp(name.name,TSAGetMessage(ROOT_OF_FS),
			strlen(TSAGetMessage(ROOT_OF_FS))) == 0 ) {
			strcpy( path, name.name ) ;
		}
		else {
			strcpy(path, dataSetSequence->fullPath->string);
			strcat( path, name.name ) ;
		}
		namePtr = strchr(path,':');
		*namePtr = '/' ;
#ifdef DEBUG
		logerror(PRIORITYWARN,"before Open: name %s\n",namePtr);
		FLUSHLOG ;
#endif

		if (dataSetSequence->resourcePtr->resourceFlags & FS_READONLY) {
			mode = (mode & ~NWSM_OPEN_MODE_MASK) | NWSM_OPEN_READ_ONLY ;
		}
		ccode = OpenAndLockFile(namePtr,&smFileHandle,mode) ;
 
		if (!ccode) {
			dataSetHandle->smFileHandle.FileHandle =
					smFileHandle.FileHandle;
			dataSetHandle->smFileHandle.size =
				dataSetSequence->scanInformation->primaryDataStreamSize;
		}
		else
		{
			LogError(dataSetHandle->connection, 
					TSA_OPEN_ERROR, namePtr, ccode);
			LogSkippedDataSets(dataSetSequence, ccode) ;
			break ;
		}
		dataSetHandle->smFileHandle.valid = VALID_SMFILE;
		dataSetHandle->smFileHandle.outputDataStreamHeader = TRUE;
 
		break;
 
	case TYPE_DIRECTORY:
	case TYPE_FILESYSTEM:
		break;
	}

 
Return:
	if (!ccode)
	{
		*dsHandle = (UINT32)dataSetHandle;
		if (dsSequence != ERROR_LOG_SEQUENCE &&
				dsSequence != SKIPPED_DATA_SETS_SEQUENCE ) {
			dataSetSequence->dataSetIsOpen = TRUE;
		}
	}
    
 
	if (ccode && dataSetHandle)
		CloseDataSet(connectionHandle, (UINT32 *)&dataSetHandle);
 
	NWSMFreeName(&name);
Return1:
 
	return (ccode);
}


/****************************************************************************/
//    Function: OpenDataSetForRestore
//
//    Purpose:  Open the named DataSet and prepare to restore it
//              ASSUMPTIONS about this call:
//                This TSA can only use NFSNameSpace names.
//                In the future any new name sent could possibly
//                    be converted to a nfs name SO nameSpaceType
//                    is always NFSNameSpace
//
//              Check for valid parameters
//              Initialize/Create a dataSetHandle
//              If dataSetNames was passed in then copy it into
//              the dataSetHandle .
//
//    Parameters: connectionHandle       input
//                parentDSHandle         input
//                dataSetNames           input
//                mode                   input
//                dsHandle               output
//
//
//    Returns:    ccode - 0 if successful
//                       non-zero if failed
//                dsHandle - Handle to the valid dataSet
//
/****************************************************************************/
CCODE OpenDataSetForRestore(UINT32 connectionHandle,
UINT32 parentDSHandle,
NWSM_DATA_SET_NAME_LIST *newDataSetName,
UINT32 mode,
UINT32 *dsHandle)
{
	CCODE ccode = 0;
	RESTORE_DATA_SET_HANDLE *dataSetHandle = NULL;
	NWSM_DATA_SET_NAME name;
 
	memset(&name,'\0',sizeof(NWSM_DATA_SET_NAME));
	if (!connectionHandle ||
		((CONNECTION *)connectionHandle)->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		goto Return;
	}
 
	if (!dsHandle) {
		ccode = NWSMTS_INVALID_PARAMETER;
		goto Return;
	}
 
 
	if (parentDSHandle &&
		((RESTORE_DATA_SET_HANDLE *)parentDSHandle)->valid
						!= VALID_RESTORE_DATA_SET) {
		if (((RESTORE_DATA_SET_HANDLE *)parentDSHandle)->valid == 
			VALID_RESTORE_DATA_SET_ALMOST &&
			((RESTORE_DATA_SET_HANDLE *)parentDSHandle)->excluded){
			ccode = NWSMTS_DATA_SET_EXCLUDED;
		}
		else {   
			ccode = NWSMTS_INVALID_DATA_SET_HANDLE;
		}
		goto Return;
	}

	if ((ccode = InitRestoreDataSetHandle((CONNECTION *)connectionHandle,
		(RESTORE_DATA_SET_HANDLE *)parentDSHandle,
		mode, &dataSetHandle)) != 0) {
		goto Return;
	}
 
	if (newDataSetName) {
		// Get the nfs name from newDataSetName - if none is there then 
		// error SO we can assume that the data set name is always nfs 
		// from now on
		// CAUTION : This need to be changed to support other name 
		// spaces
		if ((ccode = NWSMGetDataSetName((BUFFERPTR)newDataSetName,
			NFSNameSpace,
			&name)) != 0) {  
			goto Return;
		}  
 
#ifdef DEBUG
		logerror(PRIORITYWARN,"OpenDataSetForRestore Name %s\n", 
								name.name);
		FLUSHLOG ;
#endif

		// Copy the new name into dataSetName
		if (!NWSMCopyString(&dataSetHandle->dataSetName, 
				ConvertEngineToTsa(name.name,NULL), -1))
		{  
			ccode = NWSMTS_OUT_OF_MEMORY;
			goto Return;
		}  

		if ( (RESTORE_DATA_SET_HANDLE *)parentDSHandle == NULL &&
			NWSMPathIsNotFullyQualified(NFSNameSpace,
						dataSetHandle->dataSetName->string) ) {
			ccode = NWSMTS_INVALID_PATH;
			goto Return;
		}
 
		dataSetHandle->dataSetWasRenamed = TRUE;
	}
 
Return:
	if (!ccode)
		*dsHandle = (UINT32)dataSetHandle;
	else if (dataSetHandle)
		CloseDataSet(connectionHandle, (UINT32 *)&dataSetHandle);
	NWSMFreeName(&name);
	return (ccode);
}
 
//****************************************************************************
//
//  Function:		ParseDataSetName
//
//  Purpose:		
//						
//
//  Calls:			
//
//	 lib calls:		
//						
//
//  Parameters:	
//
//  Design Notes:	
//						
//****************************************************************************
CCODE ParseDataSetName(UINT32 connection,
UINT32 nameSpaceType,
STRING dataSetName,
NWSM_DATA_SET_NAME	*name)
{
  	STRING_BUFFER				*separator1 = NULL;
  	STRING_BUFFER				*separator2 = NULL;
	NWBOOLEAN					reverseOrder;
	CCODE						ccode = 0 ;
	NWSM_DATA_SET_NAME_LIST	*nameBuffer = NULL;
	UINT32						selectionType;
	char *s1, *s2 ;

	if (!connection ||
		((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		return NWSMTS_INVALID_CONNECTION_HANDL;
	}
 
	if (dataSetName == NULL)
		return NWSMTS_INVALID_PARAMETER;

	if (nameSpaceType != NFSNameSpace)
		return NWSMTS_INVALID_NAME_SPACE_TYPE;

#ifdef DEBUG
	logerror(PRIORITYWARN,"ParseDataSetName: %s\n",dataSetName);
	FLUSHLOG ;
#endif

	ccode = GetNameSpaceTypeInfo(connection, nameSpaceType, 
				&reverseOrder, &separator1, &separator2);

	if (ccode)
		return ccode;

	s1 = strstr(dataSetName, separator1->string);
	s2 = strstr(dataSetName, separator2->string);

#ifdef DEBUG
	logerror(PRIORITYWARN,"ParseDataSetName: s1 %s, s2 %s\n", 
		(s1)?s1:"NULL",(s2)?s2:"NULL");
	FLUSHLOG ;
#endif


	if ( s1 == NULL || (s2 && s2 < s1) ) {
		return NWSMTS_INVALID_DATA_SET_NAME ;
	}

	selectionType = 0;

#ifdef DEBUG
	logerror(PRIORITYWARN,"ParseDataSetName: befor put\n");
	FLUSHLOG ;
#endif

	ccode = NWSMPutOneName((void HUGE **)&nameBuffer,
				  nameSpaceType,
				  selectionType,
				  reverseOrder,
				  separator1->string,
				  separator2->string,
				  dataSetName);

	if ( ccode ) {
		goto Return ;
	}
#ifdef DEBUG
	logerror(PRIORITYWARN,"ParseDataSetName: After put\n");
	FLUSHLOG ;
#endif


	ccode = NWSMGetOneName((void HUGE *)nameBuffer,
					  (NWSM_DATA_SET_NAME HUGE *)name);
	if ( ccode ) {
		goto Return ;
	}
#ifdef DEBUG
	logerror(PRIORITYWARN,"ParseDataSetName: After get\n");
	FLUSHLOG ;
#endif


Return :
	FreeMemory(separator1);
	FreeMemory(separator2);
	FreeMemory(nameBuffer);

	return(ccode);
}


//********************************************************************
//
//  Function:		ReleaseTargetService
//
//  Purpose:		
//						
//
//  Calls:			
//
//	 lib calls:		
//						
//
//  Parameters:	
//
//  Design Notes:	
//						
//*********************************************************************
CCODE ReleaseTargetService(UINT32 *connectionHandle)
{
	CCODE ccode = 0;
	CONNECTION *connection ;

	if (!connectionHandle || 
		((CONNECTION *)(*connectionHandle))->valid != VALID_CONNECTION){
		return(NWSMTS_INVALID_CONNECTION_HANDL);
	}

	connection = (CONNECTION *)(*connectionHandle);

	if (connection->ErrorLogFileHandle.valid == VALID_TEMPFILE)
		ccode = CloseAndDeleteTempFile(&connection->ErrorLogFileHandle);
	if (connection->SkippedDataSetsFileHandle.valid == VALID_TEMPFILE)
		ccode = CloseAndDeleteTempFile(
					&connection->SkippedDataSetsFileHandle);

	if (connection->selectionLists) {
		if (connection->selectionLists->valid == VALID_SELECTION) {
			NWSMDestroyList(
			&connection->selectionLists->definedResourcesToExclude);
			NWSMDestroyList(
			&connection->selectionLists->subdirectoriesToInclude);
			NWSMDestroyList(
			&connection->selectionLists->subdirectoriesToExclude);
			NWSMDestroyList(
				&connection->selectionLists->filesToInclude);
			NWSMDestroyList(
				&connection->selectionLists->filesToExclude);
			NWSMDestroyList(
			&connection->selectionLists->filesWithPathsToInclude);
			NWSMDestroyList(
			&connection->selectionLists->filesWithPathsToExclude);
		}
            
		free ((char *)connection->selectionLists);
		connection->selectionLists = NULL;
	}

	FreeResourceList(connection->targetInfo->resource) ;
	connection->targetInfo->resource = NULL ;

	ReleaseHardLinksList(&connection->disks);
	NWSMDestroyList(&connection->defaultIgnoreList);

	((CONNECTION *)(*connectionHandle))->valid = 0 ;
	*connectionHandle = 0 ;

	return (ccode);
}


//****************************************************************************
//
//  Function:		RenameDataSet
//
//  Purpose:		
//						
//
//  Calls:			
//
//	 lib calls:		
//						
//
//  Parameters:	
//
//  Design Notes:	
//						
//****************************************************************************

CCODE RenameDataSet(UINT32 connectionHandle,
UINT32 sequence,
UINT32 nameSpaceType,
STRING newDataSetName)
{
	CCODE ccode = 0;
	DATA_SET_SEQUENCE *dataSetSequence;
	char oldpath[MAXPATHLEN + 1], newpath[MAXPATHLEN + 1] ;
	char *oldPathPtr, *newNamePtr ;

	ref (nameSpaceType);
	if (!connectionHandle ||	
		((CONNECTION *)connectionHandle)->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		/*
		PrintErrorMessageToDebugScreen(ccode, __FILE__, RNAME, __LINE__, NULL);
		*/
		goto Return;
	}

	dataSetSequence = (DATA_SET_SEQUENCE *)sequence;
	if (!dataSetSequence || dataSetSequence->valid != VALID_SEQUENCE) {
		ccode = NWSMTS_INVALID_SEQUENCE_NUMBER;
		goto Return;
	}

	if (!newDataSetName) {
		ccode = NWSMTS_INVALID_PARAMETER;
		goto Return;
	}

	// set up to rename a file or a directory
	strcpy( oldpath, dataSetSequence->fullPath->string ) ;
	if ( strncmp(oldpath,TSAGetMessage(ROOT_OF_FS),
		strlen(TSAGetMessage(ROOT_OF_FS))) == 0 ) {
		oldPathPtr = strchr(oldpath,':');
		*oldPathPtr = '/' ;
	}

	// Now set up the new path. The newDataSetName has only the terminal
	// name.
	strcpy( newpath, oldPathPtr ) ;
	newNamePtr = ConvertEngineToTsa(newDataSetName, NULL) ;
	if (dataSetSequence->dataSetType == TYPE_FILE) {
		strcat(oldPathPtr, dataSetSequence->singleFileName);
		strcat( newpath, TSAGetMessage(UNIX_PATH_SEPARATOR)) ;
		strcat( newpath, newNamePtr ) ;
	}
	else {
		char *ptr ;

		if ( newpath[strlen(newpath)-1] == '/' ) {
			newpath[strlen(newpath)-1] = '\0' ;
		}
		if ( (ptr = strrchr(newpath,'/')) != NULL ) {
			*++ptr = '\0' ;
			strcat( newpath, newNamePtr ) ;
		}
		else {
			strcpy(newpath, newNamePtr);
		}
	}
 
	if ( rename(oldPathPtr,newpath) != 0 ) {
		ccode = HandleUnixError(errno);
	}
 
Return:
	return (ccode);
}
 

//****************************************************************************
//
//  Function:		ReturnToParent
//
//  Purpose:		
//						
//
//  Calls:			
//
//	 lib calls:		
//						
//
//  Parameters:	
//
//  Design Notes:	
//						
//****************************************************************************
CCODE ReturnToParent(UINT32 connectionHandle,
UINT32 *dsSequence)
{
	CCODE ccode = 0;

	if (!connectionHandle || 
		((CONNECTION *)connectionHandle)->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		goto Return;
	}

	if (*dsSequence == ERROR_LOG_SEQUENCE || 
				*dsSequence == SKIPPED_DATA_SETS_SEQUENCE)
		goto Return;
 
	if (!*dsSequence || 
		((DATA_SET_SEQUENCE *)*dsSequence)->valid isnt VALID_SEQUENCE){
		ccode = NWSMTS_INVALID_SEQUENCE_NUMBER;
		goto Return;
	}
 
	if (((DATA_SET_SEQUENCE *)*dsSequence)->dataSetIsOpen == TRUE) {
		ccode = NWSMTS_DATA_SET_IS_OPEN;
		goto Return;
	}
 
	if (((DATA_SET_SEQUENCE *)*dsSequence)->directoryStack) {
		PopDirectory((DATA_SET_SEQUENCE *)*dsSequence);
	}
Return: 
	return (ccode);
}
 

/*******************************************************************
Function:   ScanDataSetBegin

Purpose:    Begins the scan for a data set and allocates memory
			for scanInformation and dataSetNames. It can be used
			to determine if a data set exists.

Parameters: connectionHandle      input
			resourceName            input
			scanControl           input
			selectionList         input
			dsSequence            input/output
			scanInformation       output
			dataSetNames          output

Returns:   ccode -  0 if successful
			non-zero if failed
			dsSequence      - Passes an updated pointer to a
			sequence value
			scanInformation - returns updated scanning info
			dataSetNames    - returns list of data set names

********************************************************************/
CCODE ScanDataSetBegin(UINT32 connectionHandle,
NWSM_DATA_SET_NAME_LIST *resourceName,
NWSM_SCAN_CONTROL *scanControl,
NWSM_SELECTION_LIST *selectionList,
UINT32 *dsSequence,
NWSM_SCAN_INFORMATION **scanInformation,
NWSM_DATA_SET_NAME_LIST **dataSetNames)
{
	CCODE ccode = 0, excluded = 0;
	CONNECTION *connection = (CONNECTION *)connectionHandle;
	DATA_SET_SEQUENCE *dataSetSequence = NULL;
	NWSM_DATA_SET_NAME name;
	STRING_BUFFER *scanObjectPath = NULL;
	UINT16 dataSetType;
	CHAR *endPtr, *namePtr;
	RESOURCE_LIST	*resPtr;
	int	resourceNumber;
	NWBOOLEAN parentFlag, UnixWorkstation = FALSE ;
	struct stat	statb ;
	TARGET_INFO *targetInfo ;

	memset((char *)&name,'\0',sizeof(NWSM_DATA_SET_NAME));
	if (!connectionHandle || 
		((CONNECTION *)connectionHandle)->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		goto Return;
	}

	targetInfo = connection->targetInfo ;

	*dataSetNames = NULL;
	*dsSequence = 0;

	if (scanControl != NULL) {
		/* Check for invalid control flags */
		if (scanControl->childrenOnly && 
			(scanControl->returnChildTerminalNodeNameOnly ||
				scanControl->parentsOnly)) {
			return NWSMTS_INVALID_PARAMETER;
		}
	}

	if ( resourceName == NULL ) {
		return NWSMTS_INVALID_PARAMETER;
	}
#ifdef DEBUG
	dump_buf((BUFFERPTR)resourceName,30,dumpBuffer);
	logerror(PRIORITYWARN,"Scan Begin: dump %s\n", dumpBuffer);
	FLUSHLOG ;
#endif
	/* Get the name of the data set */
	ccode = NWSMGetOneName((BUFFERPTR)resourceName, &name);
	namePtr = ConvertEngineToTsa(name.name, NULL) ;
	if (ccode)
		goto Return ;

#ifdef DEBUG
	logerror(PRIORITYWARN,"Scan Begin: Object Name %s\n", namePtr);
	FLUSHLOG ;
#endif
	if ( stricmp(namePtr, TSAGetMessage(WORKSTATION_RESOURCE))
			&& strncmp(namePtr,TSAGetMessage(ROOT_OF_FS),
				strlen(TSAGetMessage(ROOT_OF_FS))) != 0 ) {
		ccode = NWSMTS_INVALID_PATH ;
		goto Return ;
	}

	if ( name.nameSpaceType == NWSM_TSA_DEFINED_RESOURCE_TYPE ) {
		/* See if the engine wants the Error Log */
		if (!stricmp(namePtr, TSAGetMessage(ERROR_LOG)))
		{
			*dsSequence = ERROR_LOG_SEQUENCE;
			goto Return0;
		}
		/* See if the engine wants the Skipped datasets file */
		if (!stricmp(namePtr, TSAGetMessage(SKIPPED_DATA_SETS)))
		{
			*dsSequence = SKIPPED_DATA_SETS_SEQUENCE;
			goto Return0;
		}
		
		NWSMStripEndSeparator(NFSNameSpace,
					namePtr, &endPtr);
		if ((ccode = FixPath(&scanObjectPath, namePtr)) != 0) {
			goto Return;
		}
#ifdef DEBUG
		logerror(PRIORITYWARN,"Scan TSA_DEFINED Object Name %s\n", 
					scanObjectPath->string);
		FLUSHLOG ;
#endif
		resourceNumber = IsInResourceList(scanObjectPath->string, 
						&resPtr, targetInfo) ;
		if ( resourceNumber == -1 ) {
			ccode = NWSMTS_DATA_SET_NOT_FOUND;
			goto Return;
		}
	}
	else if ((ccode = FixPath(&scanObjectPath, namePtr)) != 0) {
		goto Return;
	}

#ifdef DEBUG
	logerror(PRIORITYWARN,"Scan Begin: Object Name %s\n",
					scanObjectPath->string);
	FLUSHLOG ;
#endif
         
	// See if the name is a TSA defined resource(example: WORKSTATION)
	if (name.nameSpaceType == NWSM_TSA_DEFINED_RESOURCE_TYPE)
	{
		// See which defined resource the engine wants

		if (resourceNumber == UNIX_WORKSTATION)	// It is WORKSTATION
		{
			RESOURCE_LIST *resourceList = targetInfo->resource ;

			resourceList = resourceList->next ;
            
			if (!NWSMCopyString(&scanObjectPath, 
					resourceList->resourceName, -1)) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return;
			}
            
			resourceNumber = 1 ;
			resPtr = resourceList ;
			UnixWorkstation = TRUE ;
		}
		ccode = lstat(scanObjectPath->string, &statb) ;
		if (ccode) {
			ccode = NWSMTS_DATA_SET_NOT_FOUND;
			goto Return;
		}
          
		parentFlag = TRUE;
		dataSetType = TYPE_FILESYSTEM;
	}
	else {
		// if the entry is a directory it will have a slash on the end 
		// so strip the slash to call stat
 
		ccode = lstat(scanObjectPath->string, &statb) ;
 
		if (ccode) {
			ccode = NWSMTS_DATA_SET_NOT_FOUND;
			goto Return;
		}
          
		if ( S_ISDIR(statb.st_mode) ) {
			/*
			if (scanControl && scanControl->childrenOnly) {
				ccode = NWSMTS_DATA_SET_NOT_FOUND;
				goto Return;
			}
			*/
               
			dataSetType = TYPE_DIRECTORY;
			parentFlag = TRUE;
		}
		else {
			if (scanControl && scanControl->parentsOnly) {
				ccode = NWSMTS_DATA_SET_NOT_FOUND;
				goto Return;
			}
               
			dataSetType = TYPE_FILE;
			parentFlag = FALSE;
		}
   
		resourceNumber =  GetResourceNumber(scanObjectPath->string, 
							&resPtr, targetInfo) ;
		if ( resourceNumber == -1 ) {
			ccode = NWSMTS_DATA_SET_NOT_FOUND;
			goto Return;
		}
	}

	// Build a sequence structure
	if ((ccode = InitDataSetSequence(connection, scanControl, 
					selectionList, 
					&dataSetSequence)) != 0) {
		goto Return;
	}

	dataSetSequence->resourcePtr = resPtr ;
	dataSetSequence->resourceNumber = resourceNumber ;
	dataSetSequence->WorkstationResource = UnixWorkstation ;

#ifdef DEBUG
	logerror(PRIORITYWARN,
		"Scan Begin: Object Name %s, Resource Num %d, Scan Type %d\n",
			scanObjectPath->string, resourceNumber,
			dataSetSequence->scanControl->scanType);
	FLUSHLOG ;
#endif

BuildPaths:
 
	// Build the dataSetName structure and the fullPath structure
	// fullPath has the parent/directory name and is always a full path
	// dataSetNames has parent/child names and may not be a full path for
	// children
	dataSetSequence->dataSetType = dataSetType;
	dataSetSequence->scanInformation->parentFlag = parentFlag;
	dataSetSequence->scanInformation->creatorNameSpaceNumber = NFSNameSpace;
 
	namePtr = scanObjectPath->string ;
	if ( *namePtr == '/' )
		namePtr++ ;
	if ((ccode = BuildDataSetNamesAndPaths(namePtr, dataSetSequence)) != 0) {
		goto Return;
	}

	SetScanInformation(dataSetSequence, &statb);

	if (!(dataSetSequence->scanControl->scanType &
			NWSM_INCLUDE_REMOVABLE_FS) &&
			(resPtr->resourceFlags & FS_REMOVABLE)) {
		excluded = DATA_SET_EXCLUDED ;
	}
	else {
		excluded = ExcludeDataSet(dataSetSequence, &statb);
	}

#ifdef DEBUG
	logerror(PRIORITYWARN,"ExcludeDataSet returns: %d, dataset type %d\n",
			excluded, dataSetType);
	FLUSHLOG ;
#endif

	if (excluded == DATA_SET_EXCLUDED && UnixWorkstation == TRUE) {
		// If resource is unix workstation then find the next valid 
		// filesystem and  go back to the build path and check exclude 
		// code, otherwise there is nothing else to back up
		while (1) {
			if (FindNextFS(&resourceNumber, &resPtr, targetInfo)) {  
				ccode = NWSMTS_DATA_SET_NOT_FOUND;
				goto Return;
			}
			if (!(dataSetSequence->scanControl->scanType &
					NWSM_INCLUDE_REMOVABLE_FS) &&
					(resPtr->resourceFlags & FS_REMOVABLE)) {
				continue ;
			}
			dataSetSequence->resourcePtr = resPtr ;
			dataSetSequence->resourceNumber = resourceNumber ;
        	 
			if(!NWSMCopyString(&scanObjectPath, resPtr->resourceName, -1)) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return;
			}
			if( lstat(scanObjectPath->string, &statb) == 0 ) {
				goto BuildPaths;
			}
		}
	}

#ifdef DEBUG
	logerror(PRIORITYWARN,"Scan Control:parentOnly : %d, childrenOnly %d\n",
		(scanControl)? scanControl->childrenOnly : 0,
		(scanControl)? scanControl->parentsOnly : 0) ;
	FLUSHLOG ;
#endif


	switch (dataSetType) {
	    case TYPE_FILE:
		if (excluded) {
			ccode = NWSMTS_DATA_SET_NOT_FOUND;
		}
		else if (scanControl && 
			scanControl->returnChildTerminalNodeNameOnly) {

			namePtr = strchr(dataSetSequence->fullPath->string, ':')  ;
			*namePtr = '/' ;
			ccode = lstat(namePtr, &statb);
			if ( ccode ) {
				ccode = NWSMTS_DATA_SET_NOT_FOUND;
				break ;
			}
			*namePtr = ':' ;

			if ((ccode = PushDirectory(dataSetSequence,
					SCANNING_SINGLE_FILE, &statb)) != 0) {
				goto Return;
			}
			dataSetSequence->dataSetType = TYPE_DIRECTORY;
			
			SetScanInformation(dataSetSequence, &statb);
			/*
			if ((ccode = BuildDataSetNamesAndPaths(
					dataSetSequence->fullPath->string,
					dataSetSequence)) != 0) {
				goto Return;
			}
			*/
		}                  
		else {
			ccode = PushDirectory(dataSetSequence, SCAN_COMPLETE,
						&statb);
		}
		break;

	    case TYPE_FILESYSTEM:
	    case TYPE_DIRECTORY:
		if (excluded == DATA_SET_EXCLUDED) {
			ccode = NWSMTS_DATA_SET_NOT_FOUND;
			goto Return;
		}

		if (excluded == DATA_SET_NOT_INCLUDED) {
			/*
			if (scanControl && scanControl->childrenOnly) {
				ccode = NWSMTS_DATA_SET_NOT_FOUND;
				goto Return;
			}
			*/

			ccode = PushDirectory(dataSetSequence, 
							SCANNING_DIRS, &statb);
			if (!ccode)
				ccode = ScanDataSet(connection,dataSetSequence);
			break;
		}

		if (scanControl && scanControl->childrenOnly) {
			ccode = PushDirectory(dataSetSequence, SCANNING_FILES,
							&statb);
			if (!ccode)
				ccode = ScanDataSet(connection,dataSetSequence);
			break;
		}
		if (scanControl && scanControl->parentsOnly)
			ccode = PushDirectory(dataSetSequence, SCANNING_DIRS,
						&statb);
		else
			ccode = PushDirectory(dataSetSequence, SCANNING_FILES,
						&statb);
		break;
	} // end of switch
 
Return:
	if (ccode) {
		if (dataSetSequence)
			FreeSequenceStructure(&dataSetSequence);
Return0:
		if (scanInformation)
			*scanInformation = NULL;
		if (dataSetNames)
			*dataSetNames = NULL;
	}
	else {
		if (scanInformation)
			*scanInformation = dataSetSequence->scanInformation;
		if (dataSetNames)
			*dataSetNames = dataSetSequence->dataSetNames;
		*dsSequence = (UINT32)dataSetSequence;
		connection->numberOfActiveScans++ ;
	}
	NWSMFreeName(&name);
	return(ccode);
}


/*********************************************************************

Function:		ScanDataSetEnd

Purpose:  Called to stop a scan prematurely.  Frees the sequence
          structure.
*********************************************************************/
CCODE ScanDataSetEnd(UINT32 connectionHandle,
UINT32 *dsSequence, 
NWSM_SCAN_INFORMATION	**scanInformation,
NWSM_DATA_SET_NAME_LIST	**dataSetNames)
{
	CCODE ccode = 0;
	DATA_SET_SEQUENCE *dataSetSequence  ;
 
	if (scanInformation)
		*scanInformation = NULL;
	if (dataSetNames)
		*dataSetNames = NULL;
 
	if (!connectionHandle || 
		((CONNECTION *)connectionHandle)->valid != VALID_CONNECTION) {
		return(NWSMTS_INVALID_CONNECTION_HANDL);
	}

	dataSetSequence = (DATA_SET_SEQUENCE *)*dsSequence ;
	if (*dsSequence != ERROR_LOG_SEQUENCE &&
		*dsSequence != SKIPPED_DATA_SETS_SEQUENCE) {
		if (dataSetSequence == NULL || 
			dataSetSequence->valid != VALID_SEQUENCE){
			ccode = NWSMTS_INVALID_SEQUENCE_NUMBER;
		}
		else if (dataSetSequence->dataSetIsOpen == TRUE)
			ccode = NWSMTS_DATA_SET_IS_OPEN;
		else
			FreeSequenceStructure((DATA_SET_SEQUENCE **)dsSequence);
	}
	return (ccode);
}
 
/*********************************************************************
Function: ScanNextDataSet

Purpose:  Look for the next DataSet to be manipulated
          (Continues the scan started by ScanDataSetBegin)

Parameters: connectionHandle        input
          dsSequence              input/output
          scanInformation         output
          dataSetNames            output

Returns:    ccode - 0 if successful
          non-zero if failed

*********************************************************************/
CCODE ScanNextDataSet(UINT32 connectionHandle,
UINT32 *dsSequence,
NWSM_SCAN_INFORMATION **scanInformation,
NWSM_DATA_SET_NAME_LIST **dataSetNames)
{
	CCODE ccode = 0;
	CONNECTION *connection = (CONNECTION *)connectionHandle;

	// Check for valid connection
	if (!connectionHandle || 
		connection->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		*dsSequence = 0;
		goto Return1;
	}
 
	// Check for errors or skipped data sets
	if (*dsSequence == ERROR_LOG_SEQUENCE ||
		*dsSequence == SKIPPED_DATA_SETS_SEQUENCE) {
		ccode = NWSMTS_NO_MORE_DATA_SETS;
		goto Return1;
	}
      
	// Check for valid dataSetSequence
	if (((DATA_SET_SEQUENCE *)*dsSequence)->valid != VALID_SEQUENCE) {
		ccode = NWSMTS_INVALID_SEQUENCE_NUMBER;
		*dsSequence = 0;
		goto Return1;
	}

	if (((DATA_SET_SEQUENCE *)*dsSequence)->scanComplete == TRUE) {
		ccode = NWSMTS_NO_MORE_DATA_SETS;
		goto Return1;
	}
           
	// Scan for the next data set
	ccode = ScanDataSet(connection, (DATA_SET_SEQUENCE *)*dsSequence);
 
	if (!ccode) {
		if (scanInformation)
			*scanInformation = 
			    ((DATA_SET_SEQUENCE *)*dsSequence)->scanInformation;
		if (dataSetNames)
			*dataSetNames = 
			    ((DATA_SET_SEQUENCE *)*dsSequence)->dataSetNames;
		return(ccode);
	}
 
Return1:
	if (*dsSequence) {
		FreeSequenceStructure((DATA_SET_SEQUENCE **)dsSequence);
		*dsSequence = 0 ;
	}
 
	if (scanInformation)
		*scanInformation = NULL;
	if (dataSetNames)
		*dataSetNames = NULL;
 
	return (ccode);
}


//****************************************************************************
//
//  Function:		SeparateDataSetName
//
//  Purpose:		
//						
//
//  Calls:			
//
//	 lib calls:		
//						
//
//  Parameters:	
//
//  Design Notes:	
//						
//****************************************************************************
CCODE SeparateDataSetName(UINT32 connection,
UINT32 nameSpaceType,
STRING dataSetName,
STRING_BUFFER **parentDataSetName,
STRING_BUFFER **childDataSetName)
{
	BUFFERPTR childPtr = NULL;
	CCODE ccode = 0;
	BUFFER child[MAXNAMLEN+1];
	char *ptr = dataSetName ;
 
	if ( !connection ||
		((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		goto Return;
	}

	if (nameSpaceType != NFSNameSpace) {
		ccode = NWSMTS_INVALID_NAME_SPACE_TYPE;
		goto Return;
	}

	if ( dataSetName == NULL ) {
		ccode = NWSMTS_INVALID_PARAMETER ;
		goto Return;
	}

	ptr = ConvertEngineToTsa(dataSetName,NULL);
	childPtr = NWSMStripPathChild(nameSpaceType, ptr, child, MAXNAMLEN);
	if ( childPtr == NULL ) {
		ccode = NWSMTS_INVALID_PATH;
		goto Return;
	}

	if (parentDataSetName) {
		if (!NWSMCopyString(parentDataSetName, ptr, -1)) {
			ccode = NWSMTS_OUT_OF_MEMORY;
			goto Return;
		}
	}     
	if (childDataSetName) {
		if (childPtr == NULL) { // dataset is a filesystem name
			*childDataSetName = NULL ;
		}
		else if (!NWSMCopyString(childDataSetName, child, -1)) {
			ccode = NWSMTS_OUT_OF_MEMORY;
			if (*parentDataSetName)
				NWSMFreeString(parentDataSetName);
		}
	}
      
Return:
	if (childPtr)
		*childPtr = *child;
 
	return (ccode);
}

/****************************************************************************/
//    Function:   SetRestoreOptions
//
//    Purpose:
//
//    Parameters:   connectionHandle        input
//                  checkCRC                input
//                  dontCheckSelectionList  input
//                  selectionList           input
//
//    Returns:    ccode  0 - Success
//                    non-zero if failure
//
/********************************************************************/
CCODE SetRestoreOptions(UINT32 connectionHandle,
NWBOOLEAN checkCRC,
NWBOOLEAN dontCheckSelectionList,
NWSM_SELECTION_LIST *selectionList)
{
	CCODE ccode = 0;
	UINT32 nameHandle ;
	CONNECTION *connection = (CONNECTION *)connectionHandle ;
	NWSM_DATA_SET_NAME name;
	BUFFERPTR namePtr ;
 
	memset((char *)&name,'\0',sizeof(NWSM_DATA_SET_NAME));
	if (!connectionHandle ||
		connection->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		goto Return;
	}
	connection->dontCheckSelectionList = dontCheckSelectionList;
	if (connection->selectionLists) {
		NWSMDestroyList(&connection->selectionLists->definedResourcesToInclude);
		NWSMDestroyList(&connection->selectionLists->definedResourcesToExclude);
		NWSMDestroyList(&connection->selectionLists->subdirectoriesToInclude);
		NWSMDestroyList(&connection->selectionLists->subdirectoriesToExclude);
		NWSMDestroyList(&connection->selectionLists->filesToInclude);
		NWSMDestroyList(&connection->selectionLists->filesToExclude);
		NWSMDestroyList(&connection->selectionLists->filesWithPathsToInclude);
		NWSMDestroyList(&connection->selectionLists->filesWithPathsToExclude);
	}

	if (!selectionList) {
		if (connection->selectionLists) {  
			free((char *)connection->selectionLists);
			connection->selectionLists = NULL;
		}  
	 
		goto Return;
	}  

	if (!connection->selectionLists) {
		if ((connection->selectionLists = 
			(SELECTION *)malloc(sizeof(SELECTION))) == NULL) {  
			ccode = NWSMTS_OUT_OF_MEMORY;
			goto Return;
		}  
		NWSMInitList(&connection->selectionLists->definedResourcesToInclude, NULL);      
		NWSMInitList(&connection->selectionLists->definedResourcesToExclude, NULL);      
		NWSMInitList(&connection->selectionLists->subdirectoriesToExclude, NULL);
		NWSMInitList(&connection->selectionLists->subdirectoriesToInclude, NULL);
		NWSMInitList(&connection->selectionLists->filesToInclude, NULL);
		NWSMInitList(&connection->selectionLists->filesToExclude, NULL);
		NWSMInitList(&connection->selectionLists->filesWithPathsToInclude, NULL);
		NWSMInitList(&connection->selectionLists->filesWithPathsToExclude, NULL);
		connection->selectionLists->valid = VALID_SELECTION ;
	}
 

	if ((ccode = NWSMGetFirstName((BUFFERPTR)selectionList, &name, 
				&nameHandle)) != 0) {   
		if (ccode == NWSMUT_NO_MORE_NAMES)
			ccode = 0;
		goto Return;
	}

	do { 
		namePtr = ConvertEngineToTsa(name.name, NULL) ;
                
		if ( strncmp(namePtr,TSAGetMessage(ROOT_OF_FS),
				strlen(TSAGetMessage(ROOT_OF_FS))) == 0 ) {
            namePtr = strchr(namePtr,':');
			*namePtr = '/' ;
		}

		switch(name.selectionType) {  
		case NWSM_TSA_DEFINED_RESOURCE_INC:
			if (NWSMAppendToList(&connection->selectionLists->definedResourcesToInclude,
				namePtr, (void *)name.nameSpaceType) == NULL)
				goto Error;
			break;
		case NWSM_TSA_DEFINED_RESOURCE_EXC:
			if (NWSMAppendToList(&connection->selectionLists->definedResourcesToExclude,
				namePtr, (void *)name.nameSpaceType) == NULL)
				goto Error;
			break;
               
		case NWSM_PARENT_TO_BE_INCLUDED:
			if (NWSMAppendToList(&connection->selectionLists->subdirectoriesToInclude,
				namePtr, (void *)name.nameSpaceType) == NULL)
				goto Error;
			break;
		case NWSM_PARENT_TO_BE_EXCLUDED:
			if (NWSMAppendToList(&connection->selectionLists->subdirectoriesToExclude,
				namePtr, (void *)name.nameSpaceType) == NULL)
				goto Error;
			break;
               
		case NWSM_CHILD_TO_BE_INCLUDED:
			if (NWSMAppendToList(&connection->selectionLists->filesToInclude,
				namePtr, (void *)name.nameSpaceType) == NULL)
				goto Error;
			break;
               
		case NWSM_CHILD_TO_BE_EXCLUDED:
			if (NWSMAppendToList(&connection->selectionLists->filesToExclude,
				namePtr, (void *)name.nameSpaceType) == NULL)
				goto Error;
			break;
               
		case NWSM_INCLUDE_CHILD_BY_FULL_NAME:
			if (NWSMAppendToList(&connection->selectionLists->filesWithPathsToInclude,
				namePtr, (void *)name.nameSpaceType) == NULL)
				goto Error;
			break;
               
		case NWSM_EXCLUDE_CHILD_BY_FULL_NAME:
			if (NWSMAppendToList(&connection->selectionLists->filesWithPathsToExclude,
				namePtr, (void *)name.nameSpaceType) == NULL)
				goto Error;
			break;
               
		} // end switch
 
	} while ((ccode = NWSMGetNextName(&nameHandle, &name)) == 0);
 
	if (ccode == NWSMUT_NO_MORE_NAMES)
		ccode = 0;
 
	goto Return;
 
Error:
	ccode = NWSMTS_OUT_OF_MEMORY;
 
Return:
	NWSMFreeName(&name);
	return (ccode);
}

//*******************************************************************
//
//  Function:		GetUnsupportedOptions
//
//  Purpose:		
//
//  Parameters:	
//						
//
//  Design Notes:	
//						
//****************************************************************************
CCODE GetUnsupportedOptions(UINT32	connection,
UINT32		*unsupportedBackupOptions,
UINT32		*unsupportedRestoreOptions)
{
	if (!connection ||
			((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		return(NWSMTS_INVALID_CONNECTION_HANDL) ;
	}
	*unsupportedBackupOptions = 0 ;
	*unsupportedRestoreOptions = 0 ;

	return 0;
}

//*******************************************************************
//
//  Function:		GetModuleVersionInfo
//  Design Notes:	
//						
//****************************************************************************
CCODE GetModuleVersionInfo(UINT32	connection,
NWSM_MODULE_VERSION_INFO **infoPtr)
{
	if (!connection ||
			((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		return(NWSMTS_INVALID_CONNECTION_HANDL) ;
	}
	*infoPtr = &unixtsaModule ;
	return 0;
}

//********************************************************************
