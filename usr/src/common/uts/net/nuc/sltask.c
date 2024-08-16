/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/sltask.c	1.24"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/sltask.c,v 2.55.2.10 1995/02/13 20:50:14 hashem Exp $"


/*
 *  Netware Unix Client
 *
 *	MODULE: spitask.c
 *	ABSTRACT: Task data structure routines that are the top level
 *	structures managed in the SPI.
 *
 *	Some things that should be added eventually...
 *
 *	Statitistics on resource usage/task
 *	Diagnostics information per task.
 *
 *	Functions declared in this module:
 *
 *	Public functions:
 *	NWslInitTaskList
 *	NWslCreateTask
 *	NWslFreeTask
 *	NWslScanTasks
 *	NWslCheckTaskListStatus
 *
 *	Private Functions:
 *	GetTask
 *
 */ 

#ifdef _KERNEL_HEADERS
#include <net/tiuser.h>
#include <util/cmn_err.h>
#include <util/param.h>

#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/requester.h>
#include <net/nuc/slstruct.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/nwspiswitch.h>
#include <net/nuc/nwspi.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/spimacro.h>
#include <net/nuc/nwmp.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/sistructs.h>
#include <net/nuc/nwtypes.h>
#include <net/nuc/ncpiopack.h>
#include <net/nw/ntr.h>
#include <net/nuc/nuc_hier.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/tiuser.h>

#include <kdrivers.h>
#include <sys/nuctool.h>
#include <sys/nwctypes.h>
#include <sys/requester.h>
#include <sys/slstruct.h>
#include <sys/nucerror.h>
#include <sys/nwspiswitch.h>
#include <sys/nwspi.h>
#include <sys/slstruct.h>
#include <sys/nwportable.h>
#include <sys/nwctypes.h>
#include <sys/spilcommon.h>
#include <sys/spimacro.h>
#include <sys/nwmp.h>
#include <sys/ncpconst.h>
#include <sys/sistructs.h>
#include <sys/nwtypes.h>
#include <sys/ncpiopack.h>
#include <sys/nuc_hier.h>

#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask	NTRM_spil

LKINFO_DECL(spiTaskLockInfo, "NETNUC:NUC:spiTaskLock", 0);
LKINFO_DECL(spiTaskSleepLockInfo, "NETNUC:NUC:spiTaskSleepLockInfo",0);
extern lock_t	*nucLock;
extern sleep_t	*spiTaskListSleepLock;


/*
 *	Forward references
 *
 *	If NUC_DEBUG is turned on, make all functions externally scoped
 *	in order to track stack traceback with the kernel debugger.
 */
enum NUC_DIAG GetTask_l();

/*
 * Current interfaces and data structures for task creation, destruction, and
 * caching reflect problems with task locking.
 */
void_t		*NWslTaskFreeList;
clock_t		NWslTaskAgeTime = 10*HZ;
size_t		NWslTaskFreeListLength;

/*
 * BEGIN_MANUAL_ENTRY(NWslCreateTask.3k)
 * NAME
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWslCreateTask( taskList, credPtr, newTask )
 *		void_t		*taskList;
 *		void_t		*credPtr;
 *		SPI_TASK_T	**newTask;
 *
 * INPUT
 *		void_t		*taskList;
 *		void_t		*credPtr;
 *
 * OUTPUT
 *		SPI_TASK_T	**newTask;
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_FAILURE
 *
 * DESCRIPTION
 *		Allocate a task structure and initialize it.
 *
 * NOTES
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWslCreateTask (	void_t		*taskList,
					void_t		*credPtr,
					SPI_TASK_T	**newTask )
{
	enum NUC_DIAG	ccode = SUCCESS;

#ifdef DEBUG
	NTR_ASSERT(SLEEP_LOCKOWNED(spiTaskListSleepLock));
#endif
	if (GetTask_l ( taskList, credPtr, newTask, FALSE ) == SUCCESS) {
		SLEEP_LOCK ((*newTask)->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
		/*
		 *	If primary AND reference count is zero, renegotiate the
		 *	packet 97 and associated options.
		 */

		/*	Verify that we are a primary connection that is not authenticated
		 *	or actively being used (reference count > 0 ) before resending
		 *	the packet 97 sequence.
		 */
		if (((ncp_channel_t *)((ncp_task_t *)
			(*newTask)->protoTaskPtr)->channelPtr)->referenceCount == 0 &&
			( ((*newTask)->mode & SPI_PRIMARY_SERVICE) &&
			((*newTask)->mode & SPI_TASK_CONNECTED) &&
			!((*newTask)->mode & SPI_TASK_AUTHENTICATED))) {
			negotiateSecurityLevels_l( credPtr, ((ncp_channel_t *)
				((ncp_task_t *)(*newTask)->protoTaskPtr)->channelPtr) );
			SLEEP_UNLOCK ((*newTask)->spiTaskSleepLock);
			return(SPI_TASK_EXISTS);
		}
		SLEEP_UNLOCK ((*newTask)->spiTaskSleepLock);
		return(SPI_TASK_EXISTS);
	}

	NWtlRewindSLList(NWslTaskFreeList);
	if (NWtlGetContentsSLList(NWslTaskFreeList, (void_t *)newTask) == SUCCESS &&
		lbolt - (*newTask)->freeTime >= NWslTaskAgeTime) {

		NWtlDeleteNodeSLList(NWslTaskFreeList);
		NWslTaskFreeListLength -= 1;
		((nwcred_t *)(*newTask)->credentialsPtr)->userID =
											((nwcred_t *)credPtr)->userID;
		((nwcred_t *)(*newTask)->credentialsPtr)->groupID =
											((nwcred_t *)credPtr)->groupID;
		((nwcred_t *)(*newTask)->credentialsPtr)->pid =
											((nwcred_t *)credPtr)->pid;
		((nwcred_t *)(*newTask)->credentialsPtr)->flags =
											((nwcred_t *)credPtr)->flags;
		(*newTask)->spiServicePtr = NULL;
		(*newTask)->protoTaskPtr = NULL;
		(*newTask)->resourceCount = 0;
		(*newTask)->useCount = 0;
		(*newTask)->mode = 0;
		(*newTask)->freeTime = -lbolt;
		(*newTask)->badSignatureRetries = 0;
		(*newTask)->ipxchecksum = 0;
		if ((*newTask)->md4 != NULL) {
			kmem_free ((*newTask)->md4, sizeof(md4_t));

			NTR_PRINTF (
				"NWslCreateTask: free md4 0x%x", (*newTask)->md4, 0, 0 );

#ifdef NUCMEM_DEBUG
			NTR_PRINTF (
				"NUCMEM_FREE: NWslCreateTask: free md4_t 0x%x, size = 0x%x",
				(*newTask)->md4, sizeof(md4_t), 0 );
#endif NUCMEM_DEBUG

			(*newTask)->md4 = (md4_t *)NULL;
		}
		NWslCleanTaskFreeList(B_FALSE);
		return(SUCCESS);
	}

	/*
	 *	Allocate a new task structure
	 */
	*newTask = (SPI_TASK_T *)kmem_zalloc ( sizeof(SPI_TASK_T), KM_SLEEP );

