/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:server/xtest1di.c	1.3"

/*
 *	File:  xtest1di.c
 *
 *	This file contains the device independent parts of the input
 *	synthesis extension.
 */

/*

Copyright 1986, 1987, 1988 by Hewlett-Packard Corporation
Copyright 1986, 1987, 1988 by the Massachusetts Institute of Technology

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of M.I.T. not be used in
advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

Hewlett-Packard and M.I.T. make no representations about the 
suitability of this software for any purpose.  It is provided 
"as is" without express or implied warranty.

This software is not subject to any license of the American
Telephone and Telegraph Company or of the Regents of the
University of California.

*/

/*****************************************************************************
 * include files
 ****************************************************************************/

#define	 NEED_EVENTS
#define	 NEED_REPLIES

#include <stdio.h>
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "os.h"
#include "gcstruct.h"   
#include "extnsionst.h"
#include "dixstruct.h"
#include "opaque.h"
#define  XTestSERVER_SIDE
#include "xtestext1.h"

/*****************************************************************************
 * defines
 ****************************************************************************/

/*****************************************************************************
 * externals
 ****************************************************************************/

/*
 * holds the addresses of the routines that handle byte-swapping of replys
 */
extern void		(* ReplySwapVector[256]) ();
/*
 * holds the addresses of the routines that handle byte-swapping of events
 */
extern void		(* EventSwapVector[128]) ();
/*
 * id of client using XTestGetInput
 *
 * defined in xtest1dd.c
 */
extern ClientPtr	current_xtest_client;
/*
 * id of client using XTestFakeInput
 *
 * defined in xtest1dd.c
 */
extern ClientPtr	playback_client;

/*****************************************************************************
 * variables
 ****************************************************************************/

/*
 * Holds the request type code for this extension.  The request type code
 * for this extension may vary depending on how many extensions are installed
 * already, so the initial value given below will be added to the base request
 * code that is aquired when this extension is installed.
 */
static int 		XTestReqCode = 0;
/*
 * Holds the two event type codes for this extension.  The event type codes
 * for this extension may vary depending on how many extensions are installed
 * already, so the initial values given below will be added to the base event
 * code that is aquired when this extension is installed.
 */
int 			XTestInputActionType = 0;
int 			XTestFakeAckType = 1;
/*
 * true => monitor stealing input
 */
int			on_steal_input = FALSE;
/*
 * true => monitor alone getting input
 */
int			exclusive_steal = FALSE;
/*
 * holds the resource type assigned to this extension
 */
static RESTYPE		XTestType;
/*
 * holds the resource ID for the client currently using XTestGetInput
 */
static XID		current_client_id;

/*****************************************************************************
 * function declarations
 ****************************************************************************/

static int	ProcXTestDispatch();
static int	SProcXTestDispatch();
static void	XTestResetProc();
static int	ProcTestFakeInput();
static int	SProcTestFakeInput();
static int	ProcTestGetInput();
static int	SProcTestGetInput();
static int	ProcTestStopInput();
static int	SProcTestStopInput();
static int	ProcTestReset();
static int	SProcTestReset();
static int	ProcTestQueryInputSize();
static int	SProcTestQueryInputSize();
static void	SReplyXTestDispatch();
static void	SEventXTestDispatch();
void	NotImplemented();

void	abort_play_back();
void	return_input_array_size();
void	steal_input();
void	stop_stealing_input();
void	flush_input_actions();
void	parse_fake_input();

static void	XTestCurrentClientGone();

/*****************************************************************************
 *
 *	XTestExtension1Init
 *
 *	Called from InitExtensions in main() or from QueryExtension() if the
 *	extension is dynamically loaded.
 *
 *	XTestExtension1Init has no events or errors
 *	(other than the core errors).
 */
