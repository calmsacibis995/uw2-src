/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $XConsortium: waitfor.c,v 1.8 91/09/11 11:59:39 rws Exp $ */
/*
 * waits for input
 */
/*
 * Copyright 1990, 1991 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation and the
 * Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this protoype software
 * and its documentation to Members and Affiliates of the MIT X Consortium
 * any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Network Computing Devices, Digital or
 * MIT not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.
 *
 * NETWORK COMPUTING DEVICES, DIGITAL AND MIT DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL NETWORK COMPUTING DEVICES, DIGITAL OR MIT BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $NCDId: @(#)waitfor.c,v 4.5 1991/06/24 11:59:20 lemke Exp $
 *
 */

#include	<stdio.h>
#include	<errno.h>
#include	<sys/param.h>

#include	<X11/Xos.h>	/* strings, time, etc */

#include	"clientstr.h"
#include	"globals.h"
#include	"osdep.h"

#ifdef SVR4
#include 	<sys/select.h>
#endif /* SVR4 */
extern WorkQueuePtr workQueue;

extern int  errno;

extern void MakeNewConnections();
extern void FlushAllOutput();

extern long WellKnownConnections;
#ifdef SVR4
extern fd_set LastSelectMask;
#else
extern long LastSelectMask[];
#endif /* SVR4 */
extern long WriteMask[];
extern long ClientsWithInput[];
extern long ClientsWriteBlocked[];
extern long AllSockets[];
extern long AllClients[];
extern long OutputPending[];

extern Bool AnyClientsWriteBlocked;
extern Bool NewOutputPending;

extern int  ConnectionTranslation[];

long        LastReapTime;

/*
 * wait_for_something
 *
 * server suspends until
 * - data from clients
 * - new client connects
 * - room to write data to clients
 */

WaitForSomething(pClientsReady)
    int        *pClientsReady;
{
    struct timeval *wt,
                waittime;
    long        clientsReadable[mskcnt];
#ifdef SVR4
    fd_set      clientsWriteable;
#else
    long        clientsWriteable[mskcnt];
#endif /* SVR4 */
    long        curclient;
    int         selecterr;
    long        current_time = 0;
    long        timeout;
    int         nready,
                i;

    while (1) {
	/* handle the work Q */
	if (workQueue)
	    ProcessWorkQueue();

	if (ANYSET(ClientsWithInput)) {
	    COPYBITS(ClientsWithInput, clientsReadable);
	    break;
	}
	/*
	 * deal with KeepAlive timeouts.  if this seems to costly, SIGALRM
	 * could be used, but its more dangerous since some it could catch us
	 * at an inopportune moment (like inside un-reentrant malloc()).
	 */
	current_time = GetTimeInMillis();
	timeout = current_time - LastReapTime;
	if (timeout > ReapClientTime) {
	    ReapAnyOldClients();
	    LastReapTime = current_time;
	    timeout = ReapClientTime;
	}
	timeout = ReapClientTime - timeout;
	waittime.tv_sec = timeout / MILLI_PER_SECOND;
	waittime.tv_usec = (timeout % MILLI_PER_SECOND) *
	    (1000000 / MILLI_PER_SECOND);
	wt = &waittime;
#ifdef SVR4
	COPYBITS(AllSockets, LastSelectMask.fds_bits);
#else
	COPYBITS(AllSockets, LastSelectMask);
#endif

#ifdef SVR4
	BlockHandler((pointer) &wt, (pointer) LastSelectMask.fds_bits);
#else
	BlockHandler((pointer) &wt, (pointer) LastSelectMask);
#endif /* SVR4 */
	if (NewOutputPending)
	    FlushAllOutput();

	if (AnyClientsWriteBlocked) {
#ifdef SVR4
	    COPYBITS(ClientsWriteBlocked, clientsWriteable.fds_bits);
	    i = select(MAXSOCKS, &LastSelectMask,
		       &clientsWriteable, (fd_set *) NULL, wt);
	} else {
	    i = select(MAXSOCKS, &LastSelectMask, (fd_set *) NULL,
		       (fd_set *) NULL, wt);
#else
	    COPYBITS(ClientsWriteBlocked, clientsWriteable);
	    i = select(MAXSOCKS, (int *) LastSelectMask,
		       (int *) clientsWriteable, (int *) NULL, wt);
	} else {
	    i = select(MAXSOCKS, (int *) LastSelectMask, (int *) NULL,
		       (int *) NULL, wt);
#endif /* SVR4 */
	}
	selecterr = errno;
#ifdef SVR4
	WakeupHandler((unsigned long) i, (pointer) LastSelectMask.fds_bits);
#else
	WakeupHandler((unsigned long) i, (pointer) LastSelectMask);
#endif /* SVR4 */
	if (i <= 0) {		/* error or timeout */
#ifdef SVR4
	    FD_ZERO(&clientsWriteable);
#else
	    CLEARBITS(clientsWriteable);
#endif /* SVR4 */
	    if (i < 0) {
		if (selecterr == EBADF) {	/* somebody disconnected */
		    CheckConnections();
		} else if (selecterr != EINTR) {
		    ErrorF("WaitForSomething: select(): errno %d\n", selecterr);
		} else {
		    /*
		     * must have been broken by a signal.  go deal with any
		     * exception flags
		     */
		    return 0;
		}
	    } else {		/* must have timed out */
		ReapAnyOldClients();
		LastReapTime = GetTimeInMillis();
	    }
	} else {
#ifdef SVR4
	    if (AnyClientsWriteBlocked && ANYSET(clientsWriteable.fds_bits)) {
#else
	    if (AnyClientsWriteBlocked && ANYSET(clientsWriteable)) {
#endif /* SVR4 */
		NewOutputPending = TRUE;
#ifdef SVR4
		ORBITS(OutputPending, clientsWriteable.fds_bits, OutputPending);
		UNSETBITS(ClientsWriteBlocked, clientsWriteable.fds_bits);
#else
		ORBITS(OutputPending, clientsWriteable, OutputPending);
		UNSETBITS(ClientsWriteBlocked, clientsWriteable);
#endif /* SVR4 */
		if (!ANYSET(ClientsWriteBlocked))
		    AnyClientsWriteBlocked = FALSE;
	    }
#ifdef SVR4
	    MASKANDSETBITS(clientsReadable, LastSelectMask.fds_bits, AllClients);
	    if (LastSelectMask.fds_bits[0] & WellKnownConnections)
#else
	    MASKANDSETBITS(clientsReadable, LastSelectMask, AllClients);
	    if (LastSelectMask[0] & WellKnownConnections)
#endif /* SVR4 */
		MakeNewConnections();
	    if (ANYSET(clientsReadable))
		break;

	}
    }
    nready = 0;

    if (ANYSET(clientsReadable)) {
	ClientPtr   client;
	int         conn;

	if (current_time)	/* may not have been set */
	    current_time = GetTimeInMillis();
	for (i = 0; i < mskcnt; i++) {
	    while (clientsReadable[i]) {
		curclient = ffs(clientsReadable[i]) - 1;
		conn = ConnectionTranslation[curclient + (i << 5)];
		clientsReadable[i] &= ~(1 << curclient);
		client = clients[conn];
		if (!client)
		    continue;
		pClientsReady[nready++] = conn;
		client->last_request_time = current_time;
		client->clientGone = CLIENT_ALIVE;
	    }
	}
    }
    return nready;
}

#ifndef ANYSET
/*
 * This is not always a macro
  */
ANYSET(src)
    long       *src;
{
    int         i;

    for (i = 0; i < mskcnt; i++)
	if (src[i])
	    return (1);
    return (0);
}

#endif
