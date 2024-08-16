#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/restore2.C	1.4"

#include "tsaglobl.h"
#include <smsfids.h>

#define UNKNOWN_VALUE	0xFF
#define FULLY_QUALIFIED	1
#define NOT_FULLY_QUALIFIED	0

/**************************************************************************/
CCODE RestoreFullPaths(RESTORE_DATA_SET_HANDLE *dataSetHandle)
{
	CCODE ccode = 0, notFullyQualified = 0;
	STRING terminalName;
	UINT32 fid, length, nameSpaceType = NWSM_ALL_NAME_SPACES,
			pathLength;
	NWBOOLEAN dataSetExists = FALSE;
	UINT64 dataSize;
	BUFFERPTR dataBuffer;
	struct stat dataSetInfo;
	char *cp ;
	RESOURCE_LIST   *resPtr;
	UINT8 dataSetPathIsFullyQualified = UNKNOWN_VALUE ;

#ifdef DEBUG
	if (dataSetHandle->parentDataSetHandle != NULL) {
		logerror(PRIORITYWARN,"RestoreFullPaths parentPath %s\n", 
		      dataSetHandle->parentDataSetHandle->dataSetName->string);
	}
#endif

	if ((ccode = GetFieldToProcessSection(dataSetHandle, &fid,
		    &dataSize, &dataBuffer, NULL)) isnt 0) {
		/*
		LogError(dataSetHandle->connection, GET_FIELD_ERR,
			    ReturnDataSetName(dataSetHandle), ccode);
		*/
		goto Return;
	}

	if (fid isnt NWSM_FULL_PATHS) {
		ccode = NWSMUT_INVALID_FIELD_ID;
		goto Return;
	}

#ifdef DEBUG
	logerror(PRIORITYWARN,"RestoreFullPaths fid %x\n", fid);
#endif


	fid = 0;
	while (!dataSetHandle->dataSetName)
	{
		if ((ccode = GetFieldToProcessSection(dataSetHandle, &fid,
				&dataSize, &dataBuffer, NULL)) isnt 0) {
			/*
			LogError(dataSetHandle->connection, GET_FIELD_ERR,
				    ReturnDataSetName(dataSetHandle), ccode);
			*/
			goto Return;
		}

#ifdef DEBUG
		logerror(PRIORITYWARN,"RestoreFullPaths fid %x\n", fid);
#endif

		switch (fid) {
		case NWSM_PATH_IS_FULLY_QUALIFIED:
			dataSetPathIsFullyQualified = *(UINT8 *)dataBuffer ;
			break ;

		case NWSM_NAME_SPACE_TYPE:
			if (nameSpaceType != NWSM_ALL_NAME_SPACES)
			{
				LogError(dataSetHandle->connection,
					INVALID_FULL_PATH_FORMAT, 1);
			}
			nameSpaceType = 0;
			SMDFGetUINT64(&dataSize, &length);
			memcpy(&nameSpaceType, dataBuffer,
				    _min(length, sizeof(nameSpaceType)));
#ifdef DEBUG
			logerror(PRIORITYWARN,"nameSpaceType %x\n", 
							nameSpaceType);
#endif
			nameSpaceType = SwapUINT32(nameSpaceType);
			break;

		case NWSM_PATH_NAME:
			if (nameSpaceType is NWSM_ALL_NAME_SPACES) {
				LogError(dataSetHandle->connection,
					INVALID_FULL_PATH_FORMAT, 2);
				continue;
			}
			if (nameSpaceType != NFSNameSpace) {
				nameSpaceType = NWSM_ALL_NAME_SPACES ;
				continue ;
			}

			SMDFGetUINT64(&dataSize, &pathLength);

			if ( dataSetHandle->dataSetWasRenamed != TRUE ) {
				if (!NWSMCopyString(&dataSetHandle->dataSetName, 
							dataBuffer, pathLength)) {
					ccode = NWSMTS_OUT_OF_MEMORY;
					goto Return;
				}
			}
#ifdef DEBUG
			logerror(PRIORITYWARN,"dataSetName %s\n", 
					dataSetHandle->dataSetName->string);
#endif

			dataSetHandle->nameSpaceType = nameSpaceType;
			break;

		} 
		// end switch
	}

	if ( dataSetHandle->dataSetWasRenamed == TRUE &&
			dataSetHandle->parentDataSetHandle == NULL ) {
		notFullyQualified = 0 ; /* This was already checked in Open */
	}	
	else if ( dataSetHandle->dataSetWasRenamed == TRUE || 
		dataSetPathIsFullyQualified == UNKNOWN_VALUE) {
		notFullyQualified = NWSMPathIsNotFullyQualified(NFSNameSpace,
				dataSetHandle->dataSetName->string);
	}
	else {
		if (dataSetPathIsFullyQualified != FULLY_QUALIFIED ) {
			notFullyQualified = 1 ;
		}
	}

	if (dataSetHandle->parentDataSetHandle == NULL) {
		if (notFullyQualified) {
			ccode = NWSMTS_INVALID_PATH;
			goto Return;
		}
		cp = strchr(dataSetHandle->dataSetName->string,':');
		if (cp != NULL) {
			memcpy(dataSetHandle->dataSetName->string,
							cp,strlen(cp)+1);
			dataSetHandle->dataSetName->string[0] = '/' ;
		}
	}
	else {
		if (notFullyQualified) {
			terminalName = dataSetHandle->dataSetName->string;
		}
		else {
			terminalName = (STRING)NWSMGetPathChild(NFSNameSpace,
				dataSetHandle->dataSetName->string, NULL);
		}

#ifdef DEBUG
		logerror(PRIORITYWARN,"terminalName %s\n", terminalName);
#endif

		if (!NWSMCopyString(&dataSetHandle->fullPath,
			dataSetHandle->parentDataSetHandle->dataSetName->string,
			-1) or !NWSMCatString(&dataSetHandle->fullPath, 
				terminalName, -1)) {
			ccode = NWSMTS_OUT_OF_MEMORY;
			goto Return;
		}
		NWSMFreeString(&dataSetHandle->dataSetName);
		dataSetHandle->dataSetName = dataSetHandle->fullPath ;
		dataSetHandle->fullPath = NULL ;
	}

#ifdef DEBUG
	logerror(PRIORITYWARN,"RestoreFullPaths path %s\n", 
					dataSetHandle->dataSetName->string);
#endif

	/* Check if the restore is being attempted on a read-only 
	   filesystem
	*/
	if ( GetResourceNumber(dataSetHandle->dataSetName->string,
			&resPtr, dataSetHandle->connection->targetInfo) == -1 ) {
		ccode = NWSMTS_INVALID_PATH;
		goto Return;
	}
	if ( resPtr->resourceFlags & FS_READONLY) {
		ccode = NWSMTS_ACCESS_DENIED ;
		goto Return;
	}

	dataSetHandle->resourcePtr = resPtr ;

	if (dataSetHandle->dataSetType == TYPE_FILE &&
		!(dataSetHandle->mode & NWSM_NO_DATA_STREAMS) &&
		dataSetHandle->currentDataStreamNumber != 0xFFFFFFFF) {
		switch (dataSetHandle->dataStreamType) {
			case NWSM_CLEAR_TEXT_DATA_STREAM :
				break ;
			case NWSM_COMPRESSED_DATA_STREAM :
				ccode = NWSMTS_COMPRESSION_CONFLICT ;
				goto Return ;
			default :
				ccode = NWSMTS_OPEN_DATA_STREAM_ERR ;
				goto Return ;
		}
	}

	if (!dataSetHandle->connection->dontCheckSelectionList &&
		(ccode = ExcludedByRestoreSelectionList(dataSetHandle)) != 0)
		goto Return;

	/* Scan to see if file exists */
	dataSetExists = !lstat(dataSetHandle->dataSetName->string,
				 		&dataSetInfo);

	if (dataSetHandle->dataSetType is TYPE_FILE) {
		if (dataSetExists) {
			if ( S_ISDIR(dataSetInfo.st_mode) ) {
				ccode = NWSMTS_INVALID_PATH ;
				goto Return ;
			}
			/* if the file exists and mode is 
			   NWSM_DO_NOT_OVERWRITE_DATA_SET then set ccode and 
			   state and go to return 
			*/
			if ((dataSetHandle->mode & NWSM_OPEN_MODE_MASK) == 
					NWSM_DO_NOT_OVERWRITE_DATA_SET) {
				ccode = NWSMTS_DATA_SET_ALREADY_EXISTS;
				dataSetHandle->state = STATE_DONE;
				goto Return;
			}
		}
		else {
			/* if the data set doesn't exist then create
			   the directory path and defer the creation of the 
			   file until the NFS characteristics section is 
			   is received. If there is no NFS characteristics 
			   section, create the file just before restoring the
			   data streams. If there is no data streams create an 
               empty file when the trailer is received.
			*/
			if ((ccode = CreateDirectory(
					dataSetHandle->dataSetName->string,
	    				CREATE_PARENT_DIRECTORY, 
					DEFAULT_MODE_FOR_DIRECTORY)) != 0 ) {
				/*
				LogError(dataSetHandle->connection,
					CREATE_DIR_ENTRY_ERR,
					ReturnDataSetName(dataSetHandle),ccode);
				*/
				goto Return;
			}
		}
	}
	else { /* TYPE_DIRECTORY */
		if (dataSetExists) {
			if ( !S_ISDIR(dataSetInfo.st_mode) ) {
				ccode = NWSMTS_INVALID_PATH ;
				goto Return ;
			}
			if ( (dataSetHandle->mode & NWSM_OPEN_MODE_MASK)
							== NWSM_CREATE_PARENT_HANDLE) {
				ccode = NWSMTS_VALID_PARENT_HANDLE;
				dataSetHandle->state = STATE_DONE;
				dataSetHandle->valid = VALID_RESTORE_DATA_SET;
				goto Return;
			}
		}
	}

Return:
	if (!ccode) {
		dataSetHandle->valid = VALID_RESTORE_DATA_SET;
		if ( dataSetExists ) {
			dataSetHandle->dataSetExists = dataSetExists ;
			memcpy(&dataSetHandle->dataSetInfo, &dataSetInfo,
					sizeof(dataSetInfo));
		}
	}

	return (ccode);
}

