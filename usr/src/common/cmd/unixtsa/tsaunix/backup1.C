#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/backup1.C	1.6"

#include <tsad.h>
#include "tsaglobl.h"
#include <smsfids.h>

/****************************************************************************/
#define BEGIN_INDEX      0
#define HEADER_INDEX      1
#define DATA_STREAM_NUMBER_INDEX      2
#define DATA_STREAM_TYPE_INDEX      3
#define DATA_STREAM_SIZE_INDEX      4
#define DATA_STREAM_INVALID_INDEX   2
#define DATA_STREAM_CRC_INDEX      3
#define END_INDEX      5


static void InitDataStreamHeaderTable(
        NWSM_FIELD_TABLE_DATA *dataStreamHeaderTable,
        UINT8   tableSize)
{
        memset(dataStreamHeaderTable, '\0',
		sizeof(NWSM_FIELD_TABLE_DATA) * tableSize);
        if ( tableSize < END_INDEX + 1 )
                return ;

        dataStreamHeaderTable[BEGIN_INDEX].field.fid = 
														NWSM_BEGIN ;
        dataStreamHeaderTable[HEADER_INDEX].field.fid = 
											NWSM_DATA_STREAM_HEADER ;
        dataStreamHeaderTable[DATA_STREAM_NUMBER_INDEX].field.fid = 
											NWSM_DATA_STREAM_NUMBER;
        dataStreamHeaderTable[DATA_STREAM_TYPE_INDEX].field.fid = 
											NWSM_DATA_STREAM_TYPE ;
        dataStreamHeaderTable[DATA_STREAM_SIZE_INDEX].field.fid = 
											NWSM_DATA_STREAM_SIZE ;
        dataStreamHeaderTable[END_INDEX].field.fid = 
							NWSM_END ;
}

#define TRAILER_BEGIN_INDEX      0
#define DATA_STREAM_TRAILER_INDEX      1
#define DATA_STREAM_SKIP_FIELD_INDEX      2
#define TRAILER_END_INDEX      3

static void InitDataStreamTrailerTable(
        NWSM_FIELD_TABLE_DATA *dataStreamTrailerTable,
        UINT8   tableSize)
{
        memset(dataStreamTrailerTable, '\0',
		sizeof(NWSM_FIELD_TABLE_DATA) * tableSize);
        if ( tableSize < TRAILER_END_INDEX + 1 )
                return ;

        dataStreamTrailerTable[TRAILER_BEGIN_INDEX].field.fid = NWSM_BEGIN ;
        dataStreamTrailerTable[DATA_STREAM_TRAILER_INDEX].field.fid = NWSM_DATA_STREAM_TRAILER ;
        dataStreamTrailerTable[DATA_STREAM_SKIP_FIELD_INDEX].field.fid = NWSM_SKIP_FIELD;
        dataStreamTrailerTable[TRAILER_END_INDEX].field.fid = NWSM_END ;
}

