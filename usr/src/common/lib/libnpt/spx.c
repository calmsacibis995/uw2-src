/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnpt:spx.c	1.3"
/*
 * Copyright 1989, 1991 Unpublished Work of Novell, Inc. All Rights Reserved.
 *
 * THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL,
 * PROPRIETARY AND TRADE SECRET INFORMATION OF NOVELL, INC. ACCESS
 * TO THIS WORK IS RESTRICTED TO (I) NOVELL EMPLOYEES WHO HAVE A
 * NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
 * ASSIGNMENTS AND (II) ENTITIES OTHER THAN NOVELL WHO HAVE
 * ENTERED INTO APPROPRIATE AGREEMENTS.
 * NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 */

#if !defined(NO_SCCS_ID) && !defined(lint)
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnpt/spx.c,v 1.2 1994/03/24 18:34:13 aclark Exp $";
#endif

/********************************************************************
 *
 * Program Name:  NPT library routines
 *
 * Filename:	  Spx.c
 *
 * Date Created:  November 1, 1990
 *
 * Version:		  0.00
 *
 * Programmers:	  Joe Ivie
 *
 * Description:	This file contains the SPX connection functions
 *				for sending data through this interface.
 *
 * COPYRIGHT (c) 1988 by Novell, Inc.  All Rights Reserved.
 *
 ********************************************************************/

#include "libnpt.h"

/********************************************************************/
/*
	Global defines
*/

#define WAIT_SECONDS		30
#define SPX_DEVICE			"/dev/nspx"

uint16		MakeSpxConnID();

/********************************************************************/
/*
	Global structures
*/

struct connect {
	struct connect	*next;
	int				fd;				/* which fd to send messages to	*/
	ipxAddr_t		ipxAddress;		/* The IPX address of other end	*/
	uint16			connectID;		/* TLI connect id				*/
};


/********************************************************************/
/*
	Global variables
*/

struct connect	*connectList = (struct connect *)NULL;


/********************************************************************/
/*
	SPX connection handling routines
*/

uint16
OpenSpxConnection(ipxAddress)
ipxAddr_t	*ipxAddress;
{
	struct connect	*newConnect;
	struct t_call	*tcall;
	struct t_bind	*tbind;

	dprintf("OpenSpxConnection: addr-");
	ps_od((uint8*)ipxAddress, sizeof(ipxAddr_t));

	/*
		See if we are already talking to the requested server
	*/
	newConnect = connectList;

	while (newConnect)
	{
		dprintf("OpenSpxConnection: list address=");
		ps_od((uint8*)&newConnect->ipxAddress, sizeof(ipxAddr_t));

		if (memcmp(&newConnect->ipxAddress, ipxAddress,
				NWMAX_INTERNET_ADDRESS_LENGTH) == 0)
		{
			dprintf("OpenSpxConnection: found address on list\n");
			return(newConnect->connectID);
		}

		newConnect = newConnect->next;
	}

	/*
		Create a new Connection List Entry
	*/
	newConnect = (struct connect*)malloc(sizeof(struct connect));

	if (newConnect == NULL)
	{
		dprintf("OpenSpxConnection: malloc failed\n");
		return(FALSE);
	}

	/*
		Open the SPX device
	*/
	newConnect->fd = t_open( SPX_DEVICE, O_RDWR, NULL);

	if (newConnect->fd == -1)
	{
		dprintf("OpenSpxConnection: t_open failed\n");
		goto FailedAfterAlloc;
	}

	/*
		Bind the SPX device to any socket

		set qlen to > 0 to get a unique socket
	*/
	tbind = (struct t_bind*)t_alloc(newConnect->fd, T_BIND, T_ADDR);
	if (tbind == NULL)
	{
		dprintf("OpenSpxConnection: t_alloc failed\n");
		goto FailedAfterOpen;
	}

	memset(tbind->addr.buf, 0, (int)tbind->addr.maxlen);
	tbind->addr.len = sizeof(ipxAddr_t);
	tbind->qlen = 1;

	if (t_bind(newConnect->fd, tbind, tbind) == -1)
	{
		dprintf("OpenSpxConnection: t_bind failed\n");
		t_free((char*)tbind, T_BIND);
		goto FailedAfterOpen;
	}

	t_free((char*)tbind, T_BIND);

	/*
		Call the requested address
	*/
	tcall = (struct t_call*)t_alloc(newConnect->fd, T_CALL,
		T_ADDR|T_OPT);

	if (tcall == NULL)
	{
		dprintf("OpenSpxConnection: t_alloc failed\n");
		goto FailedAfterOpen;
	}

	memcpy(tcall->addr.buf, (char*)ipxAddress, sizeof(ipxAddr_t));
	tcall->addr.len = sizeof(ipxAddr_t);

	if (t_connect(newConnect->fd, tcall, tcall) == -1)
	{
		dprintf("OpenSpxConnection: t_connect failed, t_errno = %d\n",
			t_errno);
#ifdef BLABBY
		t_error("OpenSpxConnection");
#endif
		goto FailedAfterTalloc;
	}

	t_free((char*)tcall, T_CALL);

	/*
		Everything went Great. The connection is established.
		Finish the newConnect entry and add it to the list.
	*/
	newConnect->connectID = MakeSpxConnID();
	if (newConnect->connectID == 0)
		goto FailedAfterOpen;

	memcpy(&newConnect->ipxAddress, (char*)ipxAddress,
		sizeof(ipxAddr_t));
	newConnect->next = connectList;
	connectList = newConnect;

	return(newConnect->connectID);

FailedAfterTalloc:
	t_free((char*)tcall, T_CALL);

FailedAfterOpen:
	t_close(newConnect->fd);

FailedAfterAlloc:
	free(newConnect);
	return(FALSE);
}


