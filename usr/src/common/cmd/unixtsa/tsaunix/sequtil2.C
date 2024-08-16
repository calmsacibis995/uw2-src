#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/sequtil2.C	1.5"
/**********************************************************************
 *
 * Program Name:  Storage Management Services (tsa-ws)
 *
 * Filename:      sequtil2.c
 *
 * Date Created:  July 12, 1991
 *
 * Version:               1.0
 *
 *********************************************************************/

#include <tsad.h>
#include "tsaglobl.h"
#include <tsalib.h>

/**********************************************************************
Function: AppendDataSetNamesAndPaths

Purpose:  AppendDataSetNamesAndPaths appends a name in its 
          appropriate name space to the nameSpacePath structure
          for each supported name space.  It also calls
          BuildDataSetNames to have the full paths
          in each supported name space for this appended directory

Returns:       ccode - 0 if successful
               non-zero if failure

**********************************************************************/

CCODE AppendDataSetNamesAndPaths(STRING FileName, DATA_SET_SEQUENCE *dataSetSequence)
{
	CCODE ccode = 0;
	STRING sep1 = NULL;

#ifdef DEBUG
	logerror(PRIORITYWARN,"AppendDataSetNamesAndPaths: name %s,path %s\n", 
			FileName, dataSetSequence->fullPath->string);
#endif

	if (!EndChar(dataSetSequence->fullPath->string,
	    		TSAGetMessage(UNIX_PATH_SEPARATOR)) &&
		!EndChar(dataSetSequence->fullPath->string,
	    		TSAGetMessage(UNIX_FS_SEPARATOR)) )
		sep1 = TSAGetMessage(UNIX_PATH_SEPARATOR);

	if (!NWSMCatStrings(4, &dataSetSequence->fullPath,
	    dataSetSequence->fullPath->string,
	    sep1, FileName, TSAGetMessage(UNIX_PATH_SEPARATOR)))

		ccode = NWSMTS_OUT_OF_MEMORY;

	else
		ccode = NWSMPutOneName((void **)&dataSetSequence->dataSetNames,
		    NFSNameSpace, 0, 0,
		    TSAGetMessage(UNIX_FS_SEPARATOR),
		    TSAGetMessage(UNIX_PATH_SEPARATOR),
		    dataSetSequence->fullPath->string);

#ifdef DEBUG
	logerror(PRIORITYWARN,"AppendDataSetNamesAndPaths: path %s\n", 
			dataSetSequence->fullPath->string);
#endif
	return (ccode);
}

/*********************************************************************
Function:  BuildDataSetNamesAndPaths

Purpose:   Build DataSetNames And Paths Names

Returns:   ccode - 0 if successful
           non-zero if failure

**********************************************************************/

CCODE BuildDataSetNamesAndPaths(STRING path, DATA_SET_SEQUENCE *dataSetSequence)
/* NOTE path is always a full path
 */
{
	CCODE ccode = 0;
	STRING str, newPath = NULL ;
	CHAR save, *lastChr, *slash;
	UINT32 nameSpaceType = NFSNameSpace ;

#ifdef DEBUG
	logerror(PRIORITYWARN,"BuildDataSetnamesAndPaths: path %s\n", path);
#endif

	switch (dataSetSequence->dataSetType)
	{

	case TYPE_FILE:

		str = NWSMGetPathChild(NFSNameSpace, path, NULL);
		save = *str;
		*str = 0;

		if (!NWSMCopyString(&dataSetSequence->fullPath, 
							TSAGetMessage(ROOT_OF_FS), -1))
		{
			ccode = NWSMTS_OUT_OF_MEMORY;
			break ;
		}
		if (!NWSMCatString(&dataSetSequence->fullPath, path, -1))
		{
			ccode = NWSMTS_OUT_OF_MEMORY;
			break ;
		}

		*str = save;

		strcpy(dataSetSequence->singleFileName,str);

		if (!dataSetSequence->scanControl->returnChildTerminalNodeNameOnly) {
			str = (char *) calloc(1, strlen(path) +
					strlen(TSAGetMessage(ROOT_OF_FS)) + 1);
			if ( str == NULL ) {
				ccode = NWSMTS_OUT_OF_MEMORY;
				break ;
			}
			strcpy(str,TSAGetMessage(ROOT_OF_FS));
			strcat(str,path);
			newPath = str ;
		}
		else {
			str = dataSetSequence->fullPath->string ;
		}
			

		ccode = NWSMPutOneName((void **)&dataSetSequence->dataSetNames,
		    NFSNameSpace, 0, 0,
		    TSAGetMessage(UNIX_FS_SEPARATOR),
		    TSAGetMessage(UNIX_PATH_SEPARATOR), str);

		if ( newPath ) {
			free(newPath);
		}

		break;

	case TYPE_FILESYSTEM :
		nameSpaceType = NWSM_TSA_DEFINED_RESOURCE_TYPE ;
	case TYPE_DIRECTORY:
		lastChr = LastChar(path);
		slash = TSAGetMessage(UNIX_PATH_SEPARATOR);
		if ( *lastChr != *slash )
			lastChr = slash ;
		else
			lastChr = NULL ;

		if (!NWSMCopyString(&dataSetSequence->fullPath, 
								TSAGetMessage(ROOT_OF_FS), -1))
		{
			ccode = NWSMTS_OUT_OF_MEMORY;
			break ;
		}
		if (!NWSMCatStrings(3, &dataSetSequence->fullPath,
	    		dataSetSequence->fullPath->string,
	    		path, lastChr))

		{
			ccode = NWSMTS_OUT_OF_MEMORY;
			break ;
		}

		if (!ccode)
			ccode = NWSMPutOneName((void **)&dataSetSequence->dataSetNames,
			    nameSpaceType,
			    0, 0, TSAGetMessage(UNIX_FS_SEPARATOR),
			    slash, dataSetSequence->fullPath->string);

		break;

	default:
		ccode = NWSMTS_INVALID_DATA_SET_TYPE;
		break;
	}

#ifdef DEBUG
	logerror(PRIORITYWARN,"BuildDataSetnamesAndPaths: path %s\n", 
			    dataSetSequence->fullPath->string);
#endif
	return(ccode);
}

