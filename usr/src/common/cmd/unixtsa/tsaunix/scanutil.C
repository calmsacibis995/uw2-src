#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/scanutil.C	1.8"
/**********************************************************************
 *
 * Program Name:  Storage Management Services (tsaunix)
 *
 *********************************************************************/

#include "tsaglobl.h"
#include <tsad.h>
#include "drapi.h"

/*********************************************************************
Function:  ScanDataSet

Purpose:   Does a search of the target file system (called a scan)

Parameters: connection                                    input
            dataSetSequence                               input/output

Returns:    ccode - 0 if successful
non-zero if failure

**********************************************************************/

CCODE ScanDataSet(CONNECTION *connection,
DATA_SET_SEQUENCE *dataSetSequence)
{
	CCODE ccode = 0;


#ifdef DEBUG
	logerror(PRIORITYWARN,"Scanning: %d\n", dataSetSequence->directoryStack->scanningMode) ;
	FLUSHLOG ;
#endif

	if (dataSetSequence->directoryStack)
	{
		switch (dataSetSequence->directoryStack->scanningMode)
		{

		case SCANNING_FILES:
			ccode = ScanFiles(connection, dataSetSequence);
			break;

		case SCANNING_DIRS:
			ccode = ScanDirectories(connection, dataSetSequence);
			break;

		case SCANNING_SINGLE_FILE:
			ccode = ScanASingleFile(dataSetSequence);
			break;

		case SCAN_COMPLETE:
			PopDirectory(dataSetSequence);
			ccode = ScanDataSet(connection, dataSetSequence);
			break;

		}
	}
	else
		ccode = IncrementCurrentDataSet(connection, dataSetSequence);

	return (ccode);
}

/*********************************************************************
Function:  ScanFiles

Purpose:   Scans through files in a directory

Parameters: connection            input
            dataSetSequence       input/output

Returns:   ccode - 0 if successful
                   non-zero if failure
**********************************************************************/
CCODE ScanFiles(CONNECTION        *connection,
DATA_SET_SEQUENCE *dataSetSequence)
{
	CCODE           ccode = 0;
	CHAR *startPos ;
	struct stat *statb ;

	startPos = dataSetSequence->fullPath->string  ;
	while (1) {
		if (dataSetSequence->directoryStack->firstScan)
		{
			startPos = strchr(startPos,':'); ;
			*startPos = '/' ;
			ccode = FindFirstDirEntry(
				startPos,
			    	&dataSetSequence->directoryStack->dirEntry,
				dataSetSequence->directoryStack->scanningMode );
			dataSetSequence->directoryStack->firstScan = FALSE;
			*startPos = ':' ;
		}
		else {
			ccode = FindNextDirEntry(
				&dataSetSequence->directoryStack->dirEntry,
				dataSetSequence->directoryStack->scanningMode );
		}

		if (!ccode)
		{
			statb =
			    &dataSetSequence->directoryStack->dirEntry.statb;

			if(statb->st_dev != 
					dataSetSequence->resourcePtr->resourceDevice){
				continue ;
			}

			dataSetSequence->dataSetType = TYPE_FILE;
			SetScanInformation(dataSetSequence, statb);
			ccode = BuildFileDataSetNames(
				dataSetSequence->directoryStack->dirEntry.name,
			    	dataSetSequence,&dataSetSequence->dataSetNames,
		dataSetSequence->scanControl->returnChildTerminalNodeNameOnly) ;
			if ( ccode ) {
				return(ccode);
			}

			if (ExcludeDataSet(dataSetSequence,
			    &dataSetSequence->directoryStack->dirEntry.statb)) {
				continue ;
			}
			break;
		}
		else
		{
			dataSetSequence->directoryStack->scanningMode = SCANNING_DIRS;
			dataSetSequence->directoryStack->firstScan = TRUE;
			ccode = ScanDirectories(connection, dataSetSequence);
			break;
		}
	}

	return (ccode);
}

