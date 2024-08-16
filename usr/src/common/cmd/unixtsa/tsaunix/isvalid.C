#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/isvalid.C	1.8"
/*********************************************************************
 *
 * Filename:      isvalid.c
 *
 ********************************************************************/

#include <tsad.h>
#include "tsapi.h"
#include <smsutapi.h>
#include "tsaglobl.h"
#include "drapi.h"
#include <string.h>

/********************************************************************
Function: ExcludeDataSet

Purpose:  Checks to see if a data set has been excluded

Parameters: dataSetSequence       input
			dataSetInfo          input

Returns:    ccode - which contains information about whether
			the data set has been excluded
*********************************************************************/
CCODE ExcludeDataSet(DATA_SET_SEQUENCE *dataSetSequence,
struct stat *dataSetInfo)
{
	CCODE ccode = 0;

	ccode = ExcludedByScanControl(dataSetSequence, dataSetInfo);

	if (!ccode)
		ccode = ExcludedByBackupSelectionList(dataSetSequence);

	return (ccode);
}

/********************************************************************
Function: ExcludedByScanControl

Purpose:  Check to see if a data set excluded by scan control

Parameters: dataSetSequence                        input
			dataSetInfo                           input

Returns:   ccode - TRUE  if the data set is excluded by scan control 
					FALSE if it is not

*********************************************************************/
CCODE ExcludedByScanControl(DATA_SET_SEQUENCE *dataSetSequence,
struct stat *dataSetInfo)
{
	NWBOOLEAN ccode = FALSE;
	NWSM_SCAN_CONTROL *scanControl;

	scanControl = dataSetSequence->scanControl;

	if (!scanControl || !scanControl->bufferSize)
		goto Return;

	if (!dataSetInfo)
		goto Return;

	switch (dataSetSequence->dataSetType)
	{
	case TYPE_FILE:
		if (scanControl->parentsOnly) {
			ccode = TRUE;
			goto Return;
		}

		if ((scanControl->scanType & NWSM_EXCLUDE_SPECIAL_FILES) && 
			S_ISSPECIAL(dataSetInfo->st_mode) ) {
			ccode = TRUE;
			goto Return;
		}
		break ;

	case TYPE_DIRECTORY:
	case TYPE_FILESYSTEM :
		/* This check is not to be done here....
		if (scanControl->childrenOnly) {
			ccode = TRUE;
			goto Return;
		}
		*/
		break ;

	default:
		return(ccode);

	} 
	// end switch

	ccode = NWSMCheckDateAndTimeRange(
				scanControl->firstAccessDateAndTime,
				scanControl->lastAccessDateAndTime,
				dataSetInfo->st_atime );
	if (ccode) {
		ccode = TRUE;
		goto Return;
	}

	ccode = NWSMCheckDateAndTimeRange(
				scanControl->firstCreateDateAndTime,
				scanControl->lastCreateDateAndTime,
				dataSetInfo->st_ctime );
	if (ccode) {
		ccode = TRUE;
		goto Return;
	}

	ccode = NWSMCheckDateAndTimeRange(
				scanControl->firstModifiedDateAndTime,
				scanControl->lastModifiedDateAndTime,
				dataSetInfo->st_mtime );
	if (ccode) {
		ccode = TRUE;
		goto Return;
	}

Return:
	return (ccode);
}

