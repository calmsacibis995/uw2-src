/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ipxengmgr.c	1.11"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ipxengmgr.c,v 2.52.2.2 1994/12/21 02:46:59 ram Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: ipxengmgr.c
 *
 *	ABSTRACT: IPXEngine manager functions for starting/stopping,
 *	initialization, and passing parameters from internal structures
 *	(statistics)
 *
 *	Functions declared in this module:
 *	Public functions:
 *		IPXEngInitTables
 *	Private functions:
 */


#ifdef _KERNEL_HEADERS
#include <net/nw/ipx_app.h>
#include <io/stream.h>
#include <svc/time.h>
#include <util/param.h>

#include <net/nuc/nwctypes.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/ipxengine.h>
#include <net/nuc/ipxengtune.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/gtsendpoint.h>
#include <net/nuc/gtsconf.h>
#include <net/nw/ntr.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */
#include "sys/ipx_app.h"
#include <sys/time.h>

#include <kdrivers.h>
#include <sys/nwctypes.h>
#include <sys/nuctool.h>
#include <sys/ipxengine.h>
#include <sys/ipxengtune.h>
#include <sys/gtscommon.h>
#include <sys/gtsendpoint.h>
#include <sys/gtsconf.h>
#include <sys/nucerror.h>

#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask	NTRM_ipxeng

/*
 *	Memory region pointer. 
 *	The region is used primarily for duplication of data structures
 *	that originated in upper layers that need to be maintained in
 *	this layer for a given period of time.  i.e. Credentials structure
 */
opaque_t *ipxEngRegion;	/* memory region for IPX_engine */

/*
 * BEGIN_MANUAL_ENTRY(IPXEngInitTables(3K), \
 *			./man/kernel/ts/ipxeng/InitTables)
 * NAME
 *	IPXEngInitTables - Initialize the IPXEngine tables
 *
 * SYNOPSIS
 *	ccode_t
 *	IPXEngInitTables()
 *
 * INPUT
 *	None.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	Initializes the table allocated in space.c. Called by the IPX Engine
 *	Layer init routine to initialize the data structures associated with
 *	the IPXEngine
 *
 * NOTES
 *	ipxEngTune structure, clientList, and taskTable data structures are 
 *	located in ipxengtune.h and is setup by space.c based upon parameters
 *	in ipxengtune.h.  
 *
 *	Due to the fact that C will not allow the externalization of 
 *	dimensional arrays without having the length specification of 
 *	dimensions 2-n.  This breaks the concept of having space.c allocate
 *	the space based upon tuneable parameters.  So... we do this little trick
 *	of allocating an array of pointers to allow us to vector into the 
 *	big array of tasks.
 *
 * SEE ALSO
 *	IPXEngInitialize(3K)
 *
 * END_MANUAL_ENTRY
 */
int
IPXEngInitTables(void)
{
	register int32 i,j;

	NTR_ENTER(0, 0, 0, 0, 0, 0);

	for (i = 0; i < ipxEngTune.maxClients; i++) {
		clientList[i].taskList = &(taskTable[i*ipxEngTune.maxClientTasks]);
		clientList[i].numTasks = 0;
		clientList[i].state = IPX_CLIENT_FREE;

		for (j=0; j < ipxEngTune.maxClientTasks; j++) {
			clientList[i].taskList[j].state = IPX_TASK_FREE;
			clientList[i].taskList[j].gtsEndPoint.realTsOps =
				(opaque_t *)NWgtsOpsSw[NOVELL_IPX];
			clientList[i].taskList[j].gtsEndPoint.realEndPoint =
				(opaque_t *) &(clientList[i].taskList[j]);
		}
	}

	/*
	 * Bound the transport timeout quantum limit 
	 */
	if ( ipxEngTune.timeoutQuantumLimit < 30 ) {
		/*
	 	* Enforce our minimum
	 	*/
		ipxEngTune.timeoutQuantumLimit = 30;
	} else if ( ipxEngTune.timeoutQuantumLimit > 900 ) {
		/*
	 	* Enforce our maximum
	 	*/
		ipxEngTune.timeoutQuantumLimit = 900;
	}
	/*
	 * Translate transport quantum to tick normalized
	 */
	ipxEngTune.timeoutQuantumLimit = (ipxEngTune.timeoutQuantumLimit * HZ);

	/*
	 * Calculate the transport minimum round trip and variances, normalized
	 * to ticks
	 */
	ipxEngTune.minRoundTripTicks	=
		(ipxEngTune.minRoundTripTicks / (1000/HZ));
	ipxEngTune.minVariance		= (ipxEngTune.minVariance / (1000/HZ));

	return( NTR_LEAVE( SUCCESS ) );
}

int
IPXEngReInitTables(void)
{
	register int32 i,j;

	NTR_ENTER(0, 0, 0, 0, 0, 0);

	for (i = 0; i < ipxEngTune.maxClients; i++) {
		clientList[i].taskList = &(taskTable[i*ipxEngTune.maxClientTasks]);
		clientList[i].numTasks = 0;
		clientList[i].state = IPX_CLIENT_FREE;

		for (j=0; j < ipxEngTune.maxClientTasks; j++) {
			clientList[i].taskList[j].state = IPX_TASK_FREE;
			clientList[i].taskList[j].gtsEndPoint.realTsOps =
				(opaque_t *)NWgtsOpsSw[NOVELL_IPX];
			clientList[i].taskList[j].gtsEndPoint.realEndPoint =
				(opaque_t *) &(clientList[i].taskList[j]);
		}
	}
	return( NTR_LEAVE( SUCCESS ) );
}
