#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/restore3.C	1.9"

#include <errno.h>
#include <tsad.h>
#include "tsaglobl.h"
#include <utime.h>
#include <smsfids.h>

STATIC CCODE GetHeader(RESTORE_DATA_SET_HANDLE *dataSetHandle, 
			UINT32 *dataStreamNumber) ;
STATIC CCODE GetTrailer(RESTORE_DATA_SET_HANDLE *dataSetHandle, 
			NWBOOLEAN *dataStreamIsInvalid);

/********************************************************************/
int HandleUnixError(int errorNum)
{
	int smtsErrno ;

	switch ( errorNum ) {
	case ETXTBSY :      /* Text file busy                       */
	case EPERM   :      /* Not super-user                       */
	case EACCES  :      /* Permission denied                    */
	case EROFS   :      /* Read only file system                */
	case EBUSY   :      /* Mount device busy                    */
	case ENOLINK :      /* the link has been severed */
		smtsErrno =  NWSMTS_ACCESS_DENIED ;
		break;
	case ENOENT  :       /* No such file or directory            */
		smtsErrno =  NWSMTS_DATA_SET_NOT_FOUND ;
		break;
	case ENOSR   :      /* out of streams resources             */
	case ENOMEM  :      /* Not enough core                      */
		smtsErrno =  NWSMTS_OUT_OF_MEMORY  ;
		break;
	case EEXIST  :      /* File exists                          */
		smtsErrno = NWSMTS_DATA_SET_ALREADY_EXISTS ;
		break;
	case ENOTBLK :      /* Block device required                */
	case EISDIR  :      /* Is a directory                       */
	case ENOTDIR :      /* Not a directory                      */
	case EMLINK  :      /* Too many links                       */
	case ELOOP   :      /* Symbolic link loop */
	case ENAMETOOLONG : /* path name is too long */
	case ENODEV  :      /* No such device                       */
		smtsErrno =  NWSMTS_INVALID_PATH ;
		break;
	case E2BIG   :       /* Arg list too long                    */
	case ENOEXEC :       /* Exec format error                    */
	case EINVAL  :      /* Invalid argument                     */
	case EBADF   :       /* Bad file number                      */
	case EFAULT  :      /* Bad address                          */
	case EXDEV   :      /* Cross-device link                    */
		smtsErrno =  NWSMTS_INVALID_PARAMETER  ;
		break;
	case ENOSPC  :      /* No space left on device              */
	case EFBIG   :      /* File too large                       */
		smtsErrno =  NWSMTS_OUT_OF_DISK_SPACE  ;
		break;
	case ENOSYS  :       /* Unsupported file system operation */
		smtsErrno =  NWSMTS_UNSUPPORTED_FUNCTION  ;
		break ;
	case ENXIO   :       /* No such device or address            */
	case ENOLCK  :      /* No record locks available.           */
		smtsErrno = NWSMTS_OPEN_ERROR ;
		break;

	case EINTR   :       /* interrupted system call              */
	case EIO     :       /* I/O error                            */
	case EAGAIN  :      /* No more processes                    */
	case ENFILE  :      /* File table overflow                  */
	case EMFILE  :      /* Too many open files                  */
	case ESPIPE  :      /* Illegal seek                         */
	case EPIPE   :      /* Broken pipe                          */
	case EDEADLK :      /* Deadlock condition.                  */
	case EILSEQ :       /* Illegal byte sequence. */
	case ERESTART :     /* Restartable system call */
	case ENOTEMPTY :    /* directory not empty */
	case EUSERS  :      /* Too many users (for UFS) */
		smtsErrno = errorNum;
		break;
	}


	return(smtsErrno);
}

CCODE SetAttributes(RESTORE_DATA_SET_HANDLE *dataSetHandle)
{
	struct utimbuf timebuf ;

#ifdef DEBUG
	logerror(PRIORITYWARN,"SetAttributes 1:name %s\n",
			dataSetHandle->dataSetName->string);
	FLUSHLOG ;
#endif

	timebuf.actime = dataSetHandle->smFileHandle.LastAccessDateAndTime ; 
	timebuf.modtime = dataSetHandle->smFileHandle.LastModifyDateAndTime ; 

#ifdef DEBUG
	logerror(PRIORITYWARN,"SetAttributes:name %s, mtime %s ",
			dataSetHandle->dataSetName->string, ctime(&timebuf.modtime));
	logerror(PRIORITYWARN,"atime %s\n", ctime(&timebuf.actime));
	FLUSHLOG ;
#endif

		

	if (utime(dataSetHandle->dataSetName->string, &timebuf) == -1) {
		return(HandleUnixError(errno));
	}

	if (chmod(dataSetHandle->dataSetName->string,
				dataSetHandle->unixChars.mode) == -1) {
		return(HandleUnixError(errno));
	}

	if (lchown(dataSetHandle->dataSetName->string,
			dataSetHandle->unixChars.userID,
			dataSetHandle->unixChars.groupID) == -1) {
		if ( errno == EINVAL ) { /* uid or gid out of range */
			if (lchown(dataSetHandle->dataSetName->string,
				dataSetHandle->connection->loginUserID,
				dataSetHandle->connection->loginGroupID)== -1){
					return(HandleUnixError(errno));
			}
		}
		else {
			return(HandleUnixError(errno));
		}
	}

	return(0);
}