void
XTestExtension1Init()
{
	/*
	 * holds the pointer to the extension entry structure
	 */
	ExtensionEntry	*extEntry;
	/*
	 * This routine adds the extension to the server extension table.
	 */
	ExtensionEntry	*AddExtension();

	extEntry = AddExtension(XTestEXTENSION_NAME,
				XTestEVENT_COUNT,
				0,
				ProcXTestDispatch,
				SProcXTestDispatch,
				XTestResetProc,
				StandardMinorOpcode);
	if (extEntry)
	{
		/*
		 * remember the request code assigned to this extension
		 */
		XTestReqCode = extEntry->base;
		/*
		 * make an atom saying that this extension is present
		 */
		(void) MakeAtom(XTestEXTENSION_NAME,
				strlen(XTestEXTENSION_NAME),
				TRUE);
		/*
		 * remember the event codes assigned to this extension
		 */
		XTestInputActionType += extEntry->eventBase;
		XTestFakeAckType += extEntry->eventBase;
		/*
		 * install the routine to handle byte-swapping the replies
		 * for this extension in the ReplySwapVector table
		 */
		ReplySwapVector[XTestReqCode] = SReplyXTestDispatch;
		/*
		 * install the routine to handle byte-swapping the events
		 * for this extension in the EventSwapVector table
		 */
		EventSwapVector[XTestInputActionType] = SEventXTestDispatch;
		EventSwapVector[XTestFakeAckType] = SEventXTestDispatch;
		/*
		 * get the resource type for this extension
		 */
		XTestType = CreateNewResourceType(XTestCurrentClientGone);
		if (XTestType == 0)
		{
			FatalError("XTestExtension1Init: CreateNewResourceType failed\n");
		}
	} 
	else 
	{
		FatalError("XTestExtension1Init: AddExtensions failed\n");
	}
}

/*****************************************************************************
 *
 *	ProcXTestDispatch
 *
 *
 */
static int
ProcXTestDispatch(client)
	register ClientPtr	client;
{
	REQUEST(xReq);
	if (stuff->data == X_TestFakeInput)
	{
		return(ProcTestFakeInput(client));
	}
	else if (stuff->data == X_TestGetInput)
	{
		return(ProcTestGetInput(client));
	}
	else if (stuff->data == X_TestStopInput)
	{
		return(ProcTestStopInput(client));
	}
	else if (stuff->data == X_TestReset)
	{
		return(ProcTestReset(client));
	}
	else if (stuff->data == X_TestQueryInputSize)
	{
		return(ProcTestQueryInputSize(client));
	}
	else
	{
		SendErrorToClient(client,
				  XTestReqCode,
				  stuff->data,
				  None,
				  BadRequest);
		return(BadRequest);
	}
}

/*****************************************************************************
 *
 *	SProcXTestDispatch
 *
 *
 */
static int
SProcXTestDispatch(client)
	register ClientPtr	client;
{
	REQUEST(xReq);
	if (stuff->data == X_TestFakeInput)
	{
		return(SProcTestFakeInput(client));
	}
	else if (stuff->data == X_TestGetInput)
	{
		return(SProcTestGetInput(client));
	}
	else if (stuff->data == X_TestStopInput)
	{
		return(SProcTestStopInput(client));
	}
	else if (stuff->data == X_TestReset)
	{
		return(SProcTestReset(client));
	}
	else if (stuff->data == X_TestQueryInputSize)
	{
		return(SProcTestQueryInputSize(client));
	}
	else
	{
		SendErrorToClient(client,
				  XTestReqCode,
				  stuff->data,
				  None,
				  BadRequest);
		return(BadRequest);
	}
}

/*****************************************************************************
 *
 *	SProcTestFakeInput
 *
 *
 */
static int
SProcTestFakeInput(client)
	register ClientPtr	client;
{
	/*
	 * used in the swaps and swapl macros for temporary storage space
	 */
	register char	n;
	/*
	 * index counter
	 */
	int		i;
	/*
	 * pointer to the next input action in the request
	 */
	CARD8		*input_action_ptr;
	/*
	 * holds the type of the next input action in the request
	 */
	int		input_action_type;

	REQUEST(xTestFakeInputReq);
	/*
	 * byte-swap the fields in the request
	 */
	swaps(&stuff->length, n);
	swapl(&stuff->ack, n);
	/*
	 * have to parse and then byte-swap the input action list here
	 */
	for (i = 0; i < XTestMAX_ACTION_LIST_SIZE;)
	{
		/*
		 * point to the next input action in the request
		 */
		input_action_ptr = &(((xTestFakeInputReq *) stuff)->action_list[i]);
		/*
		 * figure out what type of input action it is
		 */
		input_action_type = (*input_action_ptr) & XTestACTION_TYPE_MASK;
		/*
		 * byte-swap the input action according to it's type
		 */
		switch (input_action_type)
		{
		case XTestKEY_ACTION:
			/*
			 * byte-swap the delay_time field
			 */
			swaps(&(((XTestKeyInfo *) input_action_ptr)->delay_time), n);
			/*
			 * advance to the next input action
			 */
			i += sizeof(XTestKeyInfo);
			break;
		case XTestMOTION_ACTION:
			/*
			 * byte-swap the delay_time field
			 */
			swaps(&(((XTestMotionInfo *) input_action_ptr)->delay_time), n);
			/*
			 * advance to the next input action
			 */
			i += sizeof(XTestMotionInfo);
			break;
		case XTestJUMP_ACTION:
			/*
			 * byte-swap the jumpx field
			 */
			swaps(&(((XTestJumpInfo *) input_action_ptr)->jumpx), n);
			/*
			 * byte-swap the jumpy field
			 */
			swaps(&(((XTestJumpInfo *) input_action_ptr)->jumpy), n);
			/*
			 * byte-swap the delay_time field
			 */
			swaps(&(((XTestJumpInfo *) input_action_ptr)->delay_time), n);
			/*
			 * advance to the next input action
			 */
			i += sizeof(XTestJumpInfo);
			break;
		default:
			/*
			 * if this is a delay input action, then byte-swap it,
			 * otherwise we have reached the end of the input
			 * actions in this request
			 */
			if (XTestUnpackDeviceID((char)*input_action_ptr) ==
			    XTestDELAY_DEVICE_ID)
			{
				/*
				 * byte-swap the delay_time field
				 */
				swapl(&(((XTestDelayInfo *) input_action_ptr)->delay_time), n);
				/*
				 * advance to the next input action
				 */
				i += sizeof(XTestDelayInfo);
			}
			else
			{
				/*
				 * if the input action header byte is 0 or
				 * ill-formed, then there are no more input
				 * actions in this request
				 */
				i = XTestMAX_ACTION_LIST_SIZE;
			}
			break;
		}
	}
	return(ProcTestFakeInput(client));
}

