/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xinput/xgetselev.c	1.2"

/************************************************************
Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, California, and the 
Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Hewlett-Packard or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

HEWLETT-PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
HEWLETT-PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

/* $XConsortium: xgetselev.c,v 1.9 90/05/18 15:35:21 rws Exp $ */

/***********************************************************************
 *
 * Extension function to get the current selected events for a given window.
 *
 */

#define	 NEED_EVENTS
#define	 NEED_REPLIES
#include "X.h"				/* for inputstr.h    */
#include "Xproto.h"			/* Request macro     */
#include "XI.h"
#include "XIproto.h"
#include "inputstr.h"			/* DeviceIntPtr	     */
#include "windowstr.h"			/* window struct     */

extern	int 		IReqCode;
extern	void		(* ReplySwapVector[256]) ();
DeviceIntPtr		LookupDeviceIntRec();

/***********************************************************************
 *
 * This procedure gets the current selected extension events.
 *
 */

int
SProcXGetSelectedExtensionEvents(client)
    register ClientPtr client;
    {
    register char n;

    REQUEST(xGetSelectedExtensionEventsReq);
    swaps(&stuff->length, n);
    swapl(&stuff->window, n);
    return(ProcXGetSelectedExtensionEvents(client));
    }

/***********************************************************************
 *
 * This procedure gets the current device select mask,
 * if the client and server have a different byte ordering.
 *
 */

int
ProcXGetSelectedExtensionEvents(client)
    register ClientPtr client;
    {
    int					i;
    int					total_length = 0;
    xGetSelectedExtensionEventsReply	rep;
    WindowPtr				pWin;
    XEventClass				*buf;
    XEventClass				*tclient;
    XEventClass				*aclient;
    XEventClass 			*ClassFromMask ();
    void				Swap32Write();
    OtherInputMasks			*pOthers;
    InputClientsPtr			others;

    REQUEST(xGetSelectedExtensionEventsReq);
    REQUEST_SIZE_MATCH(xGetSelectedExtensionEventsReq);

    rep.repType = X_Reply;
    rep.RepType = X_GetSelectedExtensionEvents;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.this_client_count = 0;
    rep.all_clients_count = 0;

    if (!(pWin = LookupWindow(stuff->window, client)))
        {
	SendErrorToClient(client, IReqCode, X_GetSelectedExtensionEvents, 0, 
		BadWindow);
	return Success;
        }

    if (pOthers=wOtherInputMasks(pWin))
	{
	for (others = pOthers->inputClients; others; others=others->next)
	    for (i=0; i<EMASKSIZE; i++)
		tclient = ClassFromMask (NULL, others->mask[i], i, 
		    &rep.all_clients_count, COUNT);

	for (others = pOthers->inputClients; others; others=others->next)
	    if (SameClient(others, client))
		{
		for (i=0; i<EMASKSIZE; i++)
		    tclient = ClassFromMask (NULL, others->mask[i], i, 
			&rep.this_client_count, COUNT);
		break;
		}

	total_length = (rep.all_clients_count + rep.this_client_count) * 
	    sizeof (XEventClass);
	rep.length = (total_length + 3) >> 2;
	buf = (XEventClass *) Xalloc (total_length);

	tclient = buf;
	aclient = buf + rep.this_client_count;
	if (others)
	    for (i=0; i<EMASKSIZE; i++)
		tclient = ClassFromMask (tclient, others->mask[i], i, NULL, CREATE);

	for (others = pOthers->inputClients; others; others=others->next)
	    for (i=0; i<EMASKSIZE; i++)
		aclient = ClassFromMask (aclient, others->mask[i], i, NULL, CREATE);
	}

    WriteReplyToClient (client, sizeof(xGetSelectedExtensionEventsReply), &rep);

    if (total_length)
	{
	client->pSwapReplyFunc = Swap32Write;
	WriteSwappedDataToClient( client, total_length, buf);
	Xfree (buf);
	}
    return Success;
    }

/***********************************************************************
 *
 * This procedure writes the reply for the XGetSelectedExtensionEvents function,
 * if the client and server have a different byte ordering.
 *
 */

SRepXGetSelectedExtensionEvents (client, size, rep)
    ClientPtr	client;
    int		size;
    xGetSelectedExtensionEventsReply	*rep;
    {
    register char n;

    swaps(&rep->sequenceNumber, n);
    swapl(&rep->length, n);
    swaps(&rep->this_client_count, n);
    swaps(&rep->all_clients_count, n);
    WriteToClient(client, size, rep);
    }