CCODE BackupDataStreams(BACKUP_DATA_SET_HANDLE *dataSetHandle,
UINT32 *bytesToRead, UINT32 *bytesRead, BUFFERPTR *buffer)
{
	CCODE            ccode = 0;
	unsigned int     _bytesRead = 0;
	unsigned int     _bytesToRead = 0;
	SMFILE_HANDLE    *smFileHandle;
	UINT32	fileSize ;
	UINT32  dataStreamNumber = NWSM_PRIMARY_DATA_STREAM_NUM ;
	UINT32  dataStreamType = NWSM_CLEAR_TEXT_DATA_STREAM ;
	UINT8	symlinkFile = 0 ;
	NWSM_FIELD_TABLE_DATA 
		dataStreamHeaderTable[END_INDEX + 1], 
		dataStreamTrailerTable[TRAILER_END_INDEX + 1];
	BUFFERPTR bufferPtr ;

	smFileHandle = &dataSetHandle->smFileHandle;

	//   Take care of 0 length files here by ignoring their data stream 
	if (!smFileHandle->size)
		goto Return;

	if (smFileHandle->outputDataStreamHeader)
	{
		InitDataStreamHeaderTable(dataStreamHeaderTable,
								END_INDEX + 1);
		InitBufferState(dataSetHandle);

		// Set the primary data stream number
		dataStreamHeaderTable[DATA_STREAM_NUMBER_INDEX].dataSizeMap =
		dataStreamHeaderTable[DATA_STREAM_NUMBER_INDEX].sizeOfData =
				SMDFSizeOfUINT32Data(dataStreamNumber) ;
		dataStreamNumber = SwapUINT32(dataStreamNumber);
		dataStreamHeaderTable[DATA_STREAM_NUMBER_INDEX].field.data =
					&dataStreamNumber ;

		// Set the data stream type
		dataStreamHeaderTable[DATA_STREAM_TYPE_INDEX].dataSizeMap =
		dataStreamHeaderTable[DATA_STREAM_TYPE_INDEX].sizeOfData =
				SMDFSizeOfUINT32Data(dataStreamType) ;
		dataStreamType = SwapUINT32(dataStreamType);
		dataStreamHeaderTable[DATA_STREAM_TYPE_INDEX].field.data =
					&dataStreamType ;

		// Set the size of the file data fid 
		dataStreamHeaderTable[DATA_STREAM_SIZE_INDEX].dataSizeMap =
		dataStreamHeaderTable[DATA_STREAM_SIZE_INDEX].sizeOfData =
			SMDFSizeOfUINT32Data(smFileHandle->size);
		fileSize = SwapUINT32(smFileHandle->size);
		dataStreamHeaderTable[DATA_STREAM_SIZE_INDEX].field.data =
						    &fileSize ;

		// Put out the header table 
		if ((ccode = PutFields(buffer, bytesToRead, 
				dataStreamHeaderTable, bytesRead, TRUE, 
			NWSM_DATA_STREAM_HEADER, dataSetHandle, NULL)) isnt 0)
			goto Return;

		/* the above PutFields call has built the dataStream header so set flag to false */
		smFileHandle->outputDataStreamHeader = FALSE;

		// If there is no more room in the buffer then exit - This call
	 	// is made repeatedly until all file data is returned 
	 	if (!*bytesToRead)
			goto Return;
	}


	if(dataSetHandle->bufferState == STATE_DATA_SYMLINK_STATE){
		_bytesRead = _min(*bytesToRead, dataSetHandle->stateDataSize);
		memcpy(*buffer,
			dataSetHandle->stateData + dataSetHandle->stateDataOffset, 
			_bytesRead);

		smFileHandle->position += _bytesRead;
		// Adjust the buffer pointer 
		*buffer += _bytesRead;
		*bytesRead += _bytesRead;
		*bytesToRead -= _bytesRead;

		if (dataSetHandle->stateDataSize > _bytesRead) {
			dataSetHandle->stateDataOffset += _bytesRead;
			dataSetHandle->stateDataSize -= _bytesRead;
			return(0);
		}
		dataSetHandle->stateDataOffset = 0;
		dataSetHandle->stateDataSize = 0;
		dataSetHandle->bufferState = BUFFER_STATE ;
		goto PutTrailer ;
	}

	// Determine the amount of data to read - this is based on how 
	// much data there is and on how much room there is in the buffer
	// for the data 
	_bytesToRead = smFileHandle->size - smFileHandle->position;

	if (S_ISLNK(dataSetHandle->dataSetSequence->scanInformation->attributes)){
		symlinkFile = 1 ;
		if ( _bytesToRead > *bytesToRead ) {
			/* the buffer is not enough to hold the complete symlink
			   path. So use the state data buffer.
			*/
			if (!dataSetHandle->stateData) {
				if ((dataSetHandle->stateData =
					(char *) malloc(STATE_DATA_SIZE)) == NULL) {
					ccode = NWSMTS_OUT_OF_MEMORY;
					goto Return;
				}
				memset(dataSetHandle->stateData,'\0',STATE_DATA_SIZE);
			}
			bufferPtr = dataSetHandle->stateData ;
			dataSetHandle->bufferState = STATE_DATA_SYMLINK_STATE ;
		}
		else {
			bufferPtr = *buffer ;
		}
	}
	else {
		_bytesToRead = _min(*bytesToRead, _bytesToRead);
	}

	// If the dataStream is corrupt then increment the position in the file 
	if (smFileHandle->dataStreamIsCorrupt)
		smFileHandle->position += _bytesToRead;
	else if ( symlinkFile ) {
		NWSM_DATA_SET_NAME name;
		char *namePtr , path[MAXPATHLEN];

		memset(&name,'\0',sizeof(NWSM_DATA_SET_NAME));
		if ((ccode = NWSMGetOneName(
			(BUFFERPTR)dataSetHandle->dataSetSequence->dataSetNames,
							&name)) != 0){
			goto Return;
		}
 
		if ( strncmp(name.name,TSAGetMessage(ROOT_OF_FS),
				strlen(TSAGetMessage(ROOT_OF_FS))) == 0 ) {
			strcpy( path, name.name ) ;
		}
		else {
			strcpy(path, dataSetHandle->dataSetSequence->fullPath->string);
			strcat( path, name.name ) ;
		}
		namePtr = strchr(path,':');
		*namePtr = '/' ;

		ccode = readlink(namePtr,bufferPtr,_bytesToRead) ;
		if ( (int)ccode == -1 || ccode != _bytesToRead) {
			int savedErr = errno ;
			LogSkippedDataSets(dataSetHandle->dataSetSequence, 
						errno) ;
			logerror(PRIORITYWARN,
				TSAGetMessage(READLINK_ERROR),
			    	namePtr, savedErr);
			LogError(dataSetHandle->connection, READLINK_ERROR,
			    			namePtr, savedErr);
			ccode = HandleUnixError(savedErr);
		}
		else {
			if(dataSetHandle->bufferState == STATE_DATA_SYMLINK_STATE){
				_bytesRead = *bytesToRead ;
				memcpy(*buffer,bufferPtr, _bytesRead);
				dataSetHandle->stateDataSize = 
						_bytesToRead - _bytesRead ;
				dataSetHandle->stateDataOffset = _bytesRead ;
			}
			else {
				_bytesRead = _bytesToRead ;
			}
			smFileHandle->position += _bytesRead;
		}

		NWSMFreeName(&name);
	}
	// Otherwise read the file data into the buffer - if there is an
	//   error reading the data then set the dataStreamIsCorrupt flag 
	else if (ReadFile(smFileHandle, _bytesToRead, buffer, &_bytesRead) 
			|| _bytesRead != _bytesToRead) {
		LogSkippedDataSets(dataSetHandle->dataSetSequence, errno) ;
		smFileHandle->dataStreamIsCorrupt = TRUE;
		smFileHandle->position += _bytesToRead - _bytesRead;
		logerror(PRIORITYWARN, TSAGetMessage(TSA_READ_ERROR),
				    	ReturnDataSetName(dataSetHandle));
		LogError(dataSetHandle->connection, TSA_READ_ERROR,
				    	ReturnDataSetName(dataSetHandle));
	}

	// Adjust the buffer pointer - this is done even if the data 
	// stream is corrupt because we still need to adjustthe buffer 
	*buffer += _bytesRead;
	*bytesRead += _bytesRead;
	*bytesToRead -= _bytesRead;

PutTrailer:
	// If the file position is the file size then we are at 
	// the end of the dataStream - put the trailer out 
	if (smFileHandle->position is smFileHandle->size)
	{
		InitBufferState(dataSetHandle);
		InitDataStreamTrailerTable(dataStreamTrailerTable,
								TRAILER_END_INDEX + 1);

		//   If data stream is invalid then build invalid field 
	    	if (smFileHandle->dataStreamIsCorrupt)
			dataStreamTrailerTable[DATA_STREAM_INVALID_INDEX].field.fid =
				NWSM_DATA_STREAM_IS_INVALID;

		else
			dataStreamTrailerTable[DATA_STREAM_INVALID_INDEX].field.fid =
		    	NWSM_SKIP_FIELD;

		//   Put out the dataStream trailer 
	    	ccode = PutFields(buffer, bytesToRead, dataStreamTrailerTable,
	    			bytesRead, TRUE, NWSM_DATA_STREAM_TRAILER,
	    			dataSetHandle, NULL);
	}

Return:

	return (ccode);
}

