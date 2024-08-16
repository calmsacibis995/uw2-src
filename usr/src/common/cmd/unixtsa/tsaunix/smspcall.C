#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/smspcall.C	1.7"

#include <tsad.h>
#include <error.h>
#include <smdr.h>
#include <smsdrerr.h>
#include "tsaglobl.h"
#include "respond.h"
#include "smsutapi.h"
#include "drapi.h"
#include "smspcall.h"

#ifdef DEBUG
void dump_buf(char *buf, int len, char *dumpBuf);
#endif

#ifdef DEBUG
unsigned int	*size_ptr ;
#endif

extern char dumpBuffer[512] ;


/* This is a kludge. This is the size of the scan control structure 
   with no boundary and alignment considerations
*/
#define SCAN_CONTROL_SIZE	63
#define SCAN_CONTROL_OTHERINFO_SIZE_INDEX	61

void MapDataSetNameList(NWSM_DATA_SET_NAME_LIST *namelist, char *buffer) ;
void MapScanControl(NWSM_SCAN_CONTROL *scanControl, char *buffer) ;
void MapSelectionList(NWSM_SELECTION_LIST *selectionList, char *buffer) ;
STATIC CCODE ConvertDataSetNameList(UINT32 connection,
NWSM_DATA_SET_NAME_LIST  *nameBuffer,
NWSM_DATA_SET_NAME_LIST  **newNameList) ;

/**********************************************************************
Function:		DNScanTargetServiceName

Purpose:		Separate out parameters and call into the DRAPI

  Parameters:	smdrThread	input/output	Thread data structure

  Design Notes:	

**********************************************************************/
CCODE DNScanTargetServiceName(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char		*inbuffer	= smdrThread->input.buffer;
	UINT32	*sequence	= (UINT32*)inbuffer;
	char		*pattern		= inbuffer + sizeof(*sequence) ;
	char		name[MAX_FULL_PATH_LENGTH];
	UINT16	len, tmp;
	char *s ;
	size_t l ;

	*sequence = SwapUINT32(*sequence);
	ccode = ScanTargetServiceName(smdrThread->connectionID, sequence, pattern, name);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;
	*sequence = SwapUINT32(*sequence);
	LoadReplyBuffer(smdrThread, sequence, sizeof(*sequence));

	s = ConvertTsaToEngine((char *)name,&l);
	len = (UINT16)l + 1 ;
	tmp = SwapUINT16(len);
	LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
	LoadReplyBuffer(smdrThread, s, len);

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	return ccode;
}

/**********************************************************************

Function:		DNConnectToTargetService

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

***********************************************************************/
CCODE DNConnectToTargetService(SMDR_THREAD *smdrThread) 
{
	ERROR_VARS;
	CCODE ccode;
	char  *inbuffer = smdrThread->input.buffer, *targetservice, *user;
	char		*authentication;
	UINT16	targetsize, usersize, targetpos, userpos, authpos;
	char passwordBuffer[MAX_PASSWORD_LENGTH+1] ;

	targetsize = SwapUINT16p(inbuffer);
	targetpos  = sizeof(targetsize);

	usersize   = SwapUINT16p(inbuffer + targetpos + targetsize);
	userpos	  = targetpos + targetsize + sizeof(usersize);

	authpos	= userpos + usersize;

	targetservice	= inbuffer + targetpos ;
	user		= inbuffer + userpos ;
	authentication	= inbuffer + authpos ;

	/*  Null terminate the authentication string */
	inbuffer[smdrThread->input.logicalSize] = 0;

#ifdef DEBUG
	logerror(PRIORITYWARN,"Calling Decrypt\n") ;
	FLUSHLOG ;
#endif

	DecryptPassword((UINT8 *)smdrThread->key,(UINT8 *)authentication,
									 (UINT8 *)passwordBuffer);

	ccode = ConnectToTargetService(&smdrThread->connectionID, 
				targetservice, user, passwordBuffer);
	ERRORNZ(ccode, 2);

	*(smdrThread->ccode) = 0;

	return 0;

	ERRORS
	{
		case 1:
			ccode = NWSMDR_DECRYPTION_FAILURE;
			break;

		case 2:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}

	return ccode;
}



/***********************************************************************

Function:		DNAuthenticateTS

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

***********************************************************************/
CCODE DNAuthenticateTS(SMDR_THREAD *smdrThread) 
{
	*(smdrThread->ccode) = SwapUINT32(NWSMTS_UNSUPPORTED_FUNCTION) ;

	return 0;
}


/**********************************************************************

Function:		DNReleaseTargetService

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNReleaseTargetService(SMDR_THREAD *smdrThread) 
{
	ERROR_VARS;
	CCODE ccode;

	ccode = ReleaseTargetService(&smdrThread->connectionID);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}

	return ccode;
}


/***********************************************************************

Function:		DNBuildResourceList

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNBuildResourceList(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;

	ccode = BuildResourceList(smdrThread->connectionID);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	return ccode;
}

/***********************************************************************

Function:		DNBeginRestoreSession

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNBeginRestoreSession(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;

	ccode = BeginRestoreSession(smdrThread->connectionID);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	return ccode;
}


/**********************************************************************

Function:		DNIsDataSetExcluded

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure
Design Notes:	

*********************************************************************/
CCODE DNIsDataSetExcluded(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char		*inbuffer = smdrThread->input.buffer;
	UINT16	*isParent		= (UINT16*)inbuffer;
	NWSM_DATA_SET_NAME_LIST *dataSetName ;

	dataSetName = (NWSM_DATA_SET_NAME_LIST *)AllocateMemory(
			SwapUINT16p(inbuffer + sizeof(*isParent) + sizeof(UINT16)) 
				+ sizeof(UINT16) ); 
	if ( dataSetName == NULL ) {
		ccode = NWSMDR_OUT_OF_MEMORY ;
		*(smdrThread->ccode) = SwapUINT32(ccode);
		return(ccode);
	}
	MapDataSetNameList(dataSetName, inbuffer+sizeof(*isParent)) ;
	
	ccode = IsDataSetExcluded(smdrThread->connectionID, 
			SwapUINT16(*isParent), dataSetName);

	FreeMemory((char *)dataSetName);

	ERRORNZ(ccode, 1);
	
	*(smdrThread->ccode) = 0;

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	return ccode;
}