#define CHECK_ACCESS(userID,groupID,loginUserID,loginGroupID,mode)	\
	loginUserID && (loginUserID != userID && (loginGroupID != groupID || \
	(mode & S_IRWXG) != S_IRWXG ) && (mode & S_IRWXO) != S_IRWXO)

CCODE CreateDataSet(RESTORE_DATA_SET_HANDLE *dataSetHandle)
{
	CCODE ccode = 0 ;
	int fd ;

#ifdef DEBUG
	logerror(PRIORITYWARN,
				"CreateDataSet path %s, mode %o, restoreFlag %d\n",
				dataSetHandle->dataSetName->string,
				dataSetHandle->unixChars.mode,
				dataSetHandle->restoreFlag);
	FLUSHLOG ;
#endif

	if ( dataSetHandle->restoreFlag == RESTORE_DATA_SET_SKIPPED ){
		dataSetHandle->skippingDataStream = TRUE;
		goto Return;
	}
	switch ( dataSetHandle->unixChars.mode & S_IFMT ) {
		case S_IFCHR :
			if ( dataSetHandle->unixChars.rdevice == -1 ) {
				dataSetHandle->unixChars.mode &= ~S_IFMT ;
				dataSetHandle->unixChars.mode |= S_IFIFO ;
			}
		case S_IFIFO :
		case S_IFBLK :
			if ( dataSetHandle->dataSetExists ) {
				if (unlink(dataSetHandle->dataSetName->string) == -1){
					ccode = HandleUnixError(errno);
					goto Return;
				}
			}
			if (mknod(dataSetHandle->dataSetName->string,
				dataSetHandle->unixChars.mode,
				dataSetHandle->unixChars.rdevice) == -1) {
				ccode = HandleUnixError(errno);
				goto Return;
			}
			if ((ccode = SetAttributes(dataSetHandle)) != 0 ) {
				goto Return;
			}
			dataSetHandle->restoreFlag = RESTORE_DATA_SET_DONE ;
			break ;
		case S_IFLNK :
			/* Wait till we get the data streams */
			goto Return;

		case S_IFDIR :
			if ( CHECK_ACCESS(dataSetHandle->unixChars.userID, 
				dataSetHandle->unixChars.groupID, 
				dataSetHandle->connection->loginUserID,
				dataSetHandle->connection->loginGroupID,
				dataSetHandle->unixChars.mode) ) {
				dataSetHandle->unixChars.userID = 
					dataSetHandle->connection->loginUserID;
				dataSetHandle->unixChars.groupID = 
					dataSetHandle->connection->loginGroupID;
				dataSetHandle->unixChars.mode = 
						DEFAULT_MODE_FOR_DIRECTORY ;
			}
#ifdef DEBUG
	logerror(PRIORITYWARN, "Calling Create directory\n");
	FLUSHLOG ;
#endif
			if ((ccode = CreateDirectory(
				dataSetHandle->dataSetName->string, 0,
				dataSetHandle->unixChars.mode)) != 0) {
				/*
				LogError(dataSetHandle->connection,
					CREATE_DIR_ENTRY_ERR,
					ReturnDataSetName(dataSetHandle),ccode);
				*/
				goto Return;
			}
			if ((ccode = SetAttributes(dataSetHandle)) != 0 ) {
				goto Return;
			}
			dataSetHandle->restoreFlag = RESTORE_DATA_SET_DONE ;
			break ;

		case S_IFREG :
			if ( dataSetHandle->dataSetExists ) {
				if (dataSetHandle->mode & NWSM_NO_DATA_STREAMS) {
					if ((ccode = SetAttributes(dataSetHandle)) != 0 ) {
						goto Return;
					}
					dataSetHandle->skippingDataStream = TRUE;
					dataSetHandle->restoreFlag = RESTORE_DATA_SET_DONE ;
					break ;
				}
				if (unlink(dataSetHandle->dataSetName->string) == -1){
					ccode = HandleUnixError(errno);
					goto Return;
				}
			}
			if ((fd = open(dataSetHandle->dataSetName->string,
					O_WRONLY | O_CREAT | O_TRUNC,
					dataSetHandle->unixChars.mode)) == -1) {
				ccode = HandleUnixError(errno);
				goto Return;
			}
			if (dataSetHandle->mode & NWSM_NO_DATA_STREAMS) {
				/* Just close the file created */
				close(fd);
				if ((ccode = SetAttributes(dataSetHandle)) != 0 ) {
					goto Return;
				}
				dataSetHandle->skippingDataStream = TRUE;
				dataSetHandle->restoreFlag = RESTORE_DATA_SET_DONE ;
				break ;
			}
				
			dataSetHandle->smFileHandle.FileHandle = fd ;
			dataSetHandle->smFileHandle.valid = VALID_SMFILE ;
			dataSetHandle->restoreFlag = 
						RESTORE_DATA_SET_JUST_CREATED ;
			break ;

		case S_IFSOCK :
		default :
			dataSetHandle->restoreFlag = RESTORE_DATA_SET_SKIPPED ;
			dataSetHandle->skippingDataStream = TRUE;
			goto Return;
	}
Return:
	return(ccode);
}