/**********************************************************************
Function:   BuildFileDataSetNames

Purpose:

Parameters:  FileName
                 dataSetSequence
                 dataSetNames
Returns:       ccode - 0 if successful
               non-zero if failure

**********************************************************************/
CCODE BuildFileDataSetNames(STRING FileName, DATA_SET_SEQUENCE *dataSetSequence,
NWSM_DATA_SET_NAME_LIST **dataSetNames,
UINT8 returnChildTerminalNodeNameOnly)
/* FileName is a terminalName
 */
{
	CCODE ccode = 0;
	STRING_BUFFER *newDataSetNamePath = NULL;
	STRING str;

	if (returnChildTerminalNodeNameOnly)
	{
		str = FileName;
	}
	else
	{
		if (!NWSMCatStrings(2, &newDataSetNamePath,
		    dataSetSequence->fullPath->string, FileName))
		{
			ccode = NWSMTS_OUT_OF_MEMORY;
			goto Return;
		}

		str = newDataSetNamePath->string;
	}

	strcpy(dataSetSequence->singleFileName,FileName);

	ccode = NWSMPutOneName((void **)dataSetNames, NFSNameSpace,
	    0, 0, TSAGetMessage(UNIX_FS_SEPARATOR),
	    TSAGetMessage(UNIX_PATH_SEPARATOR), str);

Return:
	if (newDataSetNamePath)
		NWSMFreeString(&newDataSetNamePath);

	return (ccode);
}


/**********************************************************************
Function: DelDirFromFullPath

Purpose:

Parameters:    fullPath

Returns:       0 (success)
**********************************************************************/
CCODE DelDirFromFullPath(STRING_BUFFER **fullPath)
{
	NWSMStripPathChild(NFSNameSpace, (*fullPath)->string, NULL, 0);
	return (0);
}

/**********************************************************************
Function:   FixPath

Purpose:    Make a NFS format path out of a possible netware path

Parameters:    properPath             output
              improperPath           input

Returns:       ccode - 0 if successful
              non-zero if failure

**********************************************************************/
CCODE FixPath(STRING_BUFFER **properPath, STRING improperPath)
{
	CCODE ccode = 0;
	STRING slashStr;
	char *ptr, *ptr1 ;

	slashStr = TSAGetMessage(UNIX_PATH_SEPARATOR);

	ptr = strchr(improperPath,':');
	ptr1 = strchr(improperPath,'/');

	if ( !ptr || (ptr1 && ptr1 < ptr) ) {
		if (!NWSMCopyString(properPath, improperPath, -1)) {
			ccode = NWSMTS_OUT_OF_MEMORY;
		}
	}
	else {
		ptr++ ;
		while ( *ptr == '/' ) {
			ptr++ ;
		}

		if (!NWSMCopyString(properPath, slashStr, strlen(slashStr)))
		{
			ccode = NWSMTS_OUT_OF_MEMORY;
		}
		else if (!NWSMCatString(properPath, ptr, -1))
		{
			ccode = NWSMTS_OUT_OF_MEMORY;
		}
	}

	return (ccode);
}

/*********************************************************************
Function: AddSlashToPath

Purpose:  Add a slash to the end of a path name

Returns:  ccode - always 0  

**********************************************************************/

CCODE AddSlashToPath(STRING path)
{
	char *subPath = NULL;

	if (path != NULL)
	{
		subPath = strrchr (path, '/');
		if (subPath != NULL)
		{
			if(strlen(subPath) > 1)
				strcat(path,TSAGetMessage(UNIX_PATH_SEPARATOR));
		}
		else
		{
			strcat(path,TSAGetMessage(UNIX_PATH_SEPARATOR));
		}
	}

	return (0);

}
/*********************************************************************/

