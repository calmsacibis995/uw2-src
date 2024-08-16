/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/sltask_fs.c	1.14"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/sltask_fs.c,v 2.54.2.4 1995/02/01 01:05:00 ram Exp $"

/*
 *  Netware Unix Client
 */

#include <util/param.h>
#include <net/nuc/nuctool.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/requester.h>
#include <net/tiuser.h>
#include <net/nuc/slstruct.h>
#include <net/nuc/nucerror.h>
#include <util/cmn_err.h>
#include <net/nuc/nwspiswitch.h>
#include <net/nuc/nwspi.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/spimacro.h>
#include <io/ddi.h>

#include <net/nuc/nwmp.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/sistructs.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/nuc_prototypes.h>

#define NVLT_ModMask    NVLTM_spil

extern lock_t	*nucLock;
extern sleep_t	*spiTaskListSleepLock;
extern int32	spiState;

enum NUC_DIAG NWslAutoAuthenticate(	SPI_SERVICE_T *, nwcred_t *, int16);

/*
 * BEGIN_MANUAL_ENTRY(NWslGetTask.3k)
 * NAME
 *		NWslGetTask - Get authenticated SPI_TASK_T from the list
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWslGetTask( taskList, credPtr,  taskHandle )
 *		void_t		*taskList;
 *		void_t		*credPtr;
 *		SPI_TASK_T	**taskHandle;
 *
 * INPUT
 *		void_t		*taskList;
 *		void_t		*credPtr;
 *
 * OUTPUT
 *		SPI_TASK_T	**taskHandle;
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_NO_SUCH_TASK
 *
 * DESCRIPTION
 *		Scans the task list for a task with credentials matching
 *		the credentials passed in and authenticated.
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
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWslGetTask (	SPI_SERVICE_T	*servicePtr,
				nwcred_t		*credPtr,
				SPI_TASK_T		**taskHandle,
				int16			xautoFlags )
{
	enum NUC_DIAG	ccode = SUCCESS;

	NVLT_ENTER (4);

	*taskHandle = (SPI_TASK_T *)NULL;

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if ((ccode = GetTask_l (servicePtr->taskList, credPtr, taskHandle, FALSE))
			== 0) {
		SLEEP_UNLOCK (spiTaskListSleepLock);
		SLEEP_LOCK ((*taskHandle)->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
		if ((*taskHandle)->mode & SPI_TASK_AUTHENTICATED)  {
			SLEEP_UNLOCK ((*taskHandle)->spiTaskSleepLock);
			return( NVLT_LEAVE(SUCCESS) );
		}
		SLEEP_UNLOCK ((*taskHandle)->spiTaskSleepLock);
	} else
		SLEEP_UNLOCK (spiTaskListSleepLock);
		
	ccode = NWslAutoAuthenticate( servicePtr, credPtr, xautoFlags );
	if (ccode) {
		*taskHandle = (SPI_TASK_T *)NULL;
		return( NVLT_LEAVE(FAILURE) );
	}

	/*
	 *	Try again to find an authenticated task and return either 
	 *	the SPI_TASK_T pointer or the error code.
	 */
	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if ((ccode = GetTask_l (servicePtr->taskList, credPtr, taskHandle, FALSE))
			!= 0) {
		SLEEP_UNLOCK (spiTaskListSleepLock);
		*taskHandle = (SPI_TASK_T *)NULL;
		return( NVLT_LEAVE(SPI_NO_SUCH_TASK) );
	}
	SLEEP_UNLOCK (spiTaskListSleepLock);
	SLEEP_LOCK ((*taskHandle)->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if ( !((*taskHandle)->mode & SPI_TASK_AUTHENTICATED) ) {
		SLEEP_UNLOCK ((*taskHandle)->spiTaskSleepLock);
		*taskHandle = (SPI_TASK_T *)NULL;
		return( NVLT_LEAVE(SPI_AUTHENTICATION_FAILURE) );
	}
	SLEEP_UNLOCK ((*taskHandle)->spiTaskSleepLock);

	return( NVLT_LEAVE(SUCCESS) );
}

enum NUC_DIAG
NWslAutoAuthenticate(	SPI_SERVICE_T	*servicePtr,
						nwcred_t		*credPtr,
						int16			xautoFlags )
{
	pl_t					pl;
	extern	struct	netbuf	spilInternalTaskAuthenticationAddress;
	extern	sv_t			*spilInternalTaskAuthenticationAddressSV;
	boolean_t				bool;

	NVLT_ENTER (3);
	/*
	 *	If an authenticated task doesn't exist, attempt an authentication
	 *	request by obtaining the request semaphore, copying the service name
	 *	and task credentials where the authentication routine expects them, 
	 *	and wake up the internal task authentication routine.  Then sleep on
	 *	the service name buffer until awaken by the authentication
	 *	routine.
	 */
	if ( (spilInternalTaskAuthenticationSemaphore == -1) ||
		(NWtlPSemaphore(spilInternalTaskAuthenticationSemaphore)) )
		return( NVLT_LEAVE(FAILURE) );
	
	if (servicePtr->address->len > spilInternalTaskAuthenticationAddress.maxlen)
		return( NVLT_LEAVE(FAILURE) );
	bcopy (servicePtr->address->buf,
		spilInternalTaskAuthenticationAddress.buf, servicePtr->address->len);
	spilInternalTaskAuthenticationAddress.len = servicePtr->address->len;
	NWtlGetCredUserID (credPtr, &spilInternalTaskAuthenticationUid);
	NWtlGetCredGroupID (credPtr, &spilInternalTaskAuthenticationGid);
	NWtlGetCredPid(credPtr, &spilInternalTaskAuthenticationPid);	/* SLIME */
	spilInternalTaskAuthenticationXautoFlags = xautoFlags;

	pl = LOCK (nucLock, plstr);

	SV_BROADCAST (spilInternalTaskAuthenticationQueueSV, 0);

	/*
	 *	Sleep until the task has been authenticated and we are awaken
	 *	by the task authentication routine.  If an error occurs, set 
	 *	the return code but continue on to release the semaphore.
	 */

/*************************
	if ((bool = SV_WAIT_SIG (spilInternalTaskAuthenticationAddressSV,
							 primed, nucLock)) != B_TRUE) {
		*taskHandle = (SPI_TASK_T *)NULL;
		NWtlVSemaphore_l (spilInternalTaskAuthenticationSemaphore);
		return( NVLT_LEAVE(SPI_FAILURE) );
	}
*************************/

	SV_WAIT (spilInternalTaskAuthenticationAddressSV, primed, nucLock);

	pl = LOCK (nucLock, plstr);

	if (spiState == SPI_LAYER_INACTIVE) {
		UNLOCK (nucLock, pl);
		return( NVLT_LEAVE(FAILURE) );
	}
	UNLOCK (nucLock, pl);

	/*
	 *	Release the semaphore
	 */
	if (NWtlVSemaphore_l (spilInternalTaskAuthenticationSemaphore)) {
		return( NVLT_LEAVE(FAILURE) );
	}

	return( NVLT_LEAVE(SUCCESS) );
}