#ifdef NUCMEM_DEBUG
	NTR_PRINTF(
		"NUCMEM: NWslCreateTask: alloc SPI_TASK_T * at 0x%x, size = 0x%x",
		*newTask, sizeof(SPI_TASK_T), 0 );
#endif


	(*newTask)->spiTaskSleepLock = SLEEP_ALLOC (NUCSPITASKSLOCK_HIER,
			&spiTaskSleepLockInfo, KM_SLEEP);

	(*newTask)->spiTaskLock = LOCK_ALLOC (NUCSPITASKLOCK_HIER, plstr,
			&spiTaskLockInfo, KM_SLEEP);

	(*newTask)->spiTaskSV = SV_ALLOC (KM_SLEEP);

#ifdef NUCMEM_DEBUG
	NTR_PRINTF("NUCMEM: NWslCreateTask: alloc sv_t * at 0x%x, size = 0x%x",
		(*newTask)->spiTaskSV, sizeof(sv_t), 0 );
#endif

	/*
	 *	Make a copy of the credentials structure for our use
	 */
	ccode = NWtlDupCredStruct( credPtr,
						(nwcred_t **)&((*newTask)->credentialsPtr) );
	if ( ccode != SUCCESS ) {

#ifdef NUCMEM_DEBUG
		NTR_PRINTF (
			"NUCMEM_FREE: NWslCreateTask: free sv_t * at 0x%x, size = 0x%x",
			(*newTask)->spiTaskSV, sizeof(sv_t), 0 );
#endif
		SV_DEALLOC ((*newTask)->spiTaskSV);
		LOCK_DEALLOC ((*newTask)->spiTaskLock);
		SLEEP_DEALLOC ((*newTask)->spiTaskSleepLock);
		kmem_free ( (char *)*newTask, sizeof(SPI_TASK_T));

#ifdef NUCMEM_DEBUG
		NTR_PRINTF (
			"NUCMEM_FREE: NWslCreateTask: free SPI_TASK_T at 0x%x, size = 0x%x",
			*newTask, sizeof(SPI_TASK_T), 0 );
#endif
		*newTask = NULL;
	}

	(*newTask)->badSignatureRetries = 0;
	(*newTask)->ipxchecksum = 0;
	(*newTask)->md4 = (md4_t *)NULL;
	return(ccode);
}


/*
 * BEGIN_MANUAL_ENTRY(NWslFreeTask.3k)
 * NAME
 *		NWslFreeTask - Free the task structure.
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWslFreeTask( taskHandle )
 *		SPI_TASK_T	*taskHandle;
 *
 * INPUT
 *		SPI_TASK_T	*taskHandle;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_FAILURE
 *
 * DESCRIPTION
 *		Give the task back to the memory pool.
 *	
 * NOTES
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		spiTaskSleepLock, spiTaskListSleepLock
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		spiTaskListSleepLock
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWslFreeTask (SPI_TASK_T *taskHandle)
{
	NTR_ENTER(1, taskHandle, 0, 0, 0, 0);

#ifdef NUCMEM_DEBUG
	NTR_PRINTF("NUCMEM_FREE: NWslFreeTask: free sv_t * at 0x%x, size = 0x%x",
		taskHandle->spiTaskSV, sizeof(sv_t), 0 );
#endif

#ifdef DEBUG
	NTR_ASSERT(SLEEP_LOCKOWNED(taskHandle->spiTaskSleepLock));
	NTR_ASSERT(SLEEP_LOCKOWNED(spiTaskListSleepLock));
#endif
	NTR_ASSERT(!SLEEP_LOCKBLKD(taskHandle->spiTaskSleepLock));	/* NTR_HOPE() */

	SLEEP_UNLOCK(taskHandle->spiTaskSleepLock);
	NWtlFreeCredStruct( taskHandle->credentialsPtr );
	SLEEP_DEALLOC (taskHandle->spiTaskSleepLock);
	SV_DEALLOC (taskHandle->spiTaskSV);
	LOCK_DEALLOC (taskHandle->spiTaskLock);

	if (taskHandle->md4 != NULL) {
		kmem_free (taskHandle->md4, sizeof(md4_t));

		NTR_PRINTF (
			"NWslFreeTask: free md4_t 0x%x", taskHandle->md4, 0, 0 );

#ifdef NUCMEM_DEBUG
		NTR_PRINTF (
			"NUCMEM_FREE: NWslFreeTask: free md4 * at 0x%x, size = 0x%x",
			taskHandle->md4, sizeof(md4_t), 0 );
#endif NUCME_DEBUG
	}

	kmem_free (taskHandle, sizeof(SPI_TASK_T));