#undef BEGIN_INDEX      
#undef HEADER_INDEX    
#undef DATA_STREAM_NUMBER_INDEX      
#undef DATA_STREAM_TYPE_INDEX      
#undef DATA_STREAM_SIZE_INDEX
#undef DATA_STREAM_INVALID_INDEX
#undef DATA_STREAM_CRC_INDEX
#undef END_INDEX      

#undef TRAILER_BEGIN_INDEX      
#undef DATA_STREAM_TRAILER_INDEX      
#undef DATA_STREAM_SKIP_FIELD_INDEX  
#undef TRAILER_END_INDEX

#define BEGIN_INDEX		0
#define HEADER_INDEX    1
#define EXCLUDE_OPTIONS_INDEX	2
#define END_INDEX		3

static void InitHeaderTrailerTable(
        NWSM_FIELD_TABLE_DATA *headerTrailerTable,
        UINT8   tableSize)
{
        memset(headerTrailerTable, '\0',
			sizeof(NWSM_FIELD_TABLE_DATA) * tableSize);
        if ( tableSize < END_INDEX + 1 )
                return ;

        headerTrailerTable[BEGIN_INDEX].field.fid = NWSM_BEGIN ;
        headerTrailerTable[EXCLUDE_OPTIONS_INDEX].field.fid = 
										NWSM_SKIP_FIELD ;
        headerTrailerTable[END_INDEX].field.fid = NWSM_END ;
}