static NWBOOLEAN TestForUpdate(RESTORE_DATA_SET_HANDLE *dataSetHandle, 
	struct stat *statp)
{
	if ((dataSetHandle->mode & NWSM_OPEN_MODE_MASK) == 
							NWSM_UPDATE_DATA_SET && 
		statp->st_mtime >
			dataSetHandle->smFileHandle.LastModifyDateAndTime ) {
		return(FALSE);
	}
	return(TRUE);
}

#define DATA_STREAM_HEADER_NAME_LEN 80
CCODE RestoreDataStreams(RESTORE_DATA_SET_HANDLE *dataSetHandle, UINT32 *bytesToWrite, BUFFERPTR *buffer)
{
	CCODE ccode = 0, tcode = 0;
	UINT32 _bytesToWrite, bytesWritten;
	NWBOOLEAN dataStreamIsInvalid;
	UINT32 dataStreamNumber = 0xFFFFFFFF ;
	static char symlinkPath[MAXPATHLEN + 1] ;

	// If we are at the begining of a dataStream, then get the data stream 
	// header, if the data stream isnt a PrimaryDataStream then
	// skip the data stream 
#ifdef DEBUG
	logerror(PRIORITYWARN, "in RestoreDataStreams, inDataStream %d\n",
		dataSetHandle->inDataStream);
#endif
	if (dataSetHandle->inDataStream == FALSE) {
		if ((ccode = GetHeader(dataSetHandle, &dataStreamNumber)) 
									!= 0)
			goto Return;

		if ( dataSetHandle->skippingDataStream != TRUE ) {
			if (dataSetHandle->restoreFlag == RESTORE_DATA_SET_SKIPPED){
				dataSetHandle->skippingDataStream = TRUE;
			}
			else if (dataSetHandle->dataSetType == TYPE_FILE) {
				if (dataStreamNumber != NWSM_PRIMARY_DATA_STREAM_NUM) {
					dataSetHandle->skippingDataStream = TRUE;
				}
				else if ( S_ISLNK(dataSetHandle->unixChars.mode) ) {
					if (dataSetHandle->mode & NWSM_NO_DATA_STREAMS) {
						dataSetHandle->skippingDataStream=TRUE;
					}
					else if ( !dataSetHandle->bytesLeftInDataStream ||
						dataSetHandle->bytesLeftInDataStream >
								MAXPATHLEN ) {
						/*
						LogError(dataSetHandle->connection, 
					    	INVALID_DATA_STREAM,
					    	ReturnDataSetName(dataSetHandle));
						*/
						dataSetHandle->skippingDataStream=TRUE;
					}
					else {
						/* initialize the symlinkPath */
						memset(symlinkPath,'\0',MAXPATHLEN+1);
						dataSetHandle->smFileHandle.position = 0 ;
					}
				}
				else if ( dataSetHandle->restoreFlag ==  
						RESTORE_DATA_SET_NOT_YET_CREATED ){
					if ((ccode = CreateDataSet(dataSetHandle)) != 0) {
						/* 
						LogError(dataSetHandle->connection, 
					    	FILE_CREATION_ERR, 
					    	ReturnDataSetName(dataSetHandle), 
					    	ccode);
						*/
						goto Return;
					}
				}
			}
			else {
				/*
				LogError(dataSetHandle->connection,
			    	INVALID_DATA_SET_TYPE,
			    	ReturnDataSetName(dataSetHandle),
			    	dataSetHandle->dataSetType);
				*/
				dataSetHandle->skippingDataStream = TRUE;
			}
		}
		dataSetHandle->inDataStream = TRUE;
	}

	// If there is data in the data stream then get the data from the
	// buffer and write it out to the file - bytesLeftInDataStream is
	// set when GetHeader is called 
	if (dataSetHandle->bytesLeftInDataStream) {
		// if the data stream is being skipped then reset the buffer 
		_bytesToWrite = _min(dataSetHandle->bytesLeftInDataStream, 
						*bytesToWrite);
		if (dataSetHandle->skippingDataStream == TRUE) {
			*buffer += _bytesToWrite;
			*bytesToWrite -= _bytesToWrite;
			dataSetHandle->bytesLeftInDataStream -= _bytesToWrite;
		}
		else if ( S_ISLNK(dataSetHandle->unixChars.mode) ) {
			/* collect the symlink data */
			memcpy(symlinkPath + 
				dataSetHandle->smFileHandle.position,
				*buffer, _bytesToWrite);
			*buffer += _bytesToWrite;
			*bytesToWrite -= _bytesToWrite;
			dataSetHandle->bytesLeftInDataStream -= _bytesToWrite;
			dataSetHandle->smFileHandle.position += _bytesToWrite;
		}
		// Write the data in the buffer to the file
		else {
			if ((ccode = WriteFile(&dataSetHandle->smFileHandle, 
				*buffer, _bytesToWrite, &bytesWritten)) != 0) {
				/*
				LogError(dataSetHandle->connection, FILE_WRITE_ERR, ReturnDataSetName(dataSetHandle), ccode);
				*/
				goto Return;
			}
			else {
				*buffer += bytesWritten;
				*bytesToWrite -= bytesWritten;
				dataSetHandle->bytesLeftInDataStream -= bytesWritten;
			}
		}
	}

	// If there are no more bytes in the data stream then get the
	// data stream trailer 
	if (!dataSetHandle->bytesLeftInDataStream) {
		if (dataSetHandle->skippingDataStream == FALSE) {
			if ( S_ISLNK(dataSetHandle->unixChars.mode) ) {
				/* Now Create the symbolic link */
				if ( dataSetHandle->dataSetExists ) {
					if(unlink(dataSetHandle->dataSetName->string)== -1){
						ccode = HandleUnixError(errno);
						goto Return;
					}
				}
				if (symlink(symlinkPath, 
				    dataSetHandle->dataSetName->string) != 0 ){
					ccode = HandleUnixError(errno);
					goto Return ;
				}

				if (lchown(dataSetHandle->dataSetName->string,
					dataSetHandle->unixChars.userID,
					dataSetHandle->unixChars.groupID) 
									== -1) {
					if ( errno == EINVAL ) { /* uid or gid out of range */
						if (lchown(dataSetHandle->dataSetName->string,
							dataSetHandle->connection->loginUserID,
							dataSetHandle->connection->loginGroupID)== -1){
							ccode = HandleUnixError(errno);
							goto Return ;
						}
					}
					else {
						ccode = HandleUnixError(errno);
						goto Return ;
					}
				}
			}
			else {
				/* close the file */
				close(dataSetHandle->smFileHandle.FileHandle);
				if ((ccode = 
					SetAttributes(dataSetHandle)) != 0 ) {
					goto Return;
				}
			}
			dataSetHandle->restoreFlag = RESTORE_DATA_SET_DONE ;
		}
		if (dataSetHandle->recordType != NWSM_DATA_STREAM_TRAILER)
			dataSetHandle->recordType = 0;

		if ((tcode = GetEntireTrailer(dataSetHandle, bytesToWrite, 
								buffer)) != 0) {
			if (tcode != NWSMUT_BUFFER_UNDERFLOW)
				ccode = tcode;
			goto Return;
		}

		if ((ccode = GetTrailer(dataSetHandle, &dataStreamIsInvalid)) != 0)
			goto Return;

		if (!dataSetHandle->skippingDataStream) {
				/*
			if (dataStreamIsInvalid)
				LogError(dataSetHandle->connection, INVALID_DATA_STREAM,
				    ReturnDataSetName(dataSetHandle));
				*/
		}

		dataSetHandle->skippingDataStream = 
					dataSetHandle->inDataStream = FALSE;

		// If the data stream trailer fits exactly in the buffer, 
		// ReInitializeStateInfo will not be called in WriteDataSet, so
		// do it here so next call to WriteDataSet will succeed

		ReInitializeStateInfo(dataSetHandle);
	}

Return:
	return (ccode);
}

