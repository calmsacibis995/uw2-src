/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spifile.c	1.18"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spifile.c,v 2.55.2.4 1995/02/13 07:56:00 stevbam Exp $"

/*
 *  Netware Unix Client
 *
 *	MODULE:     spifile.c
 *	ABSTRACE:   File system interface calls pertaining to file objects
 *	
 *	Functions declared in this module:
 *      NWsiReadFile
 *      NWsiWriteFile
 *      NWsiTruncateFile
 *      NWsiSetFileLock
 *      NWsiRemoveFileLock
 *      NWsiLinkFile
 *      NWsiGetFileSize
 */ 

#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <util/cmn_err.h>
#include <net/nuc/nwspiswitch.h>
#include <net/nuc/nwspi.h>
#include <net/nuc/spimacro.h>
#include <net/nuc/nucerror.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/tiuser.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/sistructs.h>
#include <net/nuc/slstruct.h>
#include <net/nuc/nuc_prototypes.h>
#include <util/debug.h>

#include <io/ddi.h>

#define NVLT_ModMask	NVLTM_spil

extern rwlock_t		*nucUpperLock;

/*
 *	SPIL layer state vector
 */
extern int32 spiState;

/*
 * BEGIN_MANUAL_ENTRY(NWsiReadFile.3k)
 * NAME
 *    NWsiReadFile - Read a number of bytes from a file.
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiReadFile ( nwcred_t         *credPtr,
 *                   SPI_HANDLE_T     *volHandle,
 *                   SPI_HANDLE_T     *fileHandle,
 *                   uint32           offset,
 *                   NUC_IOBUF_T      *buffer,
 *                   enum   NUC_DIAG  *diagnostic )
 *
 * INPUT
 *    credPtr              - Credentials of the UNIX user reading the file.
 *    volHandle            - Volume Handle of the volume that the fileHandle
 *                           resides in.
 *    fileHandle           - File handle of the file to be read.
 *    offset               - Starting offset of the read.
 *    buffer->bufferLength - Specifies the length of the buffer.
 *
 * OUTPUT
 *    buffer->buffer       - Buffer containing the read data.
 *    buffer->bufferLength - Number of bytes read.
 *    diagnostic           - Set to one of the following if an error occurs:
 *                               SPI_INACTIVE
 *                               SPI_AUTHENTICATION_FAILURE
 *                               SPI_CLIENT_RESOURCE_SHORTAGE
 *                               SPI_ACCESS_DENIED
 *                               SPI_SERVER_UNAVAILABLE
 *                               SPI_GENERAL_FAILURE
 *                               SPI_USER_MEMORY_FAULT
 *                               SPI_NO_CONNECTIONS_AVAILABLE
 *                               SPI_BAD_CONNECTION
 *                               SPI_SERVER_DOWN
 *                               SPI_LOCK_COLLISION
 *
 * RETURN VALUES
 *    SUCCESS             - Successful completion.
 *    FAILURE             - Unsuccessful completion, diagnostic contains the
 *                          reason.
 *
 * DESCRIPTION
 *    Given the file handle to a previously opened file, will load the
 *    supplied buffer with data starting at offset, with the byte count
 *    specified in the buffer structure.  The Packet Burst protocol is used
 *    if available.
 *
 * SEE ALSO
 *    NWsiOpenNode(3k), NWsiWriteFile(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiReadFile (	nwcred_t			*credPtr,
				SPI_HANDLE_T		*volHandle,
				SPI_HANDLE_T		*fileHandle,
				uint32				offset,
				NUC_IOBUF_T			*buffer,
				enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	ncp_task_t			*task;
	ncp_channel_t		*channel;
	SPI_TASK_T			*gTask;
	NCP_FILEHANDLE_T	*ncpFileHandle;
	pl_t				pl;

	NVLT_ENTER (6);

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);
	NVLT_ASSERT (fileHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (fileHandle->stamp == SPI_FHANDLE_STAMP);

	if ((gTask = fileHandle->spilTaskPtr) == NULL) {
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if (!(gTask->mode & SPI_TASK_AUTHENTICATED)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	/*
	 *	Indicate that this task is in use
	 */
	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		return (NVLT_LEAVE (FAILURE));
	}

	NWslGetTaskProtoHandle (gTask, &task);
	NWslGetHandleSProtoResHandle (fileHandle, &ncpFileHandle);
	channel = task->channelPtr;

	/*
	 *  If this read is greater than 512 bytes and this server supports
	 *  Packet Burst, redirect this request to IPXEngReadFileBurst.
	*/
	if ((channel->burstInfo != NULL) &&
			(buffer->bufferLength > channel->burstInfo->minWindowSize)) {
		ccode = NCPspPacketBurstReadFile_l (channel, (uint8 *)ncpFileHandle,
					offset, buffer);
	} else {
		/*
		 *	Read from the file into the buffer
		 */
		ccode = NCPsp2XReadFile_l (channel, (uint8 *)ncpFileHandle,
									offset, buffer);
	}

	if (ccode != SUCCESS) {
		if ((ccode == SPI_SERVER_UNAVAILABLE) ||
				(ccode == SPI_NO_CONNECTIONS_AVAILABLE) ||
				(ccode == SPI_BAD_CONNECTION) || (ccode == SPI_SERVER_DOWN)) {
			gTask->mode |= SPI_TASK_DRAINING;
		}
		*diagnostic = ccode;
		ccode = FAILURE;
	}

	/*
	 *	Indicate that this task is no longer in use
	 */
	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	NVLT_LEAVE (*diagnostic);
	return (ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiWriteFile.3k)
 * NAME
 *    NWsiWriteFile - Write buffer contents to an open file
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiWriteFile ( nwcred_t        *credPtr,
 *                    SPI_HANDLE_T    *volHandle,
 *                    SPI_HANDLE_T    *fileHandle,
 *                    uint32          offset,
 *                    NUC_IOBUF_T     *buffer,
 *                    enum  NUC_DIAG  *diagnostic )
 *
 * INPUT
 *    credPtr              - Credentials of the UNIX user writing to the file.
 *    volHandle            - Volume Handle of the volume that the fileHandle
 *                           resides in.
 *    fileHandle           - File handle of the file written to.
 *    offset               - Starting offset for the write.
 *    buffer->bufferLength - Specifies the length of the buffer.
 *    buffer->buffer       - Buffer containing the data.
 *
 * OUTPUT
 *    diagnostic           - Set to one of the following if an error occurs:
 *                               SPI_INACTIVE
 *                               SPI_AUTHENTICATION_FAILURE
 *                               SPI_CLIENT_RESOURCE_SHORTAGE
 *                               SPI_ACCESS_DENIED
 *                               SPI_SERVER_UNAVAILABLE
 *                               SPI_GENERAL_FAILURE
 *                               SPI_USER_MEMORY_FAULT
 *                               SPI_NO_CONNECTIONS_AVAILABLE
 *                               SPI_BAD_CONNECTION
 *                               SPI_SERVER_DOWN
 *                               SPI_LOCK_COLLISION
 *
 * RETURN VALUES
 *    SUCCESS             - Successful completion.
 *    FAILURE             - Unsuccessful completion, diagnostic contains the
 *                          reason.
 *
 * DESCRIPTION
 *   Write data defined in the buffer structure at offset in the file
 *   represented by fileHandle.  The Packet Burst protocol is used if
 *   available.
 *
 * SEE ALSO
 *    NWsiOpenNode(3k), NWsiReadFile(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiWriteFile (	nwcred_t			*credPtr,
				SPI_HANDLE_T		*volHandle,
				SPI_HANDLE_T		*fileHandle,
				uint32				offset,
				NUC_IOBUF_T			*buffer,
				enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	ncp_task_t			*task;
	ncp_channel_t		*channel;
	SPI_TASK_T			*gTask;
	NCP_FILEHANDLE_T	*ncpFileHandle;
	pl_t				pl;

	NVLT_ENTER (6);

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);
	NVLT_ASSERT (fileHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (fileHandle->stamp == SPI_FHANDLE_STAMP);

	if (buffer->bufferLength == 0) {
		/*
		 * No need to write anything.
		 */
		return (NVLT_LEAVE (SUCCESS));
	}

	if ((gTask = fileHandle->spilTaskPtr) == NULL) {
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if (!(gTask->mode & SPI_TASK_AUTHENTICATED)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	/*
	 *	Indicate that this task is in use
	 */
	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		return (NVLT_LEAVE (FAILURE));
	}

	NWslGetTaskProtoHandle (gTask, &task);
	NWslGetHandleSProtoResHandle (fileHandle, &ncpFileHandle);
	channel = task->channelPtr;

	/*
	 *  If this read is greater than 512 bytes and this server supports
	 *  Packet Burst, redirect this request to IPXEngWriteFileBurst.
	 */
	if ((channel->burstInfo != NULL) &&
			(buffer->bufferLength > channel->burstInfo->minWindowSize)) {
		ccode = NCPspPacketBurstWriteFile_l (channel, (uint8 *)ncpFileHandle,
									offset, buffer);
	} else {
		/*
		 *  Write to the file from the buffer using NetWare 2.x Write File
		 */
		ccode = NCPsp2XWriteFile_l (channel, (uint8 *)ncpFileHandle,
									offset, buffer);
	}

	if (ccode != SUCCESS) {
		if ((ccode == SPI_SERVER_UNAVAILABLE) ||
				(ccode == SPI_NO_CONNECTIONS_AVAILABLE) ||
				(ccode == SPI_BAD_CONNECTION) || (ccode == SPI_SERVER_DOWN)) {
			gTask->mode |= SPI_TASK_DRAINING;
		}
		*diagnostic = ccode;
		ccode = FAILURE;
	}

	/*
	 *	Indicate that this task is no longer in use
	 */
	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	NVLT_LEAVE (*diagnostic);
	return (ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiTruncateFile.3k)
 * NAME
 *    NWsiTruncateFile - Truncate a file at a specified offset
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiTruncateFile ( nwcred_t         *credPtr,
 *                       SPI_HANDLE_T     *volHandle,
 *                       SPI_HANDLE_T     *fileHandle,
 *                       uint32           offset,
 *                       enum   NUC_DIAG  *diagnostic )
 *
 * INPUT
 *    credPtr              - Credentials of the UNIX user truncating the file.
 *    volHandle            - Volume Handle of the volume that the fileHandle
 *                           resides in.
 *    fileHandle           - File handle of the file being truncated.
 *    offset               - Starting offset of the truncate.
 *
 * OUTPUT
 *    diagnostic           - Set to one of the following if an error occurs:
 *                               SPI_INACTIVE
 *                               SPI_AUTHENTICATION_FAILURE
 *                               SPI_CLIENT_RESOURCE_SHORTAGE
 *                               SPI_ACCESS_DENIED
 *                               SPI_SERVER_UNAVAILABLE
 *                               SPI_NO_CONNECTIONS_AVAILABLE
 *                               SPI_BAD_CONNECTION
 *                               SPI_SERVER_DOWN
 *                               SPI_BAD_ARGS_TO_SERVER
 *
 * RETURN VALUES
 *    SUCCESS             - Successful completion.
 *    FAILURE             - Unsuccessful completion, diagnostic contains the
 *                          reason.
 *
 * DESCRIPTION
 *    Truncates the contents of a file at the specified offset.
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiTruncateFile (	nwcred_t			*credPtr,
					SPI_HANDLE_T		*volHandle,
					SPI_HANDLE_T		*fileHandle,
					uint32				offset,
					enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	ncp_task_t			*task;
	ncp_channel_t		*channel;
	SPI_TASK_T			*gTask;
	NCP_FILEHANDLE_T	*ncpFileHandle;
	pl_t				pl;
	struct {
		NUC_IOBUF_T		buffer;
	} *KAlloc = kmem_alloc(sizeof(*KAlloc), KM_SLEEP);

	NVLT_ENTER (5);

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		ccode = FAILURE;
		goto done;
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);
	NVLT_ASSERT (fileHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (fileHandle->stamp == SPI_FHANDLE_STAMP);

	if ((gTask = fileHandle->spilTaskPtr) == NULL) {
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		ccode = FAILURE;
		goto done;
	}

	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if (!(gTask->mode & SPI_TASK_AUTHENTICATED)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		ccode = FAILURE;
		goto done;
	}

	/*
	 *	Indicate that this task is in use
	 */
	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		ccode = FAILURE;
		goto done;
	}

	NWslGetTaskProtoHandle (gTask, &task);
	NWslGetHandleSProtoResHandle (fileHandle, &ncpFileHandle);
	channel = task->channelPtr;

	/*
	 * Truncate this file.
	 */
	KAlloc->buffer.bufferLength = 0;
	KAlloc->buffer.buffer = (void_t *)NULL;
	KAlloc->buffer.memoryType = IOMEM_KERNEL;
	ccode = NCPsp2XWriteFile_l (channel, (uint8 *)ncpFileHandle,
								offset, &KAlloc->buffer);

	if (ccode != SUCCESS) {
		if ((ccode == SPI_SERVER_UNAVAILABLE) ||
				(ccode == SPI_NO_CONNECTIONS_AVAILABLE) ||
				(ccode == SPI_BAD_CONNECTION) || (ccode == SPI_SERVER_DOWN)) {
			gTask->mode |= SPI_TASK_DRAINING;
		}
		*diagnostic = ccode;
		ccode = FAILURE;
	}

	/*
	 *	Indicate that this task is no longer in use
	 */
	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	NVLT_LEAVE (*diagnostic);
done:
	kmem_free(KAlloc, sizeof(*KAlloc));
	return (ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiSetFileLock.3k)
 * NAME
 *		NWsiSetFileLock	- Perform a lock operation on byte range 
 *						  specified in lockStruct.
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiSetFileLock ( nwcred_t           *credPtr,
 *                      SPI_HANDLE_T       *volHandle,
 *                      SPI_HANDLE_T       *fileHandle,
 *                      NWSI_LOCK_T        *lockStruct,
 *                      enum    NUC_DIAG   *diagnostic )
 *
 * INPUT
 *    credPtr              - Credentials of the UNIX user setting the lock.
 *    volHandle            - Volume Handle of the volume that the fileHandle
 *                           resides in.
 *    fileHandle           - File handle of the file being locked.
 *    lockStruct->offset   - Byte offset into file where lock starts
 *    lockStruct->size     - Number of bytes in lock
 *    lockStruct->mode     - READ_LOCK or WRITE_LOCK
 *    lockStruct->type     - PHYSICAL_LOCK or LOGICAL_LOCK
 *    lockStruct->timeout  - Timeout value in milliseconds
 *
 * OUTPUT
 *    diagnostic           - Set to one of the following if an error occurs:
 *                               SPI_INACTIVE
 *                               SPI_AUTHENTICATION_FAILURE
 *                               SPI_CLIENT_RESOURCE_SHORTAGE
 *                               SPI_SERVER_UNAVAILABLE
 *                               SPI_NO_CONNECTIONS_AVAILABLE
 *                               SPI_BAD_CONNECTION
 *                               SPI_SERVER_DOWN
 *                               SPI_LOCK_COLLISION
 *                               SPI_BAD_HANDLE
 *                               SPI_LOCK_TIMEOUT
 *
 * RETURN VALUES
 *    SUCCESS             - Successful completion.
 *    FAILURE             - Unsuccessful completion, diagnostic contains the
 *                          reason.
 *
 * DESCRIPTION
 *    Lock the byte range specified by the contents of lockStruct
 *    in the file specified by fileHandle.  The SPI layer does not,
 *    interpret, the contents of the lockStruct, but passes it on
 *    to the service provider who is responsible for enforcing the
 *    correct semantic behavior.
 *
 * SEE ALSO
 *    NWsiRemoveFileLock(3k), NWsiOpenNode(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiSetFileLock (	nwcred_t			*credPtr,
					SPI_HANDLE_T		*volHandle,
					SPI_HANDLE_T		*fileHandle,
					NWSI_LOCK_T			*lockStruct,
					enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	ncp_task_t			*task;
	ncp_channel_t		*channel;
	SPI_TASK_T			*gTask;
	NCP_FILEHANDLE_T	*ncpFileHandle;
	pl_t				pl;

	NVLT_ENTER (5);

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);
	NVLT_ASSERT (fileHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (fileHandle->stamp == SPI_FHANDLE_STAMP);

	if ((gTask = fileHandle->spilTaskPtr) == NULL) {
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if (!(gTask->mode & SPI_TASK_AUTHENTICATED)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	/*
	 *	Indicate that this task is in use
	 */
	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	NWslGetTaskProtoHandle (gTask, &task);
	NWslGetHandleSProtoResHandle (fileHandle, &ncpFileHandle);
	channel = task->channelPtr;

	ccode =  NCPsp2XLockFile_l (channel, (uint8 *)ncpFileHandle, lockStruct);

	if (ccode != SUCCESS) {
		if ((ccode == SPI_SERVER_UNAVAILABLE) ||
				(ccode == SPI_NO_CONNECTIONS_AVAILABLE) ||
				(ccode == SPI_BAD_CONNECTION) || (ccode == SPI_SERVER_DOWN)) {
			gTask->mode |= SPI_TASK_DRAINING;
		}
		*diagnostic = ccode;
		ccode = FAILURE;
	}

	/*
	 *	Indicate that this task is no longer in use
	 */
	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	NVLT_LEAVE (*diagnostic);
	return (ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiRemoveFileLock.3k)
 * NAME
 *    NWsiRemoveFileLock - Remove a previous lock from a file byte range.
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiRemoveFileLock ( nwcred_t       *credPtr,
 *                         SPI_HANDLE_T   *volHandle,
 *                         SPI_HANDLE_T   *fileHandle,
 *                         NWSI_LOCK_T    *lockStruct,
 *                         enum  NUC_DIAG *diagnostic )
 *
 * INPUT
 *    credPtr              - Credentials of the UNIX user removing the lock.
 *    volHandle            - Volume Handle of the volume that the fileHandle
 *                           resides in.
 *    fileHandle           - File handle of the file being unlocked.
 *    lockStruct->offset   - Byte offset into file where unlock starts
 *    lockStruct->size     - Number of bytes in to be unlock
 *    lockStruct->mode     - READ_LOCK or WRITE_LOCK
 *    lockStruct->type     - PHYSICAL_LOCK or LOGICAL_LOCK
 *    lockStruct->timeout  - Timeout value in milliseconds
 *
 * OUTPUT
 *    diagnostic           - Set to one of the following if an error occurs:
 *                               SPI_INACTIVE
 *                               SPI_AUTHENTICATION_FAILURE
 *                               SPI_CLIENT_RESOURCE_SHORTAGE
 *                               SPI_SERVER_UNAVAILABLE
 *                               SPI_NO_CONNECTIONS_AVAILABLE
 *                               SPI_BAD_CONNECTION
 *                               SPI_SERVER_DOWN
 *                               SPI_ACCESS_DENIED
 *
 * RETURN VALUES
 *    SUCCESS             - Successful completion.
 *    FAILURE             - Unsuccessful completion, diagnostic contains the
 *                          reason.
 *
 * DESCRIPTION
 *    Removes a lock from a previously locked byte range as specified by
 *    lockStruct.  SPIL does not interpret lock information, nor does it
 *    enforce semantic behavior.  All locks on an object are released
 *    implicitly when the object is closed.
 *
 * SEE ALSO
 *    NWsiSetFileLock(3k), NWsiOpenNode(3k), NWsiCloseNode(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiRemoveFileLock (	nwcred_t			*credPtr,
						SPI_HANDLE_T		*volHandle,
						SPI_HANDLE_T		*fileHandle,
						NWSI_LOCK_T			*lockStruct,
						enum	NUC_DIAG	*diagnostic )
{ 
	enum	NUC_DIAG	ccode = SUCCESS;
	ncp_task_t			*task;
	ncp_channel_t		*channel;
	SPI_TASK_T			*gTask;
	NCP_FILEHANDLE_T	*ncpFileHandle;
	pl_t				pl;

	NVLT_ENTER (5);

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);
	NVLT_ASSERT (fileHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (fileHandle->stamp == SPI_FHANDLE_STAMP);

	if ((gTask = fileHandle->spilTaskPtr) == NULL) {
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if (!(gTask->mode & SPI_TASK_AUTHENTICATED)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	/*
	 *	Indicate that this task is in use
	 */
	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	NWslGetTaskProtoHandle (gTask, &task);
	NWslGetHandleSProtoResHandle (fileHandle, &ncpFileHandle);
	channel = task->channelPtr;

	ccode =  NCPsp2XUnlockFile_l (channel, (uint8 *)ncpFileHandle, lockStruct);

	if (ccode != SUCCESS) {
		if ((ccode == SPI_SERVER_UNAVAILABLE) ||
				(ccode == SPI_NO_CONNECTIONS_AVAILABLE) ||
				(ccode == SPI_BAD_CONNECTION) || (ccode == SPI_SERVER_DOWN)) {
			gTask->mode |= SPI_TASK_DRAINING;
		}
		*diagnostic = ccode;
		ccode = FAILURE;
	}

	/*
	 *	Indicate that this task is no longer in use
	 */
	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	NVLT_LEAVE (*diagnostic);
	return (ccode);
}


/*
 * BEGIN_MANUAL_ENTRY(NWsiLinkFile.3k)
 * NAME
 *		NWsiLinkFile - Link one file to another.
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiLinkFile(	nwcred_t          *credPtr,
 *                  SPI_HANDLE_T      *volHandle,
 *                  SPI_HANDLE_T      *currParentHandle,
 *                  char              *currName,
 *                  int32             currUniqueID,
 *                  SPI_HANDLE_T      *newParentHandle,
 *                  char              *newName,
 *                  enum    NUC_DIAG  *diagnostic )
 *
 * INPUT
 *    credPtr           - Credentials of the UNIX user trying to link a file.
 *    volHandle         - Volume Handle of the volume where the link will
 *                        take place in.
 *    currParentHandle  - Parent directory handle of the existing file.
 *    currName          - Name of the existing file.
 *    currUniqueID      - The existing file can also be identified with
 *                        its unique node ID.   If the unique ID is passed
 *                        it is used instead of the currParentHandle and/or
 *                        the currName.
 *    newParentHandle   - Parent directory handle of the new linked file.
 *                        This new link node must be identified by parent
 *                        handle and new name.
 *    newName           - Name of the new linked file.
 *
 * OUTPUT
 *    diagnostic        - Set to one of the following if an error occurs:
 *                            SPI_INACTIVE
 *                            SPI_AUTHENTICATION_FAILURE
 *                            SPI_CLIENT_RESOURCE_SHORTAGE
 *                            SPI_SERVER_UNAVAILABLE
 *                            SPI_NO_CONNECTIONS_AVAILABLE
 *                            SPI_BAD_CONNECTION
 *                            SPI_SERVER_DOWN
 *                            SPI_ACCESS_DENIED
 *                            SPI_INVALID_PATH
 *                            SPI_TOO_MANY_LINKS
 *
 * RETURN VALUES
 *    SUCCESS          - Successful completion.
 *    FAILURE          - Unsuccessful completion, diagnostic contains the
 *                       reason.
 *
 * DESCRIPTION
 *    Creates a link to a existing file identified by currParentHandle and
 *    currName.  The existing file can also be identified by its unique node
 *    ID.
 *
 * SEE ALSO
 *    NWsiOpenFile(3k), NWsiCloseFile(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiLinkFile(	nwcred_t			*credPtr,
				SPI_HANDLE_T		*volHandle,
				SPI_HANDLE_T		*currParentHandle,
				char				*currName,
				int32				currUniqueID,
				SPI_HANDLE_T		*newParentHandle,
				char				*newName,
				enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	ncp_task_t			*task;
	ncp_channel_t		*channel;
	SPI_TASK_T			*gTask;
	ncp_volume_t		*ncpVolumeHandle;
	NCP_DIRHANDLE_T		*ncpCurrDirHandle = NULL, *ncpNewDirHandle;
	pl_t				pl;

	NVLT_ENTER (8);

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);
	NVLT_ASSERT (newParentHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (newParentHandle->stamp == SPI_DHANDLE_STAMP);
	NVLT_ASSERT (newName != NULL);

	if ((currParentHandle == NULL && currName == NULL &&
			currUniqueID == -1)) {
		*diagnostic = SPI_BAD_ARGS;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
		
	if ((gTask = newParentHandle->spilTaskPtr) == NULL) {
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if (!(gTask->mode & SPI_TASK_AUTHENTICATED)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	/*
	 *	Indicate that this task is in use
	 */
	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		return (NVLT_LEAVE (FAILURE));
	}

	NWslGetTaskProtoHandle (gTask, &task);
	NWslGetHandleSProtoResHandle (volHandle, &ncpVolumeHandle);
	if (currParentHandle)
		NWslGetHandleSProtoResHandle (currParentHandle, &ncpCurrDirHandle);
	NWslGetHandleSProtoResHandle (newParentHandle, &ncpNewDirHandle);
	channel = task->channelPtr;

	ccode = NCPsp3NLinkFile2_l (channel, ncpVolumeHandle, ncpCurrDirHandle,
			currName, currUniqueID, ncpNewDirHandle, newName, credPtr);

	if (ccode != SUCCESS) {
		if ((ccode == SPI_SERVER_UNAVAILABLE) ||
				(ccode == SPI_NO_CONNECTIONS_AVAILABLE) ||
				(ccode == SPI_BAD_CONNECTION) || (ccode == SPI_SERVER_DOWN)) {
			gTask->mode |= SPI_TASK_DRAINING;
		}
		*diagnostic = ccode;
		ccode = FAILURE;
	}

	/*
	 *	Indicate that this task is no longer in use
	 */
	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	NVLT_LEAVE (*diagnostic);
	return (ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiGetFileSize.3k)
 * NAME
 *		NWsiGetFileSize - Get the current file size.
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiGetFileSize ( nwcred_t          *credPtr,
 *                      SPI_HANDLE_T      *volHandle,
 *                      SPI_HANDLE_T      *fileHandle,
 *                      uint32            *size,
 *                      enum    NUC_DIAG  *diagnostic )
 *
 * INPUT
 *    credPtr       - Credentials of the UNIX user getting the file size.
 *    volHandle     - Volume Handle of the volume where the file resides in.
 *    fileHandle    - File handle of the file.
 *
 * OUTPUT
 *    size          - Current size of the file.
 *    diagnostic    - Set to one of the following if an error occurs:
 *                        SPI_INACTIVE
 *                        SPI_AUTHENTICATION_FAILURE
 *                        SPI_CLIENT_RESOURCE_SHORTAGE
 *                        SPI_SERVER_UNAVAILABLE
 *                        SPI_NO_CONNECTIONS_AVAILABLE
 *                        SPI_BAD_CONNECTION
 *                        SPI_SERVER_DOWN
 *                        SPI_BAD_HANDLE
 *
 * RETURN VALUES
 *    SUCCESS      - Successful completion.
 *    FAILURE      - Unsuccessful completion, diagnostic contains the reason.
 *
 * DESCRIPTION
 *    Returns the actual current size of the file associated with the specified
 *    fileHandle.
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiGetFileSize (	nwcred_t			*credPtr,
					SPI_HANDLE_T		*volHandle,
					SPI_HANDLE_T		*fileHandle,
					uint32				*size,
					enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	ncp_task_t			*task;
	ncp_channel_t		*channel;
	SPI_TASK_T			*gTask;
	NCP_FILEHANDLE_T	*ncpFileHandle;
	pl_t				pl;

	NVLT_ENTER (5);

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);
	NVLT_ASSERT (fileHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (fileHandle->stamp == SPI_FHANDLE_STAMP);

	if ((gTask = fileHandle->spilTaskPtr) == NULL) {
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if (!(gTask->mode & SPI_TASK_AUTHENTICATED)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	/*
	 *	Indicate that this task is in use
	 */
	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (SPI_AUTHENTICATION_FAILURE);
		return (FAILURE);
	}

	NWslGetTaskProtoHandle (gTask, &task);
	NWslGetHandleSProtoResHandle (fileHandle, &ncpFileHandle);
	channel = task->channelPtr;

	ccode = NCPsp2XGetCurrentSizeOfFile_l (channel, (uint8 *)ncpFileHandle,
											size);

	if (ccode != SUCCESS) {
		if ((ccode == SPI_SERVER_UNAVAILABLE) ||
				(ccode == SPI_NO_CONNECTIONS_AVAILABLE) ||
				(ccode == SPI_BAD_CONNECTION) || (ccode == SPI_SERVER_DOWN)) {
			gTask->mode |= SPI_TASK_DRAINING;
		}
		*diagnostic = ccode;
		ccode = FAILURE;
	}

	/*
	 *	Indicate that this task is no longer in use
	 */
	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	NVLT_LEAVE (*diagnostic);
	return (ccode);
}
