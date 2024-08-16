#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/strip.C	1.2"

#include <smsutapi.h>

#if defined (DEBUG_CODE)
#define STATIC
#else
#define STATIC static
#endif

STATIC void GetVolumeAndLastSeparator(
		UINT32	  nameSpaceType,
		STRING	 *path,
		STRING	 *volSep,
		STRING	 *lastSep);

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMStripPathChild"
#endif
	STRING NWSMStripPathChild(
		UINT32	nameSpaceType,
		STRING	path,
		STRING	child,
		size_t	maxChildLength)
{
	STRING volSep, lastSep = NULL, ptr = NULL;
	CHAR *separatorPos, separator;

	separator = NWSMStripEndSeparator(nameSpaceType, path, &separatorPos);
	GetVolumeAndLastSeparator(nameSpaceType, &path, &volSep, &lastSep);

	if (child)
	{
		if (lastSep)
		{
			strncpy((PSTRING)child, (PSTRING)(lastSep + 1), maxChildLength - 1);
			child[maxChildLength - 1] = 0;
		}

		else 
			child[0] = 0;
	}

	if (lastSep)
	{
		*(lastSep + 1) = 0;
		ptr = lastSep + 1;
	}

	if (separator)
		*separatorPos = separator;

	return (ptr);
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMGetPathChild"
#endif
STRING NWSMGetPathChild(
		UINT32	 nameSpaceType,
		STRING	 path,
		STRING	*child)
{
	STRING volSep, lastSep, ptr;
	CHAR *separatorPos, separator;

	separator = NWSMStripEndSeparator(nameSpaceType, path, &separatorPos);
	GetVolumeAndLastSeparator(nameSpaceType, &path, &volSep, &lastSep);

	if (lastSep)
		ptr = lastSep + 1;

	else 
		ptr = NULL;


	if (separator)
		*separatorPos = separator;

	if (child)
		*child = ptr;

	return (ptr);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMPathIsNotFullyQualified"
#endif
NWBOOLEAN NWSMPathIsNotFullyQualified(
		UINT32	nameSpaceType,
		STRING	path)
{
	STRING volSep, lastSep;
	CHAR *separatorPos;
	NWBOOLEAN rcode = TRUE;
	CHAR separator;

	separator = NWSMStripEndSeparator(nameSpaceType, path, &separatorPos);
	GetVolumeAndLastSeparator(nameSpaceType, &path, &volSep, &lastSep);

	if (volSep)
		rcode = FALSE;

	if (separator)
		*separatorPos = separator;
	return (rcode);
}


#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMLegalDOSName"
#endif
NWBOOLEAN NWSMLegalDOSName(
		STRING	name)
{
	STRING dotOffset;
	int length;
	NWBOOLEAN rcode = FALSE;

	length = strlen((PSTRING)name);
	if (length > 12 or name[0] is '.')
		goto Return;

	if ((dotOffset = strchr((PSTRING)name, '.')) is 0)
	{
		if (length > 8)
			goto Return;
	}

	else
	{/*	If there are more than 8 chars before the '.', error */
		if ((dotOffset - name) > 8)
			goto Return;

	/*	If has dot AND (has another dot OR (more than 3 chars OR
			0 char after dot)) then it's invalid */
		else if (strchr((PSTRING)dotOffset + 1, '.') or
				(length - (dotOffset - name + 1) ) > 3)
			goto Return;
	}

	rcode = TRUE;

Return:
    return(rcode);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMGetVolume"
#endif
STRING NWSMGetVolume(
		STRING	*ptr,
		UINT32	 nameSpaceType,
		STRING	 volume)
{
	STRING volSep;
	CHAR save;

#if defined(DEBUG_CODE)
	if (!ptr or !volume or nameSpaceType > MaxNameSpaces)
		goto Error;
#endif

	if ((volSep = strchr((PSTRING)*ptr, ':')) is NULL)
		goto Error;

	save = *(++volSep);
	if (nameSpaceType is MACNameSpace and save isnt ':')
		goto Error;

	*volSep = 0;
	if (strlen((PSTRING)*ptr) > 16)
	{
		*volSep = save;
		goto Error;
	}

	strncpy((PSTRING)volume, (PSTRING)*ptr, 16);
	volume[16] = 0;
	*volSep = save;

	if (nameSpaceType is MACNameSpace)
		++volSep;

	*ptr = volSep;

	return (volume);

Error:
	return (NULL);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "GetVolumeAndLastSeparator"
#endif
STATIC void GetVolumeAndLastSeparator(
		UINT32	 nameSpaceType,
		STRING	*path,
		STRING	*volSep,
		STRING	*lastSep)
{/*	PATH_SEPERATOR	*/
	STRING backSlash = NULL, slash, firstslash = NULL;

	*lastSep = NULL;
	switch ((UINT16)nameSpaceType)
	{
	case NFSNameSpace:
		*volSep = strchr((PSTRING)*path, (int)':');

		firstslash = strchr((PSTRING)*path, (int)'/');
		if (  firstslash != NULL && firstslash < *volSep ) {
			*volSep = firstslash ;
		}
		slash = strrchr((PSTRING)*path, (int)'/');

		if (slash and slash > *volSep)
			*lastSep = slash;
		else if (*volSep and strlen((PSTRING)*volSep + 1))
			*lastSep = *volSep;
		else
			*lastSep = NULL ;
		break;

	case DOSNameSpace:
		backSlash	= strrchr((PSTRING)*path, (int)'\\');
	case FTAMNameSpace:
	case OS2NameSpace:
		*volSep = strchr((PSTRING)*path, (int)':');
		slash = strrchr((PSTRING)*path, (int)'/');

		if (backSlash and backSlash > slash)
			slash = backSlash;

		if (slash and slash > *volSep)
			*lastSep = slash;
		else if (*volSep and strlen((PSTRING)*volSep + 1))
			*lastSep = *volSep;
		else
			*lastSep = NULL ;
		break;

	case MACNameSpace:
		*volSep = strstr((PSTRING)*path, "::");
		if (!*volSep)
		{
			*path = NULL;
			return;
		}

		*lastSep = strrchr((PSTRING)*path, (int)':');
		if (*lastSep and *lastSep is *volSep + 1 and
					!strlen((PSTRING)(*lastSep + 1)))
			*lastSep = NULL;
		break;

	default:
		*path = NULL;
		return;
	}
}

CHAR NWSMStripEndSeparator(
		UINT32	  nameSpaceType,
		STRING	  path,
		CHAR	**separatorPos)
{
	STRING endPos;
	CHAR endChar = 0;

	endPos = LastChar(path);
	switch ((UINT16)nameSpaceType)
	{
	case DOSNameSpace:
		if (*endPos is '\\')
		{
			endChar = *endPos;
			*endPos = 0;
			if (separatorPos)
				*separatorPos = (CHAR *)endPos;
			break;
		}

	case NFSNameSpace:
	case FTAMNameSpace:
	case OS2NameSpace:
		if (*endPos is '/')
		{
			endChar = *endPos;
			*endPos = 0;
			if (separatorPos)
				*separatorPos = (CHAR *)endPos;
		}
		break;

	case MACNameSpace:
		if (*endPos is ':' and *(endPos - 1) isnt ':')
		{
			endChar = *endPos;
			*endPos = 0;
			if (separatorPos)
				*separatorPos = (CHAR *)endPos;
		}
		break;
	}

	return (endChar);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMFixDOSPath"
#endif
STRING NWSMFixDOSPath(
		STRING	path,
		STRING	newPath)
{
	STRING ptr, returnPath = path, sepChars = ":/\\", slash = "/";

	strupr((PSTRING)path);
	for (ptr = strchr((PSTRING)path, '\\'); ptr;
			ptr = strchr((PSTRING)ptr, '\\'))
		*ptr = '/';

	if ((ptr = strstr((PSTRING)path, ":/")) isnt NULL)
		memmove((PSTRING)ptr + 1, (PSTRING)ptr + 2,
				strlen((PSTRING)ptr + 2) + 1);

	if (newPath and !EndChar(path, sepChars))
		returnPath = NWSMStr(2, (PSTRING)newPath,
				(PSTRING)path, (PSTRING)slash);

	return (returnPath);
}