/**********************************************************************

Function:		DNRenameDataSet

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNRenameDataSet(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char *inbuffer = smdrThread->input.buffer;
	UINT32 *sequence = (UINT32*)inbuffer; 
	char *nameSpaceType	= inbuffer + sizeof(*sequence) ;
	char *newDataSetName	= inbuffer + 2 * sizeof(UINT32) ;


	ccode = RenameDataSet(smdrThread->connectionID, SwapUINT32(*sequence), 
			SwapUINT32p(nameSpaceType), newDataSetName);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	return ccode;
}

/**********************************************************************

Function:		DNFixDataSetName

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNFixDataSetName(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char *inbuffer = smdrThread->input.buffer;
	char *nameSpaceType	= inbuffer ;
	char *isParent = inbuffer + sizeof(UINT32) ;
	char *wildAllowed = inbuffer + sizeof(UINT16) +  sizeof(UINT16);
	char *dataSetName = inbuffer + sizeof(UINT32) + 3 * sizeof(UINT16);
	STRING_BUFFER *ptr = NULL ;
	UINT16 newlen, tmplen ;
	char *s ;
	size_t l ;

	ccode = FixDataSetName(smdrThread->connectionID, dataSetName,
			SwapUINT32p(nameSpaceType), SwapUINT16p(isParent),
			SwapUINT16p(wildAllowed), &ptr);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	s = ConvertTsaToEngine(ptr->string, &l);
	newlen = (UINT16)l + 1 ;
	tmplen = SwapUINT16(newlen);
	LoadReplyBuffer(smdrThread, &tmplen, sizeof(tmplen));
	LoadReplyBuffer(smdrThread, s, newlen);
	NWSMFreeString(&ptr);

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	NWSMFreeString(&ptr);
	return ccode;
}


/*********************************************************************

Function:		DNDeleteDataSet

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNDeleteDataSet(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	UINT32 *sequence = (UINT32*)(smdrThread->input.buffer);


	ccode = DeleteDataSet(smdrThread->connectionID, SwapUINT32(*sequence));
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}

	return 0;
}

/*********************************************************************

Function:		DNOpenDataSerForRestore

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNOpenDataSetForRestore(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char *inbuffer = smdrThread->input.buffer;
	UINT32
		*mode			= (UINT32 *)inbuffer,
		*parent			= (UINT32 *)(inbuffer + sizeof(*mode)),
		handle;
	NWSM_DATA_SET_NAME_LIST *dataSetName ;

	/* Here it is OK. Because we landed on a integer boundary */
	dataSetName	= (NWSM_DATA_SET_NAME_LIST*)(inbuffer + 
						sizeof(*mode) +	sizeof(*parent));

	dataSetName->bufferSize = SwapUINT16(dataSetName->bufferSize);
	dataSetName->dataSetNameSize = 
			SwapUINT16(dataSetName->dataSetNameSize);
	if (dataSetName->bufferSize == 0)
		dataSetName = NULL;
	else
		dataSetName->bufferSize = dataSetName->dataSetNameSize + 
						sizeof(dataSetName->bufferSize);

	ccode = OpenDataSetForRestore(smdrThread->connectionID, 
			*parent, dataSetName, 
			SwapUINT32(*mode), &handle);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	LoadReplyBuffer(smdrThread, &handle, sizeof(handle));

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}

	return ccode;
}


/*********************************************************************

Function:		DNOpenDataSetForBackup

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNOpenDataSetForBackup(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char *inbuffer = smdrThread->input.buffer;
	UINT32
		*sequence		= (UINT32*)inbuffer,
		*mode			= (UINT32*)(inbuffer + sizeof(*sequence)),
		handle;

	ccode = OpenDataSetForBackup(smdrThread->connectionID, 
			SwapUINT32(*sequence), 
			SwapUINT32(*mode), &handle);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	LoadReplyBuffer(smdrThread, &handle, sizeof(handle));

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}

	return ccode;
}


/*********************************************************************

Function:		DNReadDataSet

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNReadDataSet(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char *inbuffer = smdrThread->input.buffer;
	UINT32
		*handle			= (UINT32*)inbuffer,
		*maxSizeToRead	= (UINT32*)(inbuffer + sizeof(*handle)),
		readSize;

	ccode = ReadDataSet(smdrThread->connectionID, *handle, 
		SwapUINT32(*maxSizeToRead), 
		&readSize, smdrThread->data.buffer);
	ERRORNZ(ccode, 1);


	smdrThread->data.logicalSize = readSize;

	*(smdrThread->ccode) = 0;

	readSize = SwapUINT32(readSize);
	LoadReplyBuffer(smdrThread, &readSize, sizeof(readSize));
	
	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}

	return ccode;
}


/*********************************************************************

Function:		DNWriteDataSet

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNWriteDataSet(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char *inbuffer = smdrThread->input.buffer;
	UINT32
		*handle			= (UINT32*)inbuffer,
		*writesize		= (UINT32*)(inbuffer + sizeof(*handle));

	ccode = WriteDataSet(smdrThread->connectionID, *handle, 
			SwapUINT32(*writesize), smdrThread->data.buffer);

	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}

	return ccode;
}


/*********************************************************************

Function:		DNCloseDataSet

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNCloseDataSet(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char *inbuffer = smdrThread->input.buffer;
	UINT32
		*handle			= (UINT32*)inbuffer;

	ccode = CloseDataSet(smdrThread->connectionID, handle);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}

	return ccode;
}


/*********************************************************************

Function:		DNScanSupportedNameSpaces

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNScanSupportedNameSpaces(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char *inbuffer		= smdrThread->input.buffer ;
	UINT32 *sequence	= (UINT32*)inbuffer, nameSpaceType;
	char
		*resourceName	= inbuffer + sizeof(*sequence),
		nameSpaceName[MAX_FULL_PATH_LENGTH];
	UINT16
		len, tmp;
	char *s ;
	size_t l ;

	*sequence = SwapUINT32(*sequence); 
	ccode = ScanSupportedNameSpaces(smdrThread->connectionID, 
			sequence, resourceName,
			&nameSpaceType, nameSpaceName);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	*sequence = SwapUINT32(*sequence); 
	LoadReplyBuffer(smdrThread, sequence, sizeof(*sequence));

	nameSpaceType =SwapUINT32(nameSpaceType); 
	LoadReplyBuffer(smdrThread, &nameSpaceType, sizeof(nameSpaceType));

	s = ConvertTsaToEngine((char *)nameSpaceName, &l);
	len = (UINT16)l + 1 ;
	tmp = SwapUINT16(len) ;
	LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
	LoadReplyBuffer(smdrThread, s, len);

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	return ccode;
}


/*********************************************************************

Function:		DNSetRestoreOptions

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNSetRestoreOptions(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char *inbuffer = smdrThread->input.buffer;
	char
		*checkCRC		= inbuffer ,
		*ignore			= inbuffer + sizeof(UINT16) ;

	NWSM_SELECTION_LIST
		*selectionList;

	/*  Move SelectionList to temporary location */
	selectionList = (NWSM_SELECTION_LIST *)(inbuffer + 
									2 * sizeof(UINT16));

	selectionList->bufferSize = SwapUINT16(selectionList->bufferSize) ;
	selectionList->selectionListSize = 
			SwapUINT16(selectionList->selectionListSize) ;
	if (!selectionList->bufferSize)
		selectionList = NULL;

	else
		selectionList->bufferSize = selectionList->selectionListSize+
				sizeof(selectionList->bufferSize);

	ccode = SetRestoreOptions(smdrThread->connectionID, 
			SwapUINT16p(checkCRC), SwapUINT16p(ignore), 
			selectionList);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	return ccode;
}