/****************************************************************************/

CCODE CreateHeaderOrTrailer(BACKUP_DATA_SET_HANDLE *dataSetHandle,
UINT32 *bytesToRead, UINT32 *bytesRead, BUFFERPTR *buffer, NWBOOLEAN isHeader)
{
	UINT32 fid;
	NWSM_FIELD_TABLE_DATA headerTrailerTable[END_INDEX + 1] ;
	UINT8 excludeOptions = 0 ;

	InitHeaderTrailerTable(headerTrailerTable, END_INDEX + 1);
	InitBufferState(dataSetHandle);

	switch (dataSetHandle->dataSetSequence->dataSetType)
	{
	    case TYPE_FILESYSTEM:
		fid = isHeader ? NWSM_VOLUME_HEADER : NWSM_VOLUME_TRAILER;
		break;

	    case TYPE_FILE:
		fid = isHeader ? NWSM_FILE_HEADER : NWSM_FILE_TRAILER;
		if ((dataSetHandle->mode & NWSM_NO_DATA_STREAMS) ||
			(dataSetHandle->dataSetSequence->scanControl->scanType &
				NWSM_EXCLUDE_DATA_STREAMS) ) {
			SMDFSetBit1(excludeOptions) ;
		}
		break;

	    case TYPE_DIRECTORY:
		fid = isHeader ? NWSM_DIRECTORY_HEADER : NWSM_DIRECTORY_TRAILER;
		break;
	}

	headerTrailerTable[HEADER_INDEX].field.fid = fid;
	if ( excludeOptions ) {
		headerTrailerTable[EXCLUDE_OPTIONS_INDEX].field.fid =
							NWSM_BACKUP_OPTIONS ;
		headerTrailerTable[EXCLUDE_OPTIONS_INDEX].dataSizeMap = excludeOptions ;
	}
	return (PutFields(buffer, bytesToRead, headerTrailerTable,
			    bytesRead, TRUE, fid, dataSetHandle, NULL));
}

#undef BEGIN_INDEX		
#undef HEADER_INDEX
#undef EXCLUDE_OPTIONS_INDEX
#undef END_INDEX	

/****************************************************************************/
/****************************************************************************/
 
#define BEGIN_INDEX		0
#define HEADER_INDEX    1
#define OFFSET_TO_END_INDEX    2
#define FILE_ACCESS_MODE_INDEX 3 
#define GROUP_ID_INDEX         4
#define RDEVICE_INDEX          5
#define NUMBER_OF_LINKS        6
#define LINKED_INDEX           7
#define FIRST_CREATED_INDEX    8
#define ACS_FLAGS_INDEX        9
#define USER_ID_INDEX          10
#define MY_FLAGS_INDEX         11
#define FSID_INDEX             12
#define FILEID_INDEX           13
#define END_INDEX		14
 