#ifdef NUCMEM_DEBUG
	NTR_PRINTF(
		"NUCMEM_FREE: NWslFreeTask: free SPI_TASK_T * at 0x%x, size = 0x%x",
		taskHandle, sizeof(SPI_TASK_T), 0 );
#endif

	return (NTR_LEAVE (SUCCESS));
}

/*
 * BEGIN_MANUAL_ENTRY(NWslScanTasks.3k)
 * NAME
 *		NWslScanTasks -	Scan the task list for the next task.
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWslScanTasks( taskList, lastCredStruct, mode, taskHandle )
 *		void_t		*taskList;
 *		void_t		*lastCredStruct;
 *		bmask_t		*mode;
 *		SPI_TASK_T	**taskHandle;
 *
 * INPUT
 *		void_t		*taskList;
 *		void_t		*lastCredStruct;
 *		bmask_t		*mode;
 *
 * OUTPUT
 *		void_t		*lastCredStruct;
 *		SPI_TASK_T	**taskHandle;
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_NO_MORE_TASK
 *
 * DESCRIPTION
 *		Scan tasks in the list.
 *
 * NOTES
 *		Assumes the following:
 *		1. Last Cred Struct was allocated by the caller.
 *		2. A credentials structure with a user ID of -1 indicates
 *			a scan to begin at the list head.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWslScanTasks (void_t *taskList, void_t *lastCredStruct, bmask_t *mode,
				SPI_TASK_T **taskHandle)
{
	enum NUC_DIAG	ccode = SUCCESS;
	uint32			userID;

	NWtlGetCredUserID( lastCredStruct, &userID );

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if ( userID == -1 ) { /* Initial state (start at the beginning */
		NWtlRewindSLList( taskList );
		if (NWtlGetContentsSLList( taskList, (void_t *)taskHandle ))
			ccode = SPI_NO_MORE_TASK;
		else {
			NWtlCopyCredStruct((*taskHandle)->credentialsPtr, lastCredStruct);
			*mode = (*taskHandle)->mode;	
		}
	} else {
		/*
		 *	Set the list pointer to the last one...
		 */
		if (GetTask_l ( taskList, lastCredStruct, taskHandle, FALSE )) {
			SLEEP_UNLOCK (spiTaskListSleepLock);
			return(SPI_NO_MORE_TASK);
		}

		/*
		 *	Jump to the next
		 */
		if (NWtlNextNodeSLList( taskList )) {
			SLEEP_UNLOCK (spiTaskListSleepLock);
			return(SPI_NO_MORE_TASK);
		}
	

		/*
		 *	Get the contents from the data structure
		 */
		if (NWtlGetContentsSLList(taskList, (void_t *)taskHandle ) == SUCCESS) {
			NWtlCopyCredStruct((*taskHandle)->credentialsPtr, lastCredStruct);
			*mode = (*taskHandle)->mode;	
		}
		else
			ccode = SPI_NO_MORE_TASK;
	}
	SLEEP_UNLOCK (spiTaskListSleepLock);

	return(ccode);
}

