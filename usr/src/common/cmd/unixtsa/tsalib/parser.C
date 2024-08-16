#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/parser.C	1.2"

#include <smsutapi.h>

#if defined(DEBUG_CODE) or defined(MSC)
#include <stdio.h>
#endif

#if defined(__TURBOC__) || defined(MSC)
#define K32  32768
#define K32L 32768L
#endif


#define MAX_UINT32	(UINT32)0xFFFFFFFF
#define CheckUnderflow(n)	if (bufferSize < (n))\
								{ ccode = NWSMUT_BUFFER_UNDERFLOW; goto Return; }\
							else bufferSize -= (n)

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "SMDFGetNextField"
#endif
CCODE SMDFGetNextField(
		BUFFERPTR	buffer,		/*	A pointer to the field to be parsed. */
		UINT32		bufferSize,	/*	The size of the buffer containing the field. */
		SMDF_FIELD_DATA *field)
	/*	UINT32		fid				The value of the Field ID parsed.
	 *	UINT64		dataSize		The size of the data.
	 *	BUFFERPTR	data			The variable which will recieve the address
	 *									of the data.
	 *	UINT32		bytesTransfered	The number of bytes read from the buffer.
	 *	UINT64		dataOverflow	The number of bytes of data which where
	 *									not transfered. */
{
	BUFFERPTR tempFieldPtr = buffer, ptr1, ptr2 = (BUFFERPTR)&field->fid;
	CCODE ccode = 0;
	UINT32 sizeOfData = 0;
	UINT8 longFid = FALSE;

	SMDFZeroUINT64(&field->dataOverflow);

/*	Get the Origin */
	CheckUnderflow(sizeof(UINT8));
	/*  fid '0' is a NULL 1 byte fid, for space filling */
	while (!*tempFieldPtr)
	{
		tempFieldPtr++;
		CheckUnderflow(sizeof(UINT8));
	}

	if (*tempFieldPtr AND 0x80)
	{/*	We have an Origin */
		if (*tempFieldPtr AND 0x40)
		{/*	The Origin is 2 bytes */
			CheckUnderflow(sizeof(UINT8));
			tempFieldPtr += sizeof(UINT16);
		}

		else /*	The Origin is 1 byte */
			tempFieldPtr++;

	/*	Get the FID */
		CheckUnderflow(sizeof(UINT8));
		if (*tempFieldPtr AND 0x80)
		{/*	The FID is 2 bytes */
			CheckUnderflow(sizeof(UINT8));
			tempFieldPtr += sizeof(UINT16);
			longFid = TRUE;
		}

		else /*	The FID is 1 byte */
			tempFieldPtr++;
	}

	else /*	No origin and the FID is 1 byte */
		tempFieldPtr++;

	field->fid = 0;
	for (ptr1 = tempFieldPtr - 1; ptr1 >= buffer; *ptr2++ = *ptr1--);

	if (!field->fid)	// fid '0' is a NULL 1 byte fid, for space filling
	{
		field->data = NULL;
		sizeOfData = 0;
		SMDFSetUINT64(&field->dataSize, &sizeOfData, 4);
		goto Return;
	}

	// we need to swap the fid. Because it is in little endian (intel)
	field->fid = SwapUINT32(field->fid);

	if (SMDFFixedFid(field->fid))
	{
		field->data = (void *)tempFieldPtr;
		sizeOfData = SMDFFixedSize(field->fid);
		SMDFSetUINT64(&field->dataSize, &sizeOfData, 4);

		if (sizeOfData > bufferSize)
			goto DataOverflow;

		else
			goto Return;
	}

/*	Get the dataSize */
	CheckUnderflow(sizeof(UINT8));
	if (*tempFieldPtr AND 0x80)
	{
		if (*tempFieldPtr AND 0x40) /*	We got 1 to 6 bits */
		{
			field->data = (void *)tempFieldPtr;
			sizeOfData = 1;
			SMDFSetUINT64(&field->dataSize, &sizeOfData, 4);
		}

		else
		{/* We got size of size */
			UINT16 sizeOfSize;

			sizeOfSize = 1 << (*tempFieldPtr++ AND 3);
			CheckUnderflow(sizeOfSize);
			SMDFSetUINT64(&field->dataSize, (void *)tempFieldPtr, sizeOfSize);
			SwapUINT64buf(&field->dataSize);

			tempFieldPtr += sizeOfSize;
			field->data = (void *)tempFieldPtr;

			if (SMDF_GT(&field->dataSize, bufferSize))
				goto DataOverflow;

			sizeOfData = *(UINT32 *)&field->dataSize;
		}
	}

	else
	{/*	0x00 to 0x7F size range */
		sizeOfData = *tempFieldPtr++;
		field->data = sizeOfData ? (void *)tempFieldPtr : NULL;
		SMDFSetUINT64(&field->dataSize, &sizeOfData, 4);

		if (sizeOfData > bufferSize)
			goto DataOverflow;
	}

Return:
	if (!ccode)
	{
		tempFieldPtr += sizeOfData;
		field->bytesTransfered = tempFieldPtr - buffer;
	}
	return (ccode);

DataOverflow:
	field->bytesTransfered = tempFieldPtr - buffer + bufferSize;
	field->dataOverflow = field->dataSize;
	SMDFDecrementUINT64(&field->dataOverflow, bufferSize);
	return (ccode);
}

