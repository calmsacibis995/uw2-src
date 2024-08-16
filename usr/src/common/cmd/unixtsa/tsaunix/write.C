#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/write.C	1.5"

#include <tsad.h>
#include <ctype.h>
#include <error.h>
#include <smsutapi.h>
#include "tsaglobl.h"
#include <smsfids.h>
#include "filesys.h"



/********************************************************************/
void ReInitializeStateInfo(RESTORE_DATA_SET_HANDLE *dataSetHandle)
{
        dataSetHandle->state = STATE_GET_ENTIRE_SUBRECORD;
        dataSetHandle->stateDataSize = dataSetHandle->stateDataOffset = 0;
        SMDFZeroUINT64(&dataSetHandle->offsetToEnd);
        dataSetHandle->bufferPtr = dataSetHandle->begin = NULL;
        dataSetHandle->recordType = 0;

        if (dataSetHandle->stateDataHandle.valid == VALID_TEMPFILE) {
                dataSetHandle->stateDataHandle.size = 0;
                dataSetHandle->stateDataHandle.position = 0;
        }
}

/********************************************************************
Function: WriteDataSet

Purpose:  Writes buffer data to the specified dataSet

Parameters: connectionHandle     input
			dsHandle             input
			bytesToWrite         input
			buffer               input

Returns:    ccode - 0 if successful
			non-zero if failed
*********************************************************************/
CCODE  WriteDataSet( UINT32    connectionHandle,
UINT32    dsHandle,
UINT32    bytesToWrite,
BUFFERPTR buffer)
{
	CCODE ccode = 0;
	RESTORE_DATA_SET_HANDLE *dataSetHandle = 
				(RESTORE_DATA_SET_HANDLE *)dsHandle;

	if (((CONNECTION *)connectionHandle)->valid != VALID_CONNECTION)
	{
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		goto Return;
	}

	if (dsHandle == 0 || NotValidOrEvenClose(dataSetHandle->valid))
	{
		ccode = NWSMTS_INVALID_DATA_SET_HANDLE;
		goto Return;
	}

	//
	// This loop will first get an entire section (i.e. a Full Paths
	// section  or a data set characteristics section and then restore
	// that section.   GetEntireSection() sets the apropriate restore state.
	// The set to STATE_GET_ENTIRE_SUBRECORD at the end of the loop.
	// The loop breaks when GetEntireSection() returns an error indicating
	// that a complete section cannot be found (the buffer is empty),
	// or when RestoreDataStreams() empties the buffer, or when a general
	// error occurs.  Exception: the entire section is not gotten for the
	// data streams section
	//

#ifdef DEBUG
	logerror(PRIORITYWARN,"WriteDataSet: state : %d, size : %d\n",
					dataSetHandle->state, bytesToWrite);
	FLUSHLOG ;
	dump_buf(buffer,40,dumpBuffer);
	logerror(PRIORITYWARN,"Write: %s\n", dumpBuffer);
	FLUSHLOG ;
#endif

	loop
	{
		switch (dataSetHandle->state)
		{
		case STATE_GET_ENTIRE_SUBRECORD:
			if (!bytesToWrite)
				goto Return;
			if (((ccode = GetEntireSection(dataSetHandle,
				 &bytesToWrite, &buffer)) != 0) ||
			    (!bytesToWrite) ||
			    (dataSetHandle->state == STATE_DONE))
				goto Return;
			continue;

		case STATE_FULL_PATHS:

#ifdef DEBUG
			logerror(PRIORITYWARN,"RestoreFullPaths\n");
	FLUSHLOG ;
#endif

			if ((ccode = RestoreFullPaths(dataSetHandle)) != 0)
				goto Return;
			break;

		case STATE_CHARACTERISTICS:
#ifdef DEBUG
			logerror(PRIORITYWARN,"RestoreCharacteristics\n");
	FLUSHLOG ;
#endif
			if((ccode = RestoreCharacteristics(dataSetHandle)) != 0)
				goto Return;
			break;

		case STATE_NFS_CHARACTERISTICS:
#ifdef DEBUG
			logerror(PRIORITYWARN,"RestoreNFSCharacteristics\n");
	FLUSHLOG ;
#endif
			if ((ccode = RestoreNFSCharacteristics(dataSetHandle)) 
					!= 0)
				goto Return;
			break;

		case STATE_DATA_STREAMS:
#ifdef DEBUG
			logerror(PRIORITYWARN,"RestoreDataStreams\n");
	FLUSHLOG ;
#endif
			if ((ccode = RestoreDataStreams(dataSetHandle,
					&bytesToWrite, &buffer)) != 0 ||
						!bytesToWrite)
				goto Return;
			break;

		case STATE_DONE:
			/* This state is only reached on a second call to the 
			   after a trailer has been processed. If there is data
			   after the trailer then bytesToWrite will contain a 
			   value so error */

			if (bytesToWrite) {
				ccode = NWSMTS_INVALID_DATA;
				/*LogError(dataSetHandle->connection, 
					INVALID_DATA, 
					ReturnDataSetName(dataSetHandle));
				*/
			}
			goto Return;
		}

		ReInitializeStateInfo(dataSetHandle);
	}

Return:
	return (ccode);
}

