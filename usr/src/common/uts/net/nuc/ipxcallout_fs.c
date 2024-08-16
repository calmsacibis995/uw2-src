/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ipxcallout_fs.c	1.7"
#ident  "$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ipxcallout_fs.c,v 2.51.2.2 1994/12/21 02:46:21 ram Exp $"

/*
 *  Netware Unix Client
 *
 */

#include <io/stream.h>
#include <svc/time.h>
#include <util/param.h>

#include <net/nuc/nwctypes.h>
#include <net/nuc/nuctool.h>
#include <net/nw/ipx_app.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/gtsendpoint.h>
#include <net/nuc/ipxengine.h>
#include <net/nuc/ipxengtune.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/nuc_prototypes.h>

#define NVLT_ModMask    NVLTM_ipxeng

extern  void_t  IPXEngConnectionTimeout();
extern  void_t  IPXEngConnectionTimeoutBurst();
extern ccode_t IPXEngMaintainFragmentList();
extern void_t IPXEngWriteBurstClockInterrupt();


/*
 * BEGIN_MANUAL_ENTRY(IPXEngHalt(3K), \
 *      ./man/kernel/ts/ipxeng/Halt)
 * NAME
 *  IPXEngCalloutHandler - Callback routine passed to clock
 *
 * SYNOPSIS
 *  ccode_t
 *  IPXEngCalloutHandler()
 *
 * INPUT
 *  None.
 *
 * OUTPUT
 *  None.
 *
 * RETURN VALUES
 *  None.
 *
 * DESCRIPTION
 *  Is called when the IPXEng layer is to be halted.  The only necessary
 *  thing to do here is to remove any retransmission callouts, so that
 *  a panic doen't occur when a callout occurs on bogus endpoints, or
 *  worse on dynamically linking kernels, which would cause a panic on
 *  a page fault to function that is not longer linked in.
 *
 * NOTES
 *
 * SEE ALSO
 *  IPXEngConnectionTimeout(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
ipxenhalt()
{
	register	int	i,j;
	void			*timeoutFunction;

    NVLT_ENTER (0);

    for (i = 0; i < ipxEngTune.maxClients; i++) {
        if (clientList[i].numTasks > 0) {
            for (j = 0; j < ipxEngTune.maxClientTasks; j++) {
                if (clientList[i].taskList[j].state & (IPX_TASK_TRANSMIT))
                    /*
                     * Remove connections retransmission
                     * Handler
                     */
                    if (clientList[i].taskList[j].state & (IPX_TASK_BURST))
                        timeoutFunction = (void *)IPXEngConnectionTimeoutBurst;
                    else
                        timeoutFunction = (void *)IPXEngConnectionTimeout;
                    if (clientList[i].taskList[j].callOutID)
                        untimeout(clientList[i].taskList[j].callOutID);
					clientList[i].taskList[j].callOutID = 0;
            }
        }
    }

    return (NVLT_LEAVE (0));
}