/********************************************************************/
STATIC CCODE GetHeader(RESTORE_DATA_SET_HANDLE *dataSetHandle, 
					UINT32 *dataStreamNumber)
{
	BUFFERPTR data;
	CCODE ccode = 0;
	UINT32 fid ;
	UINT64 dataSize;
	UINT32 dataStreamType = 0 ;

	dataSetHandle->bytesLeftInDataStream = 0;
#ifdef DEBUG
	logerror(PRIORITYWARN, "in GetHeader 0\n");
#endif

	if ((ccode = GetFieldToProcessSection(dataSetHandle, &fid, &dataSize, 
					&data, NULL)) != 0) {
		/*
		LogError(dataSetHandle->connection, GET_FIELD_ERR,
		    ReturnDataSetName(dataSetHandle), ccode);
		*/
		goto Return;
	}

	if (fid != NWSM_DATA_STREAM_HEADER) {
		ccode = NWSMTS_EXPECTING_HEADER;
		goto Return;
	}

	fid = 0;
#ifdef DEBUG
	logerror(PRIORITYWARN, "in GetHeader\n");
#endif
	while (fid != NWSM_DATA_STREAM_HEADER) {
		if ((ccode = GetFieldToProcessSection(dataSetHandle, &fid, 
						&dataSize, &data, NULL)) != 0) {
			/*
			LogError(dataSetHandle->connection, GET_FIELD_ERR,
			    ReturnDataSetName(dataSetHandle), ccode);
			*/
			goto Return;
		}

		switch (fid)
		{
		case NWSM_DATA_STREAM_SIZE:
			if ((ccode = SMDFSetUINT32Data(&dataSize, data, 
				&dataSetHandle->bytesLeftInDataStream)) != 0)
				goto Return;
			dataSetHandle->bytesLeftInDataStream =
			     SwapUINT32(dataSetHandle->bytesLeftInDataStream) ;
#ifdef DEBUG
			logerror(PRIORITYWARN,
					"datastreamSize %d, name %s\n",
					dataSetHandle->bytesLeftInDataStream,
					dataSetHandle->dataSetName->string);
#endif
			break;

		case NWSM_DATA_STREAM_NUMBER:
			if ((ccode = SMDFSetUINT32Data(&dataSize, data,
					dataStreamNumber)) != 0) {
				ccode = NWSMTS_OVERFLOW ;
				goto Return;
			}
			*dataStreamNumber = SwapUINT32(*dataStreamNumber) ;
#ifdef DEBUG
			logerror(PRIORITYWARN,
					"datastreamNumber %d, name %s\n",
					*dataStreamNumber,
					dataSetHandle->dataSetName->string);
#endif
			break;
		
		case NWSM_DATA_STREAM_TYPE :
			if ((ccode = SMDFSetUINT32Data(&dataSize, data,
					&dataStreamType)) != 0) {
				ccode = NWSMTS_OVERFLOW ;
				goto Return;
			}

			dataStreamType = SwapUINT32(dataStreamType) ;

			switch (dataStreamType) {
				case NWSM_CLEAR_TEXT_DATA_STREAM :
					break ;

				case NWSM_COMPRESSED_DATA_STREAM :
					ccode = NWSMTS_COMPRESSION_CONFLICT ;
					goto Return;

				default :
					ccode = NWSMTS_OPEN_DATA_STREAM_ERR ;
					goto Return;
			}
			break;
		}
	}

Return:
	return (ccode);
}