CCODE GetEntireSection(RESTORE_DATA_SET_HANDLE *dataSetHandle, UINT32 *bytesToWrite, BUFFERPTR *buffer)
{
	BUFFERPTR dataBufferPtr;
	CCODE ccode = 0;
	UINT32 fid, offsetToEnd, size;
	UINT64 dataSize;

	/* Loop through the buffer to obtain a section trailer or section 
	header. If a section header is found continue looping until a section 
	field is found. A section field would be full paths or data set 
	charateristics, for example
	*/
	dataSetHandle->buffer = *buffer;
	while (!dataSetHandle->recordType)
	{
		dataSetHandle->bufferState = BUFFER_STATE;

		/* This test to see if dataSetHandle->offsetToEnd is > 0.  
		OffsetToEnd is always false on the first call to the routine.  
		offsetToEnd is used for subsequent calls to the routine when we
		have found an offsetToEnd field after a section field and the 
		data we are looking at is unusable or we do not need it.We have
		not found a usable section so we are skipping over unusable 
		data by adjusting offsetToEnd, bytesToWrite and buffer.
		*/
		if (SMDF_GT(&dataSetHandle->offsetToEnd, 0))
		{
OffsetToEnd:
			/* If offsetToEnd is less than what is in the buffer 
			   then skip to the next section
			*/
			if (SMDF_LE(&dataSetHandle->offsetToEnd, *bytesToWrite))
			{
				SMDFGetUINT64(&dataSetHandle->offsetToEnd, &offsetToEnd);
				*buffer += offsetToEnd;
				*bytesToWrite -= offsetToEnd;
				SMDFZeroUINT64(&dataSetHandle->offsetToEnd);

				/* The buffer is pointing at the last field of 
				   the data.  Get the field so we are pointing 
				   past the last field.
				*/
				if ((ccode = GetFieldToGetSection(dataSetHandle,
						buffer, bytesToWrite, &fid, 
						&dataSize, &dataBufferPtr)))
					goto Return;
			}
			/* If offsetToEnd is past the end of the buffer then 
			decrement offsetToEnd by how much is left in the buffer.
			The rest will be skipped on subsequent calls to this 
			routine.
			*/
			else
			{
				SMDFDecrementUINT64(&dataSetHandle->offsetToEnd, *bytesToWrite);
				*bytesToWrite = 0;
				goto Return;
			}
		}

		/* Set a pointer to the begining of the buffer then find the 
		first fid. If a fid is a section type (full paths, data 
		characteristics) then set recordType.
		*/
		dataSetHandle->buffer = *buffer;
		if ((ccode = GetFieldToGetSection(dataSetHandle, buffer, 
				bytesToWrite, &fid, &dataSize,
				&dataBufferPtr)) isnt 0)
			goto Return;

#ifdef DEBUG
		logerror(PRIORITYWARN,"GetEntireSection: fid %x dataSize %d %d\n",
				 fid, dataSize.v[1], dataSize.v[0]);
#endif

		switch (fid)
		{
		case NWSM_OFFSET_TO_END:
			SMDFZeroUINT64(&dataSetHandle->offsetToEnd);
			SMDFGetUINT64(&dataSize, &size);
			SMDFSetUINT64(&dataSetHandle->offsetToEnd, 
						dataBufferPtr, (UINT16)size);
			switch (size) {
			case sizeof(UINT64) :
				SwapUINT64buf(&dataSetHandle->offsetToEnd);
				break ;
			case sizeof(UINT32) :
				SwapUINT32buf(
					(char *)&dataSetHandle->offsetToEnd);
				break ;
			case sizeof(UINT16) :
				SwapUINT16buf(
					(char *)&dataSetHandle->offsetToEnd);
				break ;
			}

			if (SMDF_GT(&dataSetHandle->offsetToEnd, 0))
				goto OffsetToEnd;
			break;

		case NWSM_VOLUME_HEADER:
			dataSetHandle->dataSetType = TYPE_FILESYSTEM;
			dataSetHandle->recordFID = fid;
			break;

		case NWSM_DIRECTORY_HEADER:
			dataSetHandle->dataSetType = TYPE_DIRECTORY;
			dataSetHandle->recordFID = fid;
			break;

		case NWSM_FILE_HEADER:
			dataSetHandle->dataSetType = TYPE_FILE;
			dataSetHandle->recordFID = fid;
			break;

		case NWSM_BACKUP_OPTIONS :
			if ( dataSetHandle->dataSetType ) {
				dataSetHandle->backupOptions = *(UINT8 *)dataBufferPtr;
			}
			break;

		case NWSM_DATA_STREAM_NUMBER :
			if (dataSetHandle->dataSetType == TYPE_FILE) {
				SMDFSetUINT32Data(&dataSize, dataBufferPtr,
					&dataSetHandle->currentDataStreamNumber);
				dataSetHandle->currentDataStreamNumber =
					SwapUINT32(dataSetHandle->currentDataStreamNumber);
				if ( dataSetHandle->currentDataStreamNumber !=
					NWSM_PRIMARY_DATA_STREAM_NUM )
				{
					dataSetHandle->currentDataStreamNumber = 0xFFFFFFFF ;
				}
			}
			break;

		case NWSM_DATA_STREAM_TYPE :
			if (dataSetHandle->dataSetType == TYPE_FILE &&
				dataSetHandle->currentDataStreamNumber != 0xFFFFFFFF) {
				SMDFSetUINT32Data(&dataSize, dataBufferPtr,
					&dataSetHandle->dataStreamType);
				dataSetHandle->dataStreamType =
					SwapUINT32(dataSetHandle->dataStreamType);
				
			}
			break ;

		case NWSM_FULL_PATHS:
			dataSetHandle->recordType = STATE_FULL_PATHS;
			dataSetHandle->recordFID = fid;
			dataSetHandle->begin = dataSetHandle->buffer;
			break;

		case NWSM_CHARACTERISTICS:
			dataSetHandle->recordType = STATE_CHARACTERISTICS;
			dataSetHandle->recordFID = fid;
			dataSetHandle->begin = dataSetHandle->buffer;
			break;

		case NWSM_NFS_CHARACTERISTICS:
			dataSetHandle->recordType = STATE_NFS_CHARACTERISTICS;
			dataSetHandle->recordFID = fid;
			dataSetHandle->begin = dataSetHandle->buffer;
			break;

		case NWSM_DATA_STREAM_HEADER:
			dataSetHandle->recordType = STATE_DATA_STREAMS;
			dataSetHandle->recordFID = fid;
			dataSetHandle->begin = dataSetHandle->buffer;
			break;

		case NWSM_FILE_TRAILER:
			if ( dataSetHandle->restoreFlag == 
					RESTORE_DATA_SET_NOT_YET_CREATED ){
				/* we reach this logic only if there was no 
                   data streams section.
                */
				
#ifdef DEBUG
				logerror(PRIORITYWARN,"calling CreateDataSet 2\n");
#endif
				if((ccode = CreateDataSet(dataSetHandle)) != 0){
					goto Return ;
				}
				if( dataSetHandle->restoreFlag ==
					RESTORE_DATA_SET_JUST_CREATED ){
					/* This means it was a regular file. because,
                       device files are fully handled in createDataSet
                       and symbolic links are ignored if there is no
                       data streams.
                    */
					UnlockAndClose(
						dataSetHandle->smFileHandle.FileHandle) ;
					if ((ccode = SetAttributes(dataSetHandle)) != 0) {
						goto Return;
					}
					dataSetHandle->restoreFlag = RESTORE_DATA_SET_DONE ;
				}
			}

			dataSetHandle->recordType = STATE_DONE;
			dataSetHandle->recordFID = fid;
			dataSetHandle->begin = dataSetHandle->buffer;
			break;

		case NWSM_DIRECTORY_TRAILER:
		case NWSM_VOLUME_TRAILER:
			if ( dataSetHandle->restoreFlag == 
					RESTORE_DATA_SET_NOT_YET_CREATED ){
				if((ccode = CreateDataSet(dataSetHandle)) != 0){
					/*
					LogError(dataSetHandle->connection,
						FILE_CREATION_ERR,
					ReturnDataSetName(dataSetHandle),
						ccode);
					*/
					goto Return;
				}
			}
			dataSetHandle->recordType = STATE_DONE;
			dataSetHandle->recordFID = fid;
			dataSetHandle->begin = dataSetHandle->buffer;
			break;

		default:
			if ( fid != NWSM_HEADER_DEBUG_STRING ) {
				dataSetHandle->recordFID = fid;
			}
		}

		/* If we have found a valid section and if there was underflow 
		then copy the underflow data to the stateDataBuffer.An underflow
		would occur in the routine GetFieldToGetSection(...).  An 
		underflow occurs when there is not enough data in a buffer to 
		get fid and data size information from a field.  In 
		GetFieldToGetSection(...) the underflow data is copied to the 
		dataUnderflow buffer. We need to copy this data into stateData.
		*/
		if (dataSetHandle->recordType and dataSetHandle->usedDataUnderflowSize)
		{
			if (!dataSetHandle->stateData)
			{
				if ((dataSetHandle->stateData = (char *) malloc(STATE_DATA_SIZE)) is NULL)
				{
					ccode = NWSMTS_OUT_OF_MEMORY;
					goto Return;
				}
			}

			memcpy(dataSetHandle->stateData, dataSetHandle->dataUnderflow, dataSetHandle->usedDataUnderflowSize);
			dataSetHandle->stateDataSize = dataSetHandle->usedDataUnderflowSize;
			dataSetHandle->bufferState = STATE_DATA_BUFFER_STATE;
		}
	}

	/* We will not get to this point until there is a record type.
	If there is an offsetToEnd value then it is the offset to the end
	of the section.  We need to find the end of the section.  If it is
	not in the buffer then we need to copy the entire buffer to the
	stateDataFile.
	*/
	if (SMDF_GT(&dataSetHandle->offsetToEnd, 0))
	{
OffsetToEnd2:
		/* If offsetToEnd is in the buffer then find the end. Determine
		   if the fid at the end matches the recordType fid.  
		*/
		if (SMDF_LE(&dataSetHandle->offsetToEnd, *bytesToWrite))
		{
			SMDFGetUINT64(&dataSetHandle->offsetToEnd, &offsetToEnd);
			*buffer += offsetToEnd;
			*bytesToWrite -= offsetToEnd;
			SMDFZeroUINT64(&dataSetHandle->offsetToEnd);
			if ((ccode = GetFieldToGetSection(dataSetHandle, buffer, bytesToWrite, &fid, &dataSize,
			    &dataBufferPtr)) isnt 0)
				goto Return;

			if (fid isnt dataSetHandle->recordFID)
			{
				ccode = NWSMTS_BUFFER_UNDERFLOW;
				goto Return;
			}

			if (dataSetHandle->bufferState is BUFFER_STATE)
				dataSetHandle->bufferBytesToWrite = *buffer - dataSetHandle->begin;

			else
				dataSetHandle->bufferBytesToWrite = *buffer - dataSetHandle->buffer;

			dataSetHandle->state = dataSetHandle->recordType;
		}

		else
		{
			SMDFDecrementUINT64(&dataSetHandle->offsetToEnd, *bytesToWrite);
			*buffer += *bytesToWrite;
			*bytesToWrite = 0;
			ccode = CopyToStateData(dataSetHandle, *buffer);
			goto Return;
		}
	}

	else
	{
		loop
		{
			if ((ccode = GetFieldToGetSection(dataSetHandle, buffer,
					bytesToWrite, &fid, &dataSize,
					&dataBufferPtr)) isnt 0)
				goto Return;
			if (fid is NWSM_OFFSET_TO_END)
			{
				SMDFZeroUINT64(&dataSetHandle->offsetToEnd);
				SMDFGetUINT64(&dataSize, &size);
				SMDFSetUINT64(&dataSetHandle->offsetToEnd, dataBufferPtr, (UINT16)size);
				switch (size) {
				case sizeof(UINT64) :
					SwapUINT64buf(
					   &dataSetHandle->offsetToEnd);
					break ;
				case sizeof(UINT32) :
					SwapUINT32buf(
					   (char *)&dataSetHandle->offsetToEnd);
					break ;
				case sizeof(UINT16) :
					SwapUINT16buf(
					   (char *)&dataSetHandle->offsetToEnd);
					break ;
				}
				if (SMDF_GT(&dataSetHandle->offsetToEnd, 0))
					goto OffsetToEnd2;
			}
			else if (fid is dataSetHandle->recordFID)
			{
				if (dataSetHandle->bufferState is BUFFER_STATE)
					dataSetHandle->bufferBytesToWrite = *buffer - dataSetHandle->begin;
				else
					dataSetHandle->bufferBytesToWrite = *buffer - dataSetHandle->buffer;
				dataSetHandle->state = dataSetHandle->recordType;
				goto Return;
			}
		}
	}

Return:
	if (ccode is NWSMUT_BUFFER_UNDERFLOW)
		ccode = 0;

	if (dataSetHandle->state isnt STATE_GET_ENTIRE_SUBRECORD)
		dataSetHandle->stateDataHandle.position = 0;
	return (ccode);
}