uint16
MakeSpxConnID()
{
	struct connect	*nextConnect;
	uint16			nextConnectID;

	nextConnectID = 1;
	nextConnect = connectList;

	while (nextConnect)
	{
		if (nextConnect->connectID == nextConnectID)
		{
			nextConnectID++;
			nextConnect = connectList;
		}
		else
			nextConnect = nextConnect->next;
	}

	return(nextConnectID);
}


CloseSpxConnection(connectID)
uint16		connectID;
{
	struct connect	*nextConnect;
	struct connect	*prevConnect;

	nextConnect = connectList;
	prevConnect = NULL;

	while (nextConnect)
	{
		if (nextConnect->connectID == connectID)
		{
			dprintf("CloseSpxConnection: removing connect id=%d\n", connectID);
			if (prevConnect)
			{
				prevConnect->next = nextConnect->next;
				nextConnect->next = NULL;
			}
			else
				connectList = nextConnect->next;

			t_close(nextConnect->fd);
			free((char*)nextConnect);

			return(TRUE);
		}

		prevConnect = nextConnect;
		nextConnect = nextConnect->next;
	}

	dprintf("CloseSpxConnection: connection not found\n");
	return(FALSE);
}


uint16
GetSpxReply(connectID, request, reply)
	uint16			connectID;		/* connection to send to		*/
	NptRequest_t	*request;		/* request information			*/
	NptReply_t		*reply;			/* may be NULL if no data needed */
{
	uint8			buffer[BUFSIZ];
	int				bufferSize, flags;
	int 			rtnCode;
	time_t			sendTime;
	uint16			temp16;
	uint32			temp32;
	struct connect	*connectInfo;
	struct t_discon *disconInfo;

	dprintf("GetSpxReply: id=%d req=%08X, rep=%08X\n",
		connectID, request, reply);

	/*
		Verify the connection ID
	*/
	connectInfo = connectList;
	while (connectInfo)
	{
		if (connectInfo->connectID == connectID)
			break;

		connectInfo = connectInfo->next;
	}

	if (!connectInfo)
	{
		dprintf("GetSpxReply: bad connectID %x\n", connectID);
		return(PSC_CONNECTION_TERMINATED);
	}

	/*
		Build the request buffer
	*/
	if (!request)
	{
		dprintf("GetSpxReply: bad request address\n");
		return(PSC_CONNECTION_TERMINATED);
	}

	memset((char*)buffer, 0, BUFSIZ);

	buffer[0] = request->FunctionNumber;
	bufferSize = 1;

	switch (request->FunctionNumber)
	{
	case CMD_LOGIN_TO_PRINT_SERVER:
		strncpy((char*)&buffer[1], (char*)request->FileServerName,
			NWMAX_SERVER_NAME_LENGTH);
		bufferSize += NWMAX_SERVER_NAME_LENGTH;
		/* Connection Number */
		temp16 = REVGETINT16(request->ConnectionNumber);
		memcpy((char*)&buffer[bufferSize], (char*)&temp16,
			sizeof(uint16));
		bufferSize += sizeof(uint16);
		break;
	case CMD_DOWN:
		buffer[bufferSize++] = request->Immediately;
		buffer[bufferSize++] = request->JobOutcome;
		break;
	case CMD_GET_PRINTER_STATUS:
		buffer[bufferSize++] = request->PrinterNumber;
		break;
	case CMD_STOP_PRINTER:
		buffer[bufferSize++] = request->PrinterNumber;
		buffer[bufferSize++] = request->JobOutcome;
		break;
	case CMD_START_PRINTER:
		buffer[bufferSize++] = request->PrinterNumber;
		break;
	case CMD_MOUNT_FORM:
		buffer[bufferSize++] = request->PrinterNumber;
		/* FormNumber Number */
		temp16 = REVGETINT16(request->FormNumber);
		memcpy((char*)&buffer[bufferSize], (char*)&temp16,
			sizeof(uint16));
		bufferSize += sizeof(uint16);
		break;
	case CMD_REWIND_PRINT_JOB:
		buffer[bufferSize++] = request->PrinterNumber;
		buffer[bufferSize++] = request->ByPage;
		buffer[bufferSize++] = request->Relative;
		/* CopyNumber */
		temp16 = REVGETINT16(request->CopyNumber);
		memcpy((char*)&buffer[bufferSize], (char*)&temp16,
			sizeof(uint16));
		bufferSize += sizeof(uint16);
		temp32 = REVGETINT32(request->Offset);
		memcpy((char*)&buffer[bufferSize], (char*)&temp32,
			sizeof(uint32));
		bufferSize += sizeof(uint32);
		break;
	case CMD_EJECT_PAGE:
		buffer[bufferSize++] = request->PrinterNumber;
		break;
	case CMD_MARK_PAGE:
		buffer[bufferSize++] = request->PrinterNumber;
		buffer[bufferSize++] = request->Character;
		break;
	case CMD_CHANGE_SERVICE_MODE:
		buffer[bufferSize++] = request->PrinterNumber;
		buffer[bufferSize++] = request->ServiceMode;
		break;
	case CMD_GET_JOB_STATUS:
		buffer[bufferSize++] = request->PrinterNumber;
		break;
	case CMD_ABORT_JOB:
		buffer[bufferSize++] = request->PrinterNumber;
		buffer[bufferSize++] = request->JobOutcome;
		break;
	case CMD_SCAN_QUEUE_LIST:		/* aka GetQueuesServiced */
		buffer[bufferSize++] = request->PrinterNumber;
		temp16 = REVGETINT16(request->Sequence16);
		memcpy((char*)&buffer[bufferSize], (char*)&temp16,
			sizeof(uint16));
		bufferSize += sizeof(uint16);
		break;
	case CMD_CHANGE_QUEUE_PRIORITY:
		buffer[bufferSize++] = request->PrinterNumber;
		strncpy((char*)&buffer[bufferSize],
			(char*)request->FileServerName,
			NWMAX_SERVER_NAME_LENGTH);
		bufferSize += NWMAX_SERVER_NAME_LENGTH;
		strncpy((char*)&buffer[bufferSize], (char*)request->QueueName,
			NWMAX_QUEUE_NAME_LENGTH);
		bufferSize += NWMAX_QUEUE_NAME_LENGTH;
		buffer[bufferSize++] = request->Priority;
		break;
	case CMD_ADD_QUEUE:
		buffer[bufferSize++] = request->PrinterNumber;
		strncpy((char*)&buffer[bufferSize],
			(char*)request->FileServerName,
			NWMAX_SERVER_NAME_LENGTH);
		bufferSize += NWMAX_SERVER_NAME_LENGTH;
		strncpy((char*)&buffer[bufferSize], (char*)request->QueueName,
			NWMAX_QUEUE_NAME_LENGTH);
		bufferSize += NWMAX_QUEUE_NAME_LENGTH;
		buffer[bufferSize++] = request->Priority;
		break;
	case CMD_DELETE_QUEUE:
		buffer[bufferSize++] = request->PrinterNumber;
		buffer[bufferSize++] = request->Immediately;
		buffer[bufferSize++] = request->JobOutcome;
		strncpy((char*)&buffer[bufferSize],
			(char*)request->FileServerName,
			NWMAX_SERVER_NAME_LENGTH);
		bufferSize += NWMAX_SERVER_NAME_LENGTH;
		strncpy((char*)&buffer[bufferSize], (char*)request->QueueName,
			NWMAX_QUEUE_NAME_LENGTH);
		bufferSize += NWMAX_QUEUE_NAME_LENGTH;
		break;
	case CMD_GET_PRINTERS_FOR_QUEUE:
		strncpy((char*)&buffer[bufferSize],
			(char*)request->FileServerName,
			NWMAX_SERVER_NAME_LENGTH);
		bufferSize += NWMAX_SERVER_NAME_LENGTH;
		strncpy((char*)&buffer[bufferSize], (char*)request->QueueName,
			NWMAX_QUEUE_NAME_LENGTH);
		bufferSize += NWMAX_QUEUE_NAME_LENGTH;
		buffer[bufferSize++] = request->MaxNumberOfEntries;
		break;
	case CMD_SCAN_NOTIFY_LIST:
		buffer[bufferSize++] = request->PrinterNumber;
		temp16 = REVGETINT16(request->Sequence16);
		memcpy((char*)&buffer[bufferSize], (char*)&temp16,
			sizeof(uint16));
		bufferSize += sizeof(uint16);
		break;
	case CMD_CHANGE_NOTIFY:
	case CMD_ADD_NOTIFY:
		buffer[bufferSize++] = request->PrinterNumber;
		strncpy((char*)&buffer[bufferSize],
			(char*)request->FileServerName,
			NWMAX_SERVER_NAME_LENGTH);
		bufferSize += NWMAX_SERVER_NAME_LENGTH;
		strncpy((char*)&buffer[bufferSize], (char*)request->ObjectName,
			NWMAX_OBJECT_NAME_LENGTH);
		bufferSize += NWMAX_OBJECT_NAME_LENGTH;
		temp16 = REVGETINT16(request->ObjectType);
		memcpy((char*)&buffer[bufferSize], (char*)&temp16,
			sizeof(uint16));
		bufferSize += sizeof(uint16);
		temp16 = REVGETINT16(request->FirstNotice);
		memcpy((char*)&buffer[bufferSize], (char*)&temp16,
			sizeof(uint16));
		bufferSize += sizeof(uint16);
		temp16 = REVGETINT16(request->Interval);
		memcpy((char*)&buffer[bufferSize], (char*)&temp16,
			sizeof(uint16));
		bufferSize += sizeof(uint16);
		break;
	case CMD_DELETE_NOTIFY:
		buffer[bufferSize++] = request->PrinterNumber;
		strncpy((char*)&buffer[bufferSize],
			(char*)request->FileServerName,
			NWMAX_SERVER_NAME_LENGTH);
		bufferSize += NWMAX_SERVER_NAME_LENGTH;
		strncpy((char*)&buffer[bufferSize], (char*)request->ObjectName,
			NWMAX_OBJECT_NAME_LENGTH);
		bufferSize += NWMAX_OBJECT_NAME_LENGTH;
		temp16 = REVGETINT16(request->ObjectType);
		memcpy((char*)&buffer[bufferSize], (char*)&temp16,
			sizeof(uint16));
		bufferSize += sizeof(uint16);
		break;
	case CMD_ATTACH_TO_FILE_SERVER:
		strncpy((char*)&buffer[bufferSize],
			(char*)request->FileServerName,
			NWMAX_SERVER_NAME_LENGTH);
		bufferSize += NWMAX_SERVER_NAME_LENGTH;
		strncpy((char*)&buffer[bufferSize], (char*)request->Password,
			NWMAX_PROPERTY_VALUE_LENGTH);
		bufferSize += NWMAX_PROPERTY_VALUE_LENGTH;
		break;
	case CMD_DETACH_FROM_FILE_SERVER:
		buffer[bufferSize++] = request->JobOutcome;
		buffer[bufferSize++] = request->Immediately;
		strncpy((char*)&buffer[bufferSize],
			(char*)request->FileServerName,
			NWMAX_SERVER_NAME_LENGTH);
		bufferSize += NWMAX_SERVER_NAME_LENGTH;
		break;
	case CMD_GET_ATTACHED_SERVERS:
		buffer[bufferSize++] = request->Sequence8;
		break;
	case CMD_GET_REMOTE:
		buffer[bufferSize++] = request->PrinterNumber;
		break;
	case CMD_CONNECT_REMOTE:
		buffer[bufferSize++] = request->PrinterNumber;
		break;
	case CMD_SET_REMOTE_MODE:
		buffer[bufferSize++] = request->PrinterNumber;
		buffer[bufferSize++] = request->Shared;
		break;
	/*
		Commands that do not use any extra fields
	*/
	case CMD_GET_PRINT_SERVER_INFO:
	case CMD_CANCEL_DOWN:
		break;
	default:
		dprintf("GetSpxReply: ERROR unknown request type %02X\n",
			request->FunctionNumber);
		return(PSE_INVALID_REQUEST);
	}

	/*
		Send the buffer to the other end.
	*/
	dprintf("GetSpxReply: send(%d)=", bufferSize);
	ps_od(buffer, bufferSize);
	flags = T_MORE;
	rtnCode = t_snd(connectInfo->fd, buffer, bufferSize, flags);
	if (rtnCode != bufferSize)
	{
		if (rtnCode == -1) {
			if (t_errno == TLOOK) {
				if (t_look(connectInfo->fd) == T_DISCONNECT) {
					dprintf("GetSpxReply: Connection Closed\n");
					if ((disconInfo = (struct t_discon *) t_alloc(connectInfo->fd, T_DIS, T_ALL)) == NULL) {
						dprintf("GetSpxReply: Unable to allocate t_discon buffer.  Can't get disconnect after t_snd.\n");
					} else {
						(void) t_rcvdis(connectInfo->fd, disconInfo);
					}
					return(PSC_CONNECTION_TERMINATED);
				}
			}
		        dprintf("GetSpxReply: Error Sending.  t_errno=%d, errno=%d\n", t_errno, errno);
		} else {
			dprintf("GetSpxReply: Sent %d bytes, expected %d.\n", rtnCode, bufferSize);
		}
		return(PSC_CONNECTION_TERMINATED);
	}
	sendTime = time(NULL);

	/*
		Wait for a reply
	*/
TryAgain:
	if (t_rcv(connectInfo->fd, (char*)buffer, BUFSIZ, &flags) < 0) {
		if (t_errno == TSYSERR) {
			if (errno == EINTR) {
				if (time(0) < (sendTime + WAIT_SECONDS)) {
					dprintf("GetSpxReply: Interrupted system call on t_rcv, retrying.\n");
					goto TryAgain;
				}
				dprintf("GetSpxReply: Timed out waiting for response.\n");
			} else {
				dprintf("GetSpxReply: System error on t_rcv: t_errno=%d, errno=%d\n", t_errno, errno);
			}
		} else if ((t_errno == TLOOK) && (t_look(connectInfo->fd) == T_DISCONNECT)) {
			if ((disconInfo = (struct t_discon *) t_alloc(connectInfo->fd, T_DIS, T_ALL)) == NULL) {
				dprintf("GetSpxReply: Unable to allocate t_discon buffer.  Can't get disconnect after t_rcv.\n");
			} else {
				(void) t_rcvdis(connectInfo->fd, disconInfo);
			}
		} else {
			dprintf("GetSpxReply: Error on t_rcv: t_errno=%d\n", t_errno);
		}
		return(PSC_CONNECTION_TERMINATED);
	}

	/*
		Build the reply structure
	*/
	bufferSize = rtnCode;
	dprintf("GetSpxReply: recvd(%d)=", bufferSize);
	ps_od(buffer, bufferSize);
	memcpy((char*)&temp16, (char*)buffer, sizeof(uint16));
	if (!reply)
	{
		dprintf("doing short return\n");
		return(REVGETINT16(temp16));
	}
	bufferSize = 0;
	reply->CompletionCode = REVGETINT16(temp16);
	bufferSize += sizeof(uint16);

	switch (request->FunctionNumber)
	{
	/*
		Commands that need only a completion code
	*/
	case CMD_DOWN:
	case CMD_CANCEL_DOWN:
	case CMD_STOP_PRINTER:
	case CMD_START_PRINTER:
	case CMD_MOUNT_FORM:
	case CMD_REWIND_PRINT_JOB:
	case CMD_EJECT_PAGE:
	case CMD_MARK_PAGE:
	case CMD_CHANGE_SERVICE_MODE:
	case CMD_ABORT_JOB:
	case CMD_CHANGE_QUEUE_PRIORITY:
	case CMD_ADD_QUEUE:
	case CMD_DELETE_QUEUE:
	case CMD_CHANGE_NOTIFY:
	case CMD_ADD_NOTIFY:
	case CMD_DELETE_NOTIFY:
	case CMD_ATTACH_TO_FILE_SERVER:
	case CMD_DETACH_FROM_FILE_SERVER:
	case CMD_SET_REMOTE_MODE:
		break;
	/*
		Commands that need more than a completion code
	*/
	case CMD_LOGIN_TO_PRINT_SERVER:
		reply->AccessLevel = buffer[bufferSize++];
		break;
	case CMD_GET_PRINT_SERVER_INFO:
		reply->NptStatus = buffer[bufferSize++];
		reply->AttachedPrinters = buffer[bufferSize++];
		reply->ServiceMode = buffer[bufferSize++];
		reply->MajorVersionNumber = buffer[bufferSize++];
		reply->MinorVersionNumber = buffer[bufferSize++];
		reply->RevisionNumber = buffer[bufferSize++];
		memcpy((char*)reply->SerialNumber,
			(char*)&buffer[bufferSize], 4);
		bufferSize += 4;
		reply->PSType = buffer[bufferSize++];
		break;
	case CMD_GET_PRINTER_STATUS:
		reply->NptStatus = buffer[bufferSize++];
		reply->ErrorCode = buffer[bufferSize++];
		reply->ActiveJob = buffer[bufferSize++];
		reply->ServiceMode = buffer[bufferSize++];
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->FormNumber = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		strncpy((char*)reply->FormName, (char*)&buffer[bufferSize],
			NWMAX_FORM_NAME_LENGTH);
		bufferSize += NWMAX_FORM_NAME_LENGTH;
		strncpy((char*)reply->PrinterName, (char*)&buffer[bufferSize],
			NWMAX_OBJECT_NAME_LENGTH);
		bufferSize += NWMAX_OBJECT_NAME_LENGTH;
		break;
	case CMD_GET_JOB_STATUS:
		/* File Server Name */
		strncpy((char*)reply->FileServerName,
			(char*)&buffer[bufferSize],
			NWMAX_SERVER_NAME_LENGTH);
		bufferSize += NWMAX_SERVER_NAME_LENGTH;
		/* Queue Name */
		strncpy((char*)reply->QueueName, (char*)&buffer[bufferSize],
			NWMAX_QUEUE_NAME_LENGTH);
		bufferSize += NWMAX_QUEUE_NAME_LENGTH;
		/* Job Queue Number */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->JobQueueNumber = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* Job Description */
		strncpy((char*)reply->JobDescription,
			(char*)&buffer[bufferSize],
			NWMAX_JOB_DESCRIPTION_LENGTH);
		bufferSize += NWMAX_JOB_DESCRIPTION_LENGTH;
		/* Copies In Job */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->CopiesInJob = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* Copy Size */
		memcpy((char*)&temp32, (char*)&buffer[bufferSize],
			sizeof(uint32));
		reply->CopySize = REVGETINT16(temp32);
		bufferSize += sizeof(uint32);
		/* Copies Printed */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->CopiesPrinted = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* Bytes Into Current Copy */
		memcpy((char*)&temp32, (char*)&buffer[bufferSize],
			sizeof(uint32));
		reply->BytesIntoCurrentCopy = REVGETINT16(temp32);
		bufferSize += sizeof(uint32);
		/* Form Number */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->FormNumber = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* Text Byte Stream */
		reply->TextByteStream = buffer[bufferSize++];
		break;
	case CMD_SCAN_QUEUE_LIST:		/* Get Queues Serviced */
		/* Sequence */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->Sequence16 = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* File Server Name */
		strncpy((char*)reply->FileServerName,
			(char*)&buffer[bufferSize],
			NWMAX_SERVER_NAME_LENGTH);
		bufferSize += NWMAX_SERVER_NAME_LENGTH;
		/* Queue Name */
		strncpy((char*)reply->QueueName, (char*)&buffer[bufferSize],
			NWMAX_QUEUE_NAME_LENGTH);
		bufferSize += NWMAX_QUEUE_NAME_LENGTH;
		reply->Priority = buffer[bufferSize++];
		break;
	case CMD_GET_PRINTERS_FOR_QUEUE:
		reply->NumPrinters = buffer[bufferSize++];
		memcpy((char*)reply->ServicingQueueArray,
			(char*)&buffer[bufferSize], (int)reply->NumPrinters);
		bufferSize += reply->NumPrinters;
		break;
	case CMD_SCAN_NOTIFY_LIST:
		/* Sequence */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->Sequence16 = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* File Server Name */
		strncpy((char*)reply->FileServerName,
			(char*)&buffer[bufferSize],
			NWMAX_SERVER_NAME_LENGTH);
		bufferSize += NWMAX_SERVER_NAME_LENGTH;
		/* Object Name */
		strncpy((char*)reply->ObjectName, (char*)&buffer[bufferSize],
			NWMAX_OBJECT_NAME_LENGTH);
		bufferSize += NWMAX_OBJECT_NAME_LENGTH;
		/* Object Type */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->ObjectType = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* First Notice */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->FirstNotice = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* Interval */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->Interval = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		break;
	case CMD_GET_ATTACHED_SERVERS:
		/* Sequence */
		reply->Sequence8 = buffer[bufferSize++];
		/* File Server Name */
		strncpy((char*)reply->FileServerName,
			(char*)&buffer[bufferSize],
			NWMAX_SERVER_NAME_LENGTH);
		bufferSize += NWMAX_SERVER_NAME_LENGTH;
		break;
	case CMD_GET_REMOTE:
		reply->PrinterNumber = buffer[bufferSize++];
		/* Printer Type */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->PrinterType = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* Printer Name */
		strncpy((char*)reply->PrinterName, (char*)&buffer[bufferSize],
			NWMAX_OBJECT_NAME_LENGTH);
		bufferSize += NWMAX_OBJECT_NAME_LENGTH;
		break;
	case CMD_CONNECT_REMOTE:
		/* PrinterType */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->PrinterType = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* Use Interrupts */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->UseInterrupts = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* IRQ Number */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->IRQNumber = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* Blocks */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->Blocks = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* Protocol */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->Protocol = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* Baud Rate */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->BaudRate = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* Data Bits */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->DataBits = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* Stop Bits */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->StopBits = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* Parity Type */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->ParityType = REVGETINT16(temp16);
		bufferSize += sizeof(uint16);
		/* Socket */
		memcpy((char*)&temp16, (char*)&buffer[bufferSize],
			sizeof(uint16));
		reply->Socket = GETINT16(temp16);
		bufferSize += sizeof(uint16);
		break;
	}

	return(reply->CompletionCode);
}


/*
	Misc stuff
*/
char*
ps_strupr(string)
char	*string;
{
	char *base;

	base = string;
	while (*string)
	{
		*string = toupper(*string);
		string++;
	}

	return(base);
}


ps_od(ptr, len)
uint8	*ptr;
int		len;
{
	if (UniqueLibNptDebugFlag)
	{
		for ( ; len > 0; len--)
			printf("%02X ", 0xFF & *ptr++);

		printf("\n");
	}
}

/********************************************************************/
/********************************************************************/