/*****************************************************************************
 *
 *	SProcTestGetInput
 *
 *
 */
static int
SProcTestGetInput(client)
	register ClientPtr	client;
{
	/*
	 * used in the swaps and swapl macros for temporary storage space
	 */
	register char	n;

	REQUEST(xTestGetInputReq);
	/*
	 * byte-swap the fields in the request
	 */
	swaps(&stuff->length, n);
	swapl(&stuff->mode, n);
	return(ProcTestGetInput(client));
}

/*****************************************************************************
 *
 *	SProcTestStopInput
 *
 *
 */
static int
SProcTestStopInput(client)
	register ClientPtr	client;
{
	/*
	 * used in the swaps and swapl macros for temporary storage space
	 */
	register char	n;

	REQUEST(xTestStopInputReq);
	/*
	 * byte-swap the length field in the request
	 */
	swaps(&stuff->length, n);
	return(ProcTestStopInput(client));
}

/*****************************************************************************
 *
 *	SProcTestReset
 *
 *
 */
static int
SProcTestReset(client)
	register ClientPtr	client;
{
	/*
	 * used in the swaps and swapl macros for temporary storage space
	 */
	register char	n;

	REQUEST(xTestResetReq);
	/*
	 * byte-swap the length field in the request
	 */
	swaps(&stuff->length, n);
	return(ProcTestReset(client));
}

/*****************************************************************************
 *
 *	SProcTestQueryInputSize
 *
 *
 */
static int
SProcTestQueryInputSize(client)
	register ClientPtr	client;
{
	/*
	 * used in the swaps and swapl macros for temporary storage space
	 */
	register char	n;

	REQUEST(xTestQueryInputSizeReq);
	/*
	 * byte-swap the length field in the request
	 */
	swaps(&stuff->length, n);
	return(ProcTestQueryInputSize(client));
}

/*****************************************************************************
 *
 *	ProcTestFakeInput
 *
 *
 */
static int
ProcTestFakeInput(client)
	register ClientPtr	client;
{
	REQUEST(xTestFakeInputReq);
	REQUEST_SIZE_MATCH(xTestFakeInputReq);
	if (playback_client == NULL || playback_client == client)
	{
		/*
		 * This extension does not need to clean up any
		 * server state when a client using this function
		 * "goes away".  The server will just process any
		 * input actions that have already been sent to it,
		 * and will then reset its association with a client.
		 */
		parse_fake_input(client, (char *)stuff);
		return(Success);
	}
	else
	{
		/*
		 * this is a request by another client to send fake
		 * input while the server is still being used
		 */
		SendErrorToClient(client,
				  XTestReqCode,
				  X_TestFakeInput,
				  None,
				  BadAccess);
		return(BadAccess);
	}
}

/*****************************************************************************
 *
 *	ProcTestGetInput
 *
 *
 */
