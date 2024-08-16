/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spidir.c	1.19"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spidir.c,v 2.59.2.3 1995/01/05 17:56:11 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	MODULE:    spidir.c
 *	ABSTRACT:  File system interface calls pertaining to directory objects
 *	
 *	Functions declared in this module:
 *		NWsiGetDirectoryPath
 *		NWsiGetDirectoryEntries
 *		NWsiGetParentNameSpaceInfo
 *
 */ 

#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <util/cmn_err.h>
#include <net/nuc/nwspiswitch.h>
#include <net/nuc/nwspi.h>
#include <net/nuc/spimacro.h>
#include <net/nuc/nucerror.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/sistructs.h>
#include <net/nuc/slstruct.h>
#include <util/debug.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>

#define NVLT_ModMask	NVLTM_spil

extern rwlock_t		*nucUpperLock;
extern sleep_t		*spiTaskListSleepLock;

/*
 *	External SPI variables
 */
extern int32 	spiState;

/*
 * BEGIN_MANUAL_ENTRY(NWsiGetDirectoryPath.3k)
 * NAME
 *    NWsiGetDirectoryPath - Get the rooted path to a directory pointed to
 *                           by a directory handle.
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiGetDirectoryPath ( nwcred_t         *credPtr,
 *                           SPI_HANDLE_T     *volHandle,
 *                           SPI_HANDLE_T     *dirHandle,
 *                           char             *path,
 *                           enum   NUC_DIAG  *diagnostic )
 *
 * INPUT
 *    credPtr     - Credentials of the UNIX user asking for the rooted path.
 *    volHandle   - Volume Handle of the volume that dirHandle resides in.
 *    dirHandle   - Directory handle of the directory.
 *
 * OUTPUT
 *    path        - Path of the specified dirHandle from the root of the
 *                  volume specified by volHandle.
 *    diagnostic  - Set to one of the following if an error occurs:
 *                      SPI_INACTIVE
 *                      SPI_AUTHENTICATION_FAILURE
 *                      SPI_CLIENT_RESOURCE_SHORTAGE
 *                      SPI_SERVER_UNAVAILABLE
 *                      SPI_NO_CONNECTIONS_AVAILABLE
 *                      SPI_BAD_CONNECTION
 *                      SPI_SERVER_DOWN
 *                      SPI_NAME_TOO_LONG
 *                      SPI_INVALID_PATH
 *                      SPI_NODE_NOT_DIRECTORY
 *
 * RETURN VALUES
 *    SUCCESS     - Successful completion.
 *    FAILURE     - Unsuccessful completion, diagnostic contains the reason.
 *
 * DESCRIPTION
 *    Given a previously opened directory, returns a path of the opened 
 *    directory from the root of the volume specified by volHandle.
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiGetDirectoryPath (	nwcred_t			*credPtr,
						SPI_HANDLE_T		*volHandle,
						SPI_HANDLE_T		*dirHandle,
						char				*path,
						enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	ncp_task_t			*task;
	ncp_channel_t		*channel;
	ncp_volume_t		*ncpVolHandle;
	NCP_DIRHANDLE_T		*ncpDirHandle;
	SPI_TASK_T			*gTask;
	int32				type;
	pl_t				pl;

	NVLT_ENTER (5);

	/*
	 *	If the spi has not been activated, don't bother attempting
	 *	to service the request
	 */
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
	NVLT_ASSERT (dirHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (dirHandle->stamp == SPI_DHANDLE_STAMP);

	gTask = dirHandle->spilTaskPtr;
	if (gTask == NULL) {
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
	 *	Indicate that this task is in use.
	 */
	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	NWslGetHandleSProtoResHandle (dirHandle, &ncpDirHandle);
	NWslGetHandleSProtoResHandle (volHandle, &ncpVolHandle);
	NWslGetTaskProtoHandle (gTask, &task);
	channel = task->channelPtr;

	/*
	 *	Get the rooted path to the specified directory handle.
	 */
	ccode = NCPsp3NGetDirPathFromHandle_l (channel, ncpVolHandle,
				ncpDirHandle, (int32)-1, FALSE, path, &type,
				NCP_UNIX_NAME_SPACE);

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
	 *	Indicate that this task is no longer in use.
	 */
	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	NVLT_LEAVE (*diagnostic);
	return (ccode);
}


/*
 * BEGIN_MANUAL_ENTRY(NWsiGetDirectoryEntries.3k)
 * NAME
 *    NWsiGetDirectoryEntries - Iteratively scan a directory for its entries.
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiGetDirectoryEntries ( nwcred_t         *credPtr,
 *                              SPI_HANDLE_T     *volHandle,
 *                              SPI_HANDLE_T     *dirHandle,
 *                              uint8            requestType,
 *                              uint32           *offset,
 *                              NUC_IOBUF_T      *buffer,
 *                              enum  NUC_DIAG   *diagnostic )
 *
 * INPUT
 *    credPtr     - Cred structure of the UNIX user getting directory entires.
 *    volHandle   - Volume handle of the volume where the dirHandle resides in.
 *    dirHandle   - Directory handle of the directory.
 *    requestType - Specifies the state the search is currently in: initialize,
 *                  continue or terminate (GET_FIRST, GET_MORE, or GET_DONE).
 *    offset      - Starting offset.
 *                     1 means to ignore the "." and return ".." and the first
 *                     real directory entry.
 *
 *                     2 means to ignore the "." and ".." and return the first
 *                     real directory entry.
 *    buffer      - Save the directory entries in this buffer.
 *
 * OUTPUT
 *    offset      - Offset of the last entry.
 *    buffer      - Contains the directory entries in NWSI_DIRECTORY_T format.
 *    diagnostic  - Set to one of the following if an error occurs:
 *                      SPI_INACTIVE
 *                      SPI_AUTHENTICATION_FAILURE
 *                      SPI_SERVER_UNAVAILABLE
 *                      SPI_NO_CONNECTIONS_AVAILABLE
 *                      SPI_BAD_CONNECTION
 *                      SPI_SERVER_DOWN
 *                      SPI_NO_MORE_ENTRIES
 *
 * RETURN VALUES
 *    SUCCESS     - Successful completion.
 *    FAILURE     - Unsuccessful completion, diagnostic contains reason.
 *
 * DESCRIPTION
 *    Provides an iterative mechanism for perusing a directory for the entries
 *    contained in it.
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiGetDirectoryEntries (	nwcred_t			*credPtr,
							SPI_HANDLE_T		*volHandle,
							SPI_HANDLE_T		*dirHandle,
							uint8				requestType,
							uint32				*offset,
							NUC_IOBUF_T			*buffer,
							enum	NUC_DIAG	*diagnostic )
{
	enum NUC_DIAG	ccode = SUCCESS;
	ncp_task_t		*task;
	ncp_volume_t	*ncpVolHandle;
	NCP_DIRHANDLE_T	*ncpDirHandle;
	ncp_channel_t	*channel;
	SPI_TASK_T		*gTask;
	pl_t			pl;

	NVLT_ENTER (7);

	/*
	 *	If the spi has not been activated, don't bother attempting
	 *	to service the request
	 */
	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT(volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT(volHandle->stamp == SPI_VOLUME_STAMP);
	NVLT_ASSERT(dirHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT(dirHandle->stamp == SPI_DHANDLE_STAMP);

	if ((gTask = dirHandle->spilTaskPtr) == NULL) {
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

	NWslGetHandleSProtoResHandle (volHandle, &ncpVolHandle);
	NWslGetHandleSProtoResHandle (dirHandle, &ncpDirHandle);
	NWslGetTaskProtoHandle (gTask, &task);
	channel = task->channelPtr;

	/*
	 * 	Get directory entries.
	 */
	ccode = NCPsp3NGetDirectoryEntries2 (channel, ncpVolHandle, ncpDirHandle,
			NCP_UNIX_NAME_SPACE, requestType, offset, buffer, credPtr);

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
 * BEGIN_MANUAL_ENTRY(NWsiGetParentNameSpaceInfo.3k)
 * NAME
 *    NWsiGetParentNameSpaceInfo -
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiGetParentNameSpaceInfo ( nwcred_t          *credPtr,
 *                                 SPI_HANDLE_T      *volHandle,
 *                                 NWSI_NAME_SPACE_T *nameSpaceInfo,
 *                                 enum   NUC_DIAG   *diagnostic )
 *
 * INPUT
 *    credPtr       - Cred structure of the UNIX user getting the parent name
 *                    space information.
 *    volHandle     - Volume handle of the volume where the parent directory
 *                    resides in.
 *    nameSpaceInfo - Name space information of the child node.  The node
 *                    number contain the unique identifier of the child node.
 *
 * OUTPUT
 *    nameSpaceInfo - Name space information of the parent node.
 *    diagnostic    - Set to one of the following if an error occurs:
 *                        SPI_INACTIVE
 *                        SPI_AUTHENTICATION_FAILURE
 *                        SPI_SERVER_UNAVAILABLE
 *                        SPI_NO_CONNECTIONS_AVAILABLE
 *                        SPI_BAD_CONNECTION
 *                        SPI_SERVER_DOWN
 *
 * RETURN VALUES
 *    SUCCESS     - Successful completion.
 *    FAILURE     - Unsuccessful completion, diagnostic contains reason.
 *
 * DESCRIPTION
 *    Returns the name space information associated with the parent node
 *    of the node specified by nameSpaceInfo->nodeNumber.
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiGetParentNameSpaceInfo (	nwcred_t			*credPtr,
								SPI_HANDLE_T		*volHandle,
								NWSI_NAME_SPACE_T	*nameSpaceInfo,
								enum	NUC_DIAG	*diagnostic )
{
	enum NUC_DIAG	ccode = SUCCESS;
	ncp_task_t		*task;
	ncp_volume_t	*ncpVolHandle;
	ncp_channel_t	*channel;
	SPI_TASK_T		*gTask;
	SPI_SERVICE_T	*service;
	char			*path;
	int32			type;
	pl_t			pl;

	NVLT_ENTER (4);

	/*
	 *	If the spi has not been activated, don't bother attempting
	 *	to service the request
	 */
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
	NVLT_ASSERT (nameSpaceInfo->nodeNumber != -1);

	service = volHandle->localResHandle;
	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if ((ccode = GetTask_l (service->taskList, credPtr, &gTask, FALSE))
				== SUCCESS) {
		SLEEP_UNLOCK (spiTaskListSleepLock);
		SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
		if (!(gTask->mode & SPI_TASK_AUTHENTICATED))  {
			/*
			 * User not authenticated to the server.
			 */
			SLEEP_UNLOCK (gTask->spiTaskSleepLock);
			*diagnostic = SPI_AUTHENTICATION_FAILURE;
			return( NVLT_LEAVE(FAILURE) );
		}
	} else {
		/*
		 * No task associated with this user.
		 */
		SLEEP_UNLOCK (spiTaskListSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		return( NVLT_LEAVE(FAILURE) );
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

	NWslGetHandleSProtoResHandle (volHandle, &ncpVolHandle);
	NWslGetTaskProtoHandle (gTask, &task);
	channel = task->channelPtr;

	path = kmem_zalloc (SPI_MAX_PATH_LENGTH, KM_SLEEP);

	/*
	 *	Get the rooted path to the partent directory using the child unique
	 *	ID.  The wantParentPath is set to TRUE to get the parent path 
	 *	instead of the child path.
	 *
	 * NOTE:
	 *	nameSpaceInfo->nodeNumber is the unique ID of the child.
	 *	The type will be set to NS_ROOT or NS_DIRECTORY by
	 *	NCPsp3NGetDirPathFromHandle_l.
	 */
	ccode = NCPsp3NGetDirPathFromHandle_l (channel, ncpVolHandle,
			(NCP_DIRHANDLE_T *)NULL, nameSpaceInfo->nodeNumber, TRUE, path,
			&type, NCP_UNIX_NAME_SPACE);

	if (ccode == SUCCESS) {
		/*
		 *	The nameSpaceInfo->nodeNumber is set to -1 to make sure that
		 *	the path (which is the parent path) is used when getting the
		 *	name space information.
		 */
		nameSpaceInfo->openFileHandle = NULL;
		nameSpaceInfo->nodeNumber = -1;
		nameSpaceInfo->nodeType = type;
		ccode = NCPsp3NGetNameSpaceInformation2_l (channel, ncpVolHandle,
				(NCP_DIRHANDLE_T *)NULL, path, nameSpaceInfo, credPtr);
	}

	kmem_free (path, SPI_MAX_PATH_LENGTH);

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
