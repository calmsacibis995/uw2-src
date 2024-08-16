#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/vstring.C	1.2"

#include <smsutapi.h>
#if defined(DEBUG_CODE)
#include <conio.h>
#define cprintf	printf
#endif

#define PATH_LENGTH_INCREMENT_VALUE 256

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMCatString"
#endif
STRING NWSMCatString(
		STRING_BUFFER	**dest,
		void			 *source,
		INT16			  srcLen)
{
	STRING string = NULL;
	UINT16 destLen, minLen;

	if (source)
	{
		if (srcLen is -1)
	 		srcLen = strlen((char *)source);

		destLen = *dest ? strlen((*dest)->string) : 0;
		minLen = srcLen + destLen + 1;


		if (!*dest or ((*dest)->size < (UINT16)(minLen +
				(UINT16)(PATH_LENGTH_INCREMENT_VALUE >> 3))))
		{
			if (!NWSMAllocString(dest, minLen + PATH_LENGTH_INCREMENT_VALUE))
				goto Return;
		}

		memcpy((*dest)->string + destLen, source, srcLen);
		(*dest)->string[destLen + srcLen] = 0;
		string = (*dest)->string;
	}

Return:
	return (string);
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMCopyString"
#endif
STRING NWSMCopyString(
		STRING_BUFFER	**dest,
		void			 *src,
		INT16			  srcLen)
{
	STRING string = NULL;

	if (src)
	{
		if (srcLen is -1)
			srcLen = strlen((char *)src);

		if (!*dest or ((UINT16)(srcLen + 
						(UINT16)(PATH_LENGTH_INCREMENT_VALUE >> 3)) >= 
						(UINT16)(*dest)->size))
			if (!NWSMAllocString(dest, srcLen + PATH_LENGTH_INCREMENT_VALUE))
				goto Return;

		memcpy((*dest)->string, src, srcLen);
		(*dest)->string[srcLen] = 0;
		string = (*dest)->string;
	}

Return:
	return (string);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMCatStrings"
#endif
STRING NWSMCatStrings(
		UINT8			  numStrings,
		STRING_BUFFER	**dest,
		void			 *src1,
		void			 *src2,
		...)
{
	STRING *tempPtr;

#if defined(DEBUG_CODE)
	if (!dest)
	{
		cprintf("NULL_POINTER (%s:%u)\n", __FILE__, __LINE__);
		goto Error;
	}
#endif

	if (!NWSMCopyString(dest, src1, -1))
		goto Error;

	for (tempPtr = (STRING *)&src2; numStrings > 1; --numStrings, ++tempPtr)
		if (*tempPtr and !NWSMCatString(dest, (void *)*tempPtr, -1))
			goto Error;

	return ((*dest)->string);

Error:
	return (NULL);
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMFreeString"
#endif
void NWSMFreeString(
		STRING_BUFFER	**string)
{
	if (string and *string)
	{
		free((char *)*string);
		*string = NULL;
	}


#if defined(DEBUG_CODE)
	else if (!string)
		cprintf("NULL_POINTER  (%s:%u)\n", __FILE__, __LINE__);
#endif
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMAllocString"
#endif
STRING_BUFFER *NWSMAllocString(
		STRING_BUFFER	**string,
		INT16			  size)
{
	STRING_BUFFER *ptr;

#if defined(DEBUG_CODE)
	if (!string)
	{
		cprintf("NULL_POINTER  (%s:%u)\n", __FILE__, __LINE__);
		goto ReturnNull;
	}
#endif

	if (*string) /* ReAlloc */
	{
		if (size <= 0)
			size = (*string)->size + PATH_LENGTH_INCREMENT_VALUE;

		if ((ptr = (STRING_BUFFER *)AllocateMemory(size)) is NULL)
		{
#if defined(DEBUG_CODE)
			cprintf("OUT OF MEMORY (%s:%u)\n", __FILE__, __LINE__);
#endif
			goto ReturnNull;
		}

		else 
			ptr->size = size - sizeof(UINT16);

		strcpy(ptr->string, (*string)->string);
		free((char *)*string);
		*string = ptr;
	}

	else
	{
		if (size <= 0)
			size = PATH_LENGTH_INCREMENT_VALUE;
		if ((*string = (STRING_BUFFER *)AllocateMemory(size)) is NULL)
		{
#if defined(DEBUG_CODE)
			cprintf("OUT OF MEMORY (%s:%u)\n", __FILE__, __LINE__);
#endif
			goto ReturnNull;
		}

		(*string)->size = size - sizeof(UINT16);
		(*string)->string[0] = 0;
	}

	return (*string);

ReturnNull:
	return (NULL);
}

