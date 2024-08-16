/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)r5server:os/WaitFor.c	1.22"

/*	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/* $Header: /cvsroot/x/server/os/WaitFor.c,v 1.1 1994/06/24 23:35:19 lef Exp $ */
/* $XConsortium: WaitFor.c,v 1.57 92/03/13 15:47:39 rws Exp $ */

/*
 * Copyright (c) 1993 Unix System Laboratories (USL)
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of USL not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  USL makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * USL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL USL
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*=========================================================================*\
|| Copyright (c) 1993 Pittsburgh Powercomputing Corporation (PPc).
|| Copyright (c) 1993 Quarterdeck Office Systems, Inc. (QOS).
||
|| Permission to use, copy, modify, and distribute this software and its
|| documentation for any purpose and without fee is hereby granted, provided
|| that the above copyright notice appear in all copies and that both that
|| copyright notice and this permission notice appear in supporting
|| documentation, and that the names of PPc and QOS not be used in
|| advertising or publicity pertaining to distribution of the software
|| without specific, written prior permission.  Neither PPc nor QOS
|| make any representations about the suitability of this software for any
|| purpose.  It is provided "as is" without express or implied warranty.
||
|| PPc AND QOS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
|| INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
|| EVENT SHALL PPc OR QOS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
|| CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
|| USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
|| OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
|| PERFORMANCE OF THIS SOFTWARE.
\*=========================================================================*/

/*****************************************************************
 * OS Dependent input routines:
 *
 *  WaitForSomething,  GetEvent
 *
 *****************************************************************/

#include "Xos.h"			/* for strings, fcntl, time */

#include <errno.h>
#include <stdio.h>
#include "X.h"
#include "misc.h"

#include <sys/param.h>
#include <signal.h>
#include "osdep.h"
#include "dixstruct.h"
#include "opaque.h"

#ifdef SVR4
#include <sys/time.h>
#include <sys/xque.h>
#include <sys/select.h>
extern xqEventQueue *queue;
# ifdef BUILTIN
#include "extensions/server/builtin/BIserver.h"
# endif
#endif

extern long AllSockets[];
extern long AllClients[];
#ifdef SVR4
extern fd_set LastSelectMask;
#else
extern long LastSelectMask[];
#endif
extern long WellKnownConnections;
extern long EnabledDevices[];
extern long ClientsWithInput[];
extern long ClientsWriteBlocked[];
extern long OutputPending[];

extern long ScreenSaverTime;               /* milliseconds */
extern long ScreenSaverInterval;               /* milliseconds */
extern int ConnectionTranslation[];

extern Bool NewOutputPending;
extern Bool AnyClientsWriteBlocked;

extern WorkQueuePtr workQueue;

extern void CheckConnections();
extern Bool EstablishNewConnections();
extern void SaveScreens();
extern void ResetOsBuffers();
extern void ProcessInputEvents();
extern void BlockHandler();
extern void WakeupHandler();

extern int errno;

#ifdef apollo
extern long apInputMask[];

static long LastWriteMask[mskcnt];
#endif

#ifdef XTESTEXT1
/*
 * defined in xtestext1dd.c
 */
extern int playback_on;
#endif /* XTESTEXT1 */

/*****************
 * 
 * WaitForSomething:
 *     Make the server suspend until there is
 *	1. data from clients or
 *	2. input events available or
 *	3. ddx notices something of interest (graphics queue ready, etc.) or
 *	4. clients that have buffered replies/events are ready
 *
 *	If the time between INPUT events is greater than ScreenSaverTime, the
 *	display is turned off (or saved, depending on the hardware).  So,
 *	WaitForSomething() has to handle this also (that's why the select()
 *	has a timeout.  For more info on ClientsWithInput, see
 *	ReadRequestFromClient().  pClientsReady is an array to store ready
 *	client->index values into.
 */

#include <sys/types.h>
#include <stropts.h>