/********************************************************************/
STATIC CCODE GetTrailer(RESTORE_DATA_SET_HANDLE *dataSetHandle, 
					NWBOOLEAN *dataStreamIsInvalid)
{
	BUFFERPTR data;
	CCODE ccode = 0;
	UINT32 fid;
	UINT64 dataSize;

	*dataStreamIsInvalid = FALSE;

	if ((ccode = GetFieldToProcessSection(dataSetHandle, &fid, &dataSize, 
						&data, NULL)) != 0) {
		/*
		LogError(dataSetHandle->connection, GET_FIELD_ERR,
		    ReturnDataSetName(dataSetHandle), ccode);
		*/
		goto Return;
	}

	if (fid != NWSM_DATA_STREAM_TRAILER)
	{
		ccode = NWSMTS_EXPECTING_TRAILER;
		goto Return;
	}

	fid = 0;
	while (fid != NWSM_DATA_STREAM_TRAILER) {
		if ((ccode = GetFieldToProcessSection(dataSetHandle, &fid, 
						&dataSize, &data, NULL)) != 0) {
			/*
			LogError(dataSetHandle->connection, GET_FIELD_ERR,
			    ReturnDataSetName(dataSetHandle), ccode);
			*/
			goto Return;
		}

		if (fid is NWSM_DATA_STREAM_IS_INVALID) {
			*dataStreamIsInvalid = TRUE;
			break;
		}
	}

Return:
	return (ccode);
}

/********************************************************************/
CCODE RestoreCharacteristics(RESTORE_DATA_SET_HANDLE *dataSetHandle)
{
	CCODE ccode = 0;
	int userID ;
	BUFFERPTR dataBuffer;
	UINT32 fid;
	UINT32 dateAndTime;
	UINT64 dataSize;
	int modifyTimeSet = 0 ;
	INT32 tzOffset ;
	ECMATime ecmaTime ;

	if ((ccode = GetFieldToProcessSection(dataSetHandle, &fid, &dataSize, 
						&dataBuffer, NULL)) != 0) {
		/*
		LogError(dataSetHandle->connection, GET_FIELD_ERR,
		    ReturnDataSetName(dataSetHandle), ccode);
		*/
		goto Return;
	}

	if (fid != NWSM_CHARACTERISTICS) {
		ccode = NWSMUT_INVALID_FIELD_ID;
		goto Return;
	}

	fid = 0;

	while (fid != NWSM_CHARACTERISTICS) {
		if (ccode = GetFieldToProcessSection(dataSetHandle, &fid, 
						&dataSize, &dataBuffer, NULL)) {
			/*
			LogError(dataSetHandle->connection, GET_FIELD_ERR,
			    ReturnDataSetName(dataSetHandle), ccode);
			*/
			goto Return;
		}

		switch (fid)
		{
		case NWSM_MODIFY_DATE_AND_TIME:
			dateAndTime = SwapUINT32p(dataBuffer);
			dataSetHandle->smFileHandle.LastModifyDateAndTime = 
				_ConvertDOSTimeToCalendar(dateAndTime);
			modifyTimeSet = 1 ;
			break ;

		case NWSM_MODIFY_DATE_TIME_ECMA:
			memcpy((char *)&ecmaTime,dataBuffer,sizeof(ECMATime));
			MapECMATime(ecmaTime);
			NWSMECMAToUnixTime( &ecmaTime, &dateAndTime, &tzOffset);
			dataSetHandle->smFileHandle.LastModifyDateAndTime =
				dateAndTime ;
			modifyTimeSet = 1 ;
			break ; 

		case NWSM_ACCESS_DATE :
			dateAndTime = SwapUINT16p(dataBuffer) << 16 ;
			dataSetHandle->smFileHandle.LastAccessDateAndTime = 
				_ConvertDOSTimeToCalendar(dateAndTime);
			break;

		case NWSM_ACCESS_DATE_TIME_ECMA:
			memcpy((char *)&ecmaTime,dataBuffer,sizeof(ECMATime));
			MapECMATime(ecmaTime);
			NWSMECMAToUnixTime( &ecmaTime, &dateAndTime, &tzOffset);
			dataSetHandle->smFileHandle.LastAccessDateAndTime =
				dateAndTime ;
			break ; 

		case NWSM_CREATION_DATE_AND_TIME:
			dateAndTime = SwapUINT32p(dataBuffer);
			dataSetHandle->smFileHandle.CreationDateAndTime = 
				_ConvertDOSTimeToCalendar(dateAndTime);
			break;

		case NWSM_CREATION_DATE_TIME_ECMA:
			memcpy((char *)&ecmaTime,dataBuffer,sizeof(ECMATime));
			MapECMATime(ecmaTime);
			NWSMECMAToUnixTime( &ecmaTime, &dateAndTime, &tzOffset);
			dataSetHandle->smFileHandle.CreationDateAndTime =
				dateAndTime ;
			break ; 

		case NWSM_OWNER_NAME :
			/* This is not handled yet. */
			userID = GetUIDByNWname(dataBuffer);
			if ( userID != -1 ) {
				dataSetHandle->unixChars.userID = userID ;
			}
			break ;

		case NWSM_OWNER_ID :
			/* This is not handled yet. */
			userID = GetUIDByNWuid(SwapUINT32p(dataBuffer));
			if ( userID != -1 ) {
				dataSetHandle->unixChars.userID = userID ;
			}
			break ;

		/*
		case NWSM_DIRECTORY :
			if ( SMDFBit1IsSet(*dataBuffer) ) {
				dataSetHandle->unixChars.mode = 
							DEFAULT_MODE_FOR_DIRECTORY ;
			}
			break ;
		*/
		}
	}

	if ( modifyTimeSet && dataSetHandle->dataSetExists ) {
		/* Now we have the modify date and time for the dataset
	   	being restored. If the dataSet is present and
	   	restore mode is UPDATE_DATA_SET, check the modify
	   	date and time of the existing dataset.
		*/
		if (TestForUpdate(dataSetHandle,
			&dataSetHandle->dataSetInfo) == FALSE) {
			dataSetHandle->restoreFlag = 
					RESTORE_DATA_SET_SKIPPED ;
		}
	}

Return:
	return (ccode);
}