/********************************************************************
Function: ExcludedByBackupSelectionList
Purpose:  Check to see if a data set excluded by the backup
		  selection lists

Parameters: dataSetHandle         input
Returns: ccode that tells if the data set is included or excluded   

Decision Table for ExcludeByBackupSelectionList

DataSet       List to be searched     If dataset Found in List/
Type                              If dataset NOT Found in List
+----------------------------------------------------------------------+
|              |                         |                             |
| File System  | ResourceToInclude       |  DATA_SET_INCLUDED/         |
|              |                         |  DATA_SET_EXCLUDED or       |
|              |                         |  DATA_SET_NOT_INCLUDED      |
|              |                         |                             |
|              | ResourceToExclude       |  DATA_SET_EXCLUDED/         |
|              |                         |  DATA_SET_INCLUDED          |
|              |                         |                             |
|   Directory  | subdirectoriesToInclude |  DATA_SET_INCLUDED/         |
|              |                         |  DATA_SET_EXCLUDED or       |
|              |                         |  DATA_SET_NOT_INCLUDED      |
|              |                         |                             |
|              | subdirectoriesToExclude |  DATA_SET_EXCLUDED/         |
|              |                         |  DATA_SET_INCLUDED          |
|              |                         |                             |
+--------------+-------------------------+-----------------------------+
|              |                         |                             |
|    File      | filesToInclude          |  DATA_SET_INCLUDED/         |
|              |                         |  DATA_SET_EXCLUDED          |
|              |                         |                             |
|              | filesToExclude          |  DATA_SET_EXCLUDED/         |
|              |                         |  DATA_SET_INCLUDED          |
|              |                         |                             |
|              | filesWithPathsToInclude |  DATA_SET_INCLUDED/         |
|              |                         |  DATA_SET_EXCLUDED only     |
|              |                         |    if parent found          |
|              |                         |                             |
|              | filesWithPathsToExclude |  DATA_SET_EXCLUDED/         |
|              |                         |  DATA_SET_INCLUDED          |
|              |                         |                             |
+----------------------------------------------------------------------+

*********************************************************************/
CCODE ExcludedByBackupSelectionList(DATA_SET_SEQUENCE *dataSetSequence)
{
	BUFFERPTR fullPathPtr;
	CCODE ccode = DATA_SET_EXCLUDED;
	NWBOOLEAN fullPath = FALSE ;
	SELECTION selectionLists ;

	fullPathPtr = dataSetSequence->fullPath->string;

	if ( strncmp(fullPathPtr,TSAGetMessage(ROOT_OF_FS),
			strlen(TSAGetMessage(ROOT_OF_FS))) == 0 ) {
		fullPathPtr += strlen(TSAGetMessage(ROOT_OF_FS)) -1 ;
		*fullPathPtr = '/' ;
		fullPath = TRUE ;
	}

	strcpy(tmpPath,fullPathPtr);
	if (dataSetSequence->dataSetType ==  TYPE_FILE) {
		strcat(tmpPath,dataSetSequence->singleFileName);
	}
#ifdef DEBUG
	logerror(PRIORITYWARN,"ExcludedByBackup:fullPath %s, tmppath %s\n",
		fullPathPtr, tmpPath);
	FLUSHLOG ;
#endif
	if ( IsInDefaultIgnoreList(dataSetSequence->connection,
				tmpPath) == TRUE ) {
		if ( fullPath == TRUE ) {
			*fullPathPtr = ':' ;
		}
		return (ccode);
	}

	selectionLists.definedResourcesToInclude =
					dataSetSequence->definedResourcesToInclude ;
	selectionLists.definedResourcesToExclude =
					dataSetSequence->definedResourcesToExclude ;
	selectionLists.subdirectoriesToInclude =
					dataSetSequence->subdirectoriesToInclude ;
	selectionLists.subdirectoriesToExclude =
					dataSetSequence->subdirectoriesToExclude ;
	selectionLists.filesToInclude =
					dataSetSequence->filesToInclude ;
	selectionLists.filesToExclude =
					dataSetSequence->filesToExclude ;
	selectionLists.filesWithPathsToInclude =
					dataSetSequence->filesWithPathsToInclude ;
	selectionLists.filesWithPathsToExclude =
					dataSetSequence->filesWithPathsToExclude ;

	switch (dataSetSequence->dataSetType)
	{
	case TYPE_FILESYSTEM:
	case TYPE_DIRECTORY:
		ccode = IncludeOrExcludeSubDirectories(
			dataSetSequence->resourcePtr->resourceName, fullPathPtr,
			&selectionLists, NFSNameSpace) ;
		break;

	case TYPE_FILE: 
		ccode = IncludeOrExcludeFilesAndFilesWithPaths(
			dataSetSequence->resourcePtr->resourceName, fullPathPtr,
			dataSetSequence->singleFileName,
			&selectionLists, NFSNameSpace) ;

		break;
	} // end switch

	if ( fullPath == TRUE ) {
		*fullPathPtr = ':' ;
	}

	return (ccode);
}

/**********************************************************************
Function: ExcludedByRestoreSelectionList

Purpose:  Check to see if a data set excluded by the restore
          selection lists

Parameters: dataSetHandle         input

Returns: ccode that tells if the data set is included or excluded   

*********************************************************************/
CCODE ExcludedByRestoreSelectionList(
RESTORE_DATA_SET_HANDLE *dataSetHandle)
{
	CCODE ccode = NWSMTS_DATA_SET_EXCLUDED;

	ccode = ExcludedByRestoreSelectionList1(dataSetHandle,
				dataSetHandle->dataSetType,
				dataSetHandle->dataSetName->string) ;

	switch (ccode ) {
	case DATA_SET_INCLUDED :
	case DATA_SET_NOT_INCLUDED :
		dataSetHandle->excluded = FALSE;
		break;

	case DATA_SET_EXCLUDED :
		dataSetHandle->excluded = TRUE;
		break;
	}
	return(ccode);
}