/********************************************************************
Function:  ScanDirectories

Purpose:   Scans through directories or subdirectories

Parameters: connection           input
           dataSetSequence      input/output

Returns:   ccode - 0 if successful
           non-zero if failure

**********************************************************************/
CCODE ScanDirectories(CONNECTION *connection,
DATA_SET_SEQUENCE *dataSetSequence)
{
	CCODE         ccode = 0;
	UINT16        excluded = 0, scanningMode;
	CHAR *startPos ;
	struct stat *statb ;

	while(1) {
		excluded = 0 ;
		startPos = dataSetSequence->fullPath->string  ;
		if (dataSetSequence->directoryStack->firstScan)
		{
			struct stat stb ;

			startPos = strchr(startPos,':'); ;
			*startPos = '/' ;

			ccode = lstat(startPos,&stb);
			if ( ccode ) {
				*startPos = ':' ;
				dataSetSequence->directoryStack->scanningMode = SCAN_COMPLETE;
				ccode = ScanDataSet(connection, dataSetSequence);
				break;
			}

			if(stb.st_dev != 
				dataSetSequence->resourcePtr->resourceDevice) {
				/* Crossing mount points not allowed */
				excluded = DATA_SET_EXCLUDED;
			}
			if (excluded == DATA_SET_EXCLUDED) {
				*startPos = ':' ;
				dataSetSequence->directoryStack->scanningMode = SCAN_COMPLETE;
				ccode = ScanDataSet(connection, dataSetSequence);
				break;
			}

			ccode = FindFirstDirEntry(
				startPos,
			    	&dataSetSequence->directoryStack->dirEntry,
				dataSetSequence->directoryStack->scanningMode );
			dataSetSequence->directoryStack->firstScan = FALSE;
			*startPos = ':' ;
		}
		else {
			ccode = FindNextDirEntry(
				&dataSetSequence->directoryStack->dirEntry,
				dataSetSequence->directoryStack->scanningMode );
		}

		while (!ccode)
		{
			/* If the element is a directory and it isnt a . or 
			a .. then break out of the loop otherwise get another
			file
			*/

			if ( strcmp(dataSetSequence->directoryStack->dirEntry.name, ".") &&
			    strcmp(dataSetSequence->directoryStack->dirEntry.name, ".."))
				break;
			else
				ccode = FindNextDirEntry(
				   &dataSetSequence->directoryStack->dirEntry,
				   dataSetSequence->directoryStack->scanningMode );
		}

		if (!ccode)
		{
			statb =
			    &dataSetSequence->directoryStack->dirEntry.statb;

			if ( statb->st_dev != 
				  dataSetSequence->resourcePtr->resourceDevice){
				/* Exclude mount points from scan */
				excluded = DATA_SET_EXCLUDED ;
				continue;
			}
			else {
				dataSetSequence->dataSetType = TYPE_DIRECTORY;
				ccode = AppendDataSetNamesAndPaths(
				dataSetSequence->directoryStack->dirEntry.name,
					dataSetSequence);
				SetScanInformation(dataSetSequence, statb);
				excluded = ExcludeDataSet(dataSetSequence, 
									statb);
			}

			if (excluded == DATA_SET_EXCLUDED) {
				DelDirFromFullPath(&dataSetSequence->fullPath);
				continue;
			}
			else if (excluded == DATA_SET_NOT_INCLUDED) {
				if (dataSetSequence->scanControl->scanType &
			    			NWSM_DO_NOT_TRAVERSE) {
					DelDirFromFullPath(
						&dataSetSequence->fullPath);
					continue;
				}
			}

			if (excluded == DATA_SET_NOT_INCLUDED)
			{
				ccode = PushDirectory(dataSetSequence,
				    					SCANNING_DIRS, NULL);
				if (!ccode)
					ccode = ScanDataSet(connection, dataSetSequence);
				goto Return;
			}

			scanningMode = SCANNING_FILES;

			if (dataSetSequence->scanControl->scanType & 
					NWSM_DO_NOT_TRAVERSE) {
				scanningMode = SCAN_COMPLETE;
			}
			else if (dataSetSequence->scanControl->parentsOnly)
				scanningMode = SCANNING_DIRS;

			ccode = PushDirectory(dataSetSequence, scanningMode, NULL);

			if (dataSetSequence->scanControl->childrenOnly) {
				ccode = ScanDataSet(connection, dataSetSequence);
			}

			break;
		}
		else
		{
			dataSetSequence->directoryStack->scanningMode = SCAN_COMPLETE;
			ccode = ScanDataSet(connection, dataSetSequence);
			break;
		}
	}

Return:

	return (ccode);
}

