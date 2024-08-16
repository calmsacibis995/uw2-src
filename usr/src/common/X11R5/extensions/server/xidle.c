/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)r5extensions:server/xidle.c	1.2"
/*
 * Copyright 1989,1991 University of Wisconsin-Madison
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Wisconsin-Madison not
 * be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  The University of
 * Wisconsin-Madison makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE UNIVERSITY OF WISCONSIN-MADISON DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE UNIVERSITY OF WISCONSIN-MADISON BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Author: Tim Theisen           Systems Programmer
 * Internet: tim@cs.wisc.edu       Department of Computer Sciences
 *     UUCP: uwvax!tim             University of Wisconsin-Madison
 *    Phone: (608)262-0438         1210 West Dayton Street
 *      FAX: (608)262-9777         Madison, WI   53706
 */

#define NEED_REPLIES
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "extnsionst.h"
#include "dixstruct.h"
#define _XIDLE_SERVER_  /* don't want Xlib structures */
#include "xidlestr.h"

#include <sys/time.h>

static unsigned char XIdleReqCode = 0;
static int XIdle();
extern int SProcSimpleReq();
static void  XIdleReset();

/****************
 * XIdleExtensionInit
 *
 * Called from InitExtensions in main() or from QueryExtension() if the
 * extension is dynamically loaded.
 *
 * XIdle has no events or errors (other than the core errors)
 ****************/

void
XIdleExtensionInit()
{
    ExtensionEntry *extEntry, *AddExtension();

    extEntry = AddExtension(XIDLENAME, 0, 0, XIdle, SProcSimpleReq,
			    XIdleReset, StandardMinorOpcode);
    if (extEntry) {
	XIdleReqCode = extEntry->base;
    }
    else {
	FatalError("XIdleExtensionInit: AddExtensions failed\n");
    }
}

static int
XIdle(client)
    register ClientPtr client;
{
    xGetIdleTimeReply rep;
    register int n;
	extern long lastEventTime;
    struct timeval  tp;

    REQUEST(xGetIdleTimeReq);
    REQUEST_SIZE_MATCH(xGetIdleTimeReq);

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
/*
    rep.time = TimeSinceLastInputEvent();
    rep.time = lastEventTime;
*/
    gettimeofday(&tp, (struct timezone *) NULL);
    rep.time = ((tp.tv_sec * 1000) + (tp.tv_usec / 1000)) - lastEventTime;

    if (client->swapped) {
	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length, n);
	swapl(&rep.time, n);
    }
    WriteToClient(client, sizeof(xGetIdleTimeReply), (char *)&rep);
    return (client->noClientException);
}

/*ARGSUSED*/
static void
XIdleReset(extEntry)
ExtensionEntry  *extEntry;
{
}