static void InitNFSCharacteristics(
        NWSM_FIELD_TABLE_DATA *NFSCharacteristics,
        UINT8   tableSize)
{
	memset(NFSCharacteristics,'\0',
			sizeof(NWSM_FIELD_TABLE_DATA) * tableSize);
	if ( tableSize < END_INDEX + 1 )
		return ;

	NFSCharacteristics[BEGIN_INDEX].field.fid = NWSM_BEGIN ;
	NFSCharacteristics[HEADER_INDEX].field.fid = NWSM_NFS_CHARACTERISTICS;

	NFSCharacteristics[OFFSET_TO_END_INDEX].field.fid = NWSM_OFFSET_TO_END ;
	NFSCharacteristics[OFFSET_TO_END_INDEX].sizeOfData = sizeof(UINT32);
	NFSCharacteristics[OFFSET_TO_END_INDEX].addressOfData = GET_ADDRESS ;
	NFSCharacteristics[OFFSET_TO_END_INDEX].dataSizeMap = sizeof(UINT32) ;

	NFSCharacteristics[FILE_ACCESS_MODE_INDEX].field.fid = NWSM_NFS_FILE_ACCESS_MODE;
	NFSCharacteristics[FILE_ACCESS_MODE_INDEX].sizeOfData = sizeof(UINT32); 
	NFSCharacteristics[FILE_ACCESS_MODE_INDEX].dataSizeMap =  sizeof(UINT32); 
 
	NFSCharacteristics[GROUP_ID_INDEX].field.fid = NWSM_NFS_GROUP_OWNER_ID ;
	NFSCharacteristics[GROUP_ID_INDEX].sizeOfData = sizeof(UINT32); 
	NFSCharacteristics[GROUP_ID_INDEX].dataSizeMap =  sizeof(UINT32); 
 
	NFSCharacteristics[RDEVICE_INDEX].field.fid = NWSM_NFS_RDEVICE ;
	NFSCharacteristics[RDEVICE_INDEX].sizeOfData = sizeof(INT32); 
	NFSCharacteristics[RDEVICE_INDEX].dataSizeMap =  sizeof(INT32); 
 
	NFSCharacteristics[NUMBER_OF_LINKS].field.fid = NWSM_NFS_NUMBER_OF_LINKS ;
	NFSCharacteristics[NUMBER_OF_LINKS].sizeOfData = sizeof(UINT32); 
	NFSCharacteristics[NUMBER_OF_LINKS].dataSizeMap =  sizeof(UINT32); 
 
	NFSCharacteristics[LINKED_INDEX].field.fid = NWSM_NFS_LINKED_FLAG ;
	NFSCharacteristics[LINKED_INDEX].sizeOfData = sizeof(UINT8); 
	NFSCharacteristics[LINKED_INDEX].dataSizeMap =  sizeof(UINT8); 
 
	NFSCharacteristics[FIRST_CREATED_INDEX].field.fid = NWSM_NFS_FIRST_CREATED_FLAG ;
	NFSCharacteristics[FIRST_CREATED_INDEX].sizeOfData = sizeof(UINT8); 
	NFSCharacteristics[FIRST_CREATED_INDEX].dataSizeMap =  sizeof(UINT8); 
 
	NFSCharacteristics[ACS_FLAGS_INDEX].field.fid = NWSM_NFS_ACS_FLAGS ;
	NFSCharacteristics[ACS_FLAGS_INDEX].sizeOfData = sizeof(UINT8); 
	NFSCharacteristics[ACS_FLAGS_INDEX].dataSizeMap =  sizeof(UINT8); 
 
	NFSCharacteristics[USER_ID_INDEX].field.fid = NWSM_NFS_USER_ID ;
	NFSCharacteristics[USER_ID_INDEX].sizeOfData = sizeof(UINT32); 
	NFSCharacteristics[USER_ID_INDEX].dataSizeMap =  sizeof(UINT32); 
 
	NFSCharacteristics[MY_FLAGS_INDEX].field.fid = NWSM_NFS_MY_FLAGS ;
	NFSCharacteristics[MY_FLAGS_INDEX].sizeOfData = sizeof(UINT32); 
	NFSCharacteristics[MY_FLAGS_INDEX].dataSizeMap =  sizeof(UINT32); 
 
	NFSCharacteristics[FSID_INDEX].field.fid = NWSM_NFS_FSID ;
	NFSCharacteristics[FSID_INDEX].sizeOfData = sizeof(UINT32); 
	NFSCharacteristics[FSID_INDEX].dataSizeMap =  sizeof(UINT32); 
 
	NFSCharacteristics[FILEID_INDEX].field.fid = NWSM_NFS_FILEID ;
	NFSCharacteristics[FILEID_INDEX].sizeOfData = sizeof(UINT32); 
	NFSCharacteristics[FILEID_INDEX].dataSizeMap =  sizeof(UINT32); 
 
	NFSCharacteristics[END_INDEX].field.fid = NWSM_END ;
}

#define HARDLINKEND 0x0001      /* terminal hard links */
#define SYMBOLICLINK    0x0002      /* symbolic link */
#define HARDLINK    0x0004      /* non-terminal hard links */
#define TRANSIT_LINK    0x0008      /* for backup/restore purpose */

#define GETTING_EXT_ENTRY       0x0001
#define GID_NOT_SET         0x0002
#define USING_NFS_GID           0x0004  /* transition code */
#define USING_NFS_FMODE         0x0008  /* for dir created by sms */