/*********************************************************************
Function: ScanASingleFile

Purpose:  Extracts information from the named dataset

Parameters: dataSetSequence

Returns:   ccode - 0 if successful
           non-zero if failure

**********************************************************************/
CCODE ScanASingleFile(DATA_SET_SEQUENCE *dataSetSequence)
{
	CCODE ccode = 0;
	char tempBuffer[MAXPATHLEN +1] ;
	CHAR *startPos ;

	startPos = dataSetSequence->fullPath->string  ;

	startPos = strchr(startPos,':');
	startPos += sizeof(CHAR);
	sprintf(tempBuffer,"/%s/%s",startPos,
			dataSetSequence->singleFileName) ;
	
	ccode = lstat(tempBuffer,
			&dataSetSequence->directoryStack->dirEntry.statb);

	if ( !ccode ) {
		SetScanInformation(dataSetSequence, 
			&dataSetSequence->directoryStack->dirEntry.statb);

		dataSetSequence->dataSetType = TYPE_FILE;

		ccode = BuildFileDataSetNames(dataSetSequence->singleFileName,
					dataSetSequence, 
					&dataSetSequence->dataSetNames,
		dataSetSequence->scanControl->returnChildTerminalNodeNameOnly) ;
		/* The include/exclude checks for this file took place in 
		   ScanDataSetBegin all scan information for this file has been
		   found so set scanComplete to TRUE 
		*/

		dataSetSequence->scanComplete = TRUE;
	}

	return (ccode);
}

