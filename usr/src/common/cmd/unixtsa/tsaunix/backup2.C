#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/backup2.C	1.3"
/*****************************************************************************
 *
 * Program Name:  Storage Management Services (TSA-DOSP)
 *
 * Filename:      backup2.c
 *
 * Date Created:  July 12, 1991
 *
 * Version:               1.0
 *
 ****************************************************************************/

#include <smsfids.h>
#include "tsaglobl.h"

#define END_DATA_SIZE                   9
/****************************************************************************/
CCODE PutFields(BUFFERPTR *buffer, UINT32 *bytesToRead,
NWSM_FIELD_TABLE_DATA table[], UINT32 *bytesRead,
NWBOOLEAN processEndField, UINT32 tailFID,
BACKUP_DATA_SET_HANDLE *dataSetHandle,
UINT32 *fileOffsetPtr)
{
	BUFFERPTR bufferPtr, overflowData;
	CCODE ccode = 0, _ccode;
	UINT32 bufferSize, byteSize, dataOverflow, size;
	UINT16 index = 0, offsetIndex = 0xffff, sync = NWSM_SYNC_DATA;
	UINT64 offsetToEnd ;
	SMDF_FIELD_DATA endField ;
	BUFFER endData[END_DATA_SIZE];

	/* ref is a macro to avoid a compiler warning */
	ref(fileOffsetPtr);

	memset(&endField,'\0',sizeof(SMDF_FIELD_DATA));
	memset(&offsetToEnd,'\0',sizeof(UINT64));

	sync = SwapUINT16(sync);

	/* Set up the sync field in the header buffer sync is A55A */
	if (table[index].field.fid == NWSM_BEGIN) {
		index++;
		table[index].dataSizeMap = table[index].sizeOfData = sizeof(sync);
		table[index].field.data = &sync;
		dataSetHandle->begin = *buffer;
	}

	/* Set up bufferPtr to point to where data should start and bufferSize 
	   to the amount of data that can
   	   fit in the bufferPtr */
	switch (dataSetHandle->bufferState) {
	case BUFFER_STATE:
		bufferPtr = *buffer;
		bufferSize = *bytesToRead;
		break;

	case STATE_DATA_BUFFER_STATE:
		if (!dataSetHandle->stateData) {
			if ((dataSetHandle->stateData = 
					(char *) malloc(STATE_DATA_SIZE)) == NULL) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				goto Return;
			}
			memset(dataSetHandle->stateData,'\0',STATE_DATA_SIZE);
		}

		bufferPtr = dataSetHandle->stateData + 
					dataSetHandle->stateDataSize;
		bufferSize = STATE_DATA_SIZE - dataSetHandle->stateDataSize;
		break;
	} 
	// end switch

	/* Put each field in the table out to the buffer */
	for (; table[index].field.fid != NWSM_END; ++index) {
		if (table[index].field.fid == NWSM_SKIP_FIELD)
			continue;

		// If the fid is the offset to end fid then set up a pointer to
		// its data area so this value can be updated later 
		if (table[index].field.fid == NWSM_OFFSET_TO_END) {
			table[index].field.data = &offsetToEnd;
			SetUINT64Value(&offsetToEnd, table[index].dataSizeMap);
			table[index].addressOfData = GET_ADDRESS;
			offsetIndex = index;
		}

Retry: 
		_ccode = SMDFPutNextField(bufferPtr, bufferSize, 
				&table[index].field, table[index].dataSizeMap, 
				table[index].sizeOfData);

		/* Get the dataOverflow value - a data overflow occurs when the
		   field is too large for the buffer */
		SMDFGetUINT64(&table[index].field.dataOverflow, &dataOverflow);

#ifdef DEBUG
		logerror(PRIORITYWARN,
			"SMDFPutNextField:fid %x, ccode %x, overflow %d\n",
			table[index].field.fid, _ccode, dataOverflow);
#endif

		/* Save into addressOfData the address where the data actually 
		   is in buffer or stateDataBuffer */
		if (!_ccode && table[index].addressOfData) {
			if ((table[index].field.fid == NWSM_OFFSET_TO_END) && 
								dataOverflow)
				goto OverflowBuffer;

			table[index].addressOfData = bufferPtr + 
				(table[index].field.bytesTransfered -
				(table[index].sizeOfData - dataOverflow));
		}

		if (_ccode) {
			/* This error occurs when a FID cannot fit into what is
			   left of the buffer - When this error occurs create 
			   the stateData buffer this data will put into the 
			   stateData buffer when we go to Retry */
			if (_ccode == NWSMUT_BUFFER_OVERFLOW) {
OverflowBuffer:
				switch (dataSetHandle->bufferState) {
				case BUFFER_STATE:
					if (!dataSetHandle->stateData) {
						if ((dataSetHandle->stateData =
						     (char *) malloc(STATE_DATA_SIZE)) ==
								 NULL) {
							ccode = 
							   NWSMTS_OUT_OF_MEMORY;
							goto Return;
						}
					}

					memset(bufferPtr,'\0',*bytesToRead);
					*buffer += *bytesToRead ;
					*bytesRead += *bytesToRead ;
					*bytesToRead = 0;
					bufferPtr = dataSetHandle->stateData;
					bufferSize = STATE_DATA_SIZE;
					dataSetHandle->bufferState = 
							STATE_DATA_BUFFER_STATE;
					goto Retry;

				case STATE_DATA_BUFFER_STATE:
					ccode = NWSM_BUFFER_OVERFLOW;
					goto Return;
				} 
				// end switch
			}
			else {
				ccode = NWSMTS_READ_ERROR;
				goto Return;
			}
		}
		else
		{
			switch (dataSetHandle->bufferState)
			{
			case BUFFER_STATE:
				*buffer += table[index].field.bytesTransfered;
				*bytesToRead -= table[index].field.bytesTransfered;
				*bytesRead += table[index].field.bytesTransfered;

				/* dataOverflow occurs when all the data for a 
				   field will not fit where the buffer is 
				   pointing */
				if (dataOverflow) {
					/* locate start of overflow data */
					overflowData = (BUFFERPTR)table[index].field.data + table[index].sizeOfData - dataOverflow;

					/* allocate stateData buffer */
					if (!dataSetHandle->stateData) {
						if ((dataSetHandle->stateData = (char *) malloc(STATE_DATA_SIZE)) == NULL)
						{
							ccode = NWSMTS_OUT_OF_MEMORY;
							goto Return;
						}
					}

					/* copy overflow data into stateData buffer */
					dataSetHandle->stateDataSize = _min(dataOverflow, STATE_DATA_SIZE);
					memcpy(dataSetHandle->stateData, overflowData, dataSetHandle->stateDataSize);

					/* dataOverflow cannot exist */
					dataOverflow -= dataSetHandle->stateDataSize;
					if (dataOverflow)
					{
						ccode = NWSM_BUFFER_OVERFLOW;
						goto Return;
					}

					dataSetHandle->bufferState = STATE_DATA_BUFFER_STATE;

					bufferPtr = dataSetHandle->stateData + dataSetHandle->stateDataSize;
					bufferSize = STATE_DATA_SIZE - dataSetHandle->stateDataSize;
				}
				else
				{
					bufferPtr += table[index].field.bytesTransfered;
					bufferSize -= table[index].field.bytesTransfered;
				}
				break;

			case STATE_DATA_BUFFER_STATE:
				if (dataOverflow)
				{
					ccode = NWSM_BUFFER_OVERFLOW;
					goto Return;
				}

				dataSetHandle->stateDataSize += table[index].field.bytesTransfered;
				bufferPtr += table[index].field.bytesTransfered;
				bufferSize -= table[index].field.bytesTransfered;
				break;

			}   
			// end switch
		} 
		// end else
	}

	if (processEndField)
	{
		BUFFERPTR offsetField = NULL;
		UINT16  dataSize;

		/* find out if there is an offset to end field and where it is */
		if (dataSetHandle->addressOfDataOffsetField)
		{
			offsetField = dataSetHandle->addressOfDataOffsetField;
			dataSize = dataSetHandle->sizeOfDataOffsetField;
		}

		else if (offsetIndex != 0xFFFF && table[offsetIndex].addressOfData)
		{
			offsetField = (BUFFERPTR)table[offsetIndex].addressOfData;
			dataSize = table[offsetIndex].dataSizeMap;
		}

		if (offsetField)
		{
			/* Determine the amount of data that has been put out 
			   from the end of the offset field to the begining of 
			   the end field  of the offsetField - the offset field
			   is in the buffer else the offset field is in the 
			   stateData buffer */
			if (offsetField < *buffer && offsetField > dataSetHandle->begin)
				size = (*buffer - (offsetField + dataSize)) + dataSetHandle->stateDataSize;

			else
				size = dataSetHandle->stateDataSize - ((offsetField + dataSize) - dataSetHandle->stateData);

			SMDFPutUINT64(&offsetToEnd, size);

			/* Determine if enough room was saved to store the 
			   offset to end value if not zero it */
			if ((byteSize = SMDFSizeOfUINT64Data(offsetToEnd)) > dataSize)
				SMDFZeroUINT64(&offsetToEnd);

			memcpy(offsetField, &offsetToEnd, dataSize);
#ifdef SPARC
			switch (dataSize) {
			case 8 : 
				memcpy(offsetField,
					(char *)SwapUINT64p(offsetField),8);
				break ;

			case 4 : 
				SwapUINT32buf(offsetField);
				break ;

			case 2 : 
				SwapUINT16buf(offsetField);
				break ;
			}
#endif
		}

		endField.fid = tailFID;
		SMDFPutNextField(endData, END_DATA_SIZE, &endField, 0, 0);

		switch (dataSetHandle->bufferState)
		{
		case BUFFER_STATE:
			if (*bytesToRead >= endField.bytesTransfered) {
				memmove(*buffer, endData, endField.bytesTransfered);
				*buffer += endField.bytesTransfered;
				*bytesToRead -= endField.bytesTransfered;
				*bytesRead += endField.bytesTransfered;
			}
			else {
				if (*bytesToRead) {
					memmove(*buffer, endData, *bytesToRead);
					*buffer += *bytesToRead;
					*bytesRead += *bytesToRead;
				}

				if (!dataSetHandle->stateData) {
					if ((dataSetHandle->stateData = (char *) malloc(STATE_DATA_SIZE)) == NULL)
					{
						ccode = NWSMTS_OUT_OF_MEMORY;
						goto Return;
					}
				}

				memmove(dataSetHandle->stateData, endData + *bytesToRead, endField.bytesTransfered - *bytesToRead);
				dataSetHandle->stateDataSize = endField.bytesTransfered - *bytesToRead;
				dataSetHandle->bufferState = STATE_DATA_BUFFER_STATE;
				*bytesToRead = 0;
			}
			break;

		case STATE_DATA_BUFFER_STATE:

			size = _min(STATE_DATA_SIZE - dataSetHandle->stateDataSize, endField.bytesTransfered);
			memmove(dataSetHandle->stateData + dataSetHandle->stateDataSize, endData, size);
			dataSetHandle->stateDataSize += size;
			if (size < endField.bytesTransfered)
			{
				ccode = NWSM_BUFFER_OVERFLOW;
				goto Return;
			}
			break;
		} 
		// end switch
	}

	else if (offsetIndex != 0xffff)
	{
		dataSetHandle->addressOfDataOffsetField = (BUFFERPTR)table[offsetIndex].addressOfData;
		dataSetHandle->sizeOfDataOffsetField = table[offsetIndex].dataSizeMap;
	}

Return:
	return (ccode);
}

/****************************************************************************/
/****************************************************************************/