/*******************************************************************/
CCODE CreateDirectory(CHAR *fullPath,
	    NWBOOLEAN mode, mode_t accessMode)
{
	char *cp = NULL, *cp1 = NULL, *ptr = NULL, *last = NULL, 
			*endCharPtr = NULL ;
	CCODE ccode = 0 ;
	struct stat statb ;
	int len ;

	len = strlen(fullPath);

	if ( fullPath[len-1] == '/' ) {
		endCharPtr = fullPath + len - 1 ;
		*endCharPtr = NULL ;
	}
	cp = fullPath ;
	ptr = cp + 1 ;
	if ( mode == CREATE_PARENT_DIRECTORY ) {
		if ((last = strrchr(ptr,'/')) != NULL) {
			*last = NULL ;
		}
		else { // we are at root
			return(0);
		}
	}

	if ((cp1 = strchr(ptr,'/')) != NULL ) {
		*cp1 = NULL ;
	}

	for (;;) {
		if ( lstat(cp,&statb) == 0 ) {
			if ( !S_ISDIR(statb.st_mode) ) {
				if ( cp1 != NULL ) {
					*cp1 = '/' ;
				}
				ccode = NWSMTS_INVALID_PATH ;
				goto Return ;
			}
		}
		else {
			if ( mkdir(cp, accessMode) ) {
				if ( cp1 != NULL ) {
					*cp1 = '/' ;
				}
				ccode = NWSMTS_CREATE_DIR_ENTRY_ERR ;
				goto Return ;
			}
		}
		if ( cp1 == NULL ) {
			break ;
		}
		*cp1 = '/' ;
		ptr = ++cp1 ;
		if ((cp1 = strchr(ptr,'/')) != NULL) {
			*cp1 = NULL ;
		}
	}
Return :
	if ( last != NULL ) {
		*last = '/' ;
	}
	if ( endCharPtr != NULL ) {
		*endCharPtr = '/' ;
	}
	return(ccode);
}

/*******************************************************************/