/*********************************************************************

Function:		DNGetTargetResourceInfo

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNGetTargetResourceInfo(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char *resource	= smdrThread->input.buffer;
	struct
	{
		UINT32 
			total, 
			free, 
			purgable,
			unpurgable,
			migrated,
			precompressed,
			compressed;

		NWBOOLEAN
			removable;

		UINT16
			blocksize;
	} reply;

	ccode = GetTargetResourceInfo(smdrThread->connectionID, resource, 
			&reply.blocksize, 
			&reply.total, 
			&reply.free, 
			&reply.removable, 
			&reply.purgable, 
			&reply.unpurgable, 
			&reply.migrated, 
			&reply.precompressed, 
			&reply.compressed);

	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;
	reply.total = SwapUINT32(reply.total); 
	reply.free = SwapUINT32(reply.free) ; 
	reply.purgable = SwapUINT32(reply.purgable) ; 
	reply.unpurgable = SwapUINT32(reply.unpurgable) ; 
	reply.migrated = SwapUINT32(reply.migrated) ; 
	reply.precompressed = SwapUINT32(reply.precompressed) ; 
	reply.compressed = SwapUINT32(reply.compressed);
	reply.removable = SwapUINT16(reply.removable); 
	reply.blocksize = SwapUINT16(reply.blocksize); 

	LoadReplyBuffer(smdrThread, &reply, sizeof(reply));

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	return ccode;
}


/*********************************************************************

Function:		DNParseDataSetName

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNParseDataSetName(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char *inbuffer = smdrThread->input.buffer;
	UINT32
		*nameSpaceType	= (UINT32*)inbuffer;
	char
		*dataSetName	= inbuffer + sizeof(*nameSpaceType) ;
	UINT16 count, tmp, *tmp1, idx ;
	NWSM_DATA_SET_NAME name ;

	memset(&name,'\0',sizeof(NWSM_DATA_SET_NAME));
	ccode = ParseDataSetName(smdrThread->connectionID, 
				SwapUINT32(*nameSpaceType), dataSetName, &name);
#ifdef DEBUG
	logerror(PRIORITYWARN," ParseDataSetName ccode = %d\n", ccode) ;
	FLUSHLOG ;
#endif
	ERRORNZ(ccode, 1);

	/*
	 Note: This portion was written with the assumption that the 
	 the name separators may be represented differently in different 
	 code sets. But then it was confirmed that the name separators are
	 same in different code sets. ..........................

	if ( convertCodeset ) {
		char *ptr ;
		int idx = 0 ;
		int size1, size2, len ;

		ConvertTsaToEngine(TSAGetMessage(UNIX_FS_SEPARATOR),&size1);
		ConvertTsaToEngine(TSAGetMessage(UNIX_PATH_SEPARATOR),&size2);

		count = 0 ;
		ptr = name.name ;
		name.name[name.separatorPositions[count]] = '\0' ;
		ConvertTsaToEngine(ptr, &len);
		idx += len ;
		name.separatorPositions[count] = idx  ;
		idx += size1 ;
		for (count = 1 ; count < name.count; count++) {
			ptr = name.name + name.namePositions[count] ;
			name.namePositions[count] = idx  ;
			if (name.separatorPositions[count]) {
				name.name[name.separatorPositions[count]] = '\0' ;
				ConvertTsaToEngine(ptr, &len);
				idx += len ;
				name.separatorPositions[count] = idx  ;
				idx += size2 ;
			}
		}
	}
	*/

	*(smdrThread->ccode) = 0;

	if ((count = name.count) != 0)
	{
#ifdef DEBUG
		logerror(PRIORITYWARN,"count = %d\n", count) ;
		dump_buf((char *)name.namePositions,
				count * sizeof(UINT16), dumpBuffer);
		logerror(PRIORITYWARN,"namePositions %s\n", dumpBuffer);
		dump_buf((char *)name.separatorPositions,
				count * sizeof(UINT16), dumpBuffer);
		logerror(PRIORITYWARN,"separatorPositions %s\n", dumpBuffer);
		FLUSHLOG ;
#endif
		tmp = SwapUINT16(count) ;
		LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
		count *= sizeof(UINT16) ;
		tmp1 = name.namePositions ;
		for ( idx=0; idx < count ; idx += sizeof(UINT16), tmp1++ ) {
			*tmp1 = SwapUINT16(*tmp1) ;
			LoadReplyBuffer(smdrThread, tmp1, sizeof(*tmp1));
		}

		LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
		tmp1 = name.separatorPositions ;
		for ( idx=0; idx < count ; idx += sizeof(*tmp1), tmp1++ ) {
			*tmp1 = SwapUINT16(*tmp1) ;
			LoadReplyBuffer(smdrThread, tmp1, sizeof(*tmp1));
		}
	}
	else
	{
		LoadReplyBuffer(smdrThread, &count, sizeof(count));
		LoadReplyBuffer(smdrThread, &count, sizeof(count));
	}

	NWSMFreeName(&name);
	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}

	NWSMFreeName(&name);
	return ccode;
}

