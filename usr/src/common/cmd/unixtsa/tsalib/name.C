#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/name.C	1.1"

#include <smsutapi.h>

#include "name.h"


CCODE NWSMFreeName(
		NWSM_DATA_SET_NAME HUGE	*name) ;

STATIC CCODE GetListData(
		NWSM_NAME				*handle,
		NWSM_DATA_SET_NAME HUGE	*name);

STATIC void  PutDataSetName(
		NWSM_NAME	*handle,
		UINT32		 nameSpaceType,
		UINT32		 selectionType,
		NWBOOLEAN	 reverseOrder,
		STRING		 sep1,
		STRING		 sep2,
		STRING		 localName,
		STRING		 name,
		UINT8		 positionCount);

STATIC CCODE CheckBufferSize(
		BUFFERPTR	*buffer,
		NWSM_NAME	*handle,
		STRING		 name,
		UINT8		 positionCount);

STATIC void  CountPositions(
		STRING	 sep1,
		STRING	 sep2,
		STRING	 name,
		UINT8	*positionCount);

/*
 * Assume that name is properly initialised
 */
#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMGetFirstName"
#endif
	CCODE NWSMGetFirstName(
		void HUGE				*buffer,
		NWSM_DATA_SET_NAME HUGE	*name,
		UINT32 HUGE				*handle)
{
	CCODE ccode = 0;

	if (!buffer or !name or !handle)
	{
		ccode = NWSMUT_INVALID_PARAMETER;
		goto Return;
	}

	if ((ccode = NWSMFreeName(name)) != 0 ) {
		goto Return;
	}

	if ((*handle = (UINT32)AllocateMemory(sizeof(NWSM_NAME))) == NULL)
	{
		ccode = NWSMUT_OUT_OF_MEMORY;
		goto Return;
	}

	(void) memset((void *)*handle, '\0', sizeof(NWSM_NAME)) ;
	HANDLE->valid = VALID;

/*	Setup context information for handle, the fields of both list
	have the same arrangement */
	HANDLE->buffer = (BUFFERPTR)buffer;
	HANDLE->count = ((NWSM_SELECTION_LIST HUGE *)buffer)->selectionCount;
	HANDLE->ptr =
			(BUFFERPTR)((NWSM_SELECTION_LIST HUGE *)buffer)->keyInformation +
			((NWSM_SELECTION_LIST HUGE *)buffer)->keyInformationSize;

	ccode = GetListData((NWSM_NAME *)*handle, name);

Return:
	if (ccode)  /* if failed */
		NWSMCloseName(handle);

	return (ccode);
}  /* End of NWSMGetFirstName */

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMGetNextName"
#endif
CCODE NWSMGetNextName(
		UINT32 HUGE				*handle,
		NWSM_DATA_SET_NAME HUGE	*name)
{
	CCODE ccode = 0;

	if (!name or !handle or !*handle)
	{
		ccode = NWSMUT_INVALID_PARAMETER;
		goto Return;
	}

	if (HANDLE->valid isnt VALID)
	{
		ccode = NWSMUT_INVALID_HANDLE;
		goto Return;
	}

	if (!(HANDLE->ptr))
	{
		ccode = NWSMUT_INVALID_HANDLE;
		goto Return;
	}

	if ((ccode = NWSMFreeName(name)) != 0 ) {
		goto Return;
	}

	ccode = GetListData((NWSM_NAME *)*handle, name);
	if (ccode == NWSMUT_NO_MORE_NAMES)
	/*	free handle if there are no more entries */
		NWSMCloseName(handle);

	Return:
		return (ccode);
}  /* End of GetNextDataSet */


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMCloseName"
#endif
CCODE NWSMCloseName(
		UINT32 HUGE	*handle)
{
	CCODE ccode = 0;

	if (!handle or !*handle or HANDLE->valid isnt VALID)
	{
		ccode = NWSMUT_INVALID_HANDLE;
		goto Return;
	}

	if (HANDLE->ptr)
	{
		HANDLE->valid = INVALID;
		free((char *)*handle);
		*handle = 0;
	}

	else
	{
		ccode = NWSMUT_INVALID_HANDLE;
		goto Return;
	}

Return:
	return (ccode);
}  /* End of CloseDataSet */


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMFreeName"
#endif
CCODE NWSMFreeName(
		NWSM_DATA_SET_NAME HUGE	*name)
{
	CCODE ccode = 0;

	if (!name) {
		ccode = NWSMUT_INVALID_PARAMETER;
		goto Return;
	}

	if (name->separatorPositions != NULL ) {
		free((char *)name->separatorPositions);
		name->separatorPositions = NULL ;
	}

	if (name->namePositions != NULL ) {
		free((char *)name->namePositions);
		name->namePositions = NULL ;
	}

Return:
	return (ccode);
}  /* End of NWSMFreeName */

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMGetDataSetName"
#endif
CCODE NWSMGetDataSetName(
		void HUGE				*buffer,
		UINT32					 nameSpaceType,
		NWSM_DATA_SET_NAME HUGE	*name)
{
	CCODE ccode = 0;
	UINT32 handle;

	if (!buffer or !name)
	{
		ccode = NWSMUT_INVALID_PARAMETER;
		goto Return;
	}

	ccode = NWSMGetFirstName(buffer, name, &handle);
	if (ccode)
		goto Return;

	if (nameSpaceType isnt name->nameSpaceType)  /* true, if match not found */
	{
		loop
		{
			ccode = NWSMGetNextName(&handle, name);
			if (ccode)
				break;

			if (nameSpaceType == name->nameSpaceType)
				break;
		}
	}

	NWSMCloseName(&handle);

	Return:
		return (ccode);

}  /* End of NWSMGetDataSetName */


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMGetOneName"
#endif
CCODE NWSMGetOneName(
		void HUGE				*buffer,
		NWSM_DATA_SET_NAME HUGE	*name)
{
	CCODE ccode;
	UINT32 handle;

	if (!buffer or !name)
	{
		ccode = NWSMUT_INVALID_PARAMETER;
		goto Return;
	}

	ccode = NWSMGetFirstName(buffer, name, &handle);

	NWSMCloseName(&handle);

Return:
	return (ccode);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMPutFirstName"
#endif
CCODE NWSMPutFirstName(
		void HUGE	**buffer,
		UINT32		  nameSpaceType,
		UINT32		  selectionType,
		NWBOOLEAN	  reverseOrder,
		STRING		  sep1,
		STRING		  sep2,
		STRING		  name,
		UINT32 HUGE	 *handle)
{
	CCODE ccode = 0;
	UINT8 positionCount, allocated = FALSE;
	STRING localName = NULL, localSep1 = NULL, localSep2 = NULL;
	UINT16 bufSize ;

	/* Check for valid parameters */
	if (!name)
	{
		ccode = NWSMUT_INVALID_PARAMETER;
		goto Return;
	}

	if (!*buffer)
	{
		if ((*buffer = calloc(1, STANDARD_INCREMENT)) == NULL)
		{
			ccode = NWSMUT_OUT_OF_MEMORY;
			goto Return;
		}

		*(UINT16 *)*buffer = STANDARD_INCREMENT;
		((NWSM_DATA_SET_NAME_LIST *)*buffer)->keyInformationSize = 0;
		allocated = TRUE;
	}

	/* Allocate handle */
	if ((*handle = (UINT32)calloc(1, sizeof(NWSM_NAME))) == 0)
	{
		ccode = NWSMUT_OUT_OF_MEMORY;
		goto Return;
	}

	/* initialize handle and header */
	memcpy((char *)&bufSize,*buffer,sizeof(UINT16));
	HANDLE->valid = VALID;
	HANDLE->buffer = (BUFFERPTR)*buffer;

	HANDLE->bufferEnd = HANDLE->buffer + bufSize;

	if (allocated)
		HANDLE->ptr = (BUFFERPTR)
				((NWSM_DATA_SET_NAME_LIST HUGE *)*buffer)->keyInformation;
	else {
             // HANDLE->ptr = (BUFFERPTR)
             // (((NWSM_DATA_SET_NAME_LIST HUGE *)*buffer)->keyInformation +
             // ((NWSM_DATA_SET_NAME_LIST HUGE *)*buffer)-> keyInformationSize);

		HANDLE->ptr = (BUFFERPTR)*buffer + 6 + 
					*((BUFFERPTR)*buffer + 5) ;
	}

	//((NWSM_DATA_SET_NAME_LIST HUGE *)*buffer)->dataSetNameSize = 0;
	*((BUFFERPTR)*buffer + 2) = *((BUFFERPTR)*buffer + 3) = 0 ;
	//((NWSM_DATA_SET_NAME_LIST HUGE *)*buffer)->nameSpaceCount = 0;
	*((BUFFERPTR)*buffer + 4) = 0 ;

	if (reverseOrder)
	{
		if (((localName = strdup((char *)name)) == NULL) or
				((localSep1 = strdup((char *)sep1)) == NULL) or
				((localSep2 = strdup((char *)sep2)) == NULL))
		{
			ccode = NWSMUT_OUT_OF_MEMORY;
			goto Return;
		}

		strrev((char *)localName);
		strrev((char *)localSep1);
		strrev((char *)localSep2);
	}

	else
	{
		localName = name;
		localSep1 = sep1;
		localSep2 = sep2;
	}
	/* set information into buffer */
	CountPositions(localSep1, localSep2, localName, &positionCount);

	if ((ccode = CheckBufferSize((BUFFERPTR *)(buffer), (NWSM_NAME *)*handle,
				name, positionCount)) == 0)
		PutDataSetName((NWSM_NAME *)*handle, nameSpaceType, selectionType,
				reverseOrder, localSep1, localSep2, localName, name,
				positionCount);

Return:
	if (reverseOrder)
	{
		if (localName)
			free((char *)localName);

		if (localSep1)
			free((char *)localSep1);

		if (localSep2)
			free((char *)localSep2);
	}

	return (ccode);
}  /* End of NWSMPutFirstName */

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMPutNextName"
#endif
CCODE NWSMPutNextName(
		void HUGE	**buffer,
		UINT32 HUGE	 *handle,
		UINT32		  nameSpaceType,
		UINT32		  selectionType,
		NWBOOLEAN	  reverseOrder,
		STRING		  sep1,
		STRING		  sep2,
		STRING		  name)
{
	CCODE ccode = 0;
	UINT8 positionCount;
	STRING localName = NULL, localSep1 = NULL, localSep2 = NULL;

	if (!handle or !*handle or !name)
	{
		ccode = NWSMUT_INVALID_PARAMETER;
		goto Return;
	}

	if (reverseOrder)
	{
		if (((localName = strdup((char *)name)) == NULL) or
				((localSep1 = strdup((char *)sep1)) == NULL) or
				((localSep2 = strdup((char *)sep2)) == NULL))
		{
			ccode = NWSMUT_OUT_OF_MEMORY;
			goto Return;
		}

		strrev((char *)localName);
		strrev((char *)localSep1);
		strrev((char *)localSep2);
	}

	else
	{
		localName = name;
		localSep1 = sep1;
		localSep2 = sep2;
	}
	
	CountPositions(localSep1, localSep2, localName, &positionCount);

	if ((ccode = CheckBufferSize((BUFFERPTR *)(buffer), (NWSM_NAME *)*handle,
			name, positionCount)) == 0)
		PutDataSetName((NWSM_NAME *)*handle, nameSpaceType, selectionType,
				reverseOrder, localSep1, localSep2, localName, name,
				positionCount);

Return:
	if (reverseOrder)
	{
		if (localName)
			free((char *)localName);

		if (localSep1)
			free((char *)localSep1);

		if (localSep2)
			free((char *)localSep2);
	}

	return (ccode);
}  /* End of NWSMPutNextName */

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMPutOneName"
#endif
CCODE NWSMPutOneName(
		void HUGE	**buffer,
		UINT32		  nameSpaceType,
		UINT32		  selectionType,
		NWBOOLEAN	  reverseOrder,
		STRING		  sep1,
		STRING		  sep2,
		STRING		  name)
{
	CCODE ccode = 0;
	UINT32 nameHandle;

	if ((ccode = NWSMPutFirstName(buffer, nameSpaceType, selectionType,
			reverseOrder, sep1, sep2, name, &nameHandle)) == 0)
		ccode = NWSMCloseName(&nameHandle);

	return (ccode);
}  /* End of NWSMPutOneName */

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "GetListData"
#endif
STATIC CCODE GetListData(
		NWSM_NAME				*handle,
		NWSM_DATA_SET_NAME HUGE	*name)
{
	CCODE ccode = 0;
	UINT8 count ;

	if (handle->index >= handle->count)
	{
		ccode = NWSMUT_NO_MORE_NAMES;
		goto Return;
	}

/*	Copy list information into "name" */

/*	get name space or selection type */
	name->nameSpaceType = SwapUINT32p(handle->ptr);
	handle->ptr += sizeof(UINT32);

/*	get selection type, for data set names this is "reserved" */
	name->selectionType = SwapUINT32p(handle->ptr);
	handle->ptr += sizeof(UINT32);

	name->count = *handle->ptr;  /* count can be zero */
	++handle->ptr;

	if (name->count)
	{
	//	name->namePositions = handle->ptr ;
	// Above line translates to
		
		if ((name->namePositions = 
			(UINT16 HUGE *)calloc(1,name->count * sizeof(UINT16))) 
				== NULL ) {
			ccode = NWSMUT_OUT_OF_MEMORY;  
			goto Return;
		}
		memcpy(name->namePositions, handle->ptr,
			name->count * sizeof(UINT16));
	// Till here
		handle->ptr += name->count * sizeof(UINT16);

	//	name->separatorPositions = handle->ptr ;
	// Above line translates to
		
		if ((name->separatorPositions = 
			(UINT16 HUGE *)calloc(1,name->count * sizeof(UINT16))) 
				== NULL ) {
			free((char *)name->namePositions);
			ccode = NWSMUT_OUT_OF_MEMORY;  
			goto Return;
		}
		memcpy((char *)name->separatorPositions, handle->ptr,
			name->count * sizeof(UINT16));
	// Till here
		handle->ptr += name->count * sizeof(UINT16);

		for ( count = 0; count < name->count ; count++ ) {
			name->separatorPositions[count] = 
				SwapUINT16(name->separatorPositions[count]);
			name->namePositions[count] = 
				SwapUINT16(name->namePositions[count]);
		}
	}

	else
	{
		name->namePositions = NULL;
		name->separatorPositions = NULL;
	}

	name->nameLength = SwapUINT16p(handle->ptr);
	handle->ptr += sizeof(UINT16);

	name->name = (STRING)handle->ptr;
	handle->ptr += name->nameLength + 1;

	/* handle->ptr now points to beginning of next element in list */
	++(handle->index);

Return:
	return (ccode);
}  /* End of GetSelectionListElement */

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "CountPositions"
#endif
STATIC void CountPositions(
		STRING	 sep1,
		STRING	 sep2,
		STRING	 name,
		UINT8	*positionCount)
{
	STRING s, prev = NULL;
	UINT32 sep2Len;

	*positionCount = 0;

	if (sep1)
	{
		if ((prev = strstr((PSTRING)name, (PSTRING)sep1)) isnt 0)
		{/*	get volume separator position */
			(*positionCount)++;
			prev += strlen((PSTRING)sep1);
		}
	}

	if (sep2 and prev and *prev)  /* get directory separator position */
	{
		sep2Len = strlen((PSTRING)sep2);
		for (s = prev; ; (*positionCount)++)
		{
			if ((s = strstr((PSTRING)s, (PSTRING)sep2)) isnt NULL)
			{
				s += sep2Len;
				prev = s;
			}

			else
				break;
		}
	}

	if (sep1 or sep2)
		if (prev and (prev isnt (name + strlen((PSTRING)name))))
		/*	handle "sys:public/old" case here */
			(*positionCount)++;

}  /* End of CountPositions */

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "CheckBufferSize"
#endif
STATIC CCODE CheckBufferSize(
		BUFFERPTR	*buffer,
		NWSM_NAME	*handle,
		STRING		 name,
		UINT8		 positionCount)
{
	BUFFERPTR buf;
	CCODE ccode = 0;
	UINT32 size;
	UINT16 bufSize ;

/*	Check if there is enough buffer space for entry */
	size = strlen((PSTRING)name) + 1 + (positionCount * 2 * sizeof(UINT16)) +
			STRUCT_SIZE;
	if ((handle->bufferEnd - handle->ptr) < size + (STANDARD_INCREMENT >> 4))
	{
		memcpy((char *)&bufSize,handle->buffer,sizeof(UINT16));
		size += bufSize + STANDARD_INCREMENT;
#if defined(__TURBOC__) || defined(MSC)
		if (size > 32767)
		{
			ccode = NWSMUT_INVALID_PARAMETER;
			*buffer = NULL;
			goto Return;
		}

		if ((buf = (BUFFERPTR)realloc((void *)*buffer, (UINT16)size)) == NULL)
#else
		if ((buf = (BUFFERPTR)realloc(*buffer, (unsigned int)size)) == NULL)
#endif
		{
			ccode = NWSMUT_OUT_OF_MEMORY;  /* buffer information is unchanged */
			*buffer = NULL;
			goto Return;
		}

		*buffer = buf;
		/* reset handle and list header information */
		handle->ptr = buf + (handle->ptr - handle->buffer);
		handle->buffer = buf;
		handle->bufferEnd = buf + size;
		memcpy(handle->buffer,(char *)&size, sizeof(UINT16));
	}

Return:
	return (ccode);
} /* End of CheckBufferSize */

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "PutDataSetName"
#endif
STATIC void PutDataSetName(
		NWSM_NAME	*handle,
		UINT32		 nameSpaceType,
		UINT32		 selectionType,
		NWBOOLEAN	 reverseOrder,
		STRING		 sep1,
		STRING		 sep2,
		STRING		 localName,
		STRING		 name,
		UINT8		 positionCount)
{
	UINT32 sep2Len;
	BUFFERPTR namePosition, separatorPosition, tempPtr, lastPosition;
	UINT16 strLength;
	UINT16 position, namesize, tempPosition ;
	UINT8 i;

/*	mark data set name space type */
	memcpy(handle->ptr, SwapUINT32buf((char *)&nameSpaceType), sizeof(UINT32));
	handle->ptr += sizeof(UINT32);

/*	set selection type, data set name list does not care about what goes here */
	memcpy(handle->ptr, SwapUINT32buf((char *)&selectionType), sizeof(UINT32));
	handle->ptr += sizeof(UINT32);

/*	set number of name and separator positions
 *	number of separators can be one less than number of name position.
 *	in this case the last separator position contains a 0.  Therefore
 *	the path information cannot begin with a separator
 *      But this argument is not true for UNIX TSA.
 */
 	*handle->ptr++ = positionCount;

	strLength = strlen((char *)name);

	if (positionCount)
	{
  	/*	get name and separator positions; set up pointers to insert name
		and separator positions */
		if (reverseOrder)
			namePosition = handle->ptr;

		else
		{
			*handle->ptr = 0 ;
			*(handle->ptr + 1) = 0 ;
			namePosition = handle->ptr ;
		}

		separatorPosition = handle->ptr + positionCount * sizeof(UINT16);

		lastPosition = NULL;

		if (sep1)   /* get volume separator position */
		{
			if ((tempPtr = strstr((PSTRING)localName, (PSTRING)sep1)) isnt 0)
			{
				position = tempPtr - localName;
				if ( position == 0 ) { /* name starting with 
							separator */
					tempPosition = strlen(sep1);
					memcpy(namePosition, 
						SwapUINT16buf((char *)&tempPosition), 
						sizeof(UINT16)) ;
				}
					

				if (reverseOrder)
					position = strLength - position - 1;

				memcpy(separatorPosition, 
					SwapUINT16buf((char *)&position), 
					sizeof(UINT16)) ;

				separatorPosition += sizeof(UINT16);
				lastPosition = tempPtr + strlen((PSTRING)sep1);
			}
		}
		namePosition += sizeof(UINT16);

		if (sep2 and lastPosition and *lastPosition)
		{
			sep2Len = strlen((PSTRING)sep2);
			for (i = 1; i < positionCount; i++)
			{
				position = lastPosition - localName;
				if (reverseOrder)
					position = strLength - position + 1;

				memcpy(namePosition, 
					SwapUINT16buf((char *)&position), 
					sizeof(UINT16)) ;

				namePosition += sizeof(UINT16);

				if ((tempPtr = strstr((PSTRING)lastPosition, (PSTRING)sep2))
						isnt NULL)
				{
				/*	get separator position */
					position = tempPtr - localName;
					if (reverseOrder)
						position = strLength - position - 1;

					memcpy(separatorPosition, 
						SwapUINT16buf((char *)&position), 
						sizeof(UINT16)) ;

					separatorPosition += sizeof(UINT16);
					lastPosition = tempPtr + sep2Len;
				}

				else
				{/*	get separator position */
					if (reverseOrder) {
						*separatorPosition = 0xFF;
						*(separatorPosition+1) = 0xFF;
					}
					else {
						*separatorPosition = 0 ;
						*(separatorPosition+1) = 0 ;
					}
					separatorPosition += sizeof(UINT16);
				}
			}
		}

		if (reverseOrder) {
			*namePosition = 0;
			*(namePosition+1) = 0;
		}

		handle->ptr += positionCount * 2 * sizeof(UINT16);
	}

	/* get name length */
	memcpy(handle->ptr, SwapUINT16buf((char *)&strLength), sizeof(UINT16));
	handle->ptr += sizeof(UINT16);

/*	get name */
	strcpy((PSTRING)handle->ptr, (PSTRING)name);

/*	reset handle and header information */
	handle->count++;
	handle->ptr += strlen((PSTRING)name) + 1;

	namesize = handle->ptr - handle->buffer - sizeof(UINT16);

	memcpy(handle->buffer + 2, SwapUINT16buf((char *)&namesize),
			sizeof(UINT16));
 	//((NWSM_DATA_SET_NAME_LIST HUGE *)handle->buffer)->nameSpaceCount++;
	(*(handle->buffer + 4))++ ;
}