#define LASTCHANGED_UNIX    0x0001      /* acs last changed by UNIX */
#define LASTCHANGED_NW      0x0002      /* acs last chagned by DOS */
 
CCODE BackupNFSCharacteristics(BACKUP_DATA_SET_HANDLE *dataSetHandle,
UINT32 *bytesToRead, UINT32 *bytesRead, BUFFERPTR *buffer)
{
	CCODE ccode = 0;
	UINT32 numberOfLinks, userID;
	DATA_SET_SEQUENCE *dataSetSequence = dataSetHandle->dataSetSequence;
	struct stat *statbp ;
	NWSM_FIELD_TABLE_DATA nfsCharacteristics[END_INDEX + 1] ; 
	UINT32  fileAccessMode;
	UINT32  groupOwnerID;
	INT32  RDevice = 0 ;
	UINT8   linkedFlag;
	UINT8   firstCreatedFlag = 1;
	UINT8  acSFlagsBit = 0 ;
	UINT32   myFlagsBit = USING_NFS_FMODE | USING_NFS_GID ;
	UINT32  fsid, fileid ;

	 
	InitNFSCharacteristics(nfsCharacteristics,END_INDEX + 1);

	InitBufferState(dataSetHandle);
 
	statbp = &dataSetSequence->directoryStack->dirEntry.statb ;

	fileAccessMode = statbp->st_mode;

	if ( S_ISSPECIAL(statbp->st_mode) ) {
		switch(statbp->st_mode & S_IFMT) {
			case S_IFIFO :
				fileAccessMode &= ~S_IFMT ;
				fileAccessMode |= S_IFCHR ;
				RDevice = -1 ;
				break ;
			default :
				RDevice = statbp->st_rdev ;
				break ;
		}
	}

	RDevice = SwapUINT32(RDevice);
	groupOwnerID = statbp->st_gid ;
	groupOwnerID = SwapUINT32(groupOwnerID);
	fileAccessMode = SwapUINT32(fileAccessMode);
	numberOfLinks = statbp->st_nlink ;
	if ( S_ISLNK(statbp->st_mode) ) {
		linkedFlag = SYMBOLICLINK ;
	}
	else {
		linkedFlag = 0 ;
	}
	numberOfLinks = SwapUINT32(numberOfLinks);
	userID = statbp->st_uid ;
	userID = SwapUINT32(userID);

	fsid = statbp->st_dev ;
	fsid = SwapUINT32(fsid);

	fileid = statbp->st_ino ;
	fileid = SwapUINT32(fileid);

	myFlagsBit = SwapUINT32(myFlagsBit);

	nfsCharacteristics[FILE_ACCESS_MODE_INDEX].field.data = &fileAccessMode;
	nfsCharacteristics[GROUP_ID_INDEX].field.data = &groupOwnerID;
	nfsCharacteristics[RDEVICE_INDEX].field.data = &RDevice;
	
	nfsCharacteristics[NUMBER_OF_LINKS].field.data = &numberOfLinks;
	nfsCharacteristics[LINKED_INDEX].field.data = &linkedFlag;
	nfsCharacteristics[FIRST_CREATED_INDEX].field.data = &firstCreatedFlag;
	nfsCharacteristics[ACS_FLAGS_INDEX].field.data = &acSFlagsBit;
	nfsCharacteristics[USER_ID_INDEX].field.data = &userID ;
	nfsCharacteristics[MY_FLAGS_INDEX].field.data = &myFlagsBit;
	nfsCharacteristics[FSID_INDEX].field.data = &fsid;
	nfsCharacteristics[FILEID_INDEX].field.data = &fileid;
 
	ccode = PutFields(buffer, bytesToRead, nfsCharacteristics, 
					bytesRead, TRUE, NWSM_NFS_CHARACTERISTICS, 
					dataSetHandle, NULL) ;
	return (ccode);
}

#undef BEGIN_INDEX		
#undef HEADER_INDEX
#undef OFFSET_TO_END_INDEX
#undef FILE_ACCESS_MODE_INDEX
#undef GROUP_ID_INDEX
#undef RDEVICE_INDEX
#undef NUMBER_OF_LINKS
#undef LINKED_INDEX
#undef FIRST_CREATED_INDEX
#undef USER_ID_INDEX
#undef ACS_FLAGS_INDEX
#undef MY_FLAGS_INDEX
#undef END_INDEX	