/*********************************************************************

Function:		DNCatDataSetName

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNCatDataSetName(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char 
		*inbuffer		= smdrThread->input.buffer;
	UINT32
		*nameSpaceType	= (UINT32*)inbuffer ;
	char
		*isParent	= inbuffer + sizeof(*nameSpaceType),
		*namesize = inbuffer + sizeof(*nameSpaceType) + sizeof(UINT16);
	UINT16
		newlen, tmp;
	char *dataSetName = inbuffer + sizeof(*nameSpaceType) + 
							2 * sizeof(UINT16);
	char *terminalName ;
	char *ptr 			= NULL;
	size_t l ;

	terminalName = dataSetName + SwapUINT16p(namesize) ;

	ccode = CatDataSetName(smdrThread->connectionID, 
			SwapUINT32(*nameSpaceType), dataSetName, terminalName,
			SwapUINT16p(isParent), &ptr);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	ptr = ConvertTsaToEngine(ptr, &l);
	newlen = (UINT16)l + 1 ;
	tmp = SwapUINT16(newlen);

	LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
	LoadReplyBuffer(smdrThread, ptr, newlen);

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	return ccode;
}

/*********************************************************************

Function:		DNGetNameSpaceTypeInfo

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNGetNameSpaceTypeInfo(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char 
		*inbuffer		= smdrThread->input.buffer;
	UINT32
		*nameSpaceType	= (UINT32*)inbuffer ;
	STRING_BUFFER
		*sep1			= NULL,
		*sep2			= NULL;
	NWBOOLEAN
		order;
	UINT16 len, tmp;
	char *s ;
	size_t l ;

	ccode = GetNameSpaceTypeInfo(smdrThread->connectionID, 
			SwapUINT32(*nameSpaceType), &order, &sep1, &sep2);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	order = SwapUINT16(order);
	LoadReplyBuffer(smdrThread, &order, sizeof(order));
	
	s = ConvertTsaToEngine((char *)sep1->string, &l);
	len = (UINT16)l + 1 ;
	tmp = SwapUINT16(len);
	LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
	LoadReplyBuffer(smdrThread, s, len);

	s = ConvertTsaToEngine((char *)sep2->string, &l);
	len = (UINT16)l + 1 ;
	tmp = SwapUINT16(len);
	LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
	LoadReplyBuffer(smdrThread, s, len);

	FreeMemory(sep1);
	FreeMemory(sep2);

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	if (sep1)
		FreeMemory(sep1);

	if (sep2)
		FreeMemory(sep2);

	return ccode;
}


/*********************************************************************

Function:		DNSeparateDataSetName

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNSeparateDataSetName(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char 
		*inbuffer		= smdrThread->input.buffer;
	UINT32
		*nameSpaceType	= (UINT32*)inbuffer;
	char
		*parent			= inbuffer + sizeof(*nameSpaceType),
		*child	= inbuffer + sizeof(*nameSpaceType) + sizeof(UINT16); 
	UINT16
		len, tmp;
	char
		dummy = 0,
		*dataSetName = inbuffer + sizeof(*nameSpaceType) + 
						2 * sizeof(UINT16);
	STRING_BUFFER
		*ptr1			= NULL,
		*ptr2			= NULL;
	char *s;
	size_t l ;

	ccode = SeparateDataSetName(smdrThread->connectionID, 
			SwapUINT32(*nameSpaceType), dataSetName,
			SwapUINT16p(parent) ? &ptr1 : NULL, 
			SwapUINT16p(child) ? &ptr2 : NULL);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	if (SwapUINT16p(parent))
	{
		if ( ptr1 ) {
			s = ConvertTsaToEngine((char *)ptr1->string, &l);
			len = (UINT16)l + 1 ;
			tmp = SwapUINT16(len);
			LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
			LoadReplyBuffer(smdrThread, s, len);
			FreeMemory(ptr1);
		}
		else {
			LoadReplyBuffer(smdrThread, &dummy, sizeof(dummy));
		}
	}

	if (SwapUINT16p(child))
	{
		if ( ptr2 ) {
			s = ConvertTsaToEngine((char *)ptr2->string, &l);
			len = (UINT16)l + 1 ;
			tmp = SwapUINT16(len);
			LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
			LoadReplyBuffer(smdrThread, s, len);
			FreeMemory(ptr2);
		}
		else {
			LoadReplyBuffer(smdrThread, &dummy, sizeof(dummy));
		}
	}

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode) ;
			ccode = 0;
			break;
	}
	
	if (ptr1)
		FreeMemory(ptr1);

	if (ptr2)
		FreeMemory(ptr2);

	return ccode;
}


/*********************************************************************

Function:		DNScanTargetServiceResource

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNScanTargetServiceResource(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char 
		*inbuffer		= smdrThread->input.buffer,
		resource[MAX_FULL_PATH_LENGTH];
	UINT32	sequence	;
	UINT16
		len, tmp;
	char *s ;
	size_t l ;

	sequence = SwapUINT32p(inbuffer) ; 
	ccode = ScanTargetServiceResource(smdrThread->connectionID, 
			&sequence, resource);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	sequence = SwapUINT32(sequence) ; 
	LoadReplyBuffer(smdrThread, &sequence, sizeof(sequence));

	s = ConvertTsaToEngine((char *)resource, &l);
	len = (UINT16)l + 1 ;
	tmp = SwapUINT16(len) ; 
	LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
	LoadReplyBuffer(smdrThread, s, len);

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	return ccode;
}

void MapDataSetNameList(NWSM_DATA_SET_NAME_LIST *namelist, char *buffer)
{
	UINT16 namelistsize ;

	namelistsize = SwapUINT16p(buffer + sizeof(UINT16)) + sizeof(UINT16) ;

	memcpy((char *)namelist,buffer,namelistsize);

	namelist->bufferSize = namelistsize ;
	namelist->dataSetNameSize = namelistsize - sizeof(UINT16) ;
	buffer += 2 * sizeof(UINT16);
	namelist->nameSpaceCount = buffer[0] ;
	buffer += sizeof(UINT8);
	namelist->keyInformationSize = buffer[0] ;
	buffer += sizeof(UINT8);
	if ( namelist->keyInformationSize ) {
		memcpy(namelist->keyInformation,buffer,
									namelist->keyInformationSize);
	}
}

void MapScanControl(NWSM_SCAN_CONTROL *scanControl, char *buffer)
{
	UINT16 scansize ;
 
	scansize = SwapUINT16p(buffer + sizeof(UINT16)) + sizeof(UINT16) ;

	memcpy((char *)scanControl,buffer,scansize);
	scanControl->scanControlSize = scansize - sizeof(UINT16);
	scanControl->bufferSize = scansize ;
	buffer += 2 * sizeof(UINT16);
	scanControl->scanType = SwapUINT32p(buffer);
	buffer += sizeof(UINT32);
	scanControl->firstAccessDateAndTime = 
				_ConvertDOSTimeToCalendar(SwapUINT32p(buffer));
	buffer += sizeof(UINT32);
	scanControl->lastAccessDateAndTime = 
				_ConvertDOSTimeToCalendar(SwapUINT32p(buffer));
	buffer += sizeof(UINT32);
	scanControl->firstCreateDateAndTime = 
				_ConvertDOSTimeToCalendar(SwapUINT32p(buffer));
	buffer += sizeof(UINT32);
	scanControl->lastCreateDateAndTime = 
				_ConvertDOSTimeToCalendar(SwapUINT32p(buffer));
	buffer += sizeof(UINT32);
	scanControl->firstModifiedDateAndTime = 
				_ConvertDOSTimeToCalendar(SwapUINT32p(buffer));
	buffer += sizeof(UINT32);
	scanControl->lastModifiedDateAndTime = 
				_ConvertDOSTimeToCalendar(SwapUINT32p(buffer));
	buffer += sizeof(UINT32);
	scanControl->firstArchivedDateAndTime = 
				_ConvertDOSTimeToCalendar(SwapUINT32p(buffer));
	buffer += sizeof(UINT32);
	scanControl->lastArchivedDateAndTime = 
				_ConvertDOSTimeToCalendar(SwapUINT32p(buffer));
	buffer += sizeof(UINT32);
	scanControl->returnChildTerminalNodeNameOnly = *buffer++ ;
	scanControl->parentsOnly = *buffer++ ;
	scanControl->childrenOnly = *buffer++ ;
	scanControl->createSkippedDataSetsFile = *buffer++ ;
	scanControl->generateCRC = *buffer++ ;
	scanControl->returnNFSHardLinksInDataSetName = *buffer++ ;
	memcpy(scanControl->reserved,buffer,6);
	buffer += 6;
	scanControl->scanChildNameSpaceType = SwapUINT32p(buffer);
	buffer += sizeof(UINT32);
	scanControl->returnNameSpaceType = SwapUINT32p(buffer);
	buffer += sizeof(UINT32);
	scanControl->callScanFilter = *buffer++ ;
	scanControl->otherInformationSize = SwapUINT16p(buffer);
	buffer += sizeof(UINT16);
	if ( scanControl->otherInformationSize ) {
		memcpy(scanControl->otherInformation,buffer,
			scanControl->otherInformationSize) ;
	}
}

void MapSelectionList(NWSM_SELECTION_LIST *selectionList, char *buffer)
{
	UINT16 selectionsize ;
	selectionsize = SwapUINT16p(buffer + sizeof(UINT16)) + 
						sizeof(UINT16) ;

	memcpy((char *)selectionList,buffer,selectionsize);
	selectionList->selectionListSize = 
			selectionsize - sizeof(UINT16) ;
	selectionList->bufferSize = selectionsize ;

}

int MapScanInformation(NWSM_SCAN_INFORMATION *scanInfo, char *buffer)
{
	char *ptr = buffer ;
	UINT32 tmp ;
	UINT16 size ;
	UINT16 dateP, timeP;

	buffer += sizeof(UINT16); /*skip scanInformationSize till later*/
	tmp = SwapUINT32(scanInfo->attributes);
	memcpy(buffer,(char *)&tmp,sizeof(UINT32));
	buffer += sizeof(UINT32);
	tmp = SwapUINT32(scanInfo->creatorID);
	memcpy(buffer,(char *)&tmp,sizeof(UINT32));
	buffer += sizeof(UINT32);
	tmp = SwapUINT32(scanInfo->creatorNameSpaceNumber);
	memcpy(buffer,(char *)&tmp,sizeof(UINT32));
	buffer += sizeof(UINT32);
	tmp = SwapUINT32(scanInfo->primaryDataStreamSize);
	memcpy(buffer,(char *)&tmp,sizeof(UINT32));
	buffer += sizeof(UINT32);
	tmp = SwapUINT32(scanInfo->totalStreamsDataSize);
	memcpy(buffer,(char *)&tmp,sizeof(UINT32));
	buffer += sizeof(UINT32);
	*buffer++ = scanInfo->modifiedFlag ;
	*buffer++ = scanInfo->deletedFlag ;
	*buffer++ = scanInfo->parentFlag ;
	memset(buffer,'\0',5);
	buffer += 5 ; /* reserved */
	_ConvertTimeToDOS(scanInfo->accessDateAndTime, &dateP, &timeP);
	tmp = (dateP << 16) | timeP ;
	tmp = SwapUINT32(tmp);
	memcpy(buffer,(char *)&tmp,sizeof(UINT32));
	buffer += sizeof(UINT32);
	_ConvertTimeToDOS(scanInfo->createDateAndTime, &dateP, &timeP);
	tmp = (dateP << 16) | timeP ;
	tmp = SwapUINT32(tmp);
	memcpy(buffer,(char *)&tmp,sizeof(UINT32));
	buffer += sizeof(UINT32);
	_ConvertTimeToDOS(scanInfo->modifiedDateAndTime, &dateP, &timeP);
	tmp = (dateP << 16) | timeP ;
	tmp = SwapUINT32(tmp);
	memcpy(buffer,(char *)&tmp,sizeof(UINT32));
	buffer += sizeof(UINT32);
	/* Since there is no archive date and time in UNIX, just advance the
       buffer pointer */
	buffer += sizeof(UINT32);
	memset(buffer,'\0',sizeof(UINT16));  /* otherInformationSize  */
	buffer += sizeof(UINT16) ; 
	
	size = buffer - ptr ;
	size = SwapUINT16(size);
	memcpy(ptr,(char *)&size,sizeof(UINT16));

	return(buffer-ptr);
}

