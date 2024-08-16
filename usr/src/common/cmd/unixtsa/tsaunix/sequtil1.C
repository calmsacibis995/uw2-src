#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/sequtil1.C	1.3"
/*********************************************************************
 *
 * Program Name:  Storage Management Services (tsa-ws)
 *
 * Filename:      sequtil1.c
 *
 * Date Created:  
 *
 * Version:      
 *
 *********************************************************************/

#include <tsad.h>
#include "tsaglobl.h"

/*********************************************************************
Function:  FreeDirectoryStack

Purpose:   Free everything in the directory stack

Returns:    void

**********************************************************************/

void FreeDirectoryStack(DATA_SET_SEQUENCE *dataSetSequence)
{
	DIRECTORY_STACK *ptr, *next;

	for (ptr = dataSetSequence->directoryStack; ptr; ptr = next)
	{
		next = ptr->next;
		free((char *)ptr);
	}
}

/*********************************************************************/
void FreeSequenceStructure(DATA_SET_SEQUENCE **dataSetSequence)
{

	NWSMFreeString(&(*dataSetSequence)->fullPath);

	NWSMDestroyList(&(*dataSetSequence)->definedResourcesToInclude);
	NWSMDestroyList(&(*dataSetSequence)->definedResourcesToExclude);
	NWSMDestroyList(&(*dataSetSequence)->subdirectoriesToInclude);
	NWSMDestroyList(&(*dataSetSequence)->subdirectoriesToExclude);
	NWSMDestroyList(&(*dataSetSequence)->filesToInclude);
	NWSMDestroyList(&(*dataSetSequence)->filesToExclude);
	NWSMDestroyList(&(*dataSetSequence)->filesWithPathsToInclude);
	NWSMDestroyList(&(*dataSetSequence)->filesWithPathsToExclude);

	FreeDirectoryStack(*dataSetSequence);

	if ((*dataSetSequence)->dataSetNames)
	{
		free((char *)(*dataSetSequence)->dataSetNames);
		(*dataSetSequence)->dataSetNames = NULL;
	}

	if ((*dataSetSequence)->scanControl)
	{
		free((char *)(*dataSetSequence)->scanControl);
		(*dataSetSequence)->scanControl = NULL;
	}

	if ((*dataSetSequence)->scanInformation)
	{
		free((char *)(*dataSetSequence)->scanInformation);
		(*dataSetSequence)->scanInformation = NULL;
	}

	if ((*dataSetSequence)->connection->numberOfActiveScans) {
		(*dataSetSequence)->connection->numberOfActiveScans-- ;
	}

	TrashValidFlag((*dataSetSequence)->valid);
	free((char *)*dataSetSequence);
	*dataSetSequence = 0;
}

/*********************************************************************
Function:  PopDirectory

Purpose:   Pop directory off of directory stack

Returns:    void

**********************************************************************/

void PopDirectory(DATA_SET_SEQUENCE *dataSetSequence)
{
	DIRECTORY_STACK *stackPtr;

	stackPtr = dataSetSequence->directoryStack->next;
	if (dataSetSequence->directoryStack->dirEntry.dirHandle != NULL){
		closedir(dataSetSequence->directoryStack->dirEntry.dirHandle);
	}
	free((char *)dataSetSequence->directoryStack);
	dataSetSequence->directoryStack = stackPtr;
	DelDirFromFullPath(&dataSetSequence->fullPath);
	dataSetSequence->validScanInformation = 0 ;
}

/**********************************************************************
Function:  PushDirectory

Purpose:   Push directory onto directory stack

Returns:       ccode - 0 if successful
               non-zero if failure
**********************************************************************/

CCODE PushDirectory(DATA_SET_SEQUENCE *dataSetSequence, 
UINT16 scanningMode,
struct stat *dirEntryStat)
{
	CCODE ccode = 0;
	DIRECTORY_STACK *localDirectoryStack = NULL;

	if ((localDirectoryStack = 
		(DIRECTORY_STACK *)calloc(1, sizeof(DIRECTORY_STACK))) is NULL)
	{
		ccode = NWSMTS_OUT_OF_MEMORY;
		goto Return;
	}

	localDirectoryStack->next = dataSetSequence->directoryStack;
	localDirectoryStack->scanningMode = scanningMode;

	/*
	if ( dataSetSequence->directoryStack ) {
		localDirectoryStack->firstScan = 
				dataSetSequence->directoryStack->firstScan ;
		localDirectoryStack->dirEntry =
				dataSetSequence->directoryStack->dirEntry ;
	}
	else {
		localDirectoryStack->firstScan = TRUE ;
	}
	*/

	if ( dataSetSequence->directoryStack ) {
		memcpy((void *)&localDirectoryStack->dirEntry,
			(void *)&dataSetSequence->directoryStack->dirEntry,
			sizeof(DIRECTORY_STRUCT)) ;
	}
	else {
		memset((void *)&localDirectoryStack->dirEntry, '\0',
				sizeof(DIRECTORY_STRUCT)) ;
		if ( dirEntryStat ) {
			memcpy((void *)&localDirectoryStack->dirEntry.statb,
				(void *)dirEntryStat, sizeof(struct stat)) ;
		}
	}

	localDirectoryStack->firstScan = TRUE ;

	dataSetSequence->directoryStack = localDirectoryStack;
	dataSetSequence->directoryStack->dirEntry.dirHandle = NULL ;

Return:
	return (ccode);
}

