/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spifsys.c	1.23"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spifsys.c,v 2.60.2.7 1995/02/06 18:23:52 mdash Exp $"

/*
 *  Netware Unix Client
 *
 *	MODULE:     spifsys.c
 *	ABSTRACT:   File system interface calls pertaining to object independent
 *              file system functions.
 *	
 *	Functions declared in this module:
 *      NWsiOpenNode
 *      NWsiCloseNode
 *      NWsiCreateNode
 *      NWsiDeleteNode
 *      NWsiRenameNode
 *      NWsiGetNameSpaceInfo
 *      NWsiSetNameSpaceInfo
 *      NWsiAutoAuthenticate
 *      NWsiGetServerTime
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
#include <net/nuc/requester.h>
#include <net/nuc/nuc_prototypes.h>
#include <util/debug.h>

#include <io/ddi.h>

#define NVLT_ModMask	NVLTM_spil
extern rwlock_t			*nucUpperLock;
extern sleep_t			*spiTaskListSleepLock;

/*
 *	External SPI variables
 */
extern int32 	spiState;	/* SPI layer state vector */

/*
 * BEGIN_MANUAL_ENTRY(NWsiOpenNode.3k)
 * NAME
 *    NWsiOpenNode - Open a file system object (file or directory)
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiOpenNode ( nwcred_t           *credPtr,
 *                   SPI_HANDLE_T       *volHandle,
 *                   SPI_HANDLE_T       *parentHandle,
 *                   char               *objectName,
 *                   int32              objectType,
 *                   int32              openFlags,
 *                   SPI_HANDLE_T       **objectHandle,
 *                   NWSI_NAME_SPACE_T  *nameSpaceInfo,
 *                   enum   NUC_DIAG    *diagnostic )
 *
 * INPUT
 *    credPtr              - Credentials of the UNIX user opening the
 *                           file/directory.
 *    volHandle            - Volume handle of the volume that the file/directory
 *                           resides in.
 *    parentHandle         - Parent directory handle of the file/directory to be
 *                           opened.
 *    objectName           - Name of the file/directory to be opened.
 *    objectType           - Node type.
 *                               NS_ROOT
 *                               NS_DIRECTORY
 *                               NS_FILE
 *                               NS_SYMBOLIC_LINK
 *    openFlags            - Open flags:
 *                               NW_READ
 *                               NW_WRITE
 *                               NW_DENY_READ
 *                               NW_DENY_WRITE
 *                               NW_EXCLUSIVE
 *                               NW_FAIL_IF_NODE_EXISTS
 *                               NW_CHECK_PARENT_SETGID
 *                               NW_INHERIT_PARENT_GID
 *    nameSpaceInfo
 *       nodeNumber        - If not set to -1, identifies the unique ID of the
 *                           file/directory to be opened.  The unique node ID is
 *                           used instead of the parentHandle and objectName
 *                           combination if passed in.
 *
 * OUTPUT
 *    objectHandle         - Returned file or directory handle.
 *    nameSpaceInfo        - Only returned if opening a file (NS_SYMBOLIC_LINK,
 *                           NS_FILE):
 *       nodeNumber        - Unique object identifier.
 *       linkNodeNumber    - Unique object identifier of the link.
 *                           regular file implies:      
 *                               nodeNumber = linkNodeNumber.
 *       nodeType          - Object type.
 *       nodePermissions   - Object permissions.
 *       nodeNumberOfLinks - Object number of links.
 *       nodeSize;         - Size in bytes of data space.
 *       userID;           - User identifier of owner.
 *       groupID;          - Group identifier of owner.
 *       accessTime;       - Time of last access .
 *       modifyTime;       - Time of last data modification.
 *       changeTime;       - Time of last name space changed.
 *                           Times measured in seconds since 00:00:00 GMT,
 *                           Jan 1, 1970.
 *    diagnostic           - Set to one of the following if an error occurs:
 *                               SPI_INACTIVE
 *                               SPI_BAD_ARGS
 *                               SPI_MEMORY_EXHAUSTED
 *                               SPI_AUTHENTICATION_FAILURE
 *                               SPI_CLIENT_RESOURCE_SHORTAGE
 *                               SPI_SERVER_RESOURCE_SHORTAGE
 *                               SPI_SERVER_UNAVAILABLE
 *                               SPI_GENERAL_FAILURE
 *                               SPI_USER_MEMORY_FAULT
 *                               SPI_TYPE_NOT_SUPPORTED
 *                               SPI_NO_CONNECTIONS_AVAILABLE
 *                               SPI_BAD_CONNECTION
 *                               SPI_SERVER_DOWN
 *                               SPI_ACCESS_DENIED
 *                               SPI_DIRECTORY_FULL
 *                               SPI_FILE_IN_USE
 *                               SPI_NODE_NOT_FOUND
 *
 * RETURN VALUES
 *    SUCCESS        - Successful completion.
 *    FAILURE        - Unsuccessful completion, diagnostic contains the reason.
 *
 * DESCRIPTION
 *    Opens a file system object node and returns an opaque handle to the
 *    caller.
 *
 * SEE ALSO
 *    NWsiCloseNode(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiOpenNode (	nwcred_t			*credPtr,
				SPI_HANDLE_T		*volHandle,
				SPI_HANDLE_T		*parentHandle,
				char				*objectName,
				int32				objectType,
				int32				openFlags,
				SPI_HANDLE_T		**objectHandle,
				NWSI_NAME_SPACE_T	*nameSpaceInfo,
				enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	ncp_task_t			*task; 
	ncp_channel_t		*channel;
	SPI_TASK_T			*gTask = NULL;
	SPI_SERVICE_T		*service;
	NCP_DIRHANDLE_T		*ncpParentDirHandle, *ncpTmpDirHandle;
	void_t				*tmpObjectHandle;
	ncp_volume_t		*ncpVolumeHandle;
	uint32				stamp;
	SPI_HANDLE_T		*newHandle;
	pl_t				pl;

	NVLT_ENTER (9);

	if ((parentHandle == NULL && objectName == NULL) && 
			nameSpaceInfo->nodeNumber == -1) {
		*diagnostic = SPI_BAD_ARGS;
		return (NVLT_LEAVE (FAILURE));
	}

	if (((objectType == NS_ROOT) || (objectType == NS_DIRECTORY)) &&
			(nameSpaceInfo->nodeNumber == -1)) {
		/*
		 *	When openning directories, we must get the unique node number
		 *	associated with the directory.
		 */
		*diagnostic = SPI_BAD_ARGS;
		return (NVLT_LEAVE (FAILURE));
	}

	*diagnostic = SUCCESS;
	*objectHandle = (SPI_HANDLE_T *)NULL;

	pl = RW_RDLOCK (nucUpperLock, plstr);

	if (spiState != SPI_LAYER_ACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);

	/*
	 *	Special case that will not validate the parent
	 *	handle if the the object is a ROOT object that has no
	 *	parent.
	 *
	 *	If not a root object, there will be a SPI_TASK_T associated
	 *	with the parent node.  Retrieve it.
	 */
	if (objectType != NS_ROOT && parentHandle != NULL) {
		NVLT_ASSERT (parentHandle != (SPI_HANDLE_T *)NULL);
		NVLT_ASSERT (parentHandle->stamp == SPI_DHANDLE_STAMP);

		NWslGetHandleSProtoResHandle (parentHandle, &ncpParentDirHandle);
		gTask = parentHandle->spilTaskPtr;
		SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	} else {
		ncpParentDirHandle = (NCP_DIRHANDLE_T *)NULL;
		service = volHandle->localResHandle;

		/*
		 *	We don't have the SPI_TASK_T, search for it using credentials of
		 *	the caller.  If this search fails, return a diagnostic.
		 */
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

	/*
	 *	Given the name space objectType, derive the correct stamp
	 *	type for handle creation
	 */
	NWslGetHandleStampType (objectType, (int32 *)&stamp);
	if (NWslAllocHandle (&newHandle, stamp)) {
		*diagnostic = SPI_MEMORY_EXHAUSTED;
		ccode = FAILURE;
		goto done;
	}

	NWslGetTaskProtoHandle (gTask, &task);
	NWslGetHandleSProtoResHandle (volHandle, &ncpVolumeHandle);
	channel = task->channelPtr;

	switch (objectType) {
		case NS_FILE:
		case NS_SYMBOLIC_LINK:
			ccode = NCPsp3NOpenFile2_l (channel, ncpVolumeHandle,
					ncpParentDirHandle, objectName, openFlags, nameSpaceInfo,
					(uint8 **)&tmpObjectHandle, credPtr);
			break;

		case NS_DIRECTORY:
		case NS_ROOT:
			/*
			 *	Creating a handle to the directory specified.  Allocate a
			 *	NCP_SIRHANDLE_T.
			 */
			ncpTmpDirHandle = (NCP_DIRHANDLE_T *)
					kmem_alloc (sizeof (NCP_DIRHANDLE_T), KM_SLEEP);
			ncpTmpDirHandle->DirectoryBase = nameSpaceInfo->nodeNumber;
			ccode = SUCCESS;
			break;

		default:
			ccode = SPI_TYPE_NOT_SUPPORTED;
			break;
	}

	if (ccode != SUCCESS) {
		if ((ccode == SPI_SERVER_UNAVAILABLE) ||
				(ccode == SPI_NO_CONNECTIONS_AVAILABLE) ||
				(ccode == SPI_BAD_CONNECTION) || (ccode == SPI_SERVER_DOWN)) {
			gTask->mode |= SPI_TASK_DRAINING;
		}
		NWslFreeHandle (newHandle);
		*diagnostic = ccode;
		ccode = FAILURE;
		goto done;
	}

	if ((objectType == NS_ROOT) || (objectType == NS_DIRECTORY)) {
		NWslSetHandleSProtoResHandle (newHandle, ncpTmpDirHandle);
	} else {
		NWslSetHandleSProtoResHandle (newHandle, tmpObjectHandle);
	}

	/*
	 *	Bump the resource count in the SPI_TASK_T structure and keep a
	 *	pointer to the SPI_TASK_T in the object handle
	 */
	gTask->resourceCount++;
	NVLT_PRINTF ("NWsiOpenNode: task->resourceCount = %d\n", 
			gTask->resourceCount, 0, 0);
	newHandle->spilTaskPtr = gTask;
	*objectHandle = newHandle;
	NVLT_PRINTF ("NWsiOpenNode: objectHandle = 0x%x\n", *objectHandle, 0, 0);
	*diagnostic = SUCCESS;

done:
	/*
	 *	Indicate that this task is no longer in use.
	 */
	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	NVLT_LEAVE (*diagnostic);
	return (ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiCloseNode.3k)
 * NAME
 *    NWsiCloseNode - Close a file system object (file or directory)
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiCloseNode ( nwcred_t         *credPtr,
 *                    SPI_HANDLE_T     *volHandle,
 *                    SPI_HANDLE_T     *objectHandle,
 *                    int32            objectType,
 *                    enum  NUC_DIAG   *diagnostic )
 *
 * INPUT
 *    credPtr      - Credentials of the UNIX user closing the file/directory.
 *    volHandle    - Volume handle of the volume that the file/directory
 *                   resides in.
 *    objectHandle - File/directory handle of the file/directory to be closed.
 *    objectType   - Node type.
 *                       NS_ROOT
 *                       NS_DIRECTORY
 *                       NS_FILE
 *                       NS_SYMBOLIC_LINK
 * OUTPUT
 *    diagnostic   - Set to one of the following if an error occurs:
 *                       SPI_INACTIVE
 *                       SPI_AUTHENTICATION_FAILURE
 *                       SPI_CLIENT_RESOURCE_SHORTAGE
 *                       SPI_SERVER_UNAVAILABLE
 *                       SPI_TYPE_NOT_SUPPORTED
 *                       SPI_NO_CONNECTIONS_AVAILABLE
 *                       SPI_BAD_CONNECTION
 *                       SPI_SERVER_DOWN
 *                       SPI_ACCESS_DENIED
 *
 * RETURN VALUES
 *    SUCCESS      - Successful completion.
 *    FAILURE      - Unsuccessful completion, diagnostic contains the reason.
 *
 * DESCRIPTION
 *    Closes a previously opened file system node.
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiCloseNode (	nwcred_t			*credPtr,
				SPI_HANDLE_T		*volHandle,
				SPI_HANDLE_T		*objectHandle,
				int32				objectType,
				enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS, ccode2;
	SPI_TASK_T			*gTask = NULL;
	ncp_task_t	 		*task; 
	ncp_channel_t		*channel;
	void_t				*tmpObjHandle;
	uint32				stamp;
	pl_t				pl;

	NVLT_ENTER (5);

	*diagnostic = SUCCESS;

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState != SPI_LAYER_ACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	/*
	 *	These functions ASSERT handle sanity
	 */
	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);

	switch (objectType) {
		case NS_FILE:
		case NS_SYMBOLIC_LINK:
			stamp = SPI_FHANDLE_STAMP;
			break;

		case NS_DIRECTORY:
		case NS_ROOT:
			stamp = SPI_DHANDLE_STAMP;
			break;

		default:
			cmn_err (CE_PANIC, "NWsiCloseNode: bad objectType\n");
			break;
	}

	NVLT_ASSERT (objectHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (objectHandle->stamp == stamp);

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	gTask = objectHandle->spilTaskPtr;
	if (gTask == NULL) {
		/*
		 * There is no task associated with this objectHandle.
		 */
		SLEEP_UNLOCK (spiTaskListSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE; 
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	/*
	 *	NUCFS file system does not care if the NWsiCloseNode is successful
	 *	or not.  It can not do anything about failure case anyway. From this
	 *	point out, We need to make sure that the gTask->resourceCount is
	 *	decremented even if there is an error.
	 */
	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);

	/*
	 *	Indicate that this task is in use by incrementing the useCount
	 *	of the SPI_TASK_T.  NWslSetTaskInUse is not used here because
	 *	if the task is draining that function will fail.  CloseNode is
	 *	the only task that should proceed on a draining task since the
	 *	NCP layer needs to free resources.  A future modification will
	 *	permit the SPIL layer to somehow free the resource.
	 */
	pl = LOCK (gTask->spiTaskLock, plstr);
	gTask->useCount++;
	UNLOCK (gTask->spiTaskLock, pl);

	NWslGetTaskProtoHandle (gTask, &task);
	NWslGetHandleSProtoResHandle (objectHandle, &tmpObjHandle);

	channel = task->channelPtr;

	switch (objectType) {
		case NS_FILE:
		case NS_SYMBOLIC_LINK:
			ccode = NCPsp2XCloseFile_l (channel, tmpObjHandle);
			break;

		case NS_DIRECTORY:
		case NS_ROOT:
			ccode = NCPsp3NDeleteDirectoryHandle_l (channel, tmpObjHandle);
			break;
	}

	/*
	 *	We do not care if the close NCP was successful or not.  We will
	 *	continue with the clean up.  However if we get any of the connection
	 *	errors, we will set the gTask->mode to SPI_TASK_DRAINING.
	 */
	if (ccode != SUCCESS) {
		if ((ccode == SPI_SERVER_UNAVAILABLE) ||
				(ccode == SPI_NO_CONNECTIONS_AVAILABLE) ||
				(ccode == SPI_BAD_CONNECTION) || (ccode == SPI_SERVER_DOWN)) {
			gTask->mode |= SPI_TASK_DRAINING;
		}
		*diagnostic = ccode;
		ccode = FAILURE;
	}

	NVLT_PRINTF ("NWsiCloseNode:  handle at %x\n", objectHandle, 0, 0);
	NWslFreeHandle (objectHandle);

	/*
	 *	Decrement the resource count in the SPI_TASK_T structure
	 */
	NVLT_ASSERT (gTask->resourceCount > 0);
	gTask->resourceCount--;
	NVLT_PRINTF ("NWsiCloseNode: task->resourceCount = %d\n", 
			gTask->resourceCount, 0, 0);

	/*
	 *	Indicate that this task is no longer in use
	 */
	NWslSetTaskNotInUse_l (gTask);

	/*
	 *	If the task is in DRAIN status (waiting to be freed after all
	 *	resources are freed) and the resource count is now zero, call
	 *	NWsiCloseService to perform final cleanup.
	 *
	 *
	 *  Blow away connection when reference count and
	 *  resource count go to 0 and one of the following are true:
	 *  1. the connection is a private connection
	 *  2. the connection is a public connection that is not permanent
	 *  3. the connection is a public connection that is not authenticated
	 *  4. the connection is a public connection that is not primary.
	 *
	 *  Note: requester uses referenceCount and nucfs uses resourceCount.
	 */
	if ((channel->referenceCount == 0) &&
			(gTask->mode & SPI_TASK_DRAINING) &&
			(gTask->resourceCount == 0) &&
			((channel->uPublicState & NWC_CONN_PRIVATE) ||
			!((gTask->mode & SPI_TASK_PERMANENT) ||
			  (gTask->mode & SPI_TASK_AUTHENTICATED) ||
			  (gTask->mode & SPI_PRIMARY_SERVICE)))) {
		NVLT_PRINTF ("NWsiCloseNode: task at %x to be freed.\n", gTask, 0, 0);
		(void)NWsiCloseServiceWithTaskPtr_l (gTask);
	}
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);
	SLEEP_UNLOCK (spiTaskListSleepLock);

	NVLT_LEAVE (*diagnostic);
	return (ccode);
}


/*
 * BEGIN_MANUAL_ENTRY(NWsiCreateNode.3k)
 * NAME
 *    NWsiCreateNode - Create a file system object (file or directory)
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiCreateNode ( nwcred_t          *credPtr,
 *                     SPI_HANDLE_T      *volHandle,
 *                     SPI_HANDLE_T      *parentHandle,
 *                     char              *objectName,
 *                     int32             openFlags,
 *                     SPI_HANDLE_T      **objectHandle,
 *                     NWSI_NAME_SPACE_T *nameSpaceInfo,
 *                     enum   NUC_DIAG   *diagnostic )
 *
 * INPUT
 *    credPtr              - Credentials of the UNIX user creating the
 *                           file/directory.
 *    volHandle            - Volume handle of the volume that the file/directory
 *                           resides in.
 *    parentHandle         - Parent directory handle of the file/directory to be
 *                           created.
 *    objectName           - Name of the file/directory to be created.
 *    openFlags            - Open flags:
 *                               NW_READ
 *                               NW_WRITE
 *                               NW_DENY_READ
 *                               NW_DENY_WRITE
 *                               NW_EXCLUSIVE
 *                               NW_FAIL_IF_NODE_EXISTS
 *                               NW_CHECK_PARENT_SETGID
 *                               NW_INHERIT_PARENT_GID
 *    nameSpaceInfo
 *       nodeType          - Node type.
 *                               NS_DIRECTORY
 *                               NS_FILE
 *                               NS_SYMBOLIC_LINK
 *
 * OUTPUT
 *    objectHandle         - Returned file handle.  If creating a directory,
 *                           the directory handle is not returned.
 *    nameSpaceInfo 
 *       nodeNumber        - Unique object identifier.
 *       linkNodeNumber    - Unique object identifier of the link.
 *                           regular file implies:      
 *                               nodeNumber = linkNodeNumber.
 *       nodeType          - Object type.
 *       nodePermissions   - Object permissions.
 *       nodeNumberOfLinks - Object number of links.
 *       nodeSize;         - Size in bytes of data space.
 *       userID;           - User identifier of owner.
 *       groupID;          - Group identifier of owner.
 *       accessTime;       - Time of last access .
 *       modifyTime;       - Time of last data modification.
 *       changeTime;       - Time of last name space changed.
 *                           Times measured in seconds since 00:00:00 GMT,
 *                           Jan 1, 1970.
 *    diagnostic           - Set to one of the following if an error occurs:
 *                               SPI_INACTIVE
 *                               SPI_AUTHENTICATION_FAILURE
 *                               SPI_CLIENT_RESOURCE_SHORTAGE
 *                               SPI_SERVER_RESOURCE_SHORTAGE
 *                               SPI_SERVER_UNAVAILABLE
 *                               SPI_TYPE_NOT_SUPPORTED
 *                               SPI_NO_CONNECTIONS_AVAILABLE
 *                               SPI_BAD_CONNECTION
 *                               SPI_SERVER_DOWN
 *                               SPI_ACCESS_DENIED
 *                               SPI_DIRECTORY_FULL
 *                               SPI_INVALID_PATH
 *
 * RETURN VALUES
 *    SUCCESS        - Successful completion.
 *    FAILURE        - Unsuccessful completion, diagnostic contains the reason.
 *
 * DESCRIPTION
 *    Creates a file or directory object.
 *
 * NOTES
 *    Will not allow creation of a root directory object.  An object handle is
 *    returned for types NS_FILE and NS_SYMBOLIC only.
 *
 *    The nameSpaceInfo->nodeType must contain a valid object type, such as
 *    NS_FILE, NS_SYMBOLIC or NS_DIRECTORY before calling this function.
 *
 * SEE ALSO
 *    NWsiOpenNode(3k), NWsiCloseNode(3k), NWsiDeleteNode(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiCreateNode (	nwcred_t			*credPtr,
					SPI_HANDLE_T		*volHandle,
					SPI_HANDLE_T		*parentHandle,
					char				*objectName,
					int32				openFlags,
					SPI_HANDLE_T		**objectHandle,
					NWSI_NAME_SPACE_T	*nameSpaceInfo,
					enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	SPI_TASK_T			*gTask = NULL;
	SPI_HANDLE_T		*newHandle = NULL; 
	ncp_task_t			*task;
	ncp_channel_t		*channel;
	void_t				*tmpObjectHandle;
	NCP_DIRHANDLE_T		*ncpParentDirHandle;
	ncp_volume_t		*ncpVolumeHandle;
	uint32				stamp;
	pl_t				pl;

	NVLT_ENTER (8);

	*diagnostic = SUCCESS;

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState != SPI_LAYER_ACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);
	NVLT_ASSERT (parentHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (parentHandle->stamp == SPI_DHANDLE_STAMP);
	NVLT_ASSERT (objectName != (char *)NULL);

	gTask = parentHandle->spilTaskPtr;
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
	 *	Indicate that this task is in use
	 */
	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	if (nameSpaceInfo->nodeType != NS_DIRECTORY) {
		NWslGetHandleStampType (nameSpaceInfo->nodeType, (int32 *)&stamp);
		if (NWslAllocHandle (&newHandle, stamp)) {
			*diagnostic = SPI_MEMORY_EXHAUSTED;
			ccode = FAILURE;
			goto done;
		}
	}

	NWslGetTaskProtoHandle (gTask, &task);

	NWslGetHandleSProtoResHandle (volHandle, &ncpVolumeHandle);
	NWslGetHandleSProtoResHandle (parentHandle, &ncpParentDirHandle);

	channel = task->channelPtr;

	switch (nameSpaceInfo->nodeType) {
		case NS_FILE:
		case NS_DIRECTORY:
		case NS_SYMBOLIC_LINK:
			/*
			 *	Create the file or directory
			 */
			ccode = NCPsp3NCreateFileOrSubdirectory2_l (channel,
					ncpVolumeHandle, ncpParentDirHandle, objectName,
					openFlags, (uint8 **)&tmpObjectHandle, nameSpaceInfo,
					credPtr);

			break;

		case NS_ROOT:
			ccode = SPI_ACCESS_DENIED;
			break;

		default:
			ccode = SPI_TYPE_NOT_SUPPORTED;
			break;
	}

	if (ccode != SUCCESS) {
		if ((ccode == SPI_SERVER_UNAVAILABLE) ||
				(ccode == SPI_NO_CONNECTIONS_AVAILABLE) ||
				(ccode == SPI_BAD_CONNECTION) || (ccode == SPI_SERVER_DOWN)) {
			gTask->mode |= SPI_TASK_DRAINING;
		}
		if (nameSpaceInfo->nodeType != NS_DIRECTORY)
			NWslFreeHandle (newHandle);
		*diagnostic = ccode;
		ccode = FAILURE;
		goto done;
	}

	/*
	 *	Set the handle returned from the service provider
	 *	in the interface handle if a handle was returned (no handle
	 *	returned in the case of a create directory request).
	 */
	if (nameSpaceInfo->nodeType != NS_DIRECTORY) {
		*objectHandle = newHandle;
		NWslSetHandleSProtoResHandle (*objectHandle, tmpObjectHandle);

#ifdef NUC_DEBUG
		cmn_err (CE_NOTE, "NWsiCreateNode:  handle at %x for %s\n", 
			newHandle, objectName );
#endif

		/*
		 *	Bump the resource count in the SPI_TASK_T structure and keep a
		 *	pointer to the SPI_TASK_T in the object handle
		 */
		gTask->resourceCount++;
		NVLT_PRINTF ("NWsiCreateNode: task->resourceCount = %d\n",
				gTask->resourceCount, 0, 0);
		(*objectHandle)->spilTaskPtr = gTask;
	} else
		*objectHandle = NULL;

done:
	/*
	 *	Indicate that this task is no longer in use
	 */
	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	NVLT_LEAVE (*diagnostic);
	return (ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiDeleteNode.3k)
 * NAME
 *    NWsiDeleteNode - Remove a file system object
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiDeleteNode ( nwcred_t        *credPtr,
 *                     SPI_HANDLE_T    *volHandle,
 *                     SPI_HANDLE_T    *parentHandle,
 *                     char            *objectName,
 *                     int32           objectType,
 *                     int32           objectID,
 *                     enum  NUC_DIAG  *diagnostic )
 *
 * INPUT
 *    credPtr       - Credentials of the UNIX user deleting the file/directory.
 *    volHandle     - Volume handle of the volume that the file/directory
 *                    resides in.
 *    parentHandle  - Parent directory handle of the file/directory to be
 *                    deleted.
 *    objectName    - Name of the file/directory to be deleted.
 *    objectType    - Node type.
 *                        NS_DIRECTORY
 *                        NS_FILE
 *                        NS_SYMBOLIC_LINK
 *    objcetID      - If not set to -1, identifies the unique ID of the
 *                    file/directory to be deleted.  The unique node ID is
 *                    used instead of the parentHandle/objectName combination
 *                    if passed in.
 * 
 * OUTPUT
 *    diagnostic    - Set to one of the following if an error occurs:
 *                        SPI_INACTIVE
 *                        SPI_AUTHENTICATION_FAILURE
 *                        SPI_CLIENT_RESOURCE_SHORTAGE
 *                        SPI_SERVER_UNAVAILABLE
 *                        SPI_TYPE_NOT_SUPPORTED
 *                        SPI_NO_CONNECTIONS_AVAILABLE
 *                        SPI_BAD_CONNECTION
 *                        SPI_SERVER_DOWN
 *                        SPI_ACCESS_DENIED
 *                        SPI_DIRECTORY_NOT_EMPTY
 *                        SPI_INVALID_PATH
 *
 * RETURN VALUES
 *    SUCCESS       - Successful completion.
 *    FAILURE       - Unsuccessful completion, diagnostic contains the reason.
 *
 * DESCRIPTION
 *    Removes a file sytem object if the permissions are sufficient.  Deletes
 *    files/directories as specified by the type field.
 *
 * SEE ALSO
 *    NWsiCreateNode(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiDeleteNode (	nwcred_t			*credPtr,
					SPI_HANDLE_T		*volHandle,
					SPI_HANDLE_T		*parentHandle,
					char				*objectName,
					int32				objectType,
					int32				objectID,
					enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	SPI_TASK_T			*gTask = NULL;
	SPI_SERVICE_T		*service;
	ncp_task_t			*task; 
	ncp_channel_t		*channel;
	NCP_DIRHANDLE_T		*ncpDirHandle = NULL;
	ncp_volume_t		*ncpVolumeHandle;
	pl_t				pl;

	NVLT_ENTER (7);

	*diagnostic = SUCCESS;

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState != SPI_LAYER_ACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);

	if (parentHandle != NULL) {
		gTask = parentHandle->spilTaskPtr;
		SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	} else {
		/*
		 *	We don't have the SPI_TASK_T, search for it using credentials of
		 *	the caller.  If this search fails, return a diagnostic.
		 */
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
	NWslGetHandleSProtoResHandle (volHandle, &ncpVolumeHandle);
	if (parentHandle)
		NWslGetHandleSProtoResHandle (parentHandle, &ncpDirHandle);
	channel = task->channelPtr;

	switch (objectType)	{
		case NS_DIRECTORY:
		case NS_FILE:
		case NS_SYMBOLIC_LINK:
			ccode = NCPsp3NDeleteFileOrDirectory2_l (channel, ncpVolumeHandle,
					ncpDirHandle, objectName, objectType, objectID, credPtr );
			break;
		default:
			ccode = SPI_TYPE_NOT_SUPPORTED;
			break;
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
 * BEGIN_MANUAL_ENTRY(NWsiRenameNode(3k), \
 *                   ./man/kernel/spil/RenameNode)
 * NAME
 *    NWsiRenameNode - Rename a node.
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiRenameNode ( nwcred_t           *credPtr,
 *                     SPI_HANDLE_T       *volumeHandle,
 *                     SPI_HANDLE_T       *oldParentHandle,
 *                     char               *oldName,
 *                     NWSI_NAME_SPACE_T  *nameSpaceInfo,
 *                     SPI_HANDLE_T       *newParentHandle,
 *                     char               *newName,
 *                     enum   NUC_DIAG    *diagnostic )
 *
 * INPUT
 *    credPtr              - Credentials of the UNIX user renaming the
 *                           file/directory.
 *    volHandle            - Volume handle of the volume that the file/directory
 *                           resides in.
 *    oldParentHandle      - Parent directory handle of the existing 
 *                           file/directory to be renamed.
 *    oldName              - Old node name.
 *    nameSpaceInfo
 *       nodeNumber        - If not set to -1, identifies the unique ID of the
 *                           existing file/directory to be renamed.  The unique
 *                           ID is used instead of the oldParentHandle/oldName
 *                           combination if passed in.
 *       nodePermissions   - If not set to -1 the attributes of the renamed
 *                           file will be changed to the specified permissions.
 *                           This is in place mostly for setting the hidden
 *                           attribute if the file/directory was renamed to a
 *                           hidden file/directory.
 *    newParentHandle      - Parent directory handle of the new file/directory.
 *    newName              - New node name.
 *
 * OUTPUT
 *    diagnostic           - Set to one of the following if an error occurs:
 *                               SPI_INACTIVE
 *                               SPI_BAD_ARGS
 *                               SPI_AUTHENTICATION_FAILURE
 *                               SPI_CLIENT_RESOURCE_SHORTAGE
 *                               SPI_SERVER_UNAVAILABLE
 *                               SPI_GENERAL_FAILURE
 *                               SPI_TYPE_NOT_SUPPORTED
 *                               SPI_NO_CONNECTIONS_AVAILABLE
 *                               SPI_BAD_CONNECTION
 *                               SPI_SERVER_DOWN
 *                               SPI_ACCESS_DENIED
 *                               SPI_INVALID_PATH
 *                               SPI_FILE_IN_USE
 *                               SPI_SET_NAME_SPACE_DENIED
 *
 * RETURN VALUES
 *    SUCCESS        - Successful completion.
 *    FAILURE        - Unsuccessful completion, diagnostic contains the reason.
 *
 * DESCRIPTION
 *    Renames the node associated with the specified oldName under the
 *    oldParentHandle directory to the specified newName in the newParentHandle
 *    directory.  
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiRenameNode (	nwcred_t  			*credPtr,
					SPI_HANDLE_T		*volumeHandle,
					SPI_HANDLE_T		*oldParentHandle,
					char				*oldName,
					NWSI_NAME_SPACE_T	*nameSpaceInfo,
					SPI_HANDLE_T		*newParentHandle,
					char				*newName,
					enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	SPI_TASK_T			*gTask = NULL;
	ncp_task_t			*task; 
	ncp_channel_t		*channel;
	ncp_volume_t		*ncpVolHandle;
	NCP_DIRHANDLE_T		*ncpOldParentHandle = NULL, *ncpNewParentHandle;
	pl_t				pl;

	NVLT_ENTER (8);

	*diagnostic = SUCCESS;

	NVLT_ASSERT (volumeHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volumeHandle->stamp == SPI_VOLUME_STAMP);
	NVLT_ASSERT (newParentHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (newParentHandle->stamp == SPI_DHANDLE_STAMP);
	NVLT_ASSERT (newName != (char *)NULL);

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	/*
	 * if oldParentHandle is NULL then the nameSpaceInfo->nodeNumber is set
	 * to the uniqueID of the existing file/directory. The nameSpaceInfo->
	 * nodeNumber will be set to the unique ID of the renamed file/directory.
	 */
	if (oldParentHandle == NULL && oldName == NULL &&
			nameSpaceInfo->nodeNumber == -1) {
		*diagnostic = SPI_BAD_ARGS; 
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	/*
	 *	If we don't have the SPI_TASK_T, search for it using credentials
	 *	of the caller.  If this search fails, return a diagnostic.
	 */
	gTask = newParentHandle->spilTaskPtr;
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
	 *	Indicate that this task is in use
	 */
	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		*diagnostic = SPI_AUTHENTICATION_FAILURE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	NWslGetTaskProtoHandle (gTask, &task);
	NWslGetHandleSProtoResHandle (volumeHandle, &ncpVolHandle);
	if (oldParentHandle)
		NWslGetHandleSProtoResHandle (oldParentHandle, &ncpOldParentHandle);
	NWslGetHandleSProtoResHandle (newParentHandle, &ncpNewParentHandle);
	channel = task->channelPtr;

	/*
	 * Have the name space ops rename the old node
	 */
	switch (nameSpaceInfo->nodeType)	{
		case NS_FILE:
		case NS_SYMBOLIC_LINK:
		case NS_DIRECTORY:
			ccode = NCPsp3NRenameFileOrDirectory2_l (channel, ncpVolHandle,
					ncpOldParentHandle, oldName, NCP_UNIX_NAME_SPACE,
					nameSpaceInfo, ncpNewParentHandle, newName, credPtr);

			if (ccode)
				break;

			if (nameSpaceInfo->nodePermissions != (uint32)-1) {
				/*
				 * The attributes of the new node must be set to the specified
				 * node permissions.
				 */
				nameSpaceInfo->nodeNumber = -1;
				ccode = NCPsp3NSetNameSpaceInformation2_l (channel,
						ncpVolHandle, ncpNewParentHandle, newName,
						nameSpaceInfo, credPtr);
			}

			break;

		default:
			ccode = SPI_TYPE_NOT_SUPPORTED;
			break;
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
 * BEGIN_MANUAL_ENTRY(NWsiGetNameSpaceInfo.3k)
 * NAME
 *    NWsiGetNameSpaceInfo - Get name space information for a file system
 *                           object.
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiGetNameSpaceInfo ( nwcred_t           *credPtr,
 *                           SPI_HANDLE_T       *volHandle,
 *                           SPI_HANDLE_T       *parentHandle,
 *                           char               *objectName,
 *                           NWSI_NAME_SPACE_T  *nameSpaceInfo,
 *                           enum   NUC_DIAG    *diagnostic )
 *
 * INPUT
 *    credPtr              - Credentials of the UNIX user getting the name
 *                           space of a file/directory.
 *    volHandle            - Volume handle of the volume that the file/directory
 *                           resides in.
 *    parentHandle         - Parent directory handle of the file/directory.
 *    objectName           - Name of the file/directory.
 *    nameSpaceInfo
 *       nodeType          - Node type.
 *       nodeNumber        - If not set to -1, identifies the unique ID of the
 *                           file/directory to get the name space information.
 *                           The unique ID is used instead of the parentHandle/
 *                           oldName combination if passed in.
 *
 * OUTPUT
 *    nameSpaceInfo 
 *       nodeNumber        - Unique object identifier.
 *       linkNodeNumber    - Unique object identifier of the link.
 *                           regular file implies:      
 *                               nodeNumber = linkNodeNumber.
 *       nodeType          - Object type.
 *       nodePermissions   - Object permissions.
 *       nodeNumberOfLinks - Object number of links.
 *       nodeSize;         - Size in bytes of data space.
 *       userID;           - User identifier of owner.
 *       groupID;          - Group identifier of owner.
 *       accessTime;       - Time of last access .
 *       modifyTime;       - Time of last data modification.
 *       changeTime;       - Time of last name space changed.
 *                           Times measured in seconds since 00:00:00 GMT,
 *                           Jan 1, 1970.
 *       openFileHandle    - If not set to -1, we know the file is open. an
 *                           extra NCP is generated to get the actual file
 *                           size of the file, not the cached file size.
 *    diagnostic           - Set to one of the following if an error occurs:
 *                               SPI_INACTIVE
 *                               SPI_BAD_ARGS
 *                               SPI_AUTHENTICATION_FAILURE
 *                               SPI_CLIENT_RESOURCE_SHORTAGE
 *                               SPI_SERVER_UNAVAILABLE
 *                               SPI_NO_ACTUAL_SIZE
 *                               SPI_NO_CONNECTIONS_AVAILABLE
 *                               SPI_BAD_CONNECTION
 *                               SPI_SERVER_DOWN
 *                               SPI_ACCESS_DENIED
 *                               SPI_BAD_ARGS_TO_SERVER
 *                               SPI_INVALID_PATH
 *                               SPI_BAD_HANDLE
 *
 * RETURN VALUES
 *    SUCCESS        - Successful completion.
 *    FAILURE        - Unsuccessful completion, diagnostic contains the reason.
 *
 * DESCRIPTION
 *    Returns the name space information on a NetWare node associated with
 *    the specified parentHandle/objectName or the nameSpaceInfo->nodeNumber.
 *
 * SEE ALSO
 *    NWsiSetNameSpaceInfo(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiGetNameSpaceInfo (	nwcred_t			*credPtr,
						SPI_HANDLE_T		*volHandle,
						SPI_HANDLE_T		*parentHandle,
						char				*objectName,
						NWSI_NAME_SPACE_T	*nameSpaceInfo,
						enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	SPI_TASK_T			*gTask = NULL;
	SPI_SERVICE_T		*service;
	ncp_task_t			*task; 
	ncp_channel_t		*channel;
	NCP_DIRHANDLE_T		*ncpParentDirHandle = NULL;
	ncp_volume_t		*ncpVolumeHandle;
	pl_t				pl;

	NVLT_ENTER (6);

	*diagnostic = SUCCESS;

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState != SPI_LAYER_ACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);

	if (parentHandle == NULL && objectName == NULL &&
			nameSpaceInfo->nodeNumber == -1) {
		*diagnostic = SPI_BAD_ARGS;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
			
	if (objectName != NULL && objectName[0]=='.' && objectName[1]=='.' &&
			objectName[2]=='\0') {
		/*
		 * The objectName can not be "..". The NWsiGetParentNameSpaceInfo
		 * should be called for this case.
		 */
		*diagnostic = SPI_BAD_ARGS;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	/*
	 *	If we dont have the SPI_TASK_T, search for it using credentials
	 *	of the caller.  If this search fails, return a diagnostic.
	 */
	if (parentHandle != NULL) {
		gTask = parentHandle->spilTaskPtr;
		SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	} else {
		/*
		 *	We don't have the SPI_TASK_T, search for it using credentials of
		 *	the caller.  If this search fails, return a diagnostic.
		 */
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
	}

	NVLT_PRINTF ("NWsiGetNameSpaceInfo: auth state = 0x%x\n",
			NWslCheckAuthState (gTask), 0, 0);

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
	NWslGetHandleSProtoResHandle (volHandle, &ncpVolumeHandle);
	if (parentHandle)
		NWslGetHandleSProtoResHandle (parentHandle, &ncpParentDirHandle);

	channel = task->channelPtr;

	ccode = NCPsp3NGetNameSpaceInformation2_l (channel, ncpVolumeHandle,
			ncpParentDirHandle, objectName, nameSpaceInfo, credPtr);

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
 * BEGIN_MANUAL_ENTRY(NWsiSetNameSpaceInfo.3k)
 * NAME
 *		NWsiSetNameSpaceInfo - Set name space information for this 
 *								object.
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiSetNameSpaceInfo ( nwcred_t           *credPtr,
 *                           SPI_HANDLE_T       *volHandle,
 *                           SPI_HANDLE_T       *parentHandle,
 *                           char               *objectName,
 *                           NWSI_NAME_SPACE_T  *nameSpaceInfo,
 *                           enum   NUC_DIAG    *diagnostic )
 * 
 * INPUT
 *    credPtr              - Credentials of the UNIX user setting the name
 *                           space of a file/directory.
 *    volHandle            - Volume handle of the volume that the file/directory
 *                           resides in.
 *    parentHandle         - Parent directory handle of the file/directory.
 *    objectName           - Name of the file/directory.
 *    nameSpaceInfo 
 *       nodeType          - Object type.
 *       nodeNumber        - If not set to -1, identifies the unique ID of the
 *                           file/directory to set the name space information.
 *                           The unique ID is used instead of the parentHandle/
 *                           oldName combination if passed in.
 *       nodePermissions   - New object permissions.
 *       userID;           - New user identifier of owner.
 *       groupID;          - New group identifier of owner.
 *       accessTime;       - New access time.
 *       modifyTime;       - new modification time.
 *       changeTime;       - new change time.
 *
 * OUTPUT
 *    diagnostic           - Set to one of the following if an error occurs:
 *                               SPI_INACTIVE
 *                               SPI_BAD_ARGS
 *                               SPI_AUTHENTICATION_FAILURE
 *                               SPI_CLIENT_RESOURCE_SHORTAGE
 *                               SPI_SERVER_UNAVAILABLE
 *                               SPI_NO_ACTUAL_SIZE
 *                               SPI_NO_CONNECTIONS_AVAILABLE
 *                               SPI_BAD_CONNECTION
 *                               SPI_SERVER_DOWN
 *                               SPI_ACCESS_DENIED
 *                               SPI_INVALID_PATH
 *                               SPI_SET_NAME_SPACE_DENIED
 *
 * RETURN VALUES
 *    SUCCESS        - Successful completion.
 *    FAILURE        - Unsuccessful completion, diagnostic contains the reason.
 *
 * DESCRIPTION
 *    Sets the name space information of a NetWare node associated with the
 *    specified parentHandle/objectName or the nameSpaceInfo->nodeNumber.
 *    Certain name space information fields (userID, groupID, nodePermissions,
 *    accessTime, modifyTime, and changeTime) can only be set with this
 *    operation.
 *
 * SEE ALSO
 *    NWsiGetNameSpaceInfo(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiSetNameSpaceInfo (	nwcred_t			*credPtr,
						SPI_HANDLE_T		*volHandle,
						SPI_HANDLE_T		*parentHandle,
						char				*objectName,
						NWSI_NAME_SPACE_T	*nameSpaceInfo,
						enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	SPI_TASK_T			*gTask = NULL;
	SPI_SERVICE_T		*service;
	ncp_task_t			*task; 
	ncp_channel_t		*channel;
	NCP_DIRHANDLE_T		*ncpParentHandle = NULL;
	ncp_volume_t		*ncpVolumeHandle;
	pl_t				pl;

	NVLT_ENTER (6);

	*diagnostic = SUCCESS;

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if ( spiState != SPI_LAYER_ACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);

	if (parentHandle == NULL && objectName == NULL &&
			nameSpaceInfo->nodeNumber == -1) {
		*diagnostic = SPI_BAD_ARGS;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	/*
	 *	If we dont have the SPI_TASK_T, search for it using credentials
	 *	of the caller.  If this search fails, return a diagnostic.
	 */
	if (parentHandle != NULL) {
		gTask = parentHandle->spilTaskPtr;
		SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	} else {
		/*
		 *	We don't have the SPI_TASK_T, search for it using credentials of
		 *	the caller.  If this search fails, return a diagnostic.
		 */
		service = volHandle->localResHandle;
		SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
		if ((ccode = GetTask_l (service->taskList, credPtr, &gTask, FALSE))
				== SUCCESS) {
			SLEEP_UNLOCK (spiTaskListSleepLock);
			SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
			if (!(gTask->mode & SPI_TASK_AUTHENTICATED)) {
				/*
				 * User not authenticated to the server.
				 */
				SLEEP_UNLOCK (gTask->spiTaskSleepLock);
				*diagnostic = SPI_AUTHENTICATION_FAILURE;
				return (NVLT_LEAVE(FAILURE));
			}
		} else {
			/*
			 * No task associated with this user.
			 */
			SLEEP_UNLOCK (spiTaskListSleepLock);
			*diagnostic = SPI_AUTHENTICATION_FAILURE;
			return (NVLT_LEAVE(FAILURE));
		}
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
	NWslGetHandleSProtoResHandle (volHandle, &ncpVolumeHandle);
	if (parentHandle)
		NWslGetHandleSProtoResHandle (parentHandle, &ncpParentHandle);
	channel = task->channelPtr;

	ccode = NCPsp3NSetNameSpaceInformation2_l (channel, ncpVolumeHandle,
				ncpParentHandle, objectName, nameSpaceInfo, credPtr);

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
 * BEGIN_MANUAL_ENTRY(NWsiAutoAuthenticate.3k)
 * NAME
 *		NWsiAutoAuthenticate - Call Xauto to prompt the user for authentication.
 *
 * SYNOPSIS
 *    enum NUC_DIAG
 *    NWsiAutoAuthenticate ( nwcred_t           *credPtr,
 *                           SPI_HANDLE_T       *volHandle,
 *                           int16              xautoFlags,	
 *                           enum   NUC_DIAG    *diagnostic )
 * 
 * INPUT
 *    credPtr              - Credentials of the UNIX user doing the auto
 *                           authentication.
 *    volHandle            - Volume handle of the volume previously mounted
 *                           and used.
 *    xautoFlags           - If NUC_SINGLE_LOGIN_ONLY bit is set, the xauto
 *                           program would only try the single login and will
 *                           not pop up the Auto Authentication panel and will
 *                           not prompt the user for the authentication
 *                           information.
 *
 * OUTPUT
 *    diagnostic           - Set to one of the following if an error occurs:
 *                               SPI_INACTIVE
 *                               SPI_AUTHENTICATION_FAILURE
 *
 * RETURN VALUES
 *    SUCCESS        - Successful completion.
 *    FAILURE        - Unsuccessful completion, diagnostic contains the reason.
 *
 * DESCRIPTION
 *    Calls the nwlogin thread of the nucd daeamon which calls the xauto
 *    program to prompt the user for NetWare authentication information.
 *    If NUC_SINGLE_LOGIN_ONLY bit is set in xautoFlags, the xauto program
 *    would only try the single login and will not pop up the Auto
 *    Authentication panel and will not prompt the user for the authentication
 *    information.
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiAutoAuthenticate (	nwcred_t			*credPtr,
						SPI_HANDLE_T		*volHandle,
						int16				xautoFlags,	
						enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	SPI_TASK_T			*gTask = NULL;
	SPI_SERVICE_T		*service;
	pl_t				pl;

	NVLT_ENTER (4);

	*diagnostic = SUCCESS;

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState != SPI_LAYER_ACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->localResHandle != (SPI_SERVICE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);

	/*
	 *	Need to check if there is a task (SPI_TASK_T) associated with the
	 *	user.  If not, this could call xauto for authentication to NetWare
	 *	server.
	 */
	service = volHandle->localResHandle;
   	ccode = NWslGetServiceTask (service, credPtr, &gTask, xautoFlags);
	if (ccode) {
		*diagnostic = SPI_AUTHENTICATION_FAILURE; 
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	return (NVLT_LEAVE (SUCCESS));
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiGetServerTime.3k)
 * NAME
 *		NWsiGetServerTime - Get server time.
 *
 * SYNOPSIS
 *   enum NUC_DIAG
 *   NWsiGetServerTime ( nwcred_t          *credPtr,
 *                       SPI_HANDLE_T      *volHandle,
 *                       int32             *serverTime,
 *                       enum   NUC_DIAG   *diagnostic )
 * 
 * INPUT
 *    credPtr     - Credentials of the UNIX user getting the current server
 *                  time.
 *    volHandle   - Volume handle of the volume in use.
 *
 * OUTPUT
 *    serverTime  - Server time.
 *    diagnostic  - Set to one of the following if an error occurs:
 *                      SPI_INACTIVE
 *                      SPI_AUTHENTICATION_FAILURE
 *                      SPI_CLIENT_RESOURCE_SHORTAGE
 *                      SPI_SERVER_UNAVAILABLE
 *                      SPI_NO_CONNECTIONS_AVAILABLE
 *                      SPI_BAD_CONNECTION
 *                      SPI_SERVER_DOWN
 *                      SPI_GENERAL_FAILURE
 *
 * RETURN VALUES
 *    SUCCESS     - Successful completion.
 *    FAILURE     - Unsuccessful completion, diagnostic contains the reason.
 *
 * DESCRIPTION
 *    Returns the NetWare server time.
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiGetServerTime (	nwcred_t			*credPtr,
					SPI_HANDLE_T		*volHandle,
					int32				*serverTime,
					enum	NUC_DIAG	*diagnostic )
{
	enum	NUC_DIAG	ccode = SUCCESS;
	SPI_TASK_T			*gTask = NULL;
	SPI_SERVICE_T		*service;
	ncp_task_t			*task;
	ncp_channel_t		*channel;
	NCP_DATETIME_T		dateTime;
	uint16				dosDate, dosTime;
	pl_t				pl;

	NVLT_ENTER (4);

	*diagnostic = SUCCESS;

	pl = RW_RDLOCK (nucUpperLock, plstr);
	if (spiState != SPI_LAYER_ACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		*diagnostic = SPI_INACTIVE;
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}
	RW_UNLOCK (nucUpperLock, pl);

	NVLT_ASSERT (volHandle != (SPI_HANDLE_T *)NULL);
	NVLT_ASSERT (volHandle->stamp == SPI_VOLUME_STAMP);

	/*
	 *  We don't have a SPI_TASK_T associated with volHandle, search the
	 *	task list using credentials of the caller.  If this search fails,
	 *	return a diagnostic.
	 */
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
		NVLT_LEAVE (*diagnostic);
		return (FAILURE);
	}

	NWslGetTaskProtoHandle (gTask, &task);
	channel = task->channelPtr;

	ccode = NCPspGetServerDateAndTime_l (channel, &dateTime);

	if (ccode != SUCCESS) {
		if ((ccode == SPI_SERVER_UNAVAILABLE) ||
				(ccode == SPI_NO_CONNECTIONS_AVAILABLE) ||
				(ccode == SPI_BAD_CONNECTION) || (ccode == SPI_SERVER_DOWN)) {
			gTask->mode |= SPI_TASK_DRAINING;
		}
		*diagnostic = ccode;
		ccode = FAILURE;
	} else {
		/*
		 * Convert the NetWare date and time to UNIX time.
		 */

		dosDate = ((dateTime.year - 80) << 9) +
				(dateTime.month << 5) +
				dateTime.day;
		dosTime = ((dateTime.hour) << 11) +
				(dateTime.minute << 5) +
				(dateTime.second >>1);

		*serverTime = ConvertDOSTimeDateToSeconds (dosTime, dosDate, 0);
	}

	/*
	 *	Indicate that this task is no longer in use
	 */
	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	NVLT_LEAVE (*diagnostic);
	return (ccode);
}
