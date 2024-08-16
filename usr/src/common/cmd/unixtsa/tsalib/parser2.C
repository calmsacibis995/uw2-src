#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/parser2.C	1.2"

#include <smsutapi.h>
#include <smsfids.h>

#if defined(__TURBOC__) || defined(MSC)
#define FFFFFFFF 0xFFFFFFFFL
#else
#define FFFFFFFF 0xFFFFFFFF
#endif

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "SMDFGetFields"
#endif
CCODE SMDFGetFields(
	UINT32 headFID,					/*	Specifies the next FID head the calling
										routine expects to find in buffer
										(i.e., block header, media trailer). */
	NWSM_GET_FIELDS_TABLE table[],	/*	Defines the field group (i.e., header)
										the calling routine expects to find in
										buffer */
	BUFFERPTR *buffer,				/*	buffer to be parsed */
	UINT32 *bufferSize)				/*	size of buffer */
{
	CCODE ccode = 0;
	int index;
	SMDF_FIELD_DATA field;

	if ((ccode = SMDFGetNextField(*buffer, *bufferSize, &field)) isnt 0)
		goto Return;

	if (field.fid isnt headFID)
	{
		ccode = NWSMUT_INVALID_FIELD_ID;
		goto Return;
 	}

	/* adjust buffer pointers to next field */
	*buffer += field.bytesTransfered;
	*bufferSize -= field.bytesTransfered;

	/* loop until matching end field is found */
	loop
	{
		if ((ccode = SMDFGetNextField(*buffer, *bufferSize, &field)) isnt 0)
			goto Return;

		/* adjust buffer pointers to next field */
		*buffer += field.bytesTransfered;
		*bufferSize -= field.bytesTransfered;

		/* if matching end FID found, break */
		if (field.fid is headFID)
			break;

		/*	See if fid is in table, if so put field information into table.
		 *	Loop until we look at all fields in table or when a specified
		 *	FID is found */

		for (index = 0; table[index].fid isnt NWSM_END; index++)
		{
			/* if field not previously found, put the data into the table;
				otherwise skip the field. */
			if (!table[index].found and field.fid is table[index].fid)
			{
				UINT32 dataSize;

				/* see if the field data can fit into table's buffer */
				SMDFGetUINT64(&field.dataSize, &dataSize);
				if (dataSize > table[index].dataSize)
				{
					ccode = NWSMUT_BUFFER_OVERFLOW;
					goto Return;
				}

#if defined(__TURBOC__) || defined(MSC)
			{
				BUFFERPTR dest, src;
				UINT32 bytesNotMoved;
				UINT16 bytesToMove;

				bytesNotMoved = dataSize;
				dest = (BUFFERPTR)table[index].data;
				src = (BUFFERPTR)field.data;
				while (bytesNotMoved)
				{
					bytesToMove = _min(bytesNotMoved, 32000);
					memcpy((void *)dest, (void *)src, bytesToMove);
					bytesNotMoved -= bytesToMove;
					dest += bytesToMove;
					src += bytesToMove;
				}
			}
#else
				memcpy(table[index].data, field.data, dataSize);
#endif
				table[index].dataSize = dataSize;
				table[index].found = TRUE;
				break;
			}
		}
	}

Return:
	return (ccode);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "SMDFPutFields"
#endif
CCODE SMDFPutFields(
		NWSM_FIELD_TABLE_DATA table[],	/*	passes the table defining the next
											field group to get */
		BUFFERPTR *buffer,				/*	points to a buffer to contain the
											fields */
		UINT32 *bufferSize,				/*	size of buffer */
		UINT32 crcFlag)					/*	CRC_YES, CRC_NO, CRC_LATER */
{
	BUFFERPTR begin = NULL;

	CCODE ccode = 0;
	SMDF_FIELD_DATA endField;
	UINT16 index,
			 offsetIndex = 0xFFFF,  /* index for offset to end FID */
			 sync = NWSM_SYNC_DATA;	/* sync data is "A55A" */

	UINT32 crc = FFFFFFFF, dataOverflow;
	UINT64 offsetToEnd, tempValue;

/* initialize structures, use memset to satisfy MicroSoft */
	memset(&endField, 0, sizeof(endField));
	memset(&offsetToEnd, 0, sizeof(offsetToEnd));

/* set up beginning and ending fields, beginning and ending FIDs must match,
	so copy the beginning FID information */
	endField.fid = table[0].field.fid;

/* set ending fid crc value to known value */
	endField.data = &crc;

/* set sync, shown as "A55A" in SMSDF document */
	table[0].dataSizeMap = table[0].sizeOfData = sizeof(sync);
	table[0].field.data = &sync;
	begin = *buffer;

/* put fields into buffer, loop until NWSM_END is found */
	for (index = 0; table[index].field.fid isnt NWSM_END; ++index)
	{
		if (table[index].field.fid is NWSM_OFFSET_TO_END)
		{
			/*	offset is not known at this point, so set up the field
				information so it can be put into the buffer now.  A pointer
				to where the offset value can be put into the buffer is set
				later on. */

			table[index].field.data = &offsetToEnd;
			SetUINT64Value(&offsetToEnd, table[index].dataSizeMap);
			table[index].addressOfData = GET_ADDRESS;  /* (void *)1 */
			offsetIndex = index;
		}

		else if (table[index].addressOfData)
		{
			/* the calling routine wants a pointer to where the data is
				or will be in the buffer, so find out the size of the size of
				data size descriptor.  This will be used later on. */

			SetUINT64Value(&tempValue, table[index].dataSizeMap);
			table[index].field.data = &tempValue;
		}

		if ((ccode = SMDFPutNextField(*buffer, *bufferSize, &table[index].field,
				table[index].dataSizeMap, table[index].sizeOfData)) isnt 0)
			goto Return;

		SMDFGetUINT64(&table[index].field.dataOverflow, &dataOverflow);
		if (dataOverflow)
		{
			ccode = NWSMUT_BUFFER_OVERFLOW;
			goto Return;
		}

/* SET addressOfData */
		/* calling routine wants the address of where the data is or where it
			can put data or a value needs to be stored (i.e., offset) */

		if (table[index].addressOfData)
		{
			table[index].addressOfData = (void *)(*buffer +
					(table[index].field.bytesTransfered -
					(table[index].sizeOfData)));
			if (table[index].field.data)
			{
				memcpy(table[index].addressOfData, table[index].field.data,
					table[index].dataSizeMap);
			}
		}
		
		*buffer += table[index].field.bytesTransfered;
		*bufferSize -= table[index].field.bytesTransfered;
	}

/*	If OFFSET_TO_END field written then calculate and set it */
/*	calculate offset from start of next field (after offset) to start of
	CRC field or end field */

	if (offsetIndex isnt 0xFFFF and table[offsetIndex].addressOfData)
	{
		/* calculate offset value */
		SMDFPutUINT64(&offsetToEnd, *buffer -
				((BUFFERPTR)table[offsetIndex].addressOfData +
				table[offsetIndex].dataSizeMap));

		if ((UINT8)SMDFSizeOfUINT64Data(offsetToEnd) > 
					(UINT8)table[offsetIndex].dataSizeMap)
		{
			ccode = NWSMUT_BUFFER_UNDERFLOW;
			goto Return;
		}

		/* see comment label SET addressOfData to see where it was set */
		memcpy(table[offsetIndex].addressOfData, &offsetToEnd,
				table[offsetIndex].dataSizeMap);
	}

	if (crcFlag is CRC_NO)
		ccode = SMDFPutNextField(*buffer, *bufferSize, &endField,
				(UINT8)0, (UINT32)0);

	else
	{
		if (crcFlag is CRC_YES)
			crc = NWSMGenerateCRC((UINT32)(*buffer - begin), crc, begin);

		else /*	hold off on calculating CRC, calling routine
				will calculate CRC */
			crc = FFFFFFFF;

		ccode = SMDFPutNextField(*buffer, *bufferSize, &endField,
				(UINT8)sizeof(crc), (UINT32)sizeof(crc));
	}

	if (ccode)
		goto Return;

	SMDFGetUINT64(&endField.dataOverflow, &dataOverflow);
	if (dataOverflow)
	{
		ccode = NWSMUT_BUFFER_OVERFLOW;
		goto Return;
	}

	*buffer += endField.bytesTransfered;
	*bufferSize -= endField.bytesTransfered;

Return:
	return (ccode);
}