/*********************************************************************
Function:  IncrementCurrentDataSet

Purpose: Find the next dataset to be scanned

Parameters:  connection,         input
dataSetSequence     input/output

Returns:   ccode - 0 if successful
non-zero if failure

**********************************************************************/
CCODE IncrementCurrentDataSet(CONNECTION *connection,
DATA_SET_SEQUENCE *dataSetSequence)
{
	CCODE   ccode = 0;
	RESOURCE_LIST *resPtr ;
	int	resourceNumber = dataSetSequence->resourceNumber ;


	if (!connection ||
			((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		return(ccode);
	}
 

	if ( dataSetSequence->WorkstationResource == FALSE ) {
		ccode = NWSMTS_NO_MORE_DATA_SETS;
		return(ccode);
	}
	while(1) {
		struct stat statb ;
		CHAR *ptr ;

		ccode = FindNextFS(&resourceNumber, &resPtr,connection->targetInfo);
		if (!ccode)
		{
			if (!(dataSetSequence->scanControl->scanType & 
					NWSM_INCLUDE_REMOVABLE_FS) && 
					(resPtr->resourceFlags & FS_REMOVABLE)) {
				continue ;
			}

			dataSetSequence->dataSetType = TYPE_FILESYSTEM;
			dataSetSequence->scanInformation->parentFlag = TRUE;
			dataSetSequence->scanInformation->creatorNameSpaceNumber
 							= NFSNameSpace;
			dataSetSequence->resourcePtr = resPtr ;
			dataSetSequence->resourceNumber = resourceNumber ;
         
			if (lstat(resPtr->resourceName, &statb)) {
				continue ;
			}

			ptr = resPtr->resourceName;
			if ( *ptr == '/' ) {
				ptr += sizeof(CHAR);
			}

			ccode = BuildDataSetNamesAndPaths(ptr, dataSetSequence) ;
			if (ccode != 0) {
				return(ccode);
			}

			SetScanInformation(dataSetSequence, &statb);

			if ((ccode = ExcludeDataSet(dataSetSequence, &statb)) 
							!= DATA_SET_EXCLUDED) {
				PushDirectory(dataSetSequence,
				    dataSetSequence->scanControl->parentsOnly ?
					SCANNING_DIRS : SCANNING_FILES, &statb);
				break;
			}
		}
		else {
			ccode = NWSMTS_NO_MORE_DATA_SETS;
			break;
		}
	}
	if (ccode == DATA_SET_NOT_INCLUDED) {
		dataSetSequence->directoryStack->scanningMode = SCANNING_DIRS;
		ccode = ScanDataSet(connection, dataSetSequence);
	}
	else if (dataSetSequence->scanControl->childrenOnly) {
		ccode = ScanDataSet(connection, dataSetSequence);
	}
	return (ccode);
}

/*********************************************************************
Function:  InitDataSetSequence

Purpose:   Allocates and initializes the DataSetSequence

Parameters: connection
            scanControl
            selectionList
            dataSetSequence

Returns:   ccode - 0 if successful
            non-zero if failure

*********************************************************************/
CCODE InitDataSetSequence( CONNECTION *connection,
NWSM_SCAN_CONTROL *scanControl,
NWSM_SELECTION_LIST *selectionList,
DATA_SET_SEQUENCE **dataSetSequence)
{
	CCODE ccode = 0;
	UINT32 size;

	if ((*dataSetSequence = (DATA_SET_SEQUENCE *)calloc(1, 
					sizeof(DATA_SET_SEQUENCE))) == NULL)
		ccode = NWSMTS_OUT_OF_MEMORY;

	else
	{
		memset((char *)*dataSetSequence,'\0', sizeof(DATA_SET_SEQUENCE)) ;

		(*dataSetSequence)->connection = connection;
		if (((*dataSetSequence)->scanInformation =
		    (NWSM_SCAN_INFORMATION *)calloc(1,
		    sizeof(NWSM_SCAN_INFORMATION))) == NULL)
		{
			ccode = NWSMTS_OUT_OF_MEMORY;
			goto Return;
		}

		size = scanControl ? scanControl->scanControlSize + sizeof(UINT16) : sizeof(NWSM_SCAN_CONTROL) - sizeof(UINT8);
		if (((*dataSetSequence)->scanControl = (NWSM_SCAN_CONTROL *)calloc(size, 1)) == NULL)
		{
			ccode = NWSMTS_OUT_OF_MEMORY;
			goto Return;
		}

		if (scanControl)
		{
			memcpy(&(*dataSetSequence)->scanControl->scanControlSize,
			    &scanControl->scanControlSize, scanControl->scanControlSize);
			(*dataSetSequence)->scanControl->bufferSize = size;
		}

		else
			(*dataSetSequence)->scanControl->scanControlSize = size - sizeof(UINT16) - sizeof(UINT8);


		(*dataSetSequence)->scanInformation->bufferSize = sizeof(NWSM_SCAN_INFORMATION);
		(*dataSetSequence)->scanInformation->scanInformationSize =
		    sizeof(NWSM_SCAN_INFORMATION) - sizeof(UINT16) - sizeof(UINT8);

		if (selectionList)
		{
			ccode = BuildSelectionLists(*dataSetSequence, selectionList);
			if (ccode)
				goto Return;
		}

		(*dataSetSequence)->valid = VALID_SEQUENCE;
	}

Return:
	return (ccode);
}

/*********************************************************************
Function: SetScanInformation

Purpose:  Initializes the SCAN_INFORMATION structure

Parameters:dataSetSequence
           dataSetInfo

Returns:   ccode - 0 if successful
           non-zero if failure

**********************************************************************/
void SetScanInformation(DATA_SET_SEQUENCE *dataSetSequence, 
struct stat *dataSetInfo)
{
	NWSM_SCAN_INFORMATION *scanInformation = 
					dataSetSequence->scanInformation;

	switch (dataSetSequence->dataSetType)
	{
	case TYPE_FILE:
		scanInformation->parentFlag = FALSE;
		break;

	case TYPE_DIRECTORY:
	case TYPE_FILESYSTEM:
		scanInformation->parentFlag = TRUE;
		break;

	default:
		return;
	}

	scanInformation->attributes         = dataSetInfo->st_mode;

	if ( S_ISSPECIAL(dataSetInfo->st_mode) ||
			dataSetSequence->dataSetType != TYPE_FILE ) {
		scanInformation->primaryDataStreamSize =
	    	scanInformation->totalStreamsDataSize =  0 ;
	}
	else {
		scanInformation->primaryDataStreamSize = dataSetInfo->st_size;
	    scanInformation->totalStreamsDataSize =  
				dataSetInfo->st_size / 4096;
		if ( dataSetInfo->st_size % 4096 ) {
			scanInformation->totalStreamsDataSize++ ;
		}
	}
	scanInformation->accessDateAndTime 	= dataSetInfo->st_atime ;
	scanInformation->createDateAndTime      = dataSetInfo->st_ctime ;
	scanInformation->modifiedDateAndTime    = dataSetInfo->st_mtime ;

	dataSetSequence->validScanInformation = 1 ;

	return;
}

/*********************************************************************
Function: BuildSelectionLists

Purpose:  Build the different selection lists (see switch statement)
           supported by this TSA.  

Parameters: dataSetSequence
            selectionList

Returns:   ccode - 0 if successful
                   non-zero if failure

**********************************************************************/
CCODE BuildSelectionLists(DATA_SET_SEQUENCE *dataSetSequence,
NWSM_SELECTION_LIST *selectionList)
{
	BUFFERPTR namePtr;
	CCODE ccode = 0;
	UINT32 nameHandle;
	NWSM_DATA_SET_NAME name;
	/*
	BUFFER newName[4096];
	*/


	/*  Get the first item in the selection list */
	memset((char *)&name,'\0',sizeof(NWSM_DATA_SET_NAME));
	if ((ccode = NWSMGetFirstName((BUFFERPTR)selectionList, &name, 
					&nameHandle)) != 0)
	{
		if (ccode == NWSMUT_NO_MORE_NAMES)
			ccode = 0;
		goto Return;
	}

	/* determine the selection type of the item from the selection list and
       append it to the appropriate include/exclude list then get the next
       item in the selection list and repeat;
         end when there are no more items in the selection list */
	do
	{
		namePtr = ConvertEngineToTsa(name.name, NULL) ;
		
		if ( strncmp(namePtr,TSAGetMessage(ROOT_OF_FS),
				strlen(TSAGetMessage(ROOT_OF_FS))) == 0 ) {
			namePtr = strchr(namePtr,':');
			*namePtr = '/' ;
		}

		switch(name.selectionType)
		{
		case DEFINED_RESOURCES_TO_EXCLUDE:
			if (NWSMAppendToList(
				&dataSetSequence->definedResourcesToExclude, 
				namePtr, (void *)name.nameSpaceType) == NULL) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return;
			}
			break;

		case DEFINED_RESOURCES_TO_INCLUDE:
			if (NWSMAppendToList(
				&dataSetSequence->definedResourcesToInclude, 
				namePtr, (void *)name.nameSpaceType) == NULL) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return;
			}
			break;

		case DIR_TO_BE_INCLUDED:
			/*     if the directory is a dos name then set the 
				appropriate slashs and capitlize the path
			namePtr = (name.nameSpaceType == DOSNameSpace) ? 
				NWSMFixDOSPath(namePtr, newName) : namePtr; 
			*/
			if (NWSMAppendToList(
				&dataSetSequence->subdirectoriesToInclude,
				namePtr, (void *)name.nameSpaceType) == NULL){
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return;
			}
			break;

		case DIR_TO_BE_EXCLUDED:
			/*     if the directory is a dos name then set the 
				appropriate slashs and capitlize the path 
			namePtr = (name.nameSpaceType == DOSNameSpace) ? 
				NWSMFixDOSPath(namePtr, newName) : namePtr;
			*/
			if (NWSMAppendToList(
				&dataSetSequence->subdirectoriesToExclude,
				namePtr, (void *)name.nameSpaceType) == NULL){
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return;
			}
			break;

		case FILE_TO_BE_INCLUDED:
			if (NWSMAppendToList(&dataSetSequence->filesToInclude, 
				namePtr, (void *)name.nameSpaceType) == NULL) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return;
			}
			break;

		case FILE_TO_BE_EXCLUDED:
			if (NWSMAppendToList(&dataSetSequence->filesToExclude, 
				namePtr, (void *)name.nameSpaceType) == NULL) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return;
			}
			break;

		case FILE_WITH_PATH_TO_BE_INCLUDED:
			/*     if the path is a dos name then set the 
			       appropriate slashs and capitlize the path 
			namePtr = (name.nameSpaceType == DOSNameSpace) ? 
				NWSMFixDOSPath(namePtr, NULL) : namePtr;
			*/
			if (NWSMAppendToList(
				&dataSetSequence->filesWithPathsToInclude, 
				namePtr, (void *)name.nameSpaceType) == NULL) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return;
			}
			break;

		case FILE_WITH_PATH_TO_BE_EXCLUDED:
			/*     if the path is a dos name then set the 
			       appropriate slashs and capitlize the path
			namePtr = (name.nameSpaceType == DOSNameSpace) ? 
				NWSMFixDOSPath(namePtr, NULL) : namePtr;
		 	*/
			if (NWSMAppendToList(
				&dataSetSequence->filesWithPathsToExclude, 
				namePtr, (void *)name.nameSpaceType) == NULL) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return;
			}
			break;
		}
	} while ((ccode = NWSMGetNextName(&nameHandle, &name)) == 0);

	if (ccode == NWSMUT_NO_MORE_NAMES)
		ccode = 0;