/*********************************************************************
Function: ExcludedByRestoreSelectionListi1

Purpose:  Check to see if a data set excluded by the restore
          selection lists

Parameters: dataSetHandle         input

Returns: ccode that tells if the data set is included or excluded   
*********************************************************************/
CCODE ExcludedByRestoreSelectionList1(
RESTORE_DATA_SET_HANDLE *dataSetHandle,
UINT32 dataSetType,
STRING path)
{
	RESTORE_DATA_SET_HANDLE 
			*parentHandle = dataSetHandle->parentDataSetHandle;
	SELECTION *selectionLists = 
				dataSetHandle->connection->selectionLists;
	CCODE ccode = NWSMTS_DATA_SET_EXCLUDED;
	CHAR *chr, terminalNode[MAXNAMLEN+1];
	STRING fullPathPtr = path;
	NWBOOLEAN fullPath = FALSE ;

	if (!selectionLists) {
		ccode = DATA_SET_INCLUDED;
		goto Return;
	}

	// ccode has been set to NWSMTS_DATA_SET_EXCLUDED 
	if (parentHandle && parentHandle->excluded) {
		goto Return;
	}

	if (fullPathPtr == NULL) {
		goto Return;
	}

	if ( (chr = strchr(fullPathPtr, ':')) != NULL ) {
		fullPathPtr = chr ;
		*fullPathPtr = '/' ;
		fullPath = TRUE ;
	}

	switch (dataSetType) {
	case TYPE_FILESYSTEM:
	case TYPE_DIRECTORY:   
		ccode = IncludeOrExcludeSubDirectories(
			dataSetHandle->resourcePtr->resourceName, fullPathPtr,
			selectionLists, NFSNameSpace) ;
		break;

	case TYPE_FILE: 
		// If the target == a file check include/exclude
		// files and files with paths 

		chr = NWSMStripPathChild(NFSNameSpace, fullPathPtr,
				terminalNode, MAXNAMLEN);

		ccode = IncludeOrExcludeFilesAndFilesWithPaths(
			dataSetHandle->resourcePtr->resourceName, fullPathPtr,
			terminalNode, selectionLists, NFSNameSpace) ;

		if ( chr ) {
			*chr = *terminalNode ;
		}
		break;

	} // end switch

Return:

	if ( fullPath == TRUE ) {
		*fullPathPtr = ':' ;
	}

	return (ccode);
}

CCODE IsInDefaultIgnoreList(CONNECTION *connection, STRING path)
{
	CCODE ccode = FALSE ;
	NWSM_LIST *tempListPtr;

	tempListPtr = NWSMGetListHead(&connection->defaultIgnoreList);
	while (tempListPtr != NULL ) {
#ifdef DEBUG
		logerror(PRIORITYWARN,"IsInDefaultIgnoreList:match %s %s\n",
			path, tempListPtr->text);
		FLUSHLOG ;
#endif
		if (match(path, tempListPtr->text) != 0) {
			break ;
		}
		tempListPtr = tempListPtr->next;
	}

	if (tempListPtr != NULL) {
		ccode = TRUE;
	}

	return (ccode);
}

//static char wildchars[] = "*[]?" ;

/*
 * match - a simple regular expression matching function for file
 *         names and paths.
 *
 * synopsis : int match(char *s, char *p)
 *
 * description :
 *     s is the path being matched. p is the pattern to match.
 *     p can contain metacharacters. only '*', '?' and [] are
 *     supported.
 *
 * returns :
 *     1 - successful match
 *     2 - partial match. Only the head of the path matches.
 *     0 - no match
 */
