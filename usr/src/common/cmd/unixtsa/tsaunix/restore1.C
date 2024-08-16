#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/restore1.C	1.1"

#include "tsaglobl.h"

#include <smsfids.h>

/****************************************************************************/
/* This call is used to get an entire field.  If this call performed
 *   and there were   no special cases then the code would look like this.
 *   /# Get a Field #/
 *      SMDFGetNextField(*buffer, *bytesToWrite, &field);
 *
 *   /# Increment the buffer pointers #/
 *      *buffer += field.bytesTransfered;
 *      *bytesToWrite -= field.bytesTransfered;
 *
 *    /# Set up data parameters #/
 *      *fid = field.fid;
 *      *dataSize = field.dataSize;
 *      *data = field.data;
 *
 *   However, there are two special cases that must be dealt with.
 *   data overflow and data underflow.  Data overflow occurs when
 *   you are getting a field from the buffer and all the data for the
 *   field is not in the buffer and the routine must exit and the engine
 *   needs to get another buffer before this can get the
 *   rest of the data.  A data underflow occurs when there is data
 *   in the buffer but there is not enough data there to determine what
 *   fid it is and what the size of the data is.
 */
/****************************************************************************/
CCODE GetFieldToGetSection(RESTORE_DATA_SET_HANDLE *dataSetHandle, BUFFERPTR *buffer,
UINT32 *bytesToWrite, UINT32 *fid, UINT64 *dataSize, BUFFERPTR *data)
{
	CCODE ccode = 0;
	UINT32 dataOverflow, size;
	SMDF_FIELD_DATA field;

	/* Determine if there was dataOverflow - dataOverflow occurs when there
 	 * is more data for a field than is   in the buffer - dataOverflowset 
	 * on a previous call - if there is dataOverflow then reset the 
	 * pointers and bytesToWrite.
	 */
	if (SMDF_GT(&dataSetHandle->dataOverflow, 0))
	{
		/* if there is more dataOverflow than there is data in the 
		   buffer then set then increment the buffer by bytesToWrite 
		   and set bytesToWrite to 0 - by setting ccode to 
		   NWSMUT_BUFFER_UNDERFLOW we might save the data at Return.
		   If recordType is set then this data is important and it will
		   be copied into stateData otherwise it is discarded.
		*/
		if (SMDF_GT(&dataSetHandle->dataOverflow, *bytesToWrite))
		{
			SMDFDecrementUINT64(&dataSetHandle->dataOverflow, *bytesToWrite);
			*buffer += *bytesToWrite;
			*bytesToWrite = 0;
			ccode = NWSMUT_BUFFER_UNDERFLOW;

			if (dataSetHandle->recordType)
				goto Return;

			else
				goto Return2;
		}

		else
		{
			SMDFGetUINT64(&dataSetHandle->dataOverflow, &dataOverflow);
			*buffer += dataOverflow;
			*bytesToWrite -= dataOverflow;
			SMDFZeroUINT64(&dataSetHandle->dataOverflow);
		}
	}

	/* If there was dataOverflow (above) then there won't be 
	   dataUnderflowSize. dataUnderflowSize occurs when there is not enough
	   data in the buffer to determine what kind of field we are looking 
	   at. dataUnderflowSize is set by a previous call to this routine. The
	   data that was left in the buffer on the previous call is put into an
	   underFlow buffer in the dataSetHandle.  Now we will take the new 
	   buffer, copy data from it into the underFlow buffer and try to 
	   determine what the field is.
	*/
	if (dataSetHandle->dataUnderflowSize)
	{
		/* How much room is left in the dataUnderflow buffer? */
		size = DATA_UNDERFLOW_SIZE - dataSetHandle->dataUnderflowSize;
		memcpy(dataSetHandle->dataUnderflow + 
			dataSetHandle->dataUnderflowSize, *buffer, size);

		/* If we still cannot get a field then we should exit the 
		   routine. */
		if ((ccode = SMDFGetNextField(dataSetHandle->dataUnderflow, 
					DATA_UNDERFLOW_SIZE, &field)) isnt 0)
			goto Return2;

		/* Determine if there was dataOverflow. This is very likely, the
		   dataUnderflow buffer is only 26 bytes. If there is 
		   dataOverflow, then there can be more data for the field than
		   there is in the buffer. If this is the case then adjust the 
		   buffer pointers to the end of the buffer and set ccode so 
		   that at Return, the data in the buffer will be copied into 
		   the stateData buffer. If all the rest of the data is in the 
		   buffer then set the buffer pointers and the code will fall
		   through to return.
		*/
		dataSetHandle->dataOverflow = field.dataOverflow;
		if (SMDF_GT(&dataSetHandle->dataOverflow, 0))
		{
			if (SMDF_GT(&dataSetHandle->dataOverflow, *bytesToWrite - size))
			{
				SMDFDecrementUINT64(&dataSetHandle->dataOverflow, *bytesToWrite - size);
				*buffer += *bytesToWrite;
				*bytesToWrite = 0;
				ccode = NWSMUT_BUFFER_UNDERFLOW;
				goto Return;
			}

			else
			{
				SMDFGetUINT64(&dataSetHandle->dataOverflow, &dataOverflow);
				*buffer += (size + dataOverflow);
				*bytesToWrite -= (size + dataOverflow);
				SMDFZeroUINT64(&dataSetHandle->dataOverflow);
			}
		}

		/* There was no overflow so just increment the buffer pointers.
		*/
		else
		{
			*buffer += (field.bytesTransfered - dataSetHandle->dataUnderflowSize);
			*bytesToWrite -= (field.bytesTransfered - dataSetHandle->dataUnderflowSize);
		}

		/* If there was underflow then there is possibly usable data in
		   the dataUnderflow buffer.  Set usedDataUnderflowSize so that
		   other routines know there is data to be read in the 
		   dataUnderflow buffer.
		*/
		dataSetHandle->usedDataUnderflowSize = dataSetHandle->dataUnderflowSize;
		dataSetHandle->dataUnderflowSize = 0;
	}

	/* This is the normal case.  There was no underFlow or overFlow on a 
	   previos call to this routine.  This case is used on the intial call 
	   to the routine. This case gets a field. If there is underflow then 
	   the data is copied into the dataUnderflow buffer.  If there is 
	   overflow then ccode is set to NWSMUT_UNDERFLOW   so that at return 
	   the data for the field in the buffer is copied to the stateData 
	   buffer.
	*/
	else
	{
		dataSetHandle->usedDataUnderflowSize = 0;
		ccode = SMDFGetNextField(*buffer, *bytesToWrite, &field);
		if (ccode is NWSMUT_BUFFER_UNDERFLOW)
			goto Underflow;
		else if (ccode)
			goto Return2;
		dataSetHandle->dataOverflow = field.dataOverflow;
		if (SMDF_GT(&dataSetHandle->dataOverflow, 0))
		{
			ccode = NWSMUT_BUFFER_UNDERFLOW;

			/* This is a special case for field headers.  It is 
			   possible that the buffer can just contain the field 
			   ID and the size of the data. If the size of the data
			   is 2 bytes then we are pretty sure this is a header 
			   field.The data will be copied into the dataUnderflow
			   buffer. SO that later we can copy the rest of the 
			   field in the dataUnderflow buffer and get the field 
			   with no problems.
       			*/
			if (!dataSetHandle->recordType and *bytesToWrite 
					<= DATA_UNDERFLOW_SIZE) {
				SMDFGetUINT64(&field.dataSize, &size);
				if (size is sizeof(UINT16)) {
					/* only need to do this if data is 
					   possibly NWSM_SYNC_DATA
					   (sizeof(UINT16) */
					SMDFZeroUINT64(
						&dataSetHandle->dataOverflow);
					goto Underflow;
				}
			}
		}

		*buffer += field.bytesTransfered;
		*bytesToWrite -= field.bytesTransfered;
	}

	goto Return;

	/* If there is data underflow then copy the data into the underflow 
	   buffer and deal with it the next time this routine is called.
 	*/
Underflow:
	dataSetHandle->dataUnderflowSize = *bytesToWrite;
	memcpy(dataSetHandle->dataUnderflow, *buffer, 
				dataSetHandle->dataUnderflowSize);
	*buffer += *bytesToWrite;
	*bytesToWrite = 0;

Return:
	/* ccode is NWSMUT_BUFFER_UNDERFLOW when there is an data overflow so 
	   if there is a record type (we want to keep this data) then copy data
	   into the   state data buffer
 	*/
	if (ccode is NWSMUT_BUFFER_UNDERFLOW)
		if (dataSetHandle->recordType)
			CopyToStateData(dataSetHandle, *buffer);

Return2:
	if (!ccode)
	{
		*fid = field.fid;
		*dataSize = field.dataSize;
		*data = (BUFFERPTR)field.data;
	}
	return (ccode);
}