/*********************************************************************

Function:		DNScanDataSetBegin

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNScanDataSetBegin(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char *inbuffer = smdrThread->input.buffer;
	UINT16	wantScanInfo ;
	NWSM_DATA_SET_NAME_LIST
		*resource,
		*myname	= NULL,
		*convertedName = NULL ;
	NWSM_SCAN_CONTROL *scanControl;
	NWSM_SELECTION_LIST *selectionList;
	NWSM_SCAN_INFORMATION	*myinfo = NULL, scanInfo ;
	UINT32	sequence;
	UINT16 dummy = 0, resourcesize, scansize, selectionsize, tmpSize;

	resourcesize = SwapUINT16p(inbuffer + 2 * sizeof(UINT16)) + 
						sizeof(UINT16) ; 
	resource = (NWSM_DATA_SET_NAME_LIST *)AllocateMemory(resourcesize);
	if ( resource == NULL ) {
		ccode = NWSMDR_OUT_OF_MEMORY ;
		*(smdrThread->ccode) = SwapUINT32(ccode);
		return(ccode);
	}
	MapDataSetNameList(resource, inbuffer+sizeof(UINT16)) ;

#ifdef DEBUG
	logerror(PRIORITYWARN,"Scan Begin: resourceSize %d\n", resourcesize);
	dump_buf((char *)resource,10,dumpBuffer);
	logerror(PRIORITYWARN,"Scan Begin: dump %s\n", dumpBuffer);
#endif

	scansize = SwapUINT16p(inbuffer +sizeof(UINT16)+resourcesize +
					sizeof(UINT16)) + sizeof(UINT16) ; 
#ifdef DEBUG
	logerror(PRIORITYWARN,"scan size = %d\n", scansize) ;
	FLUSHLOG ;
#endif
	if (scansize == 2 || scansize < SCAN_CONTROL_SIZE ) {
		scanControl = NULL ;
	}
	else {
		UINT16	otherInfoSize, rscansize ;
		/* Recalculate the scan size */
		otherInfoSize = SwapUINT16p(inbuffer + sizeof(UINT16) +
					resourcesize + SCAN_CONTROL_OTHERINFO_SIZE_INDEX);
        rscansize = sizeof(NWSM_SCAN_CONTROL) + otherInfoSize ;

		scanControl = (NWSM_SCAN_CONTROL *)AllocateMemory(rscansize);
		if ( scanControl == NULL ) {
			FreeMemory((char *)resource);
			ccode = NWSMDR_OUT_OF_MEMORY ;
			*(smdrThread->ccode) = SwapUINT32(ccode);
			return(ccode);
		}
		MapScanControl(scanControl,
						inbuffer+sizeof(UINT16)+resourcesize);
	}

	selectionsize = SwapUINT16p(inbuffer + sizeof(UINT16) +
			resourcesize + scansize + sizeof(UINT16)) + sizeof(UINT16); 

