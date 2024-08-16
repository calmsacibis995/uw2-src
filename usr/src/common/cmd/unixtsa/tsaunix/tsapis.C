#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/tsapis.C	1.10"

#include <tsad.h>
#include <ctype.h>
#include <smdr.h>
#include <smsdrerr.h>
#include <error.h>
#include <smspcode.h>
#include <tsapi.h>
#include "tsaglobl.h"
#include <smsutapi.h>
#include "respond.h"
#include "drapi.h"
#include "filesys.h"
#include <pwd.h>
#include <stdarg.h>

#ifdef SYSV
#include <crypt.h>
#endif

#ifdef SVR4
#include	<shadow.h>
#endif

/*********************************************************************
Global Variables
**********************************************************************/

void FreeResourceList(RESOURCE_LIST *resourcePtr)
{
	RESOURCE_LIST *tmp ;

	tmp = resourcePtr ;
	while( tmp != NULL ) {
		resourcePtr = tmp->next ;
		FreeMemory(tmp);
		tmp = resourcePtr ;
	}
}

/********************************************************************

Function:		BeginRestoreSession

Parameters:	connection	input

********************************************************************/
CCODE BeginRestoreSession(UINT32 connection)
{
	if (!connection ||
		((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		return NWSMTS_INVALID_CONNECTION_HANDL;
	}
	ReleaseHardLinksList(&((CONNECTION *)connection)->disks);

	return(0);
}

/********************************************************************

Function:		BuildResourceList

Parameters:	connection	input

********************************************************************/
CCODE BuildResourceList(UINT32 connection)
{
	CCODE ccode = 0 ;
	RESOURCE_LIST *resourcePtr, *tmp, *head ;
	FSTAB_PTR filep ;
	TARGET_INFO *targetInfo = ((CONNECTION *)connection)->targetInfo ;
	
	if (!connection ||
		((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		return NWSMTS_INVALID_CONNECTION_HANDL;
	}
	if (((CONNECTION *)connection)->numberOfActiveScans != 0 ) {
		return(NWSMTS_SCAN_IN_PROGRESS);
	}

	NWSMDestroyList(&((CONNECTION *)connection)->defaultIgnoreList); 

	ccode = BuildDefaultIgnoreList((CONNECTION *)connection);
	if ( ccode ) {
		return(ccode);
	}

	tmp = targetInfo->resource ;
	if ( tmp != NULL ) {
		/* Don't free workstation resource node */
		FreeResourceList(tmp->next); 
		targetInfo->resource->next = NULL ;
	}
	else {
		tmp = (RESOURCE_LIST *)AllocateMemory(sizeof(RESOURCE_LIST)); 
		if ( tmp == NULL ) {
			return(NWSMTS_OUT_OF_MEMORY);
		}
		targetInfo->resource = tmp ;
		strcpy(targetInfo->resource->resourceName, 
			TSAGetMessage(WORKSTATION_RESOURCE));
	}
	head = targetInfo->resource ;
	if ( OpenFilesystemTable(&filep) == -1) {
		logerror(PRIORITYWARN,
			TSAGetMessage(OPEN_MNTTAB_FAILED));
		return(NWSMTS_INTERNAL_ERROR);
	}
	while ((resourcePtr = GetLocalFilesystemEntry(filep)) != NULL) {
		if ( IsInDefaultIgnoreList((CONNECTION *)connection,
				resourcePtr->resourceName) == TRUE ) {
			continue ;
		}
		tmp = (RESOURCE_LIST *)AllocateMemory(sizeof(RESOURCE_LIST)); 
		if ( tmp == NULL ) {
			FreeResourceList(targetInfo->resource); 
			targetInfo->resource = NULL ;
			return(NWSMTS_OUT_OF_MEMORY);
		}
		memcpy((char *)tmp,(char *)resourcePtr,sizeof(RESOURCE_LIST)); 
		head->next = tmp ;
		head = head->next ;
	}
	CloseFilesystemTable(filep);
	
	return 0 ;
}


/********************************************************************

Function:		CatDataSetName

*********************************************************************/
CCODE CatDataSetName(UINT32 connection,
UINT32 nameSpaceType,
STRING dataSetName,
STRING terminalName,
NWBOOLEAN terminalNameIsParent,
char **newDataSetName)
{
	UINT16	dataSetNameLength;
	UINT16	terminalNameLength;
	char *tptr, *pptr = scratchBuffer, *newName = tmpPath;

	if (!connection ||
		((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		return NWSMTS_INVALID_CONNECTION_HANDL;
	}

	/*  Check to protect against NULL strings first */
	if ((dataSetName == NULL) || (terminalName == NULL))
		return NWSMTS_INVALID_PARAMETER;

	if (nameSpaceType != NFSNameSpace)
		return NWSMTS_INVALID_NAME_SPACE_TYPE;

	strcpy(pptr, ConvertEngineToTsa(dataSetName,NULL)) ;
	tptr = ConvertEngineToTsa(terminalName,NULL) ;

	dataSetNameLength = strlen(pptr);
	terminalNameLength = strlen(tptr);

	newName[0] = 0;

	/*  Case: dataSetName is not empty  */
	if (dataSetNameLength != 0) 
	{
		strcpy(newName, pptr);
		if ( strcmp( pptr,TSAGetMessage(ROOT_OF_FS) ) ) {
			if ( pptr[dataSetNameLength - 1] != '/' )
				strcat(newName, "/");
		}
	}
	
	/*  Case: terminalName is not empty */
	if (terminalNameLength != 0)
	{
		strcat(newName, terminalName);
		if (terminalNameIsParent)
			strcat(newName, "/");
	}

	*newDataSetName = newName ;

	return 0L;
}
 
/*******************************************************************
Function:           AuthenticateUser
********************************************************************/
struct passwd *AuthenticateUser(STRING targetUserName, STRING passwdPtr)
{
	static struct passwd *pwent ;
	UINT16 pwlen ;
#ifdef SVR4
	struct spwd	*shadowEntry = NULL ;
	char		*encryptedPassword ;
#endif

#ifdef DEBUG
	logerror(PRIORITYWARN,"In Authenticate User\n");
	FLUSHLOG ;
#endif

	if ((pwent = getpwnam(ConvertEngineToTsa(targetUserName,NULL))) == NULL) {
		return(NULL);
	}

	pwlen = *(UINT16 *)passwdPtr ;
	passwdPtr += sizeof(UINT16);
#ifdef SVR4
	shadowEntry = getspnam(targetUserName) ;
#endif

	if ( pwlen > 0 ) {
#ifdef SVR4
		// special magic: if password entered but we have a null password, refuse
		if ( shadowEntry != NULL && *(shadowEntry->sp_pwdp) == 0 )
		    return(NULL) ;
#endif

		passwdPtr = ConvertEngineToTsa(passwdPtr, NULL);
		if ( pwent->pw_passwd == NULL ) {
			return(NULL);
		}
#ifdef SVR4
		if ( (strcmp(pwent->pw_passwd, "x") == 0) && (shadowEntry != NULL) ) {
		    /* shadow password in use */
		    encryptedPassword = crypt(passwdPtr, shadowEntry->sp_pwdp) ;
		    if ( strcmp(encryptedPassword, shadowEntry->sp_pwdp) != 0 )
			return(NULL) ;
		    }
		else
#endif

		if (strcmp(pwent->pw_passwd,
				crypt(passwdPtr, pwent->pw_passwd))){
			return(NULL);
		}
	} else {
	    // NULL password entered
#ifdef SVR4
	    if ( (strcmp(pwent->pw_passwd, "x") == 0) && (shadowEntry != NULL) ) {
		/* shadow password in use */
		if ( *(shadowEntry->sp_pwdp) != 0 )
		    return(NULL) ;
		}
	    else
#endif
	    if ( pwent->pw_passwd != NULL ) {
		return(NULL);
	    }
	}

	return(pwent);
}
 
/*********************************************************************

Function:		ConnectToTargetService

*********************************************************************/
CCODE ConnectToTargetService(UINT32 *connection,
STRING targetServiceName,
STRING targetUserName,
STRING passwdPtr)
{
	CCODE ccode = 0 ;
	struct passwd *pwentry ;

	if ((targetServiceName == NULL) || (targetUserName == NULL))
		return NWSMTS_INVALID_PARAMETER;

	if ( ConnectionHandle.valid != VALID_CONNECTION &&
		
		stricmp(ConvertEngineToTsa(targetServiceName,NULL), 
								TargetInfo.targetName) == 0) {
		/* Login to the target service with a user name and password */
		if ( (pwentry = AuthenticateUser(targetUserName,
					passwdPtr)) != NULL ) {
			*connection = (UINT32)&ConnectionHandle ;
			ConnectionHandle.targetInfo = &TargetInfo ;
			ConnectionHandle.loginUserID = pwentry->pw_uid ;
			ConnectionHandle.loginGroupID = pwentry->pw_gid ;
			setgid(pwentry->pw_gid);
			setuid(pwentry->pw_uid);
 
			if ((ccode = CreateTempFile(
			   &ConnectionHandle.ErrorLogFileHandle, "ERROR")) != 0){
				return(ccode);
			}
      
			if ((ccode = CreateTempFile(
			    &ConnectionHandle.SkippedDataSetsFileHandle,"SKIP")) != 0){
				return(ccode);
			}

			ConnectionHandle.valid = VALID_CONNECTION ;
			ccode = BuildResourceList(*connection) ;
			if ( ccode == NWSMTS_OUT_OF_MEMORY ) {
				return(ccode);
			}
			return(ccode);
		}
	}

	return NWSMTS_LOGIN_DENIED;
}

#define ConvertErrorNum(e) TSA_BEGIN_ERROR_CODES + \
				(NWSMTS_BEGIN_ERROR_CODES - e)
#define OutOfErrorNumberRange(e) ( e > TSA_END_ERROR_CODES || \
					e < TSA_BEGIN_ERROR_CODES )

/**********************************************************************

Function:		ConvertError

**********************************************************************/
CCODE ConvertError(UINT32 connectionHandle,
UINT32 error,
STRING message)
{
	CCODE ccode = 0 ;
	STRING errMessage = NULL ;
	UINT32 errIndex;
	TARGET_INFO *targetInfo = ((CONNECTION *)connectionHandle)->targetInfo ;

	if (!connectionHandle ||
		((CONNECTION *)connectionHandle)->valid != VALID_CONNECTION) {
		return NWSMTS_INVALID_CONNECTION_HANDL;
	}

	if (message == NULL)
		return NWSMTS_INVALID_PARAMETER;

	/*  Clear message as a default */
	strcpy(message, "");

	errIndex = ConvertErrorNum(error) ;
	if ( OutOfErrorNumberRange(errIndex) ) {
		ccode = NWSMTS_INVALID_MESSAGE_NUMBER ;
		errIndex = ConvertErrorNum(ccode) ;
	}
	errMessage = TSAGetMessage(errIndex);

	sprintf(message,"(%s %s %d) %s", targetInfo->targetType, 
			targetInfo->targetVersion, errIndex, errMessage);

	return (ccode);
}

/*********************************************************************

Function:		GetNameSpaceTypeInfo

*********************************************************************/
CCODE GetNameSpaceTypeInfo(UINT32 connection,
UINT32 nameSpaceType,
NWBOOLEAN *reverseOrder,
STRING_BUFFER **firstSeparator,
STRING_BUFFER **secondSeparator)
{
	if (!connection ||
		((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		return NWSMTS_INVALID_CONNECTION_HANDL;
	}
	switch (nameSpaceType)
	{
		case NFSNameSpace:
		 	if ( AllocateStringBuffer(firstSeparator, 
						FIRST_SEPARATOR_SIZE + 1) == NULL ) {
				return(NWSMTS_OUT_OF_MEMORY);
			}
			strcpy((*firstSeparator)->string, ":");

		 	if ( AllocateStringBuffer(secondSeparator, 
						SECOND_SEPARATOR_SIZE + 1) == NULL ) {
				FreeMemory(*firstSeparator);
				return(NWSMTS_OUT_OF_MEMORY);
			}
			strcpy((*secondSeparator)->string, "/");

			*reverseOrder = FALSE;

			return 0L;

		default:
			return NWSMTS_INVALID_NAME_SPACE_TYPE;
	}
}

/********************************************************************

Function:		GetTargetResourceInfo

**********************************************************************/
CCODE GetTargetResourceInfo(UINT32 connection,
STRING resourceName,
UINT16 *blockSize,
UINT32 *totalBlocks,
UINT32 *freeBlocks,
NWBOOLEAN *resourceIsRemovable,
UINT32 *purgeableBlocks,
UINT32 *notYetPurgeableBlocks,
UINT32 *migratedSectors,
UINT32 *preCompressedSectors,
UINT32 *compressedSectors)
{
	CCODE		ccode;
	RESOURCE_INFO_STRUCTURE resInfo ;
	char *resPtr = resourceName ;
	NWBOOLEAN fullPath = FALSE ;

	if (!connection ||
		((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		return NWSMTS_INVALID_CONNECTION_HANDL;
	}
	if (resourceName == NULL)
		return NWSMTS_INVALID_PARAMETER;

	resPtr = ConvertEngineToTsa(resourceName, NULL) ; 

	if (!strcmp(resPtr, TSAGetMessage(WORKSTATION_RESOURCE)))
		return NWSMTS_INVALID_PARAMETER;
	
	if (strncmp(resPtr,TSAGetMessage(ROOT_OF_FS),
			strlen(TSAGetMessage(ROOT_OF_FS))) == 0 ) {
		resPtr = strchr(resPtr, ':') ; 
		*resPtr = '/' ;
		fullPath = TRUE ;
	}

	memset((char *)&resInfo, '\0', sizeof(RESOURCE_INFO_STRUCTURE));
	/*
      Look for this resource and see that is valid before making the 
      check
	*/
	ccode = GetResourceInfo(resPtr, &resInfo);

	if ( fullPath == TRUE ) {
		*resPtr = ':' ;
	}
	if (ccode)
		return NWSMTS_RESOURCE_NAME_NOT_FOUND ;

	if (!ccode)
	{
		*blockSize = resInfo.blockSize ;
		*totalBlocks = resInfo.totalBlocks ;
		*freeBlocks = resInfo.freeBlocks ;
		*resourceIsRemovable = resInfo.resourceIsRemovable ;
		*purgeableBlocks = resInfo.purgeableBlocks ;
		*notYetPurgeableBlocks = resInfo.notYetPurgeableBlocks ;
		*migratedSectors = resInfo.migratedSectors ;
		*preCompressedSectors = resInfo.preCompressedSectors ;
		*compressedSectors = resInfo.compressedSectors ;
	}
	return ccode;
}

/*********************************************************************

Function:		GetTargetServiceType

**********************************************************************/
CCODE GetTargetServiceType(UINT32 connection,
STRING targetServiceName,
STRING targetServiceType,
STRING targetServiceVersion)
{
	TARGET_INFO *targetInfo = &TargetInfo ;

	ref(connection);
	if ((targetServiceName == NULL) || 
		 (targetServiceType == NULL) || 
		 (targetServiceVersion == NULL))
		return NWSMTS_INVALID_PARAMETER;

	if (stricmp(targetInfo->targetName, 
			ConvertEngineToTsa(targetServiceName, NULL)) == 0) {
		strcpy(targetServiceType, targetInfo->targetType);
		strcpy(targetServiceVersion, targetInfo->targetVersion);

		return 0;
	}

	return NWSMTS_INVALID_PARAMETER;
}

/********************************************************************
Function:           ScanSupportedNameSpaces

Design Notes:	Currently we are supporting only NFS namespace. SO the 
      resourceName is not properly used here. But if we support other
      name spaces, the supported name spaces will be attribute of a 
      resource and this function need to be changed.
*********************************************************************/
CCODE ScanSupportedNameSpaces(UINT32 connection,
UINT32 *sequence,
STRING resourceName,
UINT32 *nameSpaceType,
STRING nameSpaceName)
{
	RESOURCE_LIST	*resPtr;
	char *namePtr = resourceName ;

	if (!connection ||
		((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		return NWSMTS_INVALID_CONNECTION_HANDL;
	}
	if ((resourceName == NULL) || (nameSpaceName == NULL))
		return NWSMTS_INVALID_PARAMETER;

	if (strncmp(namePtr,TSAGetMessage(ROOT_OF_FS),
			strlen(TSAGetMessage(ROOT_OF_FS))) == 0 ) {
		namePtr = strchr(namePtr, ':') ; 
		*namePtr = '/' ;
	}

	if ( IsInResourceList(namePtr, 
			&resPtr, ((CONNECTION *)connection)->targetInfo) == -1 ) {
		strcpy(nameSpaceName, TSAGetMessage(TSA_NONE_STRING));
		return NWSMTS_RESOURCE_NAME_NOT_FOUND ;
	}

	if (*sequence == FINAL_SEQUENCE) {
   		return NWSMTS_NO_MORE_NAMES;
	}

	*nameSpaceType = NFSNameSpace ;
	strcpy(nameSpaceName, TSAGetMessage(NFS_NAME_SPACE_STRING));
	*sequence = FINAL_SEQUENCE ;
	return(0);
}


/*********************************************************************

Function:		ScanTargetServiceName

*********************************************************************/
CCODE ScanTargetServiceName(UINT32 connection,
UINT32 *sequenceNumber,
STRING scanPattern,
STRING targetServiceName)
{
	TARGET_INFO *targetInfo = &TargetInfo ;

	ref(connection);
	if ((scanPattern == NULL) || (targetServiceName == NULL))
		return NWSMTS_INVALID_PARAMETER;

	if (*sequenceNumber == 0)
	{
		/* See if the scan pattern matches the target name */
		if (strcmp(targetInfo->targetName, 
				ConvertEngineToTsa(scanPattern, NULL)) == 0) {
			(*sequenceNumber)++;
			strcpy(targetServiceName, targetInfo->targetName);
			return 0;
		}
	}

	return NWSMTS_NO_MORE_DATA;
}


/*********************************************************************

Function:		ScanTargetServiceResource

*********************************************************************/
CCODE ScanTargetServiceResource(UINT32 connection,
UINT32 *sequence,
STRING resourceName)
{
	CCODE ccode;
	RESOURCE_LIST	*resourcePtr;

	if (!connection ||
		((CONNECTION *)connection)->valid != VALID_CONNECTION) {
		return NWSMTS_INVALID_CONNECTION_HANDL;
	}
	if (resourceName == NULL)
		return NWSMTS_INVALID_PARAMETER;

	if ( *sequence == 0 ) {
		ccode = BuildResourceList(connection) ;
		if ( ccode == NWSMTS_OUT_OF_MEMORY ) {
			return(ccode);
		}
		strcpy(resourceName, TSAGetMessage(WORKSTATION_RESOURCE));
		if ( ccode ) {
			*sequence = FINAL_SEQUENCE ;
		}
		else {
			resourcePtr = 
					((CONNECTION *)connection)->targetInfo->resource ;
			if ( resourcePtr->next == NULL ) {
				*sequence = FINAL_SEQUENCE ;
			}
			else {
				*sequence = (UINT32)(resourcePtr->next) ;
			}
		}
		return(0);
	}
	else if ( *sequence == FINAL_SEQUENCE ) {
		*sequence = 0 ;
		return NWSMTS_RESOURCE_NAME_NOT_FOUND;
	}

	resourcePtr = (RESOURCE_LIST *)(*sequence) ;

	strcpy(resourceName, TSAGetMessage(ROOT_OF_FS));	
	strcat(resourceName, resourcePtr->resourceName+1);	
	if ( *(resourcePtr->resourceName+1) ) {
		/* Non root file system. tag a slash at the end */
		strcat(resourceName,TSAGetMessage(UNIX_PATH_SEPARATOR));
	}
	if ( resourcePtr->next == NULL ) {
		*sequence = FINAL_SEQUENCE ;
	}
	else {
		*sequence = (UINT32)(resourcePtr->next) ;
	}

	return(0);
}

/*********************************************************************

Function:		AllocateStringBuffer

*********************************************************************/
void *AllocateStringBuffer(STRING_BUFFER **buffer, UINT16 length)
{
	if (*buffer != NULL) {
	 	if ((*buffer)->size >= length) {
			return((void *)*buffer);
		}
		FreeMemory(*buffer);
	}
	*buffer = (STRING_BUFFER *)AllocateMemory(length + sizeof(UINT16));
	if ( *buffer == NULL ) {
		return(NULL);
	}

	(*buffer)->size = length ;
	return((void *)*buffer);
}


/*********************************************************************

Function:		AllocateUINT16Buffer

Design Notes: buffer size to Allocate should be 
			(length * sizeof(UINT16)) + sizeof(UINT16)
*********************************************************************/
void *AllocateUINT16Buffer(UINT16_BUFFER **buffer, UINT16 length)
{
	if (*buffer != NULL)
	{
	 	if ((*buffer)->size >= length) {
			return((void *)*buffer);
		}
		FreeMemory(*buffer);
	}
	*buffer = (UINT16_BUFFER *)AllocateMemory((length * sizeof(UINT16)) + 
					sizeof(UINT16));
	if ( *buffer == NULL ) {
		return(NULL);
	}
	(*buffer)->size = length;
	return((void *)*buffer);
}

/*******************************************************************
Function: CloseAndDeleteTempFile

Purpose:  Closes and deletes the file pointed to by tempFileHandle

Parameters: tempFileHandle    input

Returns:  ccode - 0 if successful
          non-zero if failure

********************************************************************/

CCODE CloseAndDeleteTempFile(TEMP_FILE_HANDLE *tempFileHandle)
{
	if (tempFileHandle->valid == VALID_TEMPFILE) {
		TrashValidFlag(tempFileHandle->valid);
		close(tempFileHandle->fileHandle);
		if (tempFileHandle->filePath) {
			unlink (tempFileHandle->filePath);
			free(tempFileHandle->filePath);
		}
	}
	else {   
		return(NWSMTS_INVALID_HANDLE) ;
	}
	return(0);
}

/********************************************************************
Function: CreateTempFile

Purpose:  Create the file named in tempFileHandle

Parameters: tempFileHandle    input/output

Returns:  ccode - 0 if successful
          non-zero if failure
          tempFileHandle - initializes the structure

********************************************************************/
CCODE CreateTempFile(TEMP_FILE_HANDLE *tempFileHandle, char *pfx)
{
	int fh;
	char *tempFilename ;

	if ((tempFilename = tempnam("/var/tmp", pfx)) == NULL) {
		return(NWSMTS_OUT_OF_MEMORY);
	}
 
	if ((fh = open (tempFilename, O_RDWR | O_CREAT | O_TRUNC)) == -1) {  
		free(tempFilename);
		if (errno == EACCES) {
			return(NWSMTS_ACCESS_DENIED) ;
		}
		else {
			return(NWSMTS_CREATE_ERROR) ;
		}
	}  
 
	tempFileHandle->valid = VALID_TEMPFILE;
	tempFileHandle->position = 0;
	tempFileHandle->size = 0;
	tempFileHandle->fileHandle = fh;

	tempFileHandle->filePath = tempFilename ;
 
	return (0);
}

/**********************************************************************
Function: ReadTempFile

Purpose:  Read a portion of the file described in tempFileHandle

Parameters: tempFileHandle    input
			adjustBuffer      input
			buffer            output
			bytesToRead       input
			bytesRead         output

Returns:    ccode - 0 if successful
			non-zero if failure
			buffer    - read buffer
			bytesRead - actual number of bytes read

********************************************************************/
CCODE ReadTempFile(TEMP_FILE_HANDLE  *tempFileHandle,
                   NWBOOLEAN          adjustBuffer,
                   BUFFERPTR         *buffer,
                   UINT32            *bytesToRead,
                   UINT32            *bytesRead)
{
	CCODE ccode = 0;
	INT32 _bytesRead;
 
	lseek(tempFileHandle->fileHandle, tempFileHandle->position, SEEK_SET);
 
	if ( !*bytesToRead ) {
		return(ccode);
	}
	if ((_bytesRead = read(tempFileHandle->fileHandle, *buffer, 
						*bytesToRead)) > *bytesToRead) {
		if (_bytesRead > 0) {
			tempFileHandle->position += _bytesRead;
		}
		ccode = NWSMTS_READ_ERROR;
		goto Return;
	}
      
	tempFileHandle->position += _bytesRead;

	if (adjustBuffer) {  
		*buffer += _bytesRead;
		*bytesToRead -= _bytesRead;
		*bytesRead += _bytesRead;
	}
	else {
		*bytesRead = _bytesRead;
	}
 
Return:
	return (ccode);
}

/*********************************************************************
Function: WriteTempFile

Purpose:  Write to the file described by tempFileHandle

Parameters: tempFileHandle      input
			buffer            input
			bytesToWrite         input
			bytesWritten         output
			
Returns:    ccode - 0 if successful
			non-zero if failure
			bytesWritten - the number of bytes actually written to
			the file

********************************************************************/
CCODE WriteTempFile(TEMP_FILE_HANDLE *tempFileHandle,
                            BUFFERPTR buffer,
                            UINT32 bytesToWrite,
                            UINT32 *bytesWritten)
{
	CCODE ccode = 0;
	INT32 _bytesWritten;
 
	lseek(tempFileHandle->fileHandle, tempFileHandle->position, SEEK_SET);
 
	if ((_bytesWritten = write(tempFileHandle->fileHandle,
				buffer, bytesToWrite)) >= bytesToWrite) {
		if (bytesWritten)
			*bytesWritten = _bytesWritten;
 
		tempFileHandle->size += _bytesWritten;
		tempFileHandle->position += _bytesWritten;
	}
	else {
		if (_bytesWritten > 0)
			tempFileHandle->position += _bytesWritten;
		ccode = NWSMTS_WRITE_ERROR;
	}
      
	return (ccode);
}
 
/*********************************************************************
Function: ReturnDataSetName

Purpose:  Extract a data set name from a data set handle

Parameters: dsHandle       input

Returns:    returnName - The data set name being returned

*********************************************************************/
STRING ReturnDataSetName(void *dsHandle)
{
	DATA_SET_HANDLE *dataSetHandle = (DATA_SET_HANDLE *)dsHandle;
	STRING returnName = TSAGetMessage(UNKNOWN_DATA_SET_NAME);

	if (dataSetHandle->back.valid == VALID_BACKUP_DATA_SET) {
		if (dataSetHandle->back.dataSetSequence != NULL &&
		    dataSetHandle->back.dataSetSequence->fullPath != NULL &&
		    dataSetHandle->back.dataSetSequence->fullPath->string 
								!= NULL) {  
			returnName = 
			  dataSetHandle->back.dataSetSequence->fullPath->string;
		}  
	}   
	else {   
		if (dataSetHandle->rest.dataSetName != NULL &&
			dataSetHandle->rest.dataSetName->string != NULL) {  
			returnName = dataSetHandle->rest.dataSetName->string;
		}  
	}
	return (returnName);
}

void  LogSkippedDataSets(DATA_SET_SEQUENCE *dataSetSequence, 
				CCODE ccode)
{
	NWSM_DATA_SET_NAME_LIST *dataSetNames = NULL, *dataSetPtr;
	UINT16 saveBufferSize;

	if (!dataSetSequence->scanControl->createSkippedDataSetsFile) {
		goto Return;
	}
	if (!dataSetSequence->scanControl->returnChildTerminalNodeNameOnly) {
		dataSetPtr = dataSetSequence->dataSetNames;
	}
	else {
		BuildFileDataSetNames(
				dataSetSequence->directoryStack->dirEntry.name,
				dataSetSequence, &dataSetNames, FALSE);
		dataSetPtr = dataSetNames;
	}
	saveBufferSize = dataSetPtr->bufferSize;
	dataSetPtr->bufferSize = dataSetPtr->dataSetNameSize + 
					sizeof(dataSetPtr->bufferSize);
	*(UINT32 *)(dataSetPtr->keyInformation + 
			dataSetPtr->keyInformationSize + sizeof(UINT32)) = ccode;
	WriteTempFile(&dataSetSequence->connection->SkippedDataSetsFileHandle,
			(BUFFERPTR)dataSetPtr, (UINT32)dataSetPtr->bufferSize, 
			(UINT32 *)NULL);
	dataSetPtr->bufferSize = saveBufferSize;
Return:
	if (dataSetNames)
		free((char *)dataSetNames);
}

#define MAX_FOLD_WIDTH	76
#define MIN_FOLD_WIDTH	56

void LogError(CONNECTION *connection, UINT16 messageNumber, ...)
{
	BUFFERPTR cp, cpx, lineSep1, lineSep2;
	struct tm *timeOfDay;
	UINT32 lineSep1Len, lineSep2Len;
	va_list argList;
	size_t len;
	time_t  localTime;
	BUFFER buffer[4000];

	lineSep1Len = strlen(lineSep1 = TSAGetMessage(CR_LF_INDENT));
	lineSep2Len = strlen(lineSep2 = TSAGetMessage(CR_LF_CR_LF));

	/*      Print date and time */
	time(&localTime);
	timeOfDay = localtime(&localTime);
	strcpy(buffer, asctime(timeOfDay));
	strcpy(buffer + strlen(buffer) - 1, lineSep1);
	WriteTempFile(&connection->ErrorLogFileHandle, buffer, 
						strlen(buffer), NULL);

#ifdef DEBUG
	logerror(PRIORITYWARN,buffer);
	FLUSHLOG ;
#endif
	/*      Print error message */
	va_start(argList, messageNumber);
	vsprintf(buffer, TSAGetMessage(messageNumber), argList);
	va_end(argList);
	cp = buffer;
	len = strlen(cp) ;
#ifdef DEBUG
	logerror(PRIORITYWARN,buffer);
	FLUSHLOG ;
#endif

	/*      Fold line at MAX_FOLD_WIDTH columns */
	while (len > MAX_FOLD_WIDTH)
	{
		cpx = cp + MAX_FOLD_WIDTH - 1; 
		if (!isspace(*cpx)) {
			while(cpx >= cp && !isspace(*cpx)) {
				cpx-- ;
			}
			if ( cpx < cp ) {
				cpx = cp + MAX_FOLD_WIDTH - 1; 
			}
		}
		WriteTempFile(&connection->ErrorLogFileHandle, 
						cp, cpx - cp + 1, NULL);
		WriteTempFile(&connection->ErrorLogFileHandle, 
						lineSep1, lineSep1Len, NULL);
		cp += MAX_FOLD_WIDTH ;
		len -= MAX_FOLD_WIDTH ;
	}

	if (len)
		WriteTempFile(&connection->ErrorLogFileHandle, cp, len, NULL);

	WriteTempFile(&connection->ErrorLogFileHandle, lineSep2, 
							lineSep2Len, NULL);
#ifdef DEBUG
	logerror(PRIORITYWARN,lineSep2);
	FLUSHLOG ;
#endif
}
