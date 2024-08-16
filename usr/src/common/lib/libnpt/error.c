/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

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
#ident	"@(#)libnpt:error.c	1.2"
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnpt/error.c,v 1.1 1994/02/11 23:05:23 eric Exp $";
#endif

/********************************************************************
 *
 * Program Name:	NPT library routines
 *
 * Filename:		Error.c
 *
 * Date Created:	November 9, 1990
 *
 * Version:			1.00
 *
 * Programmers:		Joe Ivie
 *
 * Description:		This file contains error message translations
 *					and error enabling calls
 *
 *
 * COPYRIGHT (c) 1988 by Novell, Inc.  All Rights Reserved.
 *
 ********************************************************************/

#include "libnpt.h"

/********************************************************************/

int			UniqueLibNptDebugFlag = FALSE;

/*
	Messages
*/
struct	message {
	uint16		messageID;
	char		*message;
} messageTable[] = {
	PSC_NO_AVAILABLE_SPX_CONNECTIONS,	"No available SPX connections",
	PSC_SPX_NOT_INITIALIZED,		"SPX is not installed",
	PSC_NO_SUCH_PRINT_SERVER,	"No such print server",
	PSC_UNABLE_TO_GET_SERVER_ADDRESS,	"Unable to get server address",
	PSC_UNABLE_TO_CONNECT_TO_SERVER,	"Unable to connect to server",
	PSC_NO_AVAILABLE_IPX_SOCKETS,	"No available IPX sockets",
	PSC_ALREADY_ATTACH_TO_A_PRINTSERVER,
								"Already attached to print server",
	PSC_IPX_NOT_INITIALIZED,		"IPX not initialized",
	PSC_CONNECTION_TERMINATED,	"Connection terminated",
	PSE_SUCCESSFUL,				"Successful",
	PSE_ACCESS_DENIED,			"Access denied",
	PSE_ACCOUNT_DISABLED,		"Account disabled",
	PSE_ALREADY_ATTACHED,		"Already attached",
	PSE_ALREADY_IN_LIST,		"Already in list",
	PSE_ALREADY_IN_USE,			"Already in use",
	PSE_BINDERY_LOCKED,			"Bindery locked",
	PSE_CANT_ATTACH,			"Can't attach",
	PSE_CANT_DETACH_PRIMARY_SERVER, "Can't detach the primary server",
	PSE_CANT_LOGIN,				"Can't login",
	PSE_CANT_OPEN_CONFIG_FILE,	"Can't open config file",
	PSE_CANT_READ_CONFIG_FILE,	"Can't read config file",
	PSE_DOWN,					"Print server is DOWN",
	PSE_SERVER_MAXED_OUT,		"No free connections on file server",
	PSE_GOING_DOWN,				"Print server is going down",
	PSE_ILLEGAL_ACCT_NAME,		"Illegal account name",
	PSE_INTRUDER_DETECTION_LOCK,	"Intruder detection lock out",
	PSE_INVALID_PARAMETER,		"Invalid parameter",
	PSE_INVALID_REQUEST,		"Invalid request",
	PSE_LOGIN_DISABLED,			"Login disabled",
	PSE_NOT_ATTACHED_TO_SERVER,	"Not attached to server",
	PSE_NOT_AUTHORIZED_FOR_QUEUE,	"Not authorized for queue",
	PSE_NOT_CONNECTED,			"Not connected",
	PSE_NOT_ENOUGH_MEMORY,		"Not enough memory",
	PSE_NOT_IN_LIST,			"Not in the list",
	PSE_NOT_REMOTE_PRINTER,		"Not a Remote Printer",
	PSE_NO_ACCOUNT_BALANCE,		"No account balance",
	PSE_NO_CREDIT_LEFT,			"No credit left",
	PSE_NO_JOB_ACTIVE,			"No active job",
	PSE_NO_MORE_GRACE,			"No more grace",
	PSE_NO_RESPONSE,			"No response",
	PSE_NO_RIGHTS,				"No rights",
	PSE_NO_SUCH_QUEUE,			"No such queue",
	PSE_NO_SUCH_PRINTER,		"No such printer",
	PSE_PASSWORD_HAS_EXPIRED,	"Password has expired",
	PSE_PRINTER_BUSY,			"Printer is BUSY",
	PSE_PRINTER_ALREADY_INSTALLED,	"Printer already installed",
	PSE_NOT_CONNECTED,			"Not connected",
	PSE_QUEUE_HALTED,			"Queue halted",
	PSE_TOO_MANY_CONNECTIONS,	"Too many connections",
	PSE_TOO_MANY_FILE_SERVERS,	"Too many file servers",
	PSE_TOO_MANY_QUEUE_SERVERS,	"Too many queue servers",
	PSE_UNABLE_TO_ATTACH_TO_QUEUE,	"Unable to attach to queue",
	PSE_UNABLE_TO_VERIFY_IDENTITY,	"Unable to verify identity",
	PSE_UNAUTHORIZED_STATION,	"Unauthorized station",
	PSE_UNAUTHORIZED_TIME,		"Unauthorized time",
	PSE_UNKNOWN_FILE_SERVER,	"Unknown file server",
	PSE_UNKNOWN_PRINTER_TYPE,	"Unknown printer type",
	(uint16) -1,				"Unknown error message",
};

/********************************************************************/
/*
	Message Routines
*/

void
PSDisplayError(string, messageID)
char		*string;
uint16		messageID;
{
	int				i;

	for (i = 0; messageTable[i].messageID != (uint16) -1; i++)
		if (messageTable[i].messageID == messageID)
		{
			printf("%s: %s\n", string, messageTable[i].message);
			return;
		}

	printf("%s: %s %04X\n", string, messageTable[i].message, messageID);
}

void
PSDebug()
{
	UniqueLibNptDebugFlag = TRUE;
}

void
PSNoDebug()
{
	UniqueLibNptDebugFlag = FALSE;
}

/********************************************************************/
/********************************************************************/