/*********************************************************************/

CCODE GetEntireTrailer(RESTORE_DATA_SET_HANDLE *dataSetHandle, UINT32 *bytesToWrite, BUFFERPTR *buffer)
{
	CCODE ccode = 0;
	BUFFERPTR  dataBufferPtr;
	UINT32 fid;
	UINT64 dataSize;

	dataSetHandle->buffer = *buffer;
	if (!dataSetHandle->recordType)
	{
		dataSetHandle->bufferState = BUFFER_STATE;
		dataSetHandle->stateDataSize = 
			dataSetHandle->stateDataOffset = 0;
		SMDFZeroUINT64(&dataSetHandle->offsetToEnd);
		dataSetHandle->begin = *buffer;
		dataSetHandle->bufferPtr = NULL;

		if (dataSetHandle->stateDataHandle.valid is VALID_TEMPFILE)
		{
			dataSetHandle->stateDataHandle.size = 0;
			dataSetHandle->stateDataHandle.position   = 0;
		}

		if ((ccode = GetFieldToGetSection(dataSetHandle, buffer, 
					bytesToWrite, &fid, &dataSize,
					&dataBufferPtr)) isnt 0)
			goto Return;

		if (fid is NWSM_DATA_STREAM_TRAILER)
		{
			dataSetHandle->recordType = NWSM_DATA_STREAM_TRAILER;
			dataSetHandle->recordFID = fid;
			if (dataSetHandle->usedDataUnderflowSize)
			{
				if (!dataSetHandle->stateData)
				{
					if ((dataSetHandle->stateData = (char *) malloc(STATE_DATA_SIZE)) is NULL)
					{
						ccode = NWSMTS_OUT_OF_MEMORY;
						goto Return;
					}
				}

				memcpy(dataSetHandle->stateData, dataSetHandle->dataUnderflow, dataSetHandle->usedDataUnderflowSize);
				dataSetHandle->stateDataSize = dataSetHandle->usedDataUnderflowSize;
				dataSetHandle->bufferState = STATE_DATA_BUFFER_STATE;
			}
		}

		else
		{
			dataSetHandle->recordType = 0;
			ccode = NWSMTS_EXPECTING_TRAILER;
			/*
			LogError(dataSetHandle->connection, DATA_STREAM_TRAILER_ERR, dataSetHandle->dataSetName->string);
			*/
			goto Return;
		}
	}

	while ((ccode = GetFieldToGetSection(dataSetHandle, buffer,
				bytesToWrite, &fid, &dataSize,
				&dataBufferPtr)) is 0)
	{
		if (fid is NWSM_DATA_STREAM_TRAILER)
		{
			if (dataSetHandle->bufferState is BUFFER_STATE)
				dataSetHandle->bufferBytesToWrite = *buffer - dataSetHandle->begin;

			else
				dataSetHandle->bufferBytesToWrite = *buffer - dataSetHandle->buffer;
			break;
		}
	}

Return:
	return (ccode);
}

/********************************************************************/