static int
ProcTestGetInput(client)
	register ClientPtr	client;
{
	REQUEST(xTestGetInputReq);
	REQUEST_SIZE_MATCH(xTestGetInputReq);
	if (on_steal_input)
	{
		/*
		 * this is a request by another client to get fake input
		 * while the server is still sending input to the first client
		 */
		SendErrorToClient(client,
				  XTestReqCode,
				  X_TestGetInput,
				  None,
				  BadAccess);
		return(BadAccess);
	}
	else
	{ 
		/*
		 * Set up a resource associated with the client using this
		 * function so that this extension gets called when the 
		 * client "goes away".  This allows this extension to
		 * clean up the server state.
		 */
		current_client_id = FakeClientID(client->index);
		AddResource(current_client_id,
			    XTestType,
			    0);
		/*
		 * indicate that a client is stealing input
		 */
		on_steal_input = TRUE;
		if ((stuff->mode & XTestEXCLUSIVE) == 0)
		{
			exclusive_steal = FALSE;
		}
		else 
		{
			exclusive_steal = TRUE;
		}
		steal_input(client, stuff->mode);
		return(Success);
	}
}

/*****************************************************************************
 *
 *	ProcTestStopInput
 *
 *
 */
static int
ProcTestStopInput(client)
	register ClientPtr	client;
{
	REQUEST(xTestStopInputReq);
	REQUEST_SIZE_MATCH(xTestStopInputReq);
	if (on_steal_input && (current_xtest_client == client)) 
	{ 
		on_steal_input = FALSE;
		exclusive_steal = FALSE;
		stop_stealing_input();	
		/*
		 * remove the resource associated with this client
		 */
		FreeResource(current_client_id, RT_NONE);
		return(Success);
	}
	else
	{
		/*
		 * this is a request to stop fake input when fake input has
		 * never been started or from a client that hasn't started
		 * fake input
		 */
		SendErrorToClient(client,
				  XTestReqCode,
				  X_TestStopInput,
				  None,
				  BadAccess);
		return(BadAccess);
	}
}

/*****************************************************************************
 *
 *	ProcTestReset
 *
 *
 */
static int
ProcTestReset(client)
	register ClientPtr	client;
{
	REQUEST(xTestResetReq);
	REQUEST_SIZE_MATCH(xTestResetReq);
	on_steal_input = FALSE;
	exclusive_steal = FALSE;
	/*
	 * defined in xtest1dd.c
	 */
	stop_stealing_input();	
	/*
	 * defined in xtest1dd.c
	 */
	abort_play_back();
	return(Success);
}

/*****************************************************************************
 *
 *	ProcTestQueryInputSize
 *
 *
 */
static int
ProcTestQueryInputSize(client)
	register ClientPtr	client;
{
	REQUEST(xTestQueryInputSizeReq);
	REQUEST_SIZE_MATCH(xTestQueryInputSizeReq);
	/*
	 * defined in xtest1dd.c
	 */
	return_input_array_size(client);
	return(Success);
}

/*****************************************************************************
 *
 *	XTestResetProc
 *
 *	This function is called by the server when the server has no clients
 *	connected to it.  It must put eveything back the way it was before
 *	this extension was installed.
 */
static void
XTestResetProc()
{
	/*
	 * remove the routine to handle byte-swapping the replies
	 * for this extension in the ReplySwapVector table
	 */
	ReplySwapVector[XTestReqCode] = NotImplemented;
	/*
	 * remove the routine to handle byte-swapping the events
	 * for this extension in the EventSwapVector table
	 */
	EventSwapVector[XTestInputActionType] = NotImplemented;
	EventSwapVector[XTestFakeAckType] = NotImplemented;
	/*
	 * reset the variables initialized just once at load time
	 */
	XTestReqCode = 0;
	XTestInputActionType = 0;
	XTestFakeAckType = 1;
	on_steal_input = FALSE;
	exclusive_steal = FALSE;
}

/*****************************************************************************
 *
 *	ProcTestQueryInputSize
 *
 *	This routine is called when a client that has asked for input actions
 *	to be sent to it "goes away".  This routine must clean up the 
 *	server state.
 */
/*ARGSUSED*/
static void
XTestCurrentClientGone(value, id)
	int	value;
	XID	id;
{
	/*
	 * defined in xtest1dd.c
	 */
	flush_input_actions();
	on_steal_input = FALSE;
	exclusive_steal = FALSE;
	/*
	 * defined in xtestdd.c
	 */
	stop_stealing_input();	
}

/*****************************************************************************
 *
 *	SReplyXTestDispatch
 *
 *	Swap any replies defined in this extension.
 */