int match(char *s, char *p)
{
	char scc;
	int ok, lc;
	char c, cc, prevmatch = 0;

/*
	if ( strpbrk(p,wildchars) == NULL ) {
		if (strcmp(s,p) == 0) {
			return(1);
		}
		if (strncmp(s,p,strlen(p)) == 0) {
			return(2);
		}
		return(0);
	}
*/
	if (*s == '.' && *p != '.')
		return (0);

	for (;;) {
		scc = *s++ ;
		switch (c = *p++) {

		case '[':
			ok = 0;
			lc = 077777;
			while (cc = *p++) {
				if (cc == ']') {
					if (ok)
						break;
					return(0) ;
				}
				if (cc == '-') {
					if (lc <= scc && scc <= *p++)
						ok++;
				} 
				else if (scc == (lc = cc)) {
					ok++;
				}
			}
			if (cc == '\0') {
				return(0) ;
			}
			prevmatch = cc ;
			break ;

		case '*':
			s-- ;
			while ( *p == '*' ) p++ ;
			if (!*p) {
				if ( (s = strchr(s,'/')) == NULL ) {
					return(1);
				}
				else { 
				    if ( *++s == '\0' ) {
						return(1);
					}
					return(2);
				}
			}
			if (*p == '/') {
				p++;
				if ( (s = strchr(s,'/')) == NULL ) {
					return (0);
				}
				s++ ;
				prevmatch = *p ;
				break ;
			}
			if ( *p == '[' || *p == ']' ) {
				prevmatch = *p ;
				break  ;
			}

			if ( *(p+1) == '\0' ) {
				while ( s && *(s+1) && *(s+1) != '/') {
					s++ ;
				}
				if (s &&  *s == *p ) {
					if ( *(s+1) == '/' ) {
						return(2);
					}
					return(1) ;
				}
				return(0);
			}

			while ( s != NULL && *s != '\0' ) {
				if ( *s == '/' ) {
					return(0);
				}
				else if ( *s != *p ) {
					s++ ;
				}
				else {
					prevmatch = *p ;
					break ;
				}
			}
			if ( s == NULL || *s == '\0' )
				return(0);
			break  ;

		case 0:
			if ( scc != '\0' ) {
				if ( scc == '/' ) {
					if ( *s == '\0' ) {
						return (1);
					}
					return(2);
				}
				if (prevmatch == '/') {
					return(2);
				}
				return(0);
			}
			return (1);

		default:
			if (c != scc)
				return (0);
			prevmatch = c ;
			break ;

		case '?':
			if (scc == '\0' || scc == '/')
				return (0);
			prevmatch = c ;
			break ;
		}
	}
}

CCODE IncludeOrExcludeDefinedResources(char *fullPathPtr,
SELECTION *selectionLists)
{
	NWSM_LIST *tempListPtr;

	// 
	//   Check for TSA defined include FS 
	// 
	tempListPtr = NWSMGetListHead(
					&selectionLists->definedResourcesToInclude) ;

	if (tempListPtr != NULL) {
		while (tempListPtr != NULL &&
		    	strcmp(tempListPtr->text, fullPathPtr)) {
			tempListPtr = tempListPtr->next;
		}

		if (tempListPtr == NULL) {
			return(DATA_SET_EXCLUDED);
		}
	}

	// 
	//   Check for TSA defined exclude FS 
	// 

	tempListPtr = NWSMGetListHead(
					&selectionLists->definedResourcesToExclude);
	while (tempListPtr != NULL &&
		    strcmp(tempListPtr->text, fullPathPtr)) {
		tempListPtr = tempListPtr->next;
	}

	if (tempListPtr != NULL) {
		return(DATA_SET_EXCLUDED);
	}
	else {
		return(DATA_SET_INCLUDED);
	}
}

CCODE IncludeOrExcludeSubDirectories(char *fileSystem,
char *fullPathPtr,
SELECTION *selectionLists,
UINT32  nameSpaceType)
{
	NWSM_LIST *headPtr, *tempListPtr;

	if ( IncludeOrExcludeDefinedResources(fileSystem, selectionLists)
			== DATA_SET_EXCLUDED ) {
		return(DATA_SET_EXCLUDED);
	}
	// 
	// check include subdirectories
	//
	headPtr = NWSMGetListHead(
					&selectionLists->subdirectoriesToInclude) ;

	tempListPtr = headPtr;
	if (tempListPtr != NULL) {
		while (tempListPtr != NULL &&
			match(fullPathPtr, tempListPtr->text) == 0) {
			tempListPtr = tempListPtr->next;
		}
		// If the target was found in the Include list skip
		// this block of code and check the exclude list.
 		// If the target wasn't found 
		if (tempListPtr == NULL) {
			tempListPtr = headPtr;
			while (tempListPtr != NULL &&
			    	match(tempListPtr->text, fullPathPtr) == 0) {
				tempListPtr = tempListPtr->next;
			}
			if (tempListPtr != NULL) {
				return(DATA_SET_NOT_INCLUDED);
			}
			else {
				return(DATA_SET_EXCLUDED);
			}
		}
	}
			
	//
	// check exclude subdirectories
	//
	tempListPtr = NWSMGetListHead(
					&selectionLists->subdirectoriesToExclude);
	while (tempListPtr != NULL &&
				match(fullPathPtr, tempListPtr->text) == 0) {
		tempListPtr = tempListPtr->next;
	}
	if (tempListPtr != NULL) {
		return(DATA_SET_EXCLUDED);
	}
	else {
		return(DATA_SET_INCLUDED);
	}
}