#undef CheckUnderflow

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "SMDFPutNextField"
#endif
CCODE SMDFPutNextField(
		BUFFERPTR	buffer,		/*	A pointer in the buffer where the
									field is to be put */
		UINT32		bufferSize,	/*	The size of the buffer to put field in */
		SMDF_FIELD_DATA *field,
	/*	UINT32		fid				The value of the Field ID, range:
	 *									0x00 - 0x7F, 0x8000 - 0xFFFF,
	 *									0x808000 - 0xBFFFFF, and
	 *									0xC00000000 - 0xFFFFFFFF.
	 *	UINT64		dataSize		The number of bytes of data to be
	 *									placed in buffer.
	 *	BUFFERPTR	data			The address of the data to be put in the
	 *									buffer.
	 *	UINT32		bytesTransfered	The number of bytes of data written to the
	 *									buffer.
	 *	UINT64		dataOverflow	The number of bytes of data which will not
	 *									fit in the buffer. */
	 	UINT8		dataSizeMap,/*	The data size, 0 - 0x7F (0 to 127 bytes,
										dataSize is undefined), 0x80
										(dataSize has data size,
										SMDFPutNextField will pack it),
										0xC0 - 0xFF (bits, dataSize is
										undefined, data is in dataSizeMap). */
		UINT32		sizeOfData)	/*	The size of the data passed in to be put
										in the buffer. */
{
	BUFFERPTR tempFieldPtr = buffer, tempPtr;
	CCODE ccode = 0;
	UINT32 size, tempBufferSize, tempFID;
#if defined(MSC) || defined(UNIX)
	BUFFER tempBuffer[SMDF_MIN_PARSER_BUFFER + 1];
#else
	BUFFER tempBuffer[SMDF_MIN_PARSER_BUFFER + 1] = { 0 };
#endif
	UINT8 longFid = FALSE;

#if defined(__TURBOC__) || defined(MSC)
	BUFFERPTR dest, src;
	UINT16 bytesToMove;
	UINT32 bytesLeftToMove;
#endif

#if defined(MSC) || defined(UNIX)
	memset(tempBuffer, 0, sizeof(tempBuffer));
#endif

/*	Set the tempFieldPtr to tempBuffer if bufferSize is too small */
	if (bufferSize < SMDF_MIN_PARSER_BUFFER)
	{
		tempFieldPtr = tempBuffer;
		*tempFieldPtr++ = TRUE;
	}

/*	Put the Origin */
	size = SMDFSizeOfUINT32Data0(field->fid);
	if (size is 4 or (size is 3 and !(field->fid AND 0x400000L)))
		longFid = TRUE;

	/* Swap the fid and send it out in little endian format. */

	tempFID = SwapUINT32(field->fid);
	for (tempPtr = (BUFFERPTR)&tempFID + size - 1; size; --size)
		*tempFieldPtr++ = *tempPtr--;

/*	Put the dataSize and data */
	SMDFZeroUINT64(&field->dataOverflow);

	if (SMDFFixedFid(field->fid))
	{
		if ((size = SMDFFixedSize(field->fid)) < sizeOfData)
		{
			ccode = NWSMUT_INVALID_PARAMETER;
			goto Return;
		}

		SMDFSetUINT64(&field->dataSize, &size, 4);
	}

	else 
	{
		if (dataSizeMap AND 0x80)
		{
			if (dataSizeMap AND 0x40)
			{/*	The data size and data is 1 byte */
				*tempFieldPtr++ = dataSizeMap;

			/*	Process tempBuffer if in use */
				if (*tempBuffer)
				{
					if ((tempBufferSize = (tempFieldPtr -
								(BUFFERPTR)tempBuffer - 1)) > bufferSize)
						ccode = NWSMUT_BUFFER_OVERFLOW;

					else
					{
						memcpy((void *)buffer, (void *)&tempBuffer[1],
								(size_t)tempBufferSize);
						tempFieldPtr = buffer + tempBufferSize;
					}
				}

				goto Return;
			}

			else
			{
				UINT16 sizeOfSize;

				sizeOfSize = SMDFSizeOfFieldData(field->dataSize, dataSizeMap);
				if (!dataSizeMap)
					dataSizeMap = *(UINT8 *)&field->dataSize;

				*tempFieldPtr++ = dataSizeMap;
				if (sizeOfSize)
				{
#ifdef SPARC
					UINT64 *tempDataSize;

					tempDataSize = SwapUINT64(&field->dataSize);
					memcpy((PSTRING)tempFieldPtr, tempDataSize, sizeOfSize);
#else
					memcpy((PSTRING)tempFieldPtr, &(field->dataSize), sizeOfSize);
#endif
					tempFieldPtr += sizeOfSize;
				}

			/*	Compare sizeOfData and dataSize */
				if (SMDF_LT(&field->dataSize, sizeOfData))
				{
					ccode = NWSMUT_INVALID_PARAMETER;
					goto Return;
				}
			}
		}

		else
		{/*	The data size is 1 byte */
			*tempFieldPtr++ = dataSizeMap;
			if (sizeOfData > dataSizeMap)
			{
				ccode = NWSMUT_INVALID_PARAMETER;
				goto Return;
			}
		}
	}

/*	Process tempBuffer if in use */
	if (*tempBuffer)
	{
		if ((tempBufferSize = (tempFieldPtr - (BUFFERPTR)tempBuffer - 1)) > bufferSize)
		{
			ccode = NWSMUT_BUFFER_OVERFLOW;
			goto Return;
		}

		memcpy((void *)buffer, (void *)&tempBuffer[1], (size_t)tempBufferSize);
		tempFieldPtr = buffer + tempBufferSize;
	}

	bufferSize -= (tempFieldPtr - buffer);

/*	Move the data to the buffer */
	if (sizeOfData > bufferSize)
	{
		UINT32 dataOverflow;

		dataOverflow = sizeOfData - bufferSize;
		SMDFSetUINT64(&field->dataOverflow, &dataOverflow, 4);
		size = bufferSize;
	}

	else
		size = sizeOfData;

#if defined(__TURBOC__) || defined(MSC)
	for (dest = tempFieldPtr, src = field->data, bytesLeftToMove = size; bytesLeftToMove;
			dest += bytesToMove, src += bytesToMove, bytesLeftToMove -= bytesToMove)
	{
		bytesToMove = _min(bytesLeftToMove, K32L);
		memcpy((PSTRING)dest, (PSTRING)src, bytesToMove);
	}

#else
	if (size)
		memcpy(tempFieldPtr, field->data, size);
#endif
	tempFieldPtr += size;

Return:
	if (!ccode)
		field->bytesTransfered = tempFieldPtr - buffer;

	return (ccode);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "SMDFAddUINT64"
#endif
CCODE SMDFAddUINT64(						/* sum = a + b */
		UINT64	*a,
		UINT64	*b,
		UINT64	*sum)
{
	UINT32 carry0 = 0, carry1 = 0;
	INT16 index = 0 ;

	if ( (MAX_UINT32 - a->v[index]) < b->v[index]) {
		carry0 = 1 ;
	}
	sum->v[index] = a->v[index] + b->v[index] ;
	index++ ;

	if ( (MAX_UINT32 - a->v[index]) < (b->v[index] + carry0)) {
		carry1 = 1 ;
	}
	sum->v[index] = a->v[index] + b->v[index] + carry0 ;

	return (carry1 ? NWSMUT_BUFFER_OVERFLOW : 0);
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "Borrow"
#endif
static CCODE Borrow(
		UINT64	*value,
		INT16	 index)
{
	if (index >= 2)
		return (NWSMUT_BUFFER_UNDERFLOW);

	if (!value->v[index])
	{
		value->v[index] = 0xFFFFFFFF;
		return (Borrow(value, index + 1));
	}

	--value->v[index];
	return (0);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "SMDFSubUINT64"
#endif
CCODE SMDFSubUINT64(				/* dif = a - b */
		UINT64 *a,
		UINT64 *b,
		UINT64 *dif)
{
	INT16 index = 0 ;

	if (a->v[index] < b->v[index]) {
		if ( a->v[index+1] == 0 ) {
			return (NWSMUT_BUFFER_UNDERFLOW);
		}
		a->v[index+1]-- ;
		dif->v[index] = MAX_UINT32 - b->v[index] + a->v[index] ;
	}
	else {
		dif->v[index] = a->v[index] - b->v[index] ;
	}
	index++ ;

	if (a->v[index] < b->v[index]) {
		return (NWSMUT_BUFFER_UNDERFLOW);
	}

	dif->v[index] = a->v[index] - b->v[index] ;

	return (0);
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "SMDFSetUINT32Data"
#endif
CCODE SMDFSetUINT32Data(
		UINT64		*dataSize,
		BUFFERPTR	 buffer,
		UINT32		*data)
{
	CCODE ccode = 0;
	UINT32 sizeOfData;

	if ((ccode = SMDFGetUINT64(dataSize, &sizeOfData)) isnt 0)
		goto Return;

	if (sizeOfData > 4)
	{
		ccode = NWSMUT_BUFFER_OVERFLOW;
		goto Return;
	}

	*data = 0;
	memcpy(data, (void *)buffer, (size_t)sizeOfData);

Return:
	return (ccode);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "SMDFSetUINT64"
#endif
CCODE SMDFSetUINT64(
		UINT64	*a,
		void	*buffer,
		UINT16	 n)
{
	CCODE ccode = 0;

	SMDFZeroUINT64(a);

	if (buffer)
	{
		if (n > 8)
			ccode = NWSMUT_BUFFER_OVERFLOW;

		else if (n) {
			memcpy((void *)a, buffer, n);
		}
	}

	return (ccode);
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "SMDFGetUINT64"
#endif
CCODE SMDFGetUINT64(
		UINT64	*a,
		UINT32	*v)
{
	if (*((UINT32 *)a + 1))
		return (NWSMUT_BUFFER_OVERFLOW);
	
	*v = *(UINT32 *)a;
	return (0);
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "SMDFDecrementUINT64"
#endif
CCODE SMDFDecrementUINT64(							/* a -= b */
		UINT64	*a,
		UINT32	 b)
{
	UINT64 x, y;

	x = *a;
	SMDFPutUINT64(&y, b);
	return (SMDFSubUINT64(&x, &y, a));
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "SMDFIncrementUINT64"
#endif
CCODE SMDFIncrementUINT64(						/* a += b */
		UINT64	*a,
		UINT32	 b)
{
#if defined(MSC)
	UINT64 x, y;
	SMDFSetUINT64(&x, a, 8);
#else
	UINT64 x = *a, y;
#endif

	SMDFPutUINT64(&y, b);
	return (SMDFAddUINT64(&y, &x, a));
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "SMDFPrintUINT64"
void SMDFPrintUINT64(
		BUFFERPTR	 buffer,
		UINT64		*data,
		UINT16		 pad)
{
	INT16 i;
	UINT16 printFlag = FALSE;

	sprintf((PSTRING)buffer, "0x");
	buffer += 2;

	for (i = 2; i >= 0; i--) {
		if (data->v[i] or printFlag or pad)
		{
			sprintf((PSTRING)buffer, "%08x", data->v[i]);
			buffer += 4;
			printFlag = TRUE;
		}
	}

	if (!pad and !printFlag)
		strcat((PSTRING)buffer, "0");
}
#endif

