/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spivolume.c	1.19"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spivolume.c,v 2.57.2.5 1995/01/16 22:53:08 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	MODULE:    spivolume.c
 *	ABSTRACT:  File system interface calls pertaining to volume objects
 *	
 *  Functions declared in this module:
 *      NWsiOpenVolume
 *      NWsiCloseVolume
 *      NWsiVolumeStats
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
#include <net/nuc/nwctypes.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/sistructs.h>
#include <net/nuc/slstruct.h>
#include <util/debug.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>

#define NVLT_ModMask	NVLTM_spil

extern int32		spiState;
extern rwlock_t		*nucUpperLock;
extern sleep_t		*spiTaskListSleepLock;

/*
 *	Volume statistics and naming information
 */

/*
 * BEGIN_MANUAL_ENTRY(NWsiOpenVolume.3k)
 * NAME
 *    NWsiOpenVolume - Generate a volume handle for subsequent reference
 *
 * SYNOPSIS
 *    ccode_t
 *    NWsiOpenVolume ( nwcred_t          *credPtr,
 *                     struct netbuf     *serviceAddress,
 *                     char              *volumeName,
 *                     SPI_HANDLE_T      **volHandle,
 *                     uint8             *nucNlmMode,
 *                     uint32            *logicalBlockSize,
 *                     enum   NUC_DIAG   *diagnostic )
 *
 * INPUT
 *    credPtr          - Credentials of the UNIX user openning the volume.
 *    serviceAddress   - The NetWare server address.
 *    volumeName       - The name of the volume to be opened.
 *
 * OUTPUT
 *    volHandle        - Volume handle to be used with all subsequent file
 *                       system operations.
 *    nucNlmMode       - Set to one of the following:
 *                           NUC_NLM_NETWARE_MODE - NUC NLM in NetWare mode.
 *                           NUC_NLM_UNIX_MODE    - NUC NLM in UNIX (NFS) mode.
 *                           NUC_NLM_NONE         - None of the above.
 *    logicalBlockSize - Volumes block size.
 *    diagnostic       - Set to one of the following if an error occurs:
 *                           SPI_INACTIVE
 *                           SPI_NO_SUCH_SERVICE
 *                           SPI_MEMORY_EXHAUSTED
 *                           SPI_AUTHENTICATION_FAILURE
 *                           SPI_CLIENT_RESOURCE_SHORTAGE
 *                           SPI_SERVER_UNAVAILABLE
 *                           SPI_NO_CONNECTIONS_AVAILABLE
 *                           SPI_BAD_CONNECTION
 *                           SPI_SERVER_DOWN
 *                           SPI_ACCESS_DENIED
 *                           SPI_BAD_ARGS_TO_SERVER
 *                           SPI_NLM_NOT_LOADED
 *
 * DESCRIPTION
 *    Attains information on a volume maintained by the specified service,
 *    and returns a reference handle utilized in all subsequent file system
 *    requests.  If the NUC NLM is not installed on the NetWare server, or
 *    if the server is a 2.X server, this operation will fail.
 *
 * SEE ALSO
 *    NWsiCloseVolume(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWsiOpenVolume (	nwcred_t			*credPtr,
					struct netbuf		*serviceAddress,
					char				*volumeName,
					SPI_HANDLE_T		**volHandle,
					uint8				*nucNlmMode,
					uint32				*logicalBlockSize,
					enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	SPI_TASK_T			*gTask;
	ncp_task_t			*task;
	ncp_channel_t		*channel;
	pl_t				pl;
	int32				volumeNumber;
	SPI_SERVICE_T		*service;
	ncp_server_t		*ncpServer;
	ncp_volume_t		*ncpVolumeHandle;

	NVLT_ENTER (6);

	/*
	 * Initialize the NUC NLM mode.
	 */
	*nucNlmMode = NUC_NLM_NONE;

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		return (NVLT_LEAVE (FAILURE));
	}

	RW_UNLOCK (nucUpperLock, pl);

	/*
	 *	Get structures
	 */
	if (NWslGetService (serviceAddress, &service)) {
		*diagnostic = SPI_NO_SUCH_SERVICE;
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 * NWslGetTask calls the login thread of the nucd deamon to call xauto
	 * to authenticate the user if not already authenticated.
	 *
	 * NOTE:
	 *    This is the only NWsi routine that causes xauto to be called.
	 *    All of the other file system related NWsi calls return
	 *    SPI_AUTHENTICATION_FAILURE to NUCFS file system if the user
	 *    performing the operation is not already authenticated to the
	 *    NetWare server.  For all of the other NWsi operations it is up
	 *    to NUCFS file system layer to call NWsiAutoAutheticate to prompt
	 *    the user for the needed authentication information.  NWsiOpenVolume
	 *    is called from NWfsMountVolume.  And NWfsMountVolume cannot perform
	 *    auto-authentication with the current SPI interface because:
	 *
	 *        (1) NWsiAutoAuthenticate() requires a volume handle, and
	 *        (2) When NWsiOpenVolume() fails, no volume handle is available.
	 */
	if (NWslGetTask (service, credPtr, &gTask, FALSE) != SUCCESS) {
		/*
		 * No task associated with this user.
		 */
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 *	Indicate that this task is in use
	 */
	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 *	Extract the service protocol handle returned from OpenTask
	 */
	NWslGetTaskProtoHandle (gTask, &task);
	ncpServer = task->serverPtr;
	channel = task->channelPtr;

	/*
	 *	Find out what name spaces are available for this volume
	 *	and server. 
	 *	NOTE: This requests hits the wire
	 */
	if  ((ncpServer->majorVersion > 3) ||
		((ncpServer->majorVersion == 3) && (ncpServer->minorVersion >= 11))) {
		/*
		 *	Allocate a new handle 
		 */
		if (NWslAllocHandle (volHandle, SPI_VOLUME_STAMP)) {
			*diagnostic = SPI_MEMORY_EXHAUSTED;
			NWslSetTaskNotInUse_l (gTask);
			SLEEP_UNLOCK (gTask->spiTaskSleepLock);
			return (NVLT_LEAVE (FAILURE));
		}

		/*
		 *	Allocate space for the volume handle that will be returned
		 *	to the caller
		 */
		if (NCPsilAllocVolume (&ncpVolumeHandle)) {
			ccode = SPI_MEMORY_EXHAUSTED;
			goto error1;
		}

		/*
		 *	Get the volume number and, if appropriate, get name space
		 *	information for this volume.
		 *	NOTE: This requests hits the wire
		 */
		ccode = NCPsp2XGetVolumeNumber_l (channel, volumeName, &volumeNumber);
		if (ccode) {
			goto error;
		}

		/*
		 *	Set the volume number for this name
		 */
		ncpVolumeHandle->number = volumeNumber;

		/*
		 * NetWare server 3.11 and higher.
		 */
		ccode = NCPspIsUNIXNameSpaceSupported_l (channel, ncpVolumeHandle, 
					nucNlmMode);
		if (ccode) {
			goto error;
		}
	} else {
		/*
		 * We do not support 2.X servers.
		 */
		NWslSetTaskNotInUse_l (gTask);
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_NLM_NOT_LOADED;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	/*
	 *	Get the name space operations table and tag it to the
	 *	volume handle for subsequent reference
	 */
	NVLT_PRINTF ("NCPsiOpenVolume: nucNlmMode=%d\n", *nucNlmMode, 0, 0);
	switch (*nucNlmMode) {
		case NUC_NLM_NETWARE_MODE:
			/*
			 * NUC NLM is in the NetWare mode.
			 */
			NVLT_PRINTF ("NCPsiOpenVolume: New 95 NCPs in NetWare mode\n",
					0, 0, 0);
			
			break;

		case NUC_NLM_UNIX_MODE:
			/*
			 * NUC NLM is in the UNIX (NFS) mode.
			 */
			NVLT_PRINTF ("NCPsiOpenVolume: New 95 NCPs in UNIX (NFS) mode\n",
					0, 0, 0);
			break;

		default:
			/*
			 * NUC NLM is not loaded on the volume of interest.
			 */
			ccode = SPI_NLM_NOT_LOADED;
			goto error;
	}

	/*
	 *	Get the negotiated buffer size from the server structure
	 *	for the logical block size.
	 */
	*logicalBlockSize = ncpServer->blockSize;

	NWslSetHandleSProtoResHandle (*volHandle, ncpVolumeHandle);
	NWslSetHandleLocalResHandle (*volHandle, service); 
	(*volHandle)->spilTaskPtr = gTask;

	/*
	 *	Indicate that this task is no longer in use
	 */
	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	*diagnostic = SUCCESS;
	return (NVLT_LEAVE (SUCCESS));

error:
	NCPsilFreeVolume (ncpVolumeHandle);
error1:
	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);
	NWslFreeHandle (*volHandle);
	*diagnostic = ccode;
	NVLT_LEAVE (*diagnostic);
	return (FAILURE);
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiCloseVolume.3k)
 * NAME
 *    NWsiCloseVolume - Frees resources allocated by NWsiOpenVolume
 *
 * SYNOPSIS
 *    ccode_t
 *    NWsiCloseVolume ( nwcred_t           *credPtr,
 *                      SPI_HANDLE_T       *volHandle,
 *                      enum    NUC_DIAG   *diagnostic )
 *
 * INPUT
 *    credPtr    - Credentials of the UNIX user closing the volume.
 *    volHandle  - Volume handle to be closed.
 *
 * OUTPUT
 *    diagnostic - Set to one of the following if an error occurs:
 *                     SPI_INACTIVE
 *
 * DESCRIPTION
 *    Frees the volume handle resources that were allocated by NWsiOpenVolume.
 *
 * SEE ALSO
 *    NWsiOpenVolume(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWsiCloseVolume (	nwcred_t			*credPtr,
					SPI_HANDLE_T		*volHandle,
					enum	NUC_DIAG	*diagnostic )
{
	ncp_volume_t		*ncpVolumeHandle;
	pl_t				pl;

	NVLT_ENTER (3);

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if ( spiState == SPI_LAYER_INACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		return( NVLT_LEAVE( FAILURE ) );
	}
	RW_UNLOCK (nucUpperLock, pl);

	/*
	 *	Sanity check handle passed from file system
	 */
	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);

	NWslGetHandleSProtoResHandle (volHandle, &ncpVolumeHandle);

	if (ncpVolumeHandle)
		kmem_free (ncpVolumeHandle, sizeof (ncp_volume_t));

	if(volHandle) {
		kmem_free (volHandle, sizeof(SPI_HANDLE_T));
		volHandle = NULL;
	}

	return (NVLT_LEAVE (SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(NWsiVolumeStats.3k)
 * NAME
 *    NWsiVolumeStats - Get statistics of the specific volume
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiVolumeStats ( nwcred_t             *credPtr,
 *                      SPI_HANDLE_T         *volHandle,
 *                      NWSI_VOLUME_STATS_T  *volumeStats,
 *                      enum   NUC_DIAG      *diagnostic )
 *
 * INPUT
 *    credPtr     - Credentials of the UNIX user stating the volume.
 *    volHandle   - Volume handle to be stated.
 *
 * OUTPUT
 *    volumeStats - Contains information of the specified volume.
 *    diagnostic  - Set to one of the following if an error occurs:
 *                      SPI_INACTIVE
 *                      SPI_AUTHENTICATION_FAILURE
 *                      SPI_CLIENT_RESOURCE_SHORTAGE
 *                      SPI_SERVER_UNAVAILABLE
 *                      SPI_NO_CONNECTIONS_AVAILABLE
 *                      SPI_BAD_CONNECTION
 *                      SPI_SERVER_DOWN
 *                      SPI_ACCESS_DENIED
 *
 * DESCRIPTION
 *    Returns physical structure information on the specified volume.  Must
 *    have opened the volume prior to issuing this call.
 *
 * SEE ALSO
 *    NWsiOpenVolume(3k), NWsiCloseVolume(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiVolumeStats (	nwcred_t			*credPtr,
					SPI_HANDLE_T		*volHandle,
					NWSI_VOLUME_STATS_T	*volumeStats,
					enum NUC_DIAG		*diagnostic )
{
	enum NUC_DIAG		ccode = SUCCESS;
	ncp_volume_t		*ncpVolumeHandle;
	ncp_channel_t		*channel;
	ncp_task_t			*task;
	SPI_TASK_T			*gTask;
	SPI_SERVICE_T		*service;
	pl_t				pl;
	uint32				blockSize;
	uint32				totalBytes, freeBytes;
	uint32				totalSlots, freeSlots;

	NVLT_ENTER (4);

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		return (NVLT_LEAVE (FAILURE));
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);


	NWslGetHandleSProtoResHandle (volHandle, &ncpVolumeHandle);

	service = volHandle->localResHandle;
	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if ((ccode = GetTask_l (service->taskList, credPtr, &gTask, FALSE))
			== SUCCESS) {
		/*
		 * Unlock spiTaskListSleepLock first in order not to dead lock
		 * the system.  The lock acquisition could occur in the reverse
		 * order else where.
		 */
		SLEEP_UNLOCK (spiTaskListSleepLock);
		SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
		if (!(gTask->mode & SPI_TASK_AUTHENTICATED))  {
			/*
			 * User not authenticated to the server.
			 */
			SLEEP_UNLOCK (gTask->spiTaskSleepLock);
			*diagnostic = SPI_AUTHENTICATION_FAILURE;
			return (NVLT_LEAVE (FAILURE));
		}
	} else {
		/*
		 * No task associated with this user.
		 */
		SLEEP_UNLOCK (spiTaskListSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		return (NVLT_LEAVE (FAILURE));
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
	channel = task->channelPtr;

	/*
	 *	Get the disk information in bytes that will be converted to
	 *	logical blocks.
	 */
	blockSize = ((ncp_server_t *)task->serverPtr)->blockSize;
	ccode = NCPsp3XGetVolumeInfo_l (channel, ncpVolumeHandle->number, 
			&totalBytes, &freeBytes, &totalSlots, &freeSlots);

	if (ccode == SUCCESS) {
		/*
		 *	Massage data for file system use
		 */
		volumeStats->totalFreeBlocks = freeBytes/blockSize;
		volumeStats->totalBlocks = totalBytes/blockSize;
		volumeStats->totalNodes = totalSlots;
		volumeStats->totalFreeNodes = freeSlots;
	}

	if (ccode) {
		*diagnostic = ccode;
		ccode = FAILURE;
	} else {
		ccode = SUCCESS;
	}

	/*
	 *	Indicate that this task is no longer in use
	 */
	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	return (NVLT_LEAVE (ccode));
}