Return:
	NWSMFreeName(&name);
	return (ccode);
}

STATIC CCODE  IsInDefaultBackupList(NWSM_LIST_PTR *list, STRING path)
{
	CCODE ccode = FALSE ;
	NWSM_LIST *tempListPtr;

	tempListPtr = NWSMGetListHead(list);
	while (tempListPtr != NULL &&
			strncmp(tempListPtr->text, path,
				strlen(tempListPtr->text))) {
		tempListPtr = tempListPtr->next;
	}

	if (tempListPtr != NULL) {
		ccode = TRUE;
	}
	return (ccode);
}


/*********************************************************************
Function: BuildDefaultIgnoreList

Purpose:  Build the default backup exclusion list

Parameters: connectionHandle

Returns:   ccode - 0 if successful
                   non-zero if failure

**********************************************************************/
CCODE BuildDefaultIgnoreList(CONNECTION  *connection)
{
	CCODE ccode = 0;
	FILE *fp, *bfp ;
	char lineBuffer[MAXPATHLEN + 1] ;
	int len ;
	NWSM_LIST_PTR	defaultBackupList ;

	if (connection == NULL || connection->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		return(ccode);
	}

	/*  Open the /etc/Ignore file */
	
	if ( (fp = fopen("/etc/Ignore","r")) == NULL ) {
		return(ccode);
	}

	/* Build the default backup list */
	NWSMInitList(&defaultBackupList, NULL);
	if ( (bfp = fopen("/etc/Backup","r")) != NULL ) {
		memset(lineBuffer,'\0',MAXPATHLEN + 1);

		while ( fgets(lineBuffer, MAXPATHLEN, bfp) ) {
			len = strlen(lineBuffer);
			if ( lineBuffer[len - 1] == '\n' ) {
				lineBuffer[len - 1] = '\0' ;
			}
			if (NWSMAppendToList(&defaultBackupList,lineBuffer, 
						NULL) == NULL) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				NWSMDestroyList(&defaultBackupList);
				fclose(bfp);
				fclose(fp);
				return(ccode);
			}
		}
		fclose(bfp);
	}
	
	NWSMInitList(&connection->defaultIgnoreList, NULL);
	memset(lineBuffer,'\0',MAXPATHLEN + 1);

	while ( fgets(lineBuffer, MAXPATHLEN, fp) ) {
		len = strlen(lineBuffer);
		if ( lineBuffer[len - 1] == '\n' ) {
			lineBuffer[len - 1] = '\0' ;
		}
		if ( IsInDefaultBackupList(&defaultBackupList,lineBuffer) ) {
			continue ;
		}
		if (NWSMAppendToList(&connection->defaultIgnoreList, 
				lineBuffer, NULL) == NULL) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				NWSMDestroyList(&defaultBackupList);
				NWSMDestroyList(&connection->defaultIgnoreList);
				fclose(fp);
				return(ccode);
		}
	}

	/* throw away the default backup list */
	NWSMDestroyList(&defaultBackupList);
	fclose(fp);
	return (ccode);
}

/******************************************************************/