int
WaitForSomething(pClientsReady)
    int *pClientsReady;
{
    int i;
    struct timeval waittime, *wt;
    long timeout;
    long clientsReadable[mskcnt];
    static long timeTilFrob = 0;		/* while screen saving */
#ifdef SVR4
    fd_set clientsWritable;
#define BITS(MASK)	(MASK).fds_bits
#else
    long clientsWritable[mskcnt];
#define BITS(MASK)	(MASK)
#endif
    long curclient;
    int selecterr;
    int nready;
    long devicesReadable[mskcnt];

    CLEARBITS(clientsReadable);

    /* We need a while loop here to handle 
       crashed connections and the screen saver timeout */
    while (1)
    {
	/* deal with any blocked jobs */
	if (workQueue)
	    ProcessWorkQueue();

	if (NewOutputPending)
	    FlushAllOutput();

	if (ANYSET(ClientsWithInput))
	{
	    COPYBITS(ClientsWithInput, clientsReadable);
	    break;
	}
	if (ScreenSaverTime)
	{
	    timeout = (ScreenSaverTime -
		       (GetTimeInMillis() - lastDeviceEventTime.milliseconds));
	    if (timeout <= 0) /* may be forced by AutoResetServer() */
	    {
		long timeSinceSave;

		timeSinceSave = -timeout;
		if ((timeSinceSave >= timeTilFrob) && (timeTilFrob >= 0))
		{
		    ResetOsBuffers(); /* not ideal, but better than nothing */
		    SaveScreens(SCREEN_SAVER_ON, ScreenSaverActive);
		    if (ScreenSaverInterval)
			/* round up to the next ScreenSaverInterval */
			timeTilFrob = ScreenSaverInterval *
				((timeSinceSave + ScreenSaverInterval) /
					ScreenSaverInterval);
		    else
			timeTilFrob = -1;
		}
		timeout = timeTilFrob - timeSinceSave;
	    }
	    else
	    {
		if (timeout > ScreenSaverTime)
		    timeout = ScreenSaverTime;
		timeTilFrob = 0;
	    }
	    if (timeTilFrob >= 0)
	    {
		waittime.tv_sec = timeout / MILLI_PER_SECOND;
		waittime.tv_usec = (timeout % MILLI_PER_SECOND) *
					(1000000 / MILLI_PER_SECOND);
		wt = &waittime;
	    }
	    else
	    {
		wt = NULL;
	    }
	}
	else
	    wt = NULL;

	COPYBITS(AllSockets, BITS(LastSelectMask));
#ifdef apollo
        COPYBITS(apInputMask, LastWriteMask);
#endif
	BlockHandler((pointer)&wt, (pointer)BITS(LastSelectMask));

#ifdef SVR4
	if ((wt == NULL) || wt->tv_sec || wt->tv_usec)
	{
	    /* If mouse/keyboard input is pending, we don't want to sleep in
	     * select(), so instead wait for 0 seconds.  Note that signals
	     * must be enabled BEFORE checking for pending input so that we
	     * won't ever sleep if there is pending input (avoid critical
	     * sections).  Enabling signals AFTER checking for input would
	     * introduce a window: the xq driver could signal after checking
	     * for input but before getting into select(2) system call.  The
	     * signal would have no effect and the server would sleep until
	     * select returns whilst the pending input remains unprocessed.
	     */
	    queue->xq_sigenable = 1;
	    if (*checkForInput[0] != *checkForInput[1])
	    {
		waittime.tv_sec = 0;
		waittime.tv_usec = 0;
		wt = &waittime;
		queue->xq_sigenable = 0;
	    }
# ifdef BUILTIN
	    else if (BIGlobal.dummy_pipe >= 0)
		FD_SET(BIGlobal.dummy_pipe, &LastSelectMask);
# endif
	}
#endif /* SVR4 */

#ifdef XTESTEXT1
	/* XXX how does this interact with new write block handling? */
	/* XXX Do we need to enable signals (xq_sigenable) here? */
	if (playback_on) {
	    wt = &waittime;
	    XTestComputeWaitTime (&waittime);
	}
#endif /* XTESTEXT1 */

	/* keep this check close to select() call to minimize race */
	if (dispatchException)
	    i = -1;
	else if (AnyClientsWriteBlocked)
	{
	    COPYBITS(ClientsWriteBlocked, BITS(clientsWritable));
	    i = select(MAXSOCKS,
#ifdef SVR4
		       &LastSelectMask, &clientsWritable,
#else
		       (int *)LastSelectMask, (int *)clientsWritable,
#endif
		       NULL, wt);
	}
	else
	    i = select(MAXSOCKS,
#ifdef apollo
		       (int *)LastSelectMask, (int *)LastWriteMask,
#elif defined(SVR4)
		       &LastSelectMask, NULL,
#else
		       (int *)LastSelectMask, NULL,
#endif
		       NULL, wt);
#ifdef SVR4
	queue->xq_sigenable = 0;
#endif	
	selecterr = errno;
	WakeupHandler((unsigned long)i, (pointer)BITS(LastSelectMask));
#ifdef XTESTEXT1
	if (playback_on) {
	    i = XTestProcessInputAction (i, &waittime);
	}
#endif /* XTESTEXT1 */
	if (i <= 0) /* An error or timeout occurred */
	{

	    if (dispatchException)
		return 0;
#ifdef SVR4
	    FD_ZERO(&clientsWritable);
#else
	    CLEARBITS(clientsWritable);
#endif
	    if (i < 0) {
		if (selecterr == EBADF)    /* Some client disconnected */
		{
		    CheckConnections ();
		    if (! ANYSET (AllClients))
			return 0;
		}
		else if (selecterr != EINTR)
		    ErrorF("WaitForSomething(): select: errno=%d\n", selecterr);
#ifdef BUILTIN
		else if (BIGlobal.poll_addr == 0)
		    BIGlobal.poll_addr = BIGlobal.last_poll_addr;
#endif
	    }
#ifdef SVR4
	    /* Common break: if we lost a client, if we had an error or
	     * interrupt, or if we timed out.  We need to break to get back
	     * to Dispatch().  (If we lost a client, we want Dispatch() to
	     * check nClients for us.  If we're here due to an interrupt,
	     * it's probably due to a request for a switch of VTs, and
	     * Dispatch()'ll handle that.  If we timed out, it is likely
	     * because there is pending input, so we want Dispatch to process
	     * the input events.)
	     */
	    break;
#else
	    if (*checkForInput[0] != *checkForInput[1])
		return 0;
#endif
	}
	else
	{
#ifdef BUILTIN
	    if ((BIGlobal.dummy_pipe >= 0) &&
		FD_ISSET(BIGlobal.dummy_pipe, &LastSelectMask))
	    {
		char buf[512];

		read(BIGlobal.dummy_pipe, buf, 512);
		FD_CLR(BIGlobal.dummy_pipe, &LastSelectMask);
		i--;
	    }
#endif
	    if (AnyClientsWriteBlocked && ANYSET(BITS(clientsWritable)))
	    {
		NewOutputPending = TRUE;
		ORBITS(OutputPending, BITS(clientsWritable), OutputPending);
		UNSETBITS(ClientsWriteBlocked, BITS(clientsWritable));
		if (!ANYSET(ClientsWriteBlocked))
		    AnyClientsWriteBlocked = FALSE;
	    }

	    MASKANDSETBITS(devicesReadable, BITS(LastSelectMask), EnabledDevices);
#ifdef	hpux
		    /* call the HIL driver to gather inputs. 	*/
	    if (ANYSET(devicesReadable)) store_inputs (devicesReadable);
#endif /* hpux */

	    MASKANDSETBITS(clientsReadable, BITS(LastSelectMask), AllClients); 
	    if (BITS(LastSelectMask)[0] & WellKnownConnections) 
		QueueWorkProc(EstablishNewConnections, NULL,
			      (pointer)BITS(LastSelectMask)[0]);
	    if (ANYSET(devicesReadable) || ANYSET(clientsReadable))
		break;
	}
#ifdef SVR4
	/* Typically not reached */
	if (*checkForInput[0] != *checkForInput[1])
	  ProcessInputEvents();
#endif
    }

    nready = 0;
    if (ANYSET(clientsReadable))
    {
	for (i=0; i<mskcnt; i++)
	{
	    while (clientsReadable[i])
	    {
		curclient = ffs (clientsReadable[i]) - 1;
		pClientsReady[nready++] = 
			ConnectionTranslation[curclient + (i << 5)];
		clientsReadable[i] &= ~(1 << curclient);
	    }
	}	
    }
    return nready;
}

#ifndef ANYSET
/*
 * This is not always a macro.
 */
ANYSET(src)
    long	*src;
{
    int i;

    for (i=0; i<mskcnt; i++)
	if (src[ i ])
	    return (TRUE);
    return (FALSE);
}
#endif
