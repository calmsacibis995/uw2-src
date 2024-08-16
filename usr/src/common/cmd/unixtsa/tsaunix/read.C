#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/read.C	1.4"

/*

$Abstract: 
This file contains the ReadDataSet function

$EndLog$ 

*/

#include <tsad.h>
#include <ctype.h>
#include <smdr.h>
#include <smsdrerr.h>
#include <error.h>
#include <smspcode.h>
#include "tsapi.h"
#include <smsutapi.h>
#include "tsaglobl.h"
#include "respond.h"
#include "drapi.h"
#include "filesys.h"

/********************************************************************
Function:  ReadDataSet

Purpose:   Reads the dataset specified in dsHandle into the buffer

Parameters: connectionHandle     input
			dsHandle             input
			bytesToRead          input
			bytesRead            output
			buffer               output

Returns:    ccode - 0 if successful
			non-zero if failed
			bytesRead - count of bytes actually read 
			buffer    - read buffer with portion of dataset read   

********************************************************************/
CCODE ReadDataSet(UINT32 connectionHandle,
UINT32 dsHandle,
UINT32 bytesToRead,
UINT32 *bytesRead,
BUFFERPTR buffer)
{
	CCODE ccode         = 0;
	UINT32 _bytesToRead = 0;
	UINT32 _bytesRead   = 0;
	BACKUP_DATA_SET_HANDLE *dataSetHandle = 
					(BACKUP_DATA_SET_HANDLE *)dsHandle;

	*bytesRead = 0;

	if (dsHandle == 0) {
		ccode = NWSMTS_INVALID_DATA_SET_HANDLE;
		goto Return;
	}

	if (((CONNECTION *)connectionHandle)->valid != VALID_CONNECTION) {
		ccode = NWSMTS_INVALID_CONNECTION_HANDL;
		goto Return;
	}

	if (dataSetHandle->valid != VALID_BACKUP_DATA_SET) {
		ccode = NWSMTS_INVALID_DATA_SET_HANDLE;
		goto Return;
	}

	/* The errorLogFile and the skippedDataSetFile are special case files. 
	The data in these files are not in SMS data format.  The errorLogFIle 
	is an ASCII file containing a list of all the errors and the 
	skippedDataSetFile is a binary file containing the dataSetNames of all 
	items that were skipped from a scan or backup.  The data in this file is
	returned as is.
	*/

	if ((int)(dataSetHandle->dataSetSequence) == ERROR_LOG_SEQUENCE ||
	   (int)(dataSetHandle->dataSetSequence) == SKIPPED_DATA_SETS_SEQUENCE){
#ifdef DEBUG
 		logerror(PRIORITYWARN,"Read DataSet: %d\n",
			(int)(dataSetHandle->dataSetSequence));
		FLUSHLOG ;
#endif

		_bytesToRead = _min(bytesToRead, 
				dataSetHandle->tempFileHandle->size -
				dataSetHandle->tempFileHandle->position);
		if (_bytesToRead) {
			if ((ccode = ReadTempFile(dataSetHandle->tempFileHandle,
			    FALSE, &buffer, &_bytesToRead,
			    &_bytesRead)) == 0) {
				bytesToRead -= _bytesRead;
				*bytesRead += _bytesRead;
				buffer += _bytesRead;
			}
		}
		else {
			*bytesRead += 0;
		}
		goto Return;
	}

#ifdef DEBUG
 	logerror(PRIORITYWARN,"Read DataSet: type : %d, name : %s\n",
		dataSetHandle->dataSetSequence->dataSetType,
		dataSetHandle->dataSetSequence->fullPath->string);
	FLUSHLOG ;
#endif

	/* Data for a section is still in the stateData buffer copy it into 
	buffer. If buffer is full goto return */

	switch (dataSetHandle->bufferState) {

	case STATE_DATA_SYMLINK_STATE :
		if (dataSetHandle->stateDataSize == 0 ||
						!dataSetHandle->stateData) {
			LogError(dataSetHandle->connection, BUFFER_STATE_PROBLEM);
 			logerror(PRIORITYWARN,TSAGetMessage(BUFFER_STATE_PROBLEM));
			FLUSHLOG ;
			ccode = NWSMTS_INTERNAL_ERROR;
			goto Return ;
		}

		break ;

	default :
		if (dataSetHandle->stateDataSize == 0) {
			break ;
		}

		if (!dataSetHandle->stateData) {
			LogError(dataSetHandle->connection, BUFFER_STATE_PROBLEM);
 			logerror(PRIORITYWARN,TSAGetMessage(BUFFER_STATE_PROBLEM));
			FLUSHLOG ;
			ccode = NWSMTS_INTERNAL_ERROR;
			goto Return ;
		}

		*bytesRead = _min(dataSetHandle->stateDataSize, bytesToRead);
		memcpy(buffer,
		    dataSetHandle->stateData + dataSetHandle->stateDataOffset,
		    *bytesRead);

		bytesToRead -= *bytesRead;
		buffer += *bytesRead;

		if (dataSetHandle->stateDataSize > *bytesRead) {
			dataSetHandle->stateDataOffset += *bytesRead;
			dataSetHandle->stateDataSize -= *bytesRead;
			goto Return;
		}
		else {
			dataSetHandle->stateDataOffset = 0;
			dataSetHandle->stateDataSize = 0;
		}
		break ;
	}

	while (1) {
		switch (dataSetHandle->state) {
		case STATE_HEADER:
			// Create the SMS data format header for a data set
#ifdef DEBUG
		        logerror(PRIORITYWARN,"BackupHeader: name : %s\n", 
					ReturnDataSetName(dataSetHandle));
			FLUSHLOG ;
#endif
			ccode = CreateHeaderOrTrailer(dataSetHandle, 
					&bytesToRead, bytesRead, &buffer, TRUE);
			dataSetHandle->state = STATE_FULL_PATHS;

			// if error or if the buffer is full then goto return
			if (ccode || !bytesToRead)
				goto Return;
			continue ;

		 case STATE_FULL_PATHS:
			// Put the full paths for all supported/request name 
			// spaces into SMS data format

#ifdef DEBUG
		        logerror(PRIORITYWARN,"BackupFullPaths: name : %s\n", 
					ReturnDataSetName(dataSetHandle));
			FLUSHLOG ;
#endif

			ccode = BackupFullPaths(dataSetHandle,
					&bytesToRead, bytesRead, &buffer);

			dataSetHandle->state = STATE_CHARACTERISTICS;

			// if error or if the buffer is full then goto return
			if (ccode || !bytesToRead)
				goto Return;
			continue ;

		case STATE_CHARACTERISTICS:
			// Put all the characteristics such as fileCreationDate,
			// file attributes into SMS data format
#ifdef DEBUG
		        logerror(PRIORITYWARN,"BackupChars: name : %s\n", 
					ReturnDataSetName(dataSetHandle));
			FLUSHLOG ;
#endif
			ccode = BackupCharacteristics(dataSetHandle, 
					&bytesToRead, bytesRead, &buffer);

			dataSetHandle->state = STATE_NFS_CHARACTERISTICS;

			// if error or if the buffer is full then goto return
			if (ccode || !bytesToRead)
				goto Return;
			continue ;

		case STATE_NFS_CHARACTERISTICS:
#ifdef DEBUG
		        logerror(PRIORITYWARN,"BackupNFSChars: name : %s\n", 
					ReturnDataSetName(dataSetHandle));
			FLUSHLOG ;
#endif
			ccode = BackupNFSCharacteristics(dataSetHandle, 
					&bytesToRead, bytesRead, &buffer);

			if (
			dataSetHandle->dataSetSequence->dataSetType != TYPE_FILE || 
           	S_ISSPECIAL(dataSetHandle->dataSetSequence->scanInformation->attributes) 
			|| (dataSetHandle->mode & NWSM_NO_DATA_STREAMS) 
			|| (dataSetHandle->dataSetSequence->scanControl->scanType &
				 NWSM_EXCLUDE_DATA_STREAMS) )
				 {
					dataSetHandle->state = STATE_TRAILER;
			}
			else {
				dataSetHandle->state = STATE_DATA_STREAMS;
			}
			// if error or if the buffer is full then goto return
			if (ccode || !bytesToRead)
				goto Return;
			continue ;

		case STATE_DATA_STREAMS:
#ifdef DEBUG
		        logerror(PRIORITYWARN,"BackupDataStreams: name : %s\n", 
					ReturnDataSetName(dataSetHandle));
			FLUSHLOG ;
#endif
			ccode = BackupDataStreams(dataSetHandle,
				    	&bytesToRead, bytesRead, &buffer);

			/* if error or if the buffer is full then goto 
			   return. Note that the state is not changed before this 
			   if statement unlike other cases.
			*/
               
			if (ccode || !bytesToRead)
				goto Return;

			dataSetHandle->state = STATE_TRAILER;
			continue ;

		case STATE_TRAILER:
#ifdef DEBUG
		        logerror(PRIORITYWARN,"BackupTrailer: name : %s\n", 
					ReturnDataSetName(dataSetHandle));
			FLUSHLOG ;
#endif
			ccode = CreateHeaderOrTrailer(dataSetHandle,
				&bytesToRead, bytesRead, &buffer, FALSE);
			dataSetHandle->state = STATE_DONE;

			// if error or if the buffer is full then goto return
			if (ccode || !bytesToRead)
				goto Return;
			continue ;

		case STATE_DONE:
			goto Return;

		} 
		// end of switch
	} 
	// end of while


Return :
	return (ccode);
}