static CCODE CreateHLPath(LINK_INFORMATION *linkInfo,
	RESTORE_DATA_SET_HANDLE *dataSetHandle)
{
	struct stat stb ;

	if (lstat(linkInfo->linkPath, &stb) == 0) {
		return(0);
	}
	if ( errno != ENOENT ) {
		return(HandleUnixError(errno));
	}
	if ( CHECK_ACCESS(linkInfo->userID, linkInfo->groupID,
			dataSetHandle->connection->loginUserID,
			dataSetHandle->connection->loginGroupID,
			linkInfo->mode) ) {
		linkInfo->userID = dataSetHandle->connection->loginUserID;
		linkInfo->groupID = dataSetHandle->connection->loginGroupID;
		linkInfo->mode = DEFAULT_MODE_FOR_DIRECTORY ;
	}

	if (mkdir(linkInfo->linkPath, linkInfo->mode)) {
		return(NWSMTS_CREATE_DIR_ENTRY_ERR );
	}
	if ( lchown(linkInfo->linkPath, linkInfo->userID,
				linkInfo->groupID) == -1) {
		if ( errno == EINVAL ) { /* uid or gid out of range */
			if (lchown(linkInfo->linkPath ,
				dataSetHandle->connection->loginUserID,
				dataSetHandle->connection->loginGroupID)== -1){
				return(HandleUnixError(errno));
			}
		}
		else {
			return(HandleUnixError(errno));
		}
	}
	return(0);
}