/* move to ncp layer */
enum NUC_DIAG
NWslGetConnInfo (SPI_TASK_T *gTask, uint32 *pluConnectionReference,
				uint32 uInfoLevel, uint32 uInfoLen, char *buffer,
				uint32 handleLicense)
{
	ncp_channel_t *channel;

	channel = (ncp_channel_t *)((ncp_task_t *)gTask->protoTaskPtr)->channelPtr;

	SLEEP_LOCK (channel->connSleepLock, NUCCONNSLEEPPRI);

	switch (uInfoLevel) {
		case NWC_CONN_INFO_AUTH_STATE: {
			if (uInfoLen < sizeof(channel->authenticationState)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			bcopy((caddr_t)&channel->authenticationState, buffer,
				sizeof(channel->authenticationState));
			break;
		}

		case NWC_CONN_INFO_BCAST_STATE: {
			if (uInfoLen < sizeof(channel->broadcastState)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			bcopy((caddr_t)&channel->broadcastState, buffer,
				sizeof(channel->broadcastState));
			break;
		}

		case NWC_CONN_INFO_CONN_REF: {
			if (uInfoLen < sizeof(channel->connectionReference)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			bcopy((caddr_t)&channel->connectionReference, buffer,
				sizeof(channel->connectionReference));
			break;
		}

		case NWC_CONN_INFO_TREE_NAME: {
			extern DNListEntry_t *DNList;
			DNListEntry_t *DNPtr;
			uint32 uid;

			NWtlGetCredUserID(gTask->credentialsPtr, &uid);

			for (DNPtr = DNList; DNPtr; DNPtr = DNPtr->DNForwardPtr) {
				if (DNPtr->uid == uid) {
					if (uInfoLen >= DNPtr->bufLength) {
						bcopy((caddr_t)DNPtr->bufPtr, buffer, DNPtr->bufLength);
						break;
					}
					SLEEP_UNLOCK (channel->connSleepLock);
					return(FAILURE);
				}
			}

			if (DNPtr == (DNListEntry_t *)NULL) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}

			break;
		}

		case NWC_CONN_INFO_SECURITY_STATE: {
			if (uInfoLen < sizeof(channel->securityState)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			bcopy((caddr_t)&channel->securityState, buffer,
				sizeof(channel->securityState));
			break;
		}

		case NWC_CONN_INFO_CONN_NUMBER: {
			if (uInfoLen < sizeof(channel->connectionNumber)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			bzero( buffer, uInfoLen );
			bcopy((caddr_t)&channel->connectionNumber, buffer,
				sizeof(channel->connectionNumber));
			break;
		}

		case NWC_CONN_INFO_USER_ID: {
			int n;

			n = sizeof(((ncp_auth_t *)
				((ncp_task_t *)channel->taskPtr)->authInfoPtr)->objectID);
			if (uInfoLen < n) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			bcopy((char *)&((ncp_auth_t *)((ncp_task_t *)channel->taskPtr)->authInfoPtr)->objectID,
				buffer, n);
			break;
		}

		case NWC_CONN_INFO_SERVER_NAME: {
			char *serverName =
				(char *)((ncp_server_t *)((SPI_SERVICE_T *)(gTask->spiServicePtr))->protoServicePtr)->serverName;

			if (uInfoLen < (uint32)(strlen(serverName) + 1)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			bcopy(serverName, buffer, strlen(serverName) + 1);

			break;
		}

		case NWC_CONN_INFO_TRAN_ADDR: {
			struct netbuf *address = ((SPI_SERVICE_T *)(gTask->spiServicePtr))->address;
			pNWCTranAddr t = (pNWCTranAddr)buffer;

			if (uInfoLen < (uint32)(sizeof(NWCTranAddr))) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			if (t->pbuBuffer == NULL) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			if (copyout(address->buf,
							(caddr_t)t->pbuBuffer, address->len)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(SPI_USER_MEMORY_FAULT);
			}
			t->uLen = (uint32)address->len;
			t->uType = NWC_TRAN_TYPE_IPX;

			break;
		}

		case NWC_CONN_INFO_NDS_STATE: {
			if (uInfoLen < sizeof(channel->ndsState)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			bcopy((caddr_t)&channel->ndsState, buffer,
				sizeof(channel->ndsState));
			break;
		}

		case NWC_CONN_INFO_MAX_PACKET_SIZE: {
			if (uInfoLen < sizeof(channel->negotiatedBufferSize)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			bcopy((caddr_t)&channel->negotiatedBufferSize, buffer,
				sizeof(channel->negotiatedBufferSize));
			break;
		}

		case NWC_CONN_INFO_LICENSE_STATE: {
			uint32 licenseState = 0;

			if (uInfoLen < sizeof(licenseState)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			if (handleLicense) {
				licenseState |= NWC_HANDLE_LICENSED;
			}
			bcopy((caddr_t)&licenseState, buffer, sizeof(licenseState));

			break;
		}

		case NWC_CONN_INFO_PUBLIC_STATE: {
			if (uInfoLen < sizeof(channel->uPublicState)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			bcopy((caddr_t)&channel->uPublicState, buffer,
				sizeof(channel->uPublicState));
			break;
		}

		case NWC_CONN_INFO_SERVICE_TYPE: {
			char serviceType[] = "ncp";
			uint32 len = sizeof(serviceType);

			if (uInfoLen < len) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			bcopy(serviceType, buffer, len);

			break;
		}

		case NWC_CONN_INFO_DISTANCE: {
			uint32 uDistance = 1;

			if (uInfoLen < sizeof(uDistance)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			bcopy((caddr_t)&uDistance, buffer, sizeof(uDistance));

			break;
		}

		case NWC_CONN_INFO_RETURN_ALL: {
			pNWCConnInfo p = (pNWCConnInfo)buffer;
			uint32 uid;

			NWtlGetCredUserID(gTask->credentialsPtr, &uid);

			if (uInfoLen < sizeof(NWCConnInfo)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}

			p->uInfoVersion = 1;
			p->uAuthenticationState = channel->authenticationState;
			p->uBroadcastState = channel->broadcastState;
			p->luConnectionReference = channel->connectionReference;

			if (p->pstrTreeName) {
				extern DNListEntry_t *DNList;
				DNListEntry_t *DNPtr;
				uint32 uid;

				NWtlGetCredUserID(gTask->credentialsPtr, &uid);

				for (DNPtr = DNList; DNPtr; DNPtr = DNPtr->DNForwardPtr) {
					if (DNPtr->uid == uid) {
						if (copyout((caddr_t)DNPtr->bufPtr,
										(caddr_t)p->pstrTreeName,
										DNPtr->bufLength)) {
							SLEEP_UNLOCK (channel->connSleepLock);
							return(SPI_USER_MEMORY_FAULT);
						}
						break;
					}
				}

				if (DNPtr == (DNListEntry_t *)NULL) {
					/*
					return(SPI_FAILURE);
					*/
				}
			}

			p->luSecurityState = channel->securityState;
			p->uConnectionNumber = channel->connectionNumber;
			p->luUserId = ((ncp_auth_t *)((ncp_task_t *)channel->taskPtr)->authInfoPtr)->objectID;

			if (p->pstrServerName) {
				char *serverName =
					(char *)((ncp_server_t *)((SPI_SERVICE_T *)(gTask->spiServicePtr))->protoServicePtr)->serverName;

				if (copyout(serverName, (caddr_t)p->pstrServerName,
						(strlen(serverName) + 1))) {
					SLEEP_UNLOCK (channel->connSleepLock);
					return(SPI_USER_MEMORY_FAULT);
				}
			}

			if (p->pTranAddr) {
				struct netbuf *address = ((SPI_SERVICE_T *)(gTask->spiServicePtr))->address;

				if (p->pTranAddr->pbuBuffer == NULL) {
					SLEEP_UNLOCK (channel->connSleepLock);
					return(FAILURE);
				}
				if (copyout(address->buf,
								(caddr_t)p->pTranAddr->pbuBuffer,
								address->len)) {
					SLEEP_UNLOCK (channel->connSleepLock);
					return(SPI_USER_MEMORY_FAULT);
				}
				p->pTranAddr->uLen = (uint32)address->len;
				p->pTranAddr->uType = NWC_TRAN_TYPE_IPX;
			}

			p->uNdsState = channel->ndsState;
			p->uMaxPacketSize = channel->negotiatedBufferSize;
			p->uLicenseState = channel->licenseState;
			p->uPublicState = channel->uPublicState;

			if (p->pstrServiceType) {
				char serviceType[] = "ncp";
				uint32 len = sizeof(serviceType);

				if (copyout(serviceType,
								(caddr_t)p->pstrServiceType, len)) {
					SLEEP_UNLOCK (channel->connSleepLock);
					return(SPI_USER_MEMORY_FAULT);
				}
			}

			p->uDistance = 1;
			break;
		}

		default:
			SLEEP_UNLOCK (channel->connSleepLock);
			return(FAILURE);
	}

	/*	Hopefully, we are still holding the connSleepLock at this
	 *	point!!		-- Ram M
	 */
	*pluConnectionReference = channel->connectionReference;
	SLEEP_UNLOCK (channel->connSleepLock);

	return(SUCCESS);
}

/* move to ncp layer */
enum NUC_DIAG
NWslSetConnInfo (SPI_TASK_T *gTask, uint32 uInfoLevel, uint32 uInfoLen,
				char *buffer)
{
	ncp_channel_t *channel;

	channel = (ncp_channel_t *)((ncp_task_t *)gTask->protoTaskPtr)->channelPtr;

	SLEEP_LOCK (channel->connSleepLock, NUCCONNSLEEPPRI);

	switch (uInfoLevel) {
		case NWC_CONN_INFO_AUTH_STATE: {
			if (uInfoLen > sizeof(channel->authenticationState)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			bcopy(buffer, (caddr_t)&channel->authenticationState, uInfoLen);
			/*
			 *  Authentication state is set to NWC_AUTH_STATE_NONE
			 *  after logging out of the server, and connection becomes
			 *  unlicensed.  Unfortunately, we also keep authentication
			 *  state in SPI_TASK_T, so we've got to clear that bit also.
			 */
			if (channel->authenticationState == NWC_AUTH_STATE_NONE) {
				gTask->mode &= ~(SPI_TASK_AUTHENTICATED);
				channel->licenseState = NWC_NOT_LICENSED;
			}
			break;
		}
		case NWC_CONN_INFO_BCAST_STATE: {
			if (uInfoLen > sizeof(channel->broadcastState)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(FAILURE);
			}
			bcopy(buffer, (caddr_t)&channel->broadcastState, uInfoLen);
			break;
		}
		default:
			SLEEP_UNLOCK (channel->connSleepLock);
			return(FAILURE);
	}
	SLEEP_UNLOCK (channel->connSleepLock);
	return(SUCCESS);
}

int
compare_connection_info(p1, p2, len, uScanFlags)
char *p1;
char *p2;
uint32 len;
uint32 uScanFlags;
{
	int i;

	for (i = 0; i < (int)len; i++) {
		if (p1[i] != p2[i]) {
			return((uScanFlags & NWC_MATCH_EQUALS) ? 0 : 1);
		}
	}
	return((uScanFlags & NWC_MATCH_EQUALS) ? 1 : 0);
}

int
select_connection(uScanInfoLevel, pScanConnInfo, uScanInfoLen, uScanFlags, taskHandle, credential)
uint32 uScanInfoLevel;
char *pScanConnInfo;
uint32 uScanInfoLen;
uint32 uScanFlags;
SPI_TASK_T *taskHandle;
void_t *credential;
{
	uint32 uid;
	uint32 negotiatedBufferSize;
	uint32 n;
	char *p;
	ncp_channel_t *channel;

	channel = ((ncp_task_t *)(taskHandle)->protoTaskPtr)->channelPtr;

	SLEEP_LOCK (channel->connSleepLock, NUCCONNSLEEPPRI);

	if (uScanFlags & NWC_RETURN_PRIVATE &&
	  (channel->uPublicState & NWC_CONN_PRIVATE) == 0) {
		SLEEP_UNLOCK (channel->connSleepLock);
		return(0);
	}

	if ((uScanFlags & NWC_RETURN_PRIVATE) &&
		(NWtlCredMatch( taskHandle->credentialsPtr, credential, CRED_PID ) == 0)) {
		SLEEP_UNLOCK (channel->connSleepLock);
		return(0);
	}

	if (uScanFlags & NWC_RETURN_PUBLIC && channel->uPublicState) {
		SLEEP_UNLOCK (channel->connSleepLock);
		return(0);
	}

	if (uScanFlags & NWC_RETURN_LICENSED &&
	  (channel->licenseState & NWC_CONNECTION_LICENSED) == 0) {
		SLEEP_UNLOCK (channel->connSleepLock);
		return(0);
	}

	if (uScanFlags & NWC_RETURN_UNLICENSED && channel->licenseState) {
		SLEEP_UNLOCK (channel->connSleepLock);
		return(0);
	}

	switch (uScanInfoLevel) {

		case NWC_CONN_INFO_AUTH_STATE: {
			n = sizeof(channel->authenticationState);
			if (uScanInfoLen < n) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			p = (char *)&channel->authenticationState;
			break;
		}

		case NWC_CONN_INFO_BCAST_STATE: {
			n = sizeof(channel->broadcastState);
			if (uScanInfoLen < n) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			p = (char *)&channel->broadcastState;
			break;
		}

		case NWC_CONN_INFO_CONN_REF: {
			n = sizeof(channel->connectionReference);
			if (uScanInfoLen < n) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			p = (char *)&channel->connectionReference;
			break;
		}

		case NWC_CONN_INFO_TREE_NAME: {
			extern DNListEntry_t *DNList;
			DNListEntry_t *DNPtr;

			NWtlGetCredUserID(((ncp_task_t *)channel->taskPtr)->credStructPtr, &uid);

			for (DNPtr = DNList; DNPtr; DNPtr = DNPtr->DNForwardPtr) {
				if (DNPtr->uid == uid) {
					if (uScanInfoLen == 0 && DNPtr->bufLength == 0) {
						SLEEP_UNLOCK (channel->connSleepLock);
						return((uScanFlags & NWC_MATCH_EQUALS) ? 1 : 0);
					}
					if (uScanInfoLen == 0 || DNPtr->bufLength == 0) {
						SLEEP_UNLOCK (channel->connSleepLock);
						return((uScanFlags & NWC_MATCH_EQUALS) ? 0 : 1);
					}
					n = (uScanInfoLen < DNPtr->bufLength) ? uScanInfoLen : DNPtr->bufLength;
					p = (char *)DNPtr->bufPtr;
					break;
				}
			}
			if (DNPtr == (DNListEntry_t *)NULL) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			break;
		}

		case NWC_CONN_INFO_SECURITY_STATE: {
			n = sizeof(channel->securityState);
			if (uScanInfoLen < n) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			p = (char *)&channel->securityState;
			break;
		}

		case NWC_CONN_INFO_CONN_NUMBER: {
			n = sizeof(channel->connectionNumber);
			if (uScanInfoLen < n) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			p = (char *)&channel->connectionNumber;
			break;
		}

		case NWC_CONN_INFO_USER_ID: {
			n = sizeof(((ncp_auth_t *)((ncp_task_t *)channel->taskPtr)->authInfoPtr)->objectID);
			if (uScanInfoLen < n) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			p = (char *)&((ncp_auth_t *)((ncp_task_t *)channel->taskPtr)->authInfoPtr)->objectID;
			break;
		}

		case NWC_CONN_INFO_SERVER_NAME: {

			/*
			 * Note that the libraries do not pass in
			 * null-terminated strings.
			 */
			p = (char *)((ncp_server_t *)((ncp_task_t *)channel->taskPtr)->serverPtr)->serverName;
			n = strlen(p);
			if (uScanInfoLen != n) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			break;
		}

		case NWC_CONN_INFO_TRAN_ADDR: {
			struct netbuf *address =
				((ncp_server_t *)((ncp_task_t *)channel->taskPtr)->serverPtr)->address;

			pNWCTranAddr scanAddress = (pNWCTranAddr)pScanConnInfo;
			char scanAddressBuffer[MAX_ADDRESS_SIZE];

			/* move this to where copyin only needs to be called once */
			if (copyin((caddr_t)scanAddress->pbuBuffer,
						   scanAddressBuffer, scanAddress->uLen)) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			if (scanAddress->uType != NWC_TRAN_TYPE_IPX) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			if (scanAddress->uLen != (uint32)address->len) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			SLEEP_UNLOCK (channel->connSleepLock);
			p = address->buf;
			n = (scanAddress->uLen < address->len) ? address->len : scanAddress->uLen;
			return(compare_connection_info(scanAddressBuffer, p, n, uScanFlags));
		}

		case NWC_CONN_INFO_NDS_STATE: {
			n = sizeof(channel->ndsState);
			if (uScanInfoLen < n) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			p = (char *)&channel->ndsState;
			break;
		}

		case NWC_CONN_INFO_MAX_PACKET_SIZE: {
			negotiatedBufferSize = channel->negotiatedBufferSize;
			n = sizeof(negotiatedBufferSize);

			if (uScanInfoLen < n) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			p = (char *)&negotiatedBufferSize;
			break;
		}

		case NWC_CONN_INFO_LICENSE_STATE: {
			n = sizeof(channel->licenseState);
			if (uScanInfoLen < n) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			p = (char *)&channel->licenseState;
			break;
		}

		case NWC_CONN_INFO_PUBLIC_STATE: {
			n = sizeof(channel->uPublicState);

			if (uScanInfoLen < n) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			p = (char *)&channel->uPublicState;
			break;
		}

		case NWC_CONN_INFO_SERVICE_TYPE: {
			char serviceType[] = "ncp";
			uint32 serviceTypeLen = sizeof(serviceType);

			p = serviceType;
			n = (uScanInfoLen < serviceTypeLen) ? uScanInfoLen : serviceTypeLen;

			break;
		}

		case NWC_CONN_INFO_DISTANCE: {
			uint32 uDistance = 1;
			n = sizeof(uDistance);

			if (uScanInfoLen < n) {
				SLEEP_UNLOCK (channel->connSleepLock);
				return(0);
			}
			p = (char *)&uDistance;
			break;
		}

		case NWC_CONN_INFO_RETURN_ALL: {
			SLEEP_UNLOCK (channel->connSleepLock);
			return(1);
		}

		default:
			SLEEP_UNLOCK (channel->connSleepLock);
			return(0);
	}
	SLEEP_UNLOCK (channel->connSleepLock);
	return(compare_connection_info(pScanConnInfo, p, n, uScanFlags));
}

enum NUC_DIAG
NWslScanConnInfo (SPI_TASK_T *taskList, void_t *credPtr,
				uint32 *pluConnectionReference, uint32 uScanInfoLevel,
				char *pScanConnInfo, uint32 uScanInfoLen,
				uint32 uScanFlags, uint32 uInfoLevel,
				uint32 uInfoLen, char *buffer)
{
	enum NUC_DIAG	ccode = SUCCESS;
	enum NUC_DIAG	ccode2;
	SPI_TASK_T	*taskHandle;
	ncp_channel_t	*channel;
	int found = 0;

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	NWtlRewindSLList ((void_t *)taskList);
	while ((ccode = NWtlGetContentsSLList ((void_t *)taskList,
						(void_t *)&taskHandle)) == SUCCESS) {
		SLEEP_LOCK (taskHandle->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
		if ( (taskHandle)->mode & SPI_TASK_DRAINING ) {
			if ( (taskHandle)->resourceCount == 0 ) {
				(void)NWsiCloseServiceWithTaskPtr_l (taskHandle);
				SLEEP_UNLOCK (taskHandle->spiTaskSleepLock);
				continue;
			} else {
				SLEEP_UNLOCK (taskHandle->spiTaskSleepLock);
				if ( (ccode = NWtlNextNodeSLList((void_t *)taskList) ) != SUCCESS ) {
					break;
				}
				continue;
			}
		}

		if (NWtlCredMatch((taskHandle)->credentialsPtr, credPtr, CRED_UID)) {
			channel = ((ncp_task_t *)(taskHandle)->protoTaskPtr)->channelPtr;

			if (select_connection(uScanInfoLevel, pScanConnInfo, uScanInfoLen,
						uScanFlags, taskHandle, credPtr)) {
				if (*pluConnectionReference == 0) {
					ccode = NWslGetConnInfo (taskHandle, pluConnectionReference,
							uInfoLevel, uInfoLen, buffer, 0);
					SLEEP_UNLOCK (taskHandle->spiTaskSleepLock);
					SLEEP_UNLOCK (spiTaskListSleepLock);
					return (ccode);
				}

				if (found) {
					ccode = NWslGetConnInfo (taskHandle, pluConnectionReference,
							uInfoLevel, uInfoLen, buffer, 0);
					SLEEP_UNLOCK (taskHandle->spiTaskSleepLock);
					SLEEP_UNLOCK (spiTaskListSleepLock);
					return (ccode);
				}

				SLEEP_LOCK (channel->connSleepLock, NUCCONNSLEEPPRI);

				if (*pluConnectionReference == channel->connectionReference) {
					found = 1;
				}
				SLEEP_UNLOCK (channel->connSleepLock);
			}
		}

		SLEEP_UNLOCK (taskHandle->spiTaskSleepLock);

		/*
		 *	If this task wasn't it, advance to the next entry
		 */
		if ( (ccode = NWtlNextNodeSLList((void_t *)taskList)) != SUCCESS ) {
			ccode = SPI_NO_MORE_TASK;
			break;
		}
	}
	SLEEP_UNLOCK (spiTaskListSleepLock);

	return((ccode == SUCCESS) ? SUCCESS : SPI_NO_MORE_TASK);
}

/*
 *	P R I V A T E    F U N C T I O N S
 */
/*
 *		
 *	LOCKS THAT MAY BE HELD WHEN CALLED:
 *		spiTaskListSleepLock
 *
 *	LOCKS HELD WHEN RETURNED:
 *		spiTaskListSleepLock
 *
 */

enum NUC_DIAG
GetTask_l (void_t *taskList, void_t *credPtr, SPI_TASK_T **taskHandle,
			int sleepLockInherited)
{
	enum NUC_DIAG	ccode = SPI_NO_SUCH_TASK;
	enum NUC_DIAG	ccode2;
	ncp_channel_t *channel;

	NTR_ENTER(4, taskList, credPtr, taskHandle, sleepLockInherited, 0);

	NWtlRewindSLList( taskList );
	while ((ccode = NWtlGetContentsSLList((void_t *)taskList,
						(void_t *)taskHandle)) == SUCCESS) {
		/*
		 *	If the task just found is in draining status, look at the
		 *	resource count.  If the count is zero, remove the SPI_TASK_T
		 *	entry from the list, rewind the list, and restart the search.
		 *	If the resource count is non-zero, just skip to the next entry.
		 *
		 *	NWsiCloseNode will remove this SPI_TASK_T from the list, but only
		 *	if called.  If the connection goes bad without resources being
		 *	allocated, NWsiCloseNode would never be called, so this check
		 *	was added to free SPI_TASK_Ts with zero resources allocated.
		 */
		if (sleepLockInherited == FALSE)
			SLEEP_LOCK ((*taskHandle)->spiTaskSleepLock, NUCCONNSLEEPPRI);
		if ( (*taskHandle)->mode & SPI_TASK_DRAINING ) {
            if ( (*taskHandle)->resourceCount == 0 ) {
				(void)NWsiCloseServiceWithTaskPtr_l (*taskHandle);
				if (sleepLockInherited == FALSE)
					SLEEP_UNLOCK ((*taskHandle)->spiTaskSleepLock);
                NWtlRewindSLList( taskList );
				continue;
            } else {
				if ( (ccode = NWtlNextNodeSLList(taskList) ) != SUCCESS ) {
					if (sleepLockInherited == FALSE)
						SLEEP_UNLOCK ((*taskHandle)->spiTaskSleepLock);
					break;
				}
				if (sleepLockInherited == FALSE)
					SLEEP_UNLOCK ((*taskHandle)->spiTaskSleepLock);
				continue;
			}
		}
		/*
		 *	If the credentials match the current task, we have found it
		 */
		channel = ((ncp_task_t *)(*taskHandle)->protoTaskPtr)->channelPtr;

		if (sleepLockInherited == FALSE)
			SLEEP_UNLOCK ((*taskHandle)->spiTaskSleepLock);

		SLEEP_LOCK (channel->connSleepLock, NUCCONNSLEEPPRI);

		if (((nwcred_t *)credPtr)->flags & NWC_OPEN_PRIVATE) {
			if (channel->uPublicState == NWC_CONN_PRIVATE) {
				if ((NWtlCredMatch((*taskHandle)->credentialsPtr, credPtr, CRED_PID))) {
					SLEEP_UNLOCK (channel->connSleepLock);
					ccode = SUCCESS;
					break;
				}
			}
		} else {
			if (channel->uPublicState == NWC_CONN_PUBLIC) {
				if ((NWtlCredMatch((*taskHandle)->credentialsPtr, credPtr, CRED_UID))) {
					SLEEP_UNLOCK (channel->connSleepLock);
					ccode = SUCCESS;
					break;
				}
			}
		}

		SLEEP_UNLOCK (channel->connSleepLock);

		/*
		 *	If this task wasn't it, advance to the next entry
		 */
		if ( (ccode = NWtlNextNodeSLList(taskList)) != SUCCESS )
			break;
	}


#ifdef NUC_DEBUG
	if ( ccode == SUCCESS ) {
		if ((*taskHandle)->mode == 0) {
			cmn_err (CE_PANIC, "SPI_TASK_T under construction!\n");
		}
		NTR_LEAVE((uint_t) *taskHandle );
		return( ccode );
	} else
		return( NTR_LEAVE( ccode ) );
#else NUC_DEBUG
	return( NTR_LEAVE( ccode ) );
#endif NUC_DEBUG
}

/*
 * BEGIN_MANUAL_ENTRY(NWslSetTaskInUse.3k)
 * NAME
 *		NWslSetTaskInUse - Indicate that a thread is currently using this
 *			SPI_TASK_T.
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWslSetTaskInUse( task )
 *		SPI_TASK_T	*task;
 *
 * INPUT
 *		SPI_TASK_T	*task;
 *
 * OUTPUT
 *		SPI_TASK_T	*task;
 *
 * RETURN VALUES
 *		SPI_SUCCESS if the task is not draining or in the process of being
 *			deleted.
 *		SPI_FAILURE otherwise.
 *
 * DESCRIPTION
 *		Increments the use count of the SPI_TASK_T, indicating that a 
 *		process thread is currently using this SPI_TASK_T.  NWsiCloseService
 *		and NWsiCloseServiceWithTaskPtr will not delete this task while this
 *		counter is greater than zero.
 *		
 *		This use count is decremented when the calling function calls
 *		NWsiSetTaskNotInUse.
 *
 * NOTES
 *		If this task is in the process of being deleted by NWsiCloseService
 *		then the deleting task will sleep until the use count is zero.
 *
 * SEE ALSO
 *		NWsiSetTaskNotInUse
 *		NWsiCloseServiceWithTaskPtr
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWslSetTaskInUse_l (SPI_TASK_T *task)
{
	pl_t		pl;
	extern int32 spilInitialized;

	NTR_ENTER(1, task, 0, 0, 0, 0);

	pl = LOCK (task->spiTaskLock, plstr);
#ifdef NUC_DEBUG
	if (task->useCount < 0) {
		cmn_err (CE_PANIC, "Use count of SPI_TASK_T %x is %x!\n",
			task, task->useCount);
	}
#endif

#ifdef DEBUG_TRACE_LEV2
	{
		char buffer[16];
		sprintf(buffer, "c=%x m=%x", task->useCount, task->mode);
		NTR_STRING( buffer );
	}
#endif

	if ((task->mode & (SPI_TASK_DELETED | SPI_TASK_DRAINING)) ||
			(!spilInitialized)) {
		UNLOCK (task->spiTaskLock, pl);
		return(NTR_LEAVE(FAILURE));
	}
	task->useCount++;
	UNLOCK (task->spiTaskLock, pl);
	return(NTR_LEAVE(SUCCESS));
}

/*
 * BEGIN_MANUAL_ENTRY(NWslSetTaskNotInUse.3k)
 * NAME
 *		NWslSetTaskNotInUse - Indicate that a thread is finished using this
 *			SPI_TASK_T.
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWslSetTaskNotInUse( task )
 *		SPI_TASK_T	*task;
 *
 * INPUT
 *		SPI_TASK_T	*task;
 *
 * OUTPUT
 *		SPI_TASK_T	*task;
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *
 * DESCRIPTION
 *		Decrements the use count of the SPI_TASK_T, indicating that a 
 *		process thread is finished using this SPI_TASK_T.  NWsiCloseService
 *		and NWsiCloseServiceWithTaskPtr will not delete this task while this
 *		counter is greater than zero.
 *		
 *		This use count should have been incremented earlier when the calling 
 *		function called NWsiSetTaskInUse.
 *
 * NOTES
 *		If this task is in the process of being deleted by NWsiCloseService
 *		then the deleting task will sleep until the use count is zero.
 *		This function will wakeup any process waiting for the use count to
 *		decrement to zero.
 *		
 *	LOCKS THAT MAY BE HELD WHEN CALLED:
 *		nucUpperLock
 *
 *	LOCKS HELD WHEN RETURNED:
 *		nucUpperLock
 *
 *
 * SEE ALSO
 *		NWsiSetTaskInUse
 *		NWsiCloseServiceWithTaskPtr
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWslSetTaskNotInUse_l (SPI_TASK_T *task)
{
	pl_t		pl;

	NTR_ENTER(1, task, 0, 0, 0, 0);

	pl = LOCK (task->spiTaskLock, plstr);

#ifdef NUC_DEBUG
	if (task->useCount <= 0) {
		cmn_err (CE_PANIC, "Use count of SPI_TASK_T %x is %x!\n",
			task, task->useCount);
	}
#endif

#ifdef DEBUG_TRACE_LEV2
	{
		char buffer[16];
		sprintf(buffer, "c=%x m=%x", task->useCount, task->mode);
		NTR_STRING( buffer );
	}
#endif
	task->useCount--;

	SV_BROADCAST (task->spiTaskSV, 0);

	UNLOCK (task->spiTaskLock, pl);

	NTR_LEAVE( SUCCESS );
	return(SUCCESS);
}

/*
 * List sleep lock must be held.  Caller must rewind before calling.
 */
void
NWslCleanTaskFreeList(boolean_t	shutDown)
{
	SPI_TASK_T	*gTask;

	while (NWtlGetContentsSLList(NWslTaskFreeList, (void_t *)&gTask)
		== SUCCESS) {
		if (shutDown || lbolt - gTask->freeTime >= NWslTaskAgeTime) {
			NWtlDeleteNodeSLList(NWslTaskFreeList);
			NWslTaskFreeListLength -= 1;
			SLEEP_LOCK(gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
			NWslFreeTask(gTask);
		} else {
			break;
		}
	}
}