/****************************************************************************/
/* This routine is called after we have gotten a section and now we need to
 *   get the individual fields for a section.  This code here is mostly special
 *   case code.  If there were no special cases besides initialization code
 *   the code would look like this.
 *
 *   /# Get a field
 *    #/
 *      SMDFGetNextField(dataSetHandle->bufferPtr, dataSetHandle->bufferSize, &field);
 *
 *   /# Adjust up bufferPtr #/
 *      dataSetHandle->bufferPtr += field.bytesTransfered;
 *      dataSetHandle->bufferSize -= field.bytesTransfered;
 *
 *   /# Set up field fid, data and size #/
 *      *fid = field.fid;
 *      *dataSize = field.dataSize;
 *      *data = field.data;
 *
 *   However, there are two special cases that must be dealt with.
 *   data overflow and data underflow.  Data overflow occurs when
 *   you are getting a field from the bufferPtr and all the data for the
 *   field is not in the bufferPtr so you need to start lookin in the next 
 *   buffer to get the rest of the data.  A data underflow occurs when there
 *   is data in the buffer but there is not enough data there to determine what
 *   fid it is and what the size of the data is.
 */
/****************************************************************************/
CCODE GetFieldToProcessSection(RESTORE_DATA_SET_HANDLE *dataSetHandle, 
UINT32 *fid, UINT64 *dataSize, BUFFERPTR *data, UINT32 *dataOverflowSize)
{
	CCODE ccode = 0;
	BUFFERPTR tempAddress;
	UINT32 bytesToMove, bytesToPutBack, bytesToRead, size, tempDataSpace;
	UINT64 dataOverflow;
	SMDF_FIELD_DATA field;

	/* There should be no dataOverflowSize when this routine is called. */
	if (dataOverflowSize)
		*dataOverflowSize = 0;

	/* If this is the first time this routine is called to get a field then
	   bufferPtr will not be set. Set bufferPtr to the begining of the data.
 	*/
	if (!dataSetHandle->bufferPtr)
	{
		if (dataSetHandle->bufferState is BUFFER_STATE)
		{
			dataSetHandle->inBuffer = TRUE;
			dataSetHandle->bufferPtr = dataSetHandle->begin;
			dataSetHandle->bufferSize = 
					dataSetHandle->bufferBytesToWrite;
		}

		else
		{
			if (!dataSetHandle->stateData)
			{
				/* If the state data buffer has not been 
				   allocated then there is a problem
          			*/
				/*
				PrintErrorMessageToDebugScreen(ccode, __FILE__, RNAME, __LINE__,
				    "BufferState problem, bufferState with no stateData");

				*/
				ccode = NWSMTS_BUFFER_UNDERFLOW;
				goto Return;
			}

			dataSetHandle->inBuffer = FALSE;
			dataSetHandle->bufferPtr = dataSetHandle->stateData;
			dataSetHandle->bufferSize = dataSetHandle->stateDataSize;
		}
	}

	/* If bufferSize is zero then we have looked at all the data in the 
	   bufferPtr points at (this could be the buffer or the stateData 
	   buffer). If we have called this routine then we need more data but 
	   if we are in the buffer and there is no more data then there is an 
	   error. However, if we have read all the data in the stateData buffer
	   then we must still look at data in the buffer.
	*/
	if (dataSetHandle->bufferSize <= 0)
	{
		if (dataSetHandle->inBuffer)
		{
			ccode = NWSMTS_BUFFER_UNDERFLOW;
			goto Return;
		}

		/*  There is still data in buffer */
		dataSetHandle->inBuffer = TRUE;
		dataSetHandle->bufferPtr = dataSetHandle->buffer;
		dataSetHandle->bufferSize = dataSetHandle->bufferBytesToWrite;
	}

	/* Get a field from bufferPtr. */
	ccode = SMDFGetNextField(dataSetHandle->bufferPtr,
	    			dataSetHandle->bufferSize, &field);
	dataOverflow = field.dataOverflow;

	/* If there is an underflow then there was not enough data in 
	   bufferPtr to determine what field it was.  If we were looking in 
	   buffer then this is an error.  If there is an underFlow then we must
	   be at the end of the stateData buffer.  Copy what is left in the 
	   stateData buffer into a file and then copy enough data from buffer 
	   into a tempData buffer to determine what field it is.  Also set up 
	   bufferPtr to point to buffer.
	*/
	if (ccode is NWSMUT_BUFFER_UNDERFLOW)
	{
		if (dataSetHandle->inBuffer)
		{
			ccode = NWSMTS_BUFFER_UNDERFLOW;
			goto Return;
		}

		if (!dataSetHandle->tempData and 
		    (dataSetHandle->tempData = (char *) malloc(TEMP_DATA_BUFFER_SIZE)) is NULL)
		{
			ccode = NWSMTS_OUT_OF_MEMORY;
			goto Return;
		}

		memcpy(dataSetHandle->tempData,
		    dataSetHandle->bufferPtr, dataSetHandle->bufferSize);
		tempDataSpace = TEMP_DATA_BUFFER_SIZE - 
					dataSetHandle->bufferSize;

		/* This size is the size of how much data is needed in tempData
		   to get a field without an underflow.
		*/
		size = DATA_UNDERFLOW_SIZE - dataSetHandle->bufferSize;

		memcpy(dataSetHandle->tempData + dataSetHandle->bufferSize,
		    		dataSetHandle->buffer, size);

		tempDataSpace -= size;

		/* Get a field and set dataOverflow */
		if ((ccode = SMDFGetNextField(dataSetHandle->tempData,
		    DATA_UNDERFLOW_SIZE, &field)) isnt 0)
		{
			ccode = NWSMTS_BUFFER_UNDERFLOW;
			goto Return;
		}
		dataOverflow = field.dataOverflow;

		/* If there is dataOverflow then we need more data then has 
		   been put into tempData. tempData is a 10K buffer.The rest of
		   the data had better be in buffer. Copy as much data from 
		   buffer as we need to get the data.
		*/
		if (SMDF_GT(&dataOverflow, 0))
		{
			tempAddress = dataSetHandle->tempData + 
							DATA_UNDERFLOW_SIZE;

			dataSetHandle->inBuffer = TRUE;
			dataSetHandle->bufferPtr = dataSetHandle->buffer + size;
			dataSetHandle->bufferSize = dataSetHandle->bufferBytesToWrite - size;

			if (SMDFGetUINT64(&dataOverflow, &bytesToRead))
			{
				ccode = NWSMTS_OVERFLOW;
				goto Return;  /* If more than UINT32 in buffer,
						 we are broken */
			}

			bytesToMove = _min(tempDataSpace, bytesToRead);
			memcpy(tempAddress, dataSetHandle->bufferPtr, bytesToMove);
			dataSetHandle->bufferPtr += bytesToMove;
			dataSetHandle->bufferSize -= bytesToMove;

			/* dataOverflowSize is currently only necessary for EAs
			   - we left this here for code compatability with the 
			   TSA-311
			*/
			if (bytesToMove < bytesToRead)
			{
				if (dataOverflowSize)
					*dataOverflowSize = bytesToRead - bytesToMove;
			}

			goto Return;
		}

		/* No data overflow occurred and there was an inital underflow.
		   When we copied data into tempData from buffer, we could have
		   copied more data then we needed to get a field so we need to
		   set the buffer back to the end of the field.  The end of the
		   field is in buffer.
		*/
		else {
			if ((bytesToPutBack = DATA_UNDERFLOW_SIZE - 
						field.bytesTransfered) isnt 0) {
				dataSetHandle->inBuffer = TRUE;
				dataSetHandle->bufferPtr = dataSetHandle->buffer
						 + (size - bytesToPutBack);
				dataSetHandle->bufferSize = 
					dataSetHandle->bufferBytesToWrite -
		    			(size - bytesToPutBack);
				goto Return;
			}
		}
		goto Return;
	}   /* ccode is NWSMTS_BUFFER_UNDERFLOW */

	/* If there was an underflow then there will not be an overflow.
	   If there was an overflow then we had better not be pointing into
	   buffer because there is no more data.  If we are in stateData then
	   copy what is left in stateData into tempData, and copy the rest of
	   the data that is needed to get the data for the field
	   from the buffer into tempData.  Set up bufferPtr.
	 */
	if (SMDF_GT(&dataOverflow, 0))
	{
		if (dataSetHandle->inBuffer)
		{
			ccode = NWSMTS_BUFFER_UNDERFLOW;
			goto Return;
		}

		if (!dataSetHandle->tempData and 
		    (dataSetHandle->tempData = (char *) malloc(TEMP_DATA_BUFFER_SIZE)) is NULL)
		{
			ccode = NWSMTS_OUT_OF_MEMORY;
			goto Return;
		}

		memcpy(dataSetHandle->tempData, dataSetHandle->bufferPtr, 
				dataSetHandle->bufferSize);
		tempDataSpace = TEMP_DATA_BUFFER_SIZE - 
					dataSetHandle->bufferSize;

		if (SMDFGetUINT64(&dataOverflow, &bytesToRead))
		{
			ccode = NWSMTS_OVERFLOW;
			goto Return;   /* If more than UINT32 in buffer, we are
					  broken */
		}

		if ((bytesToMove = _min(tempDataSpace, bytesToRead)) isnt 0)
		{
			memcpy(dataSetHandle->tempData + 
				dataSetHandle->bufferSize,
				dataSetHandle->buffer, bytesToMove);

			dataSetHandle->inBuffer = TRUE;
			dataSetHandle->bufferPtr = 
					dataSetHandle->buffer + bytesToMove;
			dataSetHandle->bufferSize = 
				dataSetHandle->bufferBytesToWrite - bytesToMove;
		}

		/* dataOverflowSize is currently only necessary for EAs -we left
		   this here for code compatability with the TSA-311
		*/
		if (bytesToMove < bytesToRead)
		{
			if (dataOverflowSize)
				*dataOverflowSize = bytesToRead - bytesToMove;
		}

		/* Reset all pointers so at Return: things are pointed at the 
		   right place
		*/
		SMDFGetNextField(dataSetHandle->tempData, 
			dataSetHandle->bufferSize + bytesToMove, &field);
	}

	/* There were no problems when you got a field so set up bufferPtr */
	else
	{
		dataSetHandle->bufferPtr += field.bytesTransfered;
		dataSetHandle->bufferSize -= field.bytesTransfered;
	}

Return:
	if (!ccode)
	{
		*fid = field.fid;
		*dataSize = field.dataSize;
		*data = (BUFFERPTR)field.data;
	}
	return (ccode);
}

/****************************************************************************/
/*   Copy data from buffer to a stateData buffer.
 */
CCODE CopyToStateData(RESTORE_DATA_SET_HANDLE *dataSetHandle, BUFFERPTR buffer)
{
	CCODE ccode = 0;
	UINT32 size;

	if (dataSetHandle->bufferState is BUFFER_STATE)
	{
		if (!dataSetHandle->stateData)
		{
			if ((dataSetHandle->stateData = (char *) malloc(STATE_DATA_SIZE)) is NULL)
			{
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return;
			}
		}

		size = buffer - dataSetHandle->begin;
		dataSetHandle->stateDataSize = _min(size, STATE_DATA_SIZE);

		memcpy(dataSetHandle->stateData, dataSetHandle->begin, 
						dataSetHandle->stateDataSize);
		dataSetHandle->bufferState = STATE_DATA_BUFFER_STATE;
	}

	else
		ccode = NWSMTS_BUFFER_UNDERFLOW;

Return:
	return (ccode);
}
/****************************************************************************/
/****************************************************************************/