#ifdef DEBUG
	logerror(PRIORITYWARN,"selection size = %d\n", selectionsize) ;
	FLUSHLOG ;
#endif

	if (selectionsize == 2) {
		selectionList = NULL;
	}
	else {
		selectionList = (NWSM_SELECTION_LIST *)
						AllocateMemory(selectionsize);
		if ( selectionList == NULL ) {
			FreeMemory((char *)resource);
			if ( scanControl ) {
				FreeMemory((char *)scanControl);
			}
			ccode = NWSMDR_OUT_OF_MEMORY ;
			*(smdrThread->ccode) = SwapUINT32(ccode);
			return(ccode);
		}
		MapSelectionList(selectionList,
					inbuffer+2+resourcesize+scansize);
	}

	wantScanInfo = SwapUINT16p(inbuffer);
	ccode = ScanDataSetBegin(smdrThread->connectionID, 
			resource, scanControl, selectionList, 
			&sequence, 
			(wantScanInfo) ? &myinfo : NULL, &myname);

	FreeMemory((char *)resource);
	if ( scanControl )
		FreeMemory((char *)scanControl);
	if ( selectionList )
		FreeMemory((char *)selectionList);

	ERRORNZ(ccode, 2);
	if (myname && convertCodeset ) {
		ccode = ConvertDataSetNameList(smdrThread->connectionID,
						myname, &convertedName);
		if ( ccode != 0 ) {
			if ( convertedName != NULL ) {
				FreeMemory(convertedName);
			}
			*(smdrThread->ccode) = SwapUINT32(ccode);
			return(ccode);
		}
		myname = convertedName ;
	}

	*(smdrThread->ccode) = 0;

	sequence = SwapUINT32(sequence);
	LoadReplyBuffer(smdrThread, &sequence, sizeof(UINT32));

	if (wantScanInfo)
	{
		if (myinfo) {
			tmpSize = MapScanInformation(myinfo,(char *)&scanInfo) ;
			LoadReplyBuffer(smdrThread, 
				(char *)&scanInfo, tmpSize);
		}
		else
			LoadReplyBuffer(smdrThread, &dummy, sizeof(UINT16));
	}

	if (myname) {
		tmpSize = myname->dataSetNameSize ;
		LoadReplyBuffer(smdrThread, &myname->dataSetNameSize, 
				SwapUINT16(tmpSize));
	}
	else
		LoadReplyBuffer(smdrThread, &dummy, sizeof(UINT16));

	if ( convertedName != NULL ) {
		FreeMemory(convertedName);
	}
	return 0;

	ERRORS
	{
		case 1:
			ccode = NWSMDR_OUT_OF_MEMORY;
			*(smdrThread->ccode) = SwapUINT32(ccode);
			break;

		case 2:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	} 
	
	if ( convertedName != NULL ) {
		FreeMemory(convertedName);
	}
	return ccode;
}