static void
SReplyXTestDispatch(client_ptr, size, reply_ptr)
	ClientPtr	client_ptr;
	int		size;
	char		*reply_ptr;
{
	/*
	 * used in the swaps and swapl macros for temporary storage space
	 */
	register char	n;
	/*
	 * pointer to xTestQueryInputSizeReply
	 */
	xTestQueryInputSizeReply	*rep_ptr;

	/*
	 * there is only one reply in this extension, so byte-swap it
	 */
	rep_ptr = (xTestQueryInputSizeReply *) reply_ptr;
	swaps(&(rep_ptr->sequenceNumber), n);
	swapl(&(rep_ptr->length), n);
	swapl(&(rep_ptr->size_return), n);
	/*
	 * now write the swapped reply to the client
	 */
	WriteToClient(client_ptr, size, reply_ptr);
}

/*****************************************************************************
 *
 *	SEventXTestDispatch
 *
 *	Swap any events defined in this extension.
 */
static void
SEventXTestDispatch(from, to)
	xEvent	*from;
	xEvent	*to;
{
	/*
	 * used in the swaps and swapl macros for temporary storage space
	 */
	register char	n;
	/*
	 * index counter
	 */
	int		i;
	/*
	 * pointer to the next input action in the event
	 */
	CARD8		*input_action_ptr;
	/*
	 * holds the type of the next input action in the event
	 */
	int		input_action_type;


	/*
	 * copy the type information from the "from" event to the "to" event
	 */
	((xTestInputActionEvent *) to)->type =
	((xTestInputActionEvent *) from)->type;
	/*
	 * copy the sequence number information from the "from" event to the
	 * "to" event
	 */
	((xTestInputActionEvent *) to)->sequenceNumber =
	((xTestInputActionEvent *) from)->sequenceNumber;
	/*
	 * byte-swap the sequence number in the "to" event
	 */
	swaps(&(((xTestInputActionEvent *) to)->sequenceNumber), n);
	/*
	 * If the event is an xTestInputActionEvent, then it needs more
	 * processing.  Otherwise, it is an xTestFakeAckEvent, which
	 * has no other information in it.
	 */
	if ((((xTestInputActionEvent *) to)->type & 0x7f) ==
	    XTestInputActionType)
	{
		/*
		 * copy the input actions from the "from" event
		 * to the "to" event
		 */
		for (i = 0; i < XTestACTIONS_SIZE;)
		{
			((xTestInputActionEvent *) to)->actions[i] =
			((xTestInputActionEvent *) from)->actions[i];
		}
		/*
		 * byte-swap the input actions in the "to" event
		 */
		for (i = 0; i < XTestACTIONS_SIZE;)
		{
			/*
			 * point to the next input action in the event
			 */
			input_action_ptr = &(((xTestInputActionEvent *) to)->actions[i]);
			/*
			 * figure out what type of input action it is
			 */
			input_action_type = (*input_action_ptr) &
					    XTestACTION_TYPE_MASK;
			/*
			 * byte-swap the input action according to it's type
			 */
			switch (input_action_type)
			{
			case XTestKEY_ACTION:
				/*
				 * byte-swap the delay_time field
				 */
				swaps(&(((XTestKeyInfo *) input_action_ptr)->delay_time), n);
				/*
				 * advance to the next input action
				 */
				i += sizeof(XTestKeyInfo);
				break;
			case XTestMOTION_ACTION:
				/*
				 * byte-swap the delay_time field
				 */
				swaps(&(((XTestMotionInfo *) input_action_ptr)->delay_time), n);
				/*
				 * advance to the next input action
				 */
				i += sizeof(XTestMotionInfo);
				break;
			case XTestJUMP_ACTION:
				/*
				 * byte-swap the jumpx field
				 */
				swaps(&(((XTestJumpInfo *) input_action_ptr)->jumpx), n);
				/*
				 * byte-swap the jumpy field
				 */
				swaps(&(((XTestJumpInfo *) input_action_ptr)->jumpy), n);
				/*
				 * byte-swap the delay_time field
				 */
				swaps(&(((XTestJumpInfo *) input_action_ptr)->delay_time), n);
				/*
				 * advance to the next input action
				 */
				i += sizeof(XTestJumpInfo);
				break;
			default:
				/*
				 * if this is a delay input action, then
				 * byte-swap it, otherwise we have reached the
				 * end of the input actions in this event
				 */
				if (XTestUnpackDeviceID((char)*input_action_ptr) ==
				    XTestDELAY_DEVICE_ID)
				{
					/*
					 * byte-swap the delay_time field
					 */
					swapl(&(((XTestDelayInfo *) input_action_ptr)->delay_time), n);
					/*
					 * advance to the next input action
					 */
					i += sizeof(XTestDelayInfo);
				}
				else
				{
					/*
					 * if the input action header byte is 0
					 * or ill-formed, then there are no
					 * more input actions in this event
					 */
					i = XTestACTIONS_SIZE;
				}
				break;
			}
		}
	}
}