CCODE IncludeOrExcludeFilesAndFilesWithPaths(char *fileSystem,
char *fullPathPtr,
char *terminalNode,
SELECTION *selectionLists,
UINT32  nameSpaceType)
{
	NWSM_LIST *tempListPtr;
	BUFFERPTR pathEndPtr2;
	BUFFER fileWithOutPath[MAXNAMLEN + 1];
	CCODE ccode ;

	if ( (ccode = IncludeOrExcludeSubDirectories(fileSystem, 
			fullPathPtr, selectionLists,nameSpaceType)) 
				== DATA_SET_EXCLUDED ) {
		return(DATA_SET_EXCLUDED);
	}
	//
	//   check include file
	//
	tempListPtr = NWSMGetListHead(
					&selectionLists->filesToInclude) ;

	if (tempListPtr != NULL) {
		while (tempListPtr != NULL) {
			if (match(terminalNode, tempListPtr->text) == 1) {
				break;
			}
			tempListPtr = tempListPtr->next;
		} // end while
		if (tempListPtr == NULL) {
			ccode = DATA_SET_NOT_INCLUDED ;
		}
		else {
			ccode = DATA_SET_INCLUDED ;
		}
	}
			
	//
	//   check exclude file
	//
	tempListPtr = NWSMGetListHead(
					&selectionLists->filesToExclude);
	while (tempListPtr != NULL) {
		if (match(terminalNode, tempListPtr->text) == 1) {
			break;
		}
			
		tempListPtr = tempListPtr->next;
	} // end while
			
	if (tempListPtr != NULL) {
		return(DATA_SET_EXCLUDED);
	}

	//
	//   Include file with path ? 
	//
	tempListPtr = NWSMGetListHead(
					&selectionLists->filesWithPathsToInclude) ;
	if (tempListPtr != NULL) {
		int parentFound = 0 ;

		while (tempListPtr != NULL) {
			pathEndPtr2 = NWSMStripPathChild(nameSpaceType,
					tempListPtr->text, fileWithOutPath,
					MAXNAMLEN);
			if (match(fullPathPtr, tempListPtr->text) == 1) {
				parentFound = 1 ;
				if (match(terminalNode, fileWithOutPath) == 1) {
					break;
				}
			}

			if ( pathEndPtr2 ) {
				*pathEndPtr2 = *fileWithOutPath;
			}
			tempListPtr = tempListPtr->next;
		} // end while
			
		if (tempListPtr != NULL) {
			if ( pathEndPtr2 ) {
				*pathEndPtr2 = *fileWithOutPath;
			}
		}
		else if ( parentFound ) {
			return(DATA_SET_EXCLUDED);
		}
	}
	else if ( ccode != DATA_SET_INCLUDED ) {
		/* the parent directory is not included and the file is not
		   explicitly included, so the file is excluded
		*/
		return(DATA_SET_EXCLUDED);
	}
		
	//
	//   Exclude file with path? 
	//
	tempListPtr = NWSMGetListHead(
					&selectionLists->filesWithPathsToExclude);
	while (tempListPtr != NULL) {
		pathEndPtr2 = NWSMStripPathChild(nameSpaceType,
					tempListPtr->text, fileWithOutPath, MAXNAMLEN);
		if (match(fullPathPtr, tempListPtr->text) == 1) {
			if (match(terminalNode, fileWithOutPath) == 1) {
				break;
			}
		}
		*pathEndPtr2 = *fileWithOutPath;
		tempListPtr = tempListPtr->next;
	} // end while
			
	if (tempListPtr != NULL) {
		return(DATA_SET_EXCLUDED);
	}
	else {
		return(DATA_SET_INCLUDED);
	}
}