/********************************************************************

Function:		DNScanNextDataSet

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNScanNextDataSet(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char *inbuffer = smdrThread->input.buffer;
	UINT32 sequence ;
	UINT16 dummy = 0, wantScanInfo, tmpSize ;
	NWSM_SCAN_INFORMATION *myinfo = NULL, scanInfo ;
	NWSM_DATA_SET_NAME_LIST *myname = NULL,
		*convertedName = NULL ;

	sequence = SwapUINT32p(inbuffer);

	wantScanInfo = SwapUINT16p(inbuffer+sizeof(sequence));
#ifdef DEBUG
	logerror(PRIORITYWARN,"wantScanInfo = %d\n", wantScanInfo) ;
	FLUSHLOG ;
#endif

	ccode = ScanNextDataSet(smdrThread->connectionID, &sequence, 
			(wantScanInfo)? &myinfo: NULL, &myname);
	ERRORNZ(ccode, 1);

	if (myname && convertCodeset ) {
		ccode = ConvertDataSetNameList(smdrThread->connectionID,
						myname, &convertedName);
		if ( ccode != 0 ) {
			if ( convertedName != NULL ) {
				FreeMemory(convertedName);
			}
			*(smdrThread->ccode) = SwapUINT32(ccode);
			return(ccode);
		}
		myname = convertedName ;
	}

	*(smdrThread->ccode) = 0;

	sequence = SwapUINT32(sequence);
	LoadReplyBuffer(smdrThread, &sequence, sizeof(sequence));

	if (wantScanInfo)
	{
		if (myinfo) {
			tmpSize = MapScanInformation(myinfo,(char *)&scanInfo) ;
			LoadReplyBuffer(smdrThread, 
				(char *)&scanInfo, tmpSize);
		}
		else
			LoadReplyBuffer(smdrThread, &dummy, sizeof(dummy));
	}

	if (myname) {
		tmpSize = myname->dataSetNameSize ;
		LoadReplyBuffer(smdrThread, &myname->dataSetNameSize, 
				SwapUINT16(tmpSize));
	}
	else
		LoadReplyBuffer(smdrThread, &dummy, sizeof(dummy));

	if ( convertedName != NULL ) {
		FreeMemory(convertedName);
	}
	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	} 
	
	if ( convertedName != NULL ) {
		FreeMemory(convertedName);
	}
	
	return ccode;
}


/*********************************************************************

Function:		DNScanDataSetEnd

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNScanDataSetEnd(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char *inbuffer	= smdrThread->input.buffer;
	UINT32 sequence ;

	sequence = SwapUINT32p(inbuffer);
	ccode = ScanDataSetEnd(smdrThread->connectionID, &sequence, NULL, NULL);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	return ccode;
}


/*********************************************************************

Function:		DNReturnToParent

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNReturnToParent(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char 
		*inbuffer		= smdrThread->input.buffer;
	UINT32
		*sequence		= (UINT32*)inbuffer ;

	*sequence = SwapUINT32(*sequence);
	ccode = ReturnToParent(smdrThread->connectionID, sequence);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	*sequence = SwapUINT32(*sequence);
	LoadReplyBuffer(smdrThread, sequence, sizeof(*sequence));

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = ccode;
			ccode = 0;
			break;
	}
	
	return ccode;
}


/*********************************************************************

Function:		DNConvertError

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNConvertError(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char 
		*inbuffer		= smdrThread->input.buffer;
	UINT32 *error	= (UINT32*)inbuffer;
	UINT16	len, tmp;
	char
		str[MAX_FULL_PATH_LENGTH],
		*result ;
	size_t l ;

	ccode = ConvertError(smdrThread->connectionID, SwapUINT32(*error), str);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	result = ConvertTsaToEngine(str,&l);
	len = (UINT16)l + 1 ;
	tmp = SwapUINT16(len);
	LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
	LoadReplyBuffer(smdrThread, result, len);
	
	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	return ccode;
}


/*********************************************************************

Function:		DNGetOpenModeOptionString

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNGetOpenModeOptionString(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char 
		*inbuffer		= smdrThread->input.buffer;
	UINT16
		*option			= (UINT16*)inbuffer,
		len, tmp;
	char
		str[MAX_FULL_PATH_LENGTH];
	char *s ;
	size_t l ;

	ccode = GetOpenModeOptionString(smdrThread->connectionID, 
			(UINT8)(SwapUINT16(*option)), str);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	s = ConvertTsaToEngine(str,&l);
	len = (UINT16)l + 1 ;
	tmp = SwapUINT16(len);
	LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
	LoadReplyBuffer(smdrThread, s, len);
	
	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	return ccode;
}


/*********************************************************************

Function:		DNGetTargetScanTypeString

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

***************************************************************/
CCODE DNGetTargetScanTypeString(SMDR_THREAD *smdrThread)
{
	CCODE ccode;
	char 
		*inbuffer		= smdrThread->input.buffer;
	UINT16
		*stype			= (UINT16*)inbuffer ,
		len, tmp;
	char
		str[MAX_FULL_PATH_LENGTH];
	UINT32
		required,
		disallowed;

	ccode = GetTargetScanTypeString(smdrThread->connectionID, 
			(UINT8)SwapUINT16(*stype), str, 
			&required, &disallowed);

	*(smdrThread->ccode) = SwapUINT32(ccode);

	if (!ccode)
	{
		char *s ;
		size_t l ;

		required = SwapUINT32(required);
		LoadReplyBuffer(smdrThread, &required, sizeof(required));
		disallowed = SwapUINT32(disallowed);
		LoadReplyBuffer(smdrThread, &disallowed, sizeof(disallowed));
		s = ConvertTsaToEngine(str,&l);
		len = (UINT16)l + 1 ;
		tmp = SwapUINT16(len);
		LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
		LoadReplyBuffer(smdrThread, s, len);
	}

	return ccode;
}