CCODE RestoreNFSCharacteristics(RESTORE_DATA_SET_HANDLE *dataSetHandle)
{
	BUFFERPTR dataBuffer;
	CCODE ccode = 0 ;
	UINT32 fid, pathLength = 0, totalPathLength = 0, userID;
	LINK_INFORMATION linkInfo ;
	UINT64 dataSize;
	struct stat stb ;
	char excludedLink = 0 ;
	UINT32 fileAccessMode ;

	memset((char *)&linkInfo,'\0',sizeof(linkInfo));

	if ((ccode = GetFieldToProcessSection(dataSetHandle, &fid, &dataSize, &dataBuffer, NULL)) isnt 0)
	{
		/*
		LogError(dataSetHandle->connection, GET_FIELD_ERR, ReturnDataSetName(dataSetHandle), ccode);
		*/
		goto Return;
	}

	if (fid isnt NWSM_NFS_CHARACTERISTICS)
	{
		/* LogError(dataSetHandle->connection, ???, 
				ReturnDataSetName(dataSetHandle)); */
		ccode = NWSMTS_INVALID_DATA;
		goto Return;
	}

	fid = 0;

	while (fid isnt NWSM_NFS_CHARACTERISTICS)
	{
		if ((ccode = GetFieldToProcessSection(dataSetHandle, &fid, &dataSize, &dataBuffer, NULL)) isnt 0)
		{
			/*
			LogError(dataSetHandle->connection, GET_FIELD_ERR, ReturnDataSetName(dataSetHandle), ccode);
			*/
			goto Return;
		}

		switch (fid)
		{
		case NWSM_NFS_FILE_ACCESS_MODE:
			fileAccessMode = SwapUINT32p(dataBuffer);
#ifdef DEBUG
			logerror(PRIORITYWARN, "restoreNFSChars mode %d, name %s\n",
					fileAccessMode, dataSetHandle->dataSetName->string);
#endif
			if ( fileAccessMode == 0 ) {
				if ( dataSetHandle->dataSetType == TYPE_FILE ) {
					dataSetHandle->unixChars.mode = 
									DEFAULT_MODE_FOR_FILE ;
				}
				else {
					dataSetHandle->unixChars.mode = 
						DEFAULT_MODE_FOR_DIRECTORY ;
				}
			}
			else {
				dataSetHandle->unixChars.mode = fileAccessMode ;
			}

#ifdef DEBUG
			logerror(PRIORITYWARN,
					"restoreNFSChars mode %d, name %s\n",
					dataSetHandle->unixChars.mode,
					dataSetHandle->dataSetName->string);
#endif
			break;

		case NWSM_NFS_GROUP_OWNER_ID:
			dataSetHandle->unixChars.groupID =
							SwapUINT32p(dataBuffer);
#ifdef DEBUG
			logerror(PRIORITYWARN,"restoreNFSChars gid %d\n",
					dataSetHandle->unixChars.groupID );
#endif
			break;

		case NWSM_NFS_NUMBER_OF_LINKS:
		case NWSM_NFS_NUMBER_OF_LINKS_OLD:
			dataSetHandle->unixChars.nlinks =
							SwapUINT32p(dataBuffer);
			break;

		case NWSM_NFS_RDEVICE:
		case NWSM_NFS_RDEVICE_OLD:
			dataSetHandle->unixChars.rdevice =
							SwapUINT32p(dataBuffer);
			break;

		case NWSM_NFS_USER_ID:
			dataSetHandle->unixChars.userID =
							SwapUINT32p(dataBuffer);
#ifdef DEBUG
			logerror(PRIORITYWARN,"restoreNFSChars uid %d\n",
					dataSetHandle->unixChars.userID );
#endif
			break;

		case NWSM_NFS_FSID:
			dataSetHandle->unixChars.devNo =
							SwapUINT32p(dataBuffer);
			break;

		case NWSM_NFS_FILEID:
			dataSetHandle->unixChars.inodeNo =
							SwapUINT32p(dataBuffer);
			break;

		case NWSM_NFS_HARD_LINK_PATHS:
			/* If the dataset is a file creat the file now.
			   do symlink, mknod or open depending on the file
			   type. This has to be done here because some links
			   may be created in this case NWSM_NFS_HARD_LINK_PATHS.
			   Once we come here, we know that we are dealing with
			   a backup from NetWare.
			*/
			if ( dataSetHandle->dataSetType == TYPE_FILE ) {
				if ((ccode = CreateDataSet(dataSetHandle)) != 0) {
					/*
					LogError(dataSetHandle->connection, FILE_CREATION_ERR, ReturnDataSetName(dataSetHandle), ccode);
					*/
					goto Return;
				}
			}
			fid = 0;

			while (fid isnt NWSM_NFS_HARD_LINK_PATHS)
			{
				if ((ccode = GetFieldToProcessSection(dataSetHandle, &fid, &dataSize, &dataBuffer, NULL)) isnt 0)
				{
					/*
					LogError(dataSetHandle->connection, GET_FIELD_ERR, ReturnDataSetName(dataSetHandle), ccode);
					*/
					goto Return;
				}

				if ( dataSetHandle->dataSetType != TYPE_FILE ) {
					/* May need to add a warning here */
					continue ;
				}
				if ( dataSetHandle->restoreFlag ==
						RESTORE_DATA_SET_SKIPPED ) {
					continue ;
				}
				if ( excludedLink ) {
					if ( fid == NWSM_NFS_HL_TERMINATOR ) {
						excludedLink = 0 ;
					}
					continue ;
				}

				switch (fid)
				{
				case NWSM_NFS_HL_PATHNAME:
					if ( totalPathLength ) {
						ccode = CreateHLPath(&linkInfo, dataSetHandle);
						if ( ccode ) {
							goto Return ;
						}
					}
					linkInfo.linkPath[totalPathLength++] =
							'/' ;
					linkInfo.linkPath[totalPathLength] = 0 ;
					linkInfo.userID =
 					 dataSetHandle->connection->loginUserID;
					linkInfo.groupID =
 					 dataSetHandle->connection->loginGroupID;
					linkInfo.mode =
						DEFAULT_MODE_FOR_DIRECTORY ;
					SMDFGetUINT64(&dataSize, &pathLength);
					memcpy(
					   &linkInfo.linkPath[totalPathLength],
					   dataBuffer, pathLength);
					totalPathLength += pathLength ; 
					linkInfo.linkPath[totalPathLength] = 0 ;

        				if (!dataSetHandle->connection->dontCheckSelectionList &&
                (ccode = ExcludedByRestoreSelectionList1(dataSetHandle,
							TYPE_DIRECTORY,
						linkInfo.linkPath)) != 0){
						excludedLink = 1 ;
						totalPathLength = 0;
						pathLength = 0;
					}
					break;

				case NWSM_NFS_HL_USER_ID:
					userID = SwapUINT32p(dataBuffer);
					if ( userID != -1 )
						linkInfo.userID = userID ;
					break ;

				case NWSM_NFS_HL_GROUP_OWNER_ID:
					linkInfo.groupID = 
						SwapUINT32p(dataBuffer);
					break;

				case NWSM_NFS_HL_FILE_ACCESS_MODE:
					linkInfo.mode = 
						SwapUINT32p(dataBuffer);
					break;

				case NWSM_NFS_HL_TERMINATOR:
					if (lstat(linkInfo.linkPath,&stb) == 0){
						if(TestForUpdate(dataSetHandle,
							 &stb) == FALSE ) {
							totalPathLength = 0;
							pathLength = 0;
							break;
						}
						if (dataSetHandle->dataSetType != TYPE_FILE){
							if ( rmdir(linkInfo.linkPath) == -1 ) {
								ccode = HandleUnixError(errno);
                        		goto Return;
							}
						}
						else if ( unlink(linkInfo.linkPath) == -1 ) {
							ccode = HandleUnixError(errno);
                        	goto Return;
						}
					}

					if (dataSetHandle->dataSetType != TYPE_FILE){
						/* May need to add a warning here */
						if (symlink(dataSetHandle->dataSetName->string,
								linkInfo.linkPath) != 0 ){
							ccode = HandleUnixError(errno);
							goto Return ;
						}
					}
					else if (link(dataSetHandle->dataSetName->string,
						linkInfo.linkPath) == -1 ) {
						ccode = HandleUnixError(errno);
                        			goto Return;
					}
					totalPathLength = 0;
					pathLength = 0;
					break;
				}
			}

			break;
		}
	}
	if ( dataSetHandle->restoreFlag ==  RESTORE_DATA_SET_NOT_YET_CREATED ){
		BACKED_UP_HARDLINKS *linkStruct ;
		int rcode = 0 ;

		/* if the dataset has nlinks == 1, just creat it. Otherwise
		   check if the dataset is already created as another instance
		   of the link. If so just link it. If not already created,
		   create it and put it in the linked list of hard link files.
		*/
		if ( dataSetHandle->unixChars.nlinks == 1 ) {
#ifdef DEBUG
	logerror(PRIORITYWARN,"Calling createdataset 1\n");
	FLUSHLOG ;
#endif
			if ((ccode = CreateDataSet(dataSetHandle)) != 0) {
			/*
				LogError(dataSetHandle->connection, FILE_CREATION_ERR, ReturnDataSetName(dataSetHandle), ccode);
			*/
				goto Return;
			}
		}
		else if ( (rcode = IsInHardLinksList(
				dataSetHandle->connection->disks,
				dataSetHandle->unixChars.devNo,
				dataSetHandle->unixChars.inodeNo,
				dataSetHandle->smFileHandle.LastModifyDateAndTime,
				dataSetHandle->dataSetName->string, 
				&linkStruct)) == 0 || rcode == 1) {
#ifdef DEBUG
	logerror(PRIORITYWARN,"IsInHardLinksList : rcode %d for %s\n",
					rcode, dataSetHandle->dataSetName->string);
	FLUSHLOG ;
#endif

			if ((ccode = CreateDataSet(dataSetHandle)) != 0) {
			/*
				LogError(dataSetHandle->connection, FILE_CREATION_ERR, ReturnDataSetName(dataSetHandle), ccode);
			*/
				goto Return;
			}

			if (rcode == 0 &&
			 AddToHardLinksList(&dataSetHandle->connection->disks,
				dataSetHandle->unixChars.devNo,
				dataSetHandle->unixChars.inodeNo,
				dataSetHandle->smFileHandle.LastModifyDateAndTime,
				dataSetHandle->dataSetName->string) == -1 ) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return;
			}
		}
		else {
			if (dataSetHandle->dataSetType != TYPE_FILE){
				/* May need to add a warning here */
				dataSetHandle->restoreFlag = 
						RESTORE_DATA_SET_SKIPPED ;
				goto Return;
			}

			if (dataSetHandle->dataSetExists){
				if ( dataSetHandle->restoreFlag == 
						RESTORE_DATA_SET_SKIPPED ){
					dataSetHandle->skippingDataStream = 
									TRUE;
					goto Return;
				}
				/* unlink the file first */
				if (unlink(dataSetHandle->dataSetName->string) == -1) {
					ccode = HandleUnixError(errno);
                    goto Return;
				}
			}
						
			if ( link(linkStruct->path,
				dataSetHandle->dataSetName->string) == -1 ) {
				ccode = HandleUnixError(errno);
                        	goto Return;
			}
//			SetAttributes(dataSetHandle) ;
			dataSetHandle->skippingDataStream = TRUE;

			dataSetHandle->restoreFlag = 
						RESTORE_DATA_SET_LINKED ;
		}
	}
Return:
	return (ccode);
}
/********************************************************************/

