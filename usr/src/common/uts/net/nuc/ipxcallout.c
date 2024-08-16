/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ipxcallout.c	1.15"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ipxcallout.c,v 2.53.2.3 1995/02/09 03:45:36 doshi Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: ipxcallout.c
 *
 *	ABSTRACT: Asynchronus routines used in IPX engine for skulking and
 *	interrupt handling
 *
 *	Functions declared in this module:
 *
 *	Public functions:
 *		IPXEngConnectionTimeout
 *
 *	Private functions:
 */

#ifdef _KERNEL_HEADERS
#include <util/param.h>
#include <svc/time.h>
#include <net/nuc/nwctypes.h>	
#include <net/nuc/nuctool.h>
#include <io/stream.h>
#include <net/nw/ipx_app.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/gtsendpoint.h>
#include <net/nuc/ipxengine.h>
#include <net/nuc/ipxengtune.h>
#include <net/nw/ntr.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/nuc_prototypes.h>
#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/time.h>
#include <kdrivers.h>
#include <sys/nwctypes.h>	
#include <sys/nuctool.h>
#include <sys/ipx_app.h>
#include <sys/gtscommon.h>
#include <sys/gtsendpoint.h>
#include <sys/ipxengine.h>
#include <sys/ipxengtune.h>
#include <sys/nucerror.h>
#include <sys/ncpconst.h>
#include <sys/ncpiopack.h>
#include <sys/nuc_prototypes.h>

#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask	NTRM_ipxeng

/*
 * Forward Reference Functions
 */
void	IPXEngConnectionTimeout();


/*
 * BEGIN_MANUAL_ENTRY(IPXEngConnectionTimeout(3K), \
 *		./man/kernel/ts/ipxeng/ConnectionTimeout)
 * NAME
 *	IPXEngConnectionTimeout
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngConnectionTimeout( taskPtr )
 *
 *	ipxTask_t	*taskPtr;
 *
 * INPUT
 *	taskPtr	- IPX connection endpoint that must be aged. 
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	This is the re-transmission handler of IPX connections.  It runs out
 *	of process context on a clock interrupt off the call out list.
 *	It decides if TIME_OUT in real time has occured, in which case it
 *	terminates the request, and wakes up the NCP process context indicating
 *	a timed out error, otherwise the request is retransmitted.  It also
 *	uses the BSD 4.3 Tahoe TCP strategy in exponentially backing off
 *	a failing connection up to a sane maximal retry quantum.
 *
 * SEE ALSO
 *	IPXEngInterruptHandler(3K), IPXEngSendPacket(3K), IPXEngHalt(3K)
 *
 * END_MANUAL_ENTRY
 */
void
IPXEngConnectionTimeout (ipxTask_t *taskPtr )
{
	pl_t		pl;

	NTR_ENTER ( 1, taskPtr, 0, 0, 0, 0 );

	NTR_PRINTF( "IPXEngConnectionTimeout:  Timeout!\n", 0, 0, 0 );

	pl = LOCK (taskPtr->taskLock, plstr);

	/*	If we are not currently transmitting this must be an old
	 *	timer so just ignore it.
	 */
	taskPtr->callOutID = 0;

	if (taskPtr->state & IPX_TASK_TRANSMIT ) {
		/*
		 * Retransmission Timer expired!!
		 * Add in the latest timeout quantum to round trip
		 */
		if ( (taskPtr->waitTime += taskPtr->timerTicks) >=
			ipxEngTune.timeoutQuantumLimit ) {
			/*
			 * We have waited the maximum time, give up
			 */
			NTR_PRINTF( "IPXEngConnectionTimeout:  %d >= %d\n",
				taskPtr->waitTime, ipxEngTune.timeoutQuantumLimit, 0 );

			taskPtr->state |= IPX_TASK_TIMEDOUT;

		}else{

			taskPtr->state |= IPX_REPLY_TIMEDOUT;

			/*
			 * Calculate the new back off time, bounded by
			 * maximal single retransmit, or maximal wait
			 * quantum.  We use the BSD 4.3 Tahoe style
			 * exponential backoff up to our sanity limit.
			 */
			if ( ++(taskPtr->curBackOffDelta) > MAX_EXP_SHIFT ) {
				/*
				 * Clip the backoff delta
				 */
				taskPtr->curBackOffDelta = MAX_EXP_SHIFT;
			}

			taskPtr->timerTicks = taskPtr->reTransBeta <<
				taskPtr->curBackOffDelta;
			if ( taskPtr->timerTicks > MAX_RETRANS_QUANTUM ) {
				/*
				 * Bound this interval by our maximal retrans quantum
				 * interval.
				 */
				taskPtr->timerTicks = MAX_RETRANS_QUANTUM;
			}

			/*
				If the new interval will exceed the maximum amount of time
				we should wait for a response, clip it to the remaining
				quantum.
			*/
			if( (taskPtr->timerTicks + taskPtr->waitTime) > 
			  ipxEngTune.timeoutQuantumLimit ){
				taskPtr->timerTicks = ipxEngTune.timeoutQuantumLimit -
					taskPtr->waitTime;
			}
		}

		/*	IPXTASK_RELE (taskPtr);	*/

		NWtlWakeupPsuedoSema( taskPtr->syncSemaphore );
		UNLOCK (taskPtr->taskLock, pl);
		NTR_LEAVE( 0 );
		return;
	}

	UNLOCK (taskPtr->taskLock, pl);

	NTR_LEAVE( 0 );
	return;
}