/*********************************************************************

Function:		DNGetTargetSelectionTypeStr

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

***************************************************************/
CCODE DNGetTargetSelectionTypeStr(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char 
		*inbuffer		= smdrThread->input.buffer;
	UINT16
		*stype			= (UINT16*)inbuffer ,
		len, tmp;
	char
		str1[MAX_FULL_PATH_LENGTH],
		str2[MAX_FULL_PATH_LENGTH];
	char *s ;
	size_t l ;

	ccode = GetTargetSelectionTypeStr(smdrThread->connectionID, 
			(UINT8)(SwapUINT16(*stype)), str1, str2);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	s = ConvertTsaToEngine(str1,&l);
	len = (UINT16)l + 1 ;
	tmp = SwapUINT16(len);
	LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
	LoadReplyBuffer(smdrThread, s, len);
	
	s = ConvertTsaToEngine(str2,&l);
	len = (UINT16)l + 1 ;
	tmp = SwapUINT16(len);
	LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
	LoadReplyBuffer(smdrThread, s, len);

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}
	
	return ccode;
}


/**************************************************************

Function:		DNGetTargetServiceType

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNGetTargetServiceType(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	char
		*targetService	= smdrThread->input.buffer,
		type[MAX_FULL_PATH_LENGTH],
		version[MAX_FULL_PATH_LENGTH];
	UINT16 len, tmp;
	char *s ;
	size_t l ;

	ccode = GetTargetServiceType(smdrThread->connectionID, 
				targetService, type, version);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;
	
	s = ConvertTsaToEngine((char *)type, &l);
	len = (UINT16)l + 1 ;
	tmp = SwapUINT16(len);
	LoadReplyBuffer(smdrThread, &tmp,  sizeof(tmp));
	LoadReplyBuffer(smdrThread, s, len);

	s = ConvertTsaToEngine((char *)version, &l);
	len = (UINT16)l + 1 ;
	tmp = SwapUINT16(len);
	LoadReplyBuffer(smdrThread, &tmp, sizeof(tmp));
	LoadReplyBuffer(smdrThread, s, len);

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode);
			ccode = 0;
			break;
	}

	return ccode;
}

/***************************************************************

Function:		GetUnsupportedOptions

Purpose:		

Parameters:	

Design Notes:	
**********************************************************************/
CCODE DNGetUnsupportedOptions(SMDR_THREAD *smdrThread)
{
	ERROR_VARS;
	CCODE ccode;
	UINT32 boptions, roptions;

	ccode = GetUnsupportedOptions(smdrThread->connectionID, &boptions, &roptions);
	ERRORNZ(ccode, 1);

	*(smdrThread->ccode) = 0;

	boptions = SwapUINT32(boptions);
	LoadReplyBuffer(smdrThread, &boptions, sizeof(boptions));
	roptions = SwapUINT32(roptions);
	LoadReplyBuffer(smdrThread, &roptions, sizeof(roptions));

	return 0;

	ERRORS
	{
		case 1:
			*(smdrThread->ccode) = SwapUINT32(ccode) ;
			ccode = 0;
			break;
	}

	return ccode;
}

STATIC CCODE ConvertDataSetNameList(UINT32 connection,
NWSM_DATA_SET_NAME_LIST  *nameBuffer,
NWSM_DATA_SET_NAME_LIST  **newNameList)
{
	CCODE ccode;
	char *namePtr ;
	NWSM_DATA_SET_NAME	name ;
	NWSM_DATA_SET_NAME_LIST *nameList ;
	UINT32 nameHandle = 0, selectionType = 0, handle = 0 ;
	STRING_BUFFER *separator1 = NULL, *separator2 = NULL;
	NWBOOLEAN reverseOrder;

	memset((char *)&name,'\0',sizeof(NWSM_DATA_SET_NAME));

	ccode = NWSMGetFirstName((BUFFERPTR)nameBuffer, &name, &nameHandle);
	if ( ccode ) {
		NWSMFreeName(&name);
		return(ccode);
	}
	
	namePtr = ConvertTsaToEngine(name.name,NULL);
	ccode = GetNameSpaceTypeInfo(connection, name.nameSpaceType, 
				&reverseOrder,&separator1, &separator2);
	
	if ( ccode ) {
		NWSMFreeName(&name);
		NWSMCloseName(&nameHandle);
		return(ccode);
	}
	
	ccode = NWSMPutFirstName((void HUGE **)&nameList,name.nameSpaceType,
				selectionType, reverseOrder, separator1->string, 
				separator2->string, namePtr, &handle);
	NWSMFreeName(&name);
	if ( ccode ) {
		NWSMCloseName(&handle);
		NWSMCloseName(&nameHandle);
		FreeMemory(separator1);
		FreeMemory(separator2);
		return(ccode);
	}
	
	while((ccode = NWSMGetNextName(&nameHandle, &name)) == 0) {
		namePtr = ConvertTsaToEngine(name.name,NULL);
		ccode = GetNameSpaceTypeInfo(connection, name.nameSpaceType, 
					&reverseOrder,&separator1, &separator2);
	
		if ( ccode ) {
			NWSMFreeName(&name);
			NWSMCloseName(&nameHandle);
			NWSMCloseName(&handle);
			return(ccode);
		}
	
		ccode = NWSMPutNextName((void HUGE **)&nameList,&handle,
					name.nameSpaceType, selectionType, reverseOrder, 
					separator1->string, separator2->string, namePtr);
		NWSMFreeName(&name);
		if ( ccode ) {
			NWSMCloseName(&handle);
			NWSMCloseName(&nameHandle);
			FreeMemory(separator1);
			FreeMemory(separator2);
			return(ccode);
		}
	}
	if ( ccode == NWSMUT_NO_MORE_NAMES) {
		ccode = 0 ;
	}
	NWSMCloseName(&handle);
	FreeMemory(separator1);
	FreeMemory(separator2);
	*newNameList = nameList ;
	return(ccode);
}

/**********************************************************************

Function:		DNGetModuleVersionInfo

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNGetModuleVersionInfo(SMDR_THREAD *smdrThread) 
{
	NWSM_MODULE_VERSION_INFO *infoPtr = &unixtsaModule ;

	*(smdrThread->ccode) = 0;

	LoadReplyBuffer(smdrThread, 
				(char *)infoPtr, sizeof(NWSM_MODULE_VERSION_INFO));
	return 0;
}

/**********************************************************************

Function:		DNGetResponderVersionInfo

Purpose:		Separate out parameters and call into the DRAPI

Parameters:	smdrThread	input/output	Thread data structure

Design Notes:	

**********************************************************************/
CCODE DNGetResponderVersionInfo(SMDR_THREAD *smdrThread) 
{
	return(DNGetModuleVersionInfo(smdrThread)) ; 
}

