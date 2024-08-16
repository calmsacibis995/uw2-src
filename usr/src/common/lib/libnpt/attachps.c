/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnpt:attachps.c	1.6"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnpt/attachps.c,v 1.4 1994/06/24 16:13:08 wrees Exp $";
#endif

/********************************************************************
 *
 * Program Name:  NPT library routines
 *
 * Filename:	  AttachPS.c
 *
 * Date Created:  November 1, 1990
 *
 * Version:		  1.00
 *
 * Programmers:	  Joe Ivie
 *
 * Description:	This file contains the AttachToPrintServer routine
 *
 * COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
 *
 ********************************************************************/

#include "libnpt.h"

#define SAP_DEVICE	"/dev/ipx"
#define SAP_GENERAL_QUERY		1
#define SAP_GENERAL_RESPONSE	2
#define SAP_DELAY		10
#define SAP_SOCKET		0x0452

/* Global Structures */
struct server {
	struct server	*next;
	char			name[NWMAX_SERVER_NAME_LENGTH];
	uint16			connID;
};


/********************************************************************/
/*
	Attach To Print Server
*/

uint16
PSAttachToPrintServer(printServerName, connectID)
	char		*printServerName;	/* Print server name			*/
	uint16		*connectID;			/* SPX Connection number		*/
{
	int 				rtn;
	int 				sapFd;
	uint16				ccode = PSE_SUCCESSFUL;
	struct t_unitdata	tudata;
	ipxAddr_t			tAddress;
	int 				option;
	uint8				udata[BUFSIZ];
	ipxAddr_t			ipxAddress;
	int 				flags = 0;
	uint16				temp16;
	time_t				startTime;


	ps_strupr(printServerName);
	dprintf("PSAttachToPrintServer: ps-%s\n", printServerName);
	*connectID = 0;

	/*
		Find the address of the Print Server using SAP
	*/
	sapFd = t_open(SAP_DEVICE, O_RDWR, NULL);
	if (sapFd == -1)
	{
		ccode = PSC_NO_AVAILABLE_IPX_SOCKETS;
		return(ccode);
	}

	rtn = t_bind(sapFd, NULL, NULL);
	if (rtn == -1)
	{
		ccode = PSC_NO_AVAILABLE_IPX_SOCKETS;
		return(ccode);
	}

	/* build the sap packet */
	tudata.addr.buf = (char*)&tAddress;
	tudata.addr.maxlen = sizeof(tAddress);
	tudata.addr.len = sizeof(tAddress);
	memset((char*)tAddress.net, 0x00, IPX_NET_SIZE);
	memset((char*)tAddress.node, 0xFF, IPX_NODE_SIZE);
	temp16 = GETINT16(SAP_SOCKET);
	memcpy((char*)tAddress.sock, (char*)&temp16, sizeof(uint16));

	dprintf("Sap Request address: ");
	ps_od((uint8*)tudata.addr.buf, (int)tudata.addr.len);

	tudata.opt.buf = (char*)&option;
	tudata.opt.maxlen = sizeof(option);
	tudata.opt.len = 0;

	tudata.udata.buf = (char*)udata;
	tudata.udata.maxlen = sizeof(udata);
	temp16 = GETINT16(SAP_GENERAL_QUERY);
	memcpy(tudata.udata.buf, (char*)&temp16, sizeof(uint16));
	temp16 = 0x4700;   /*GETINT16(OT_ADVERTISING_PRINT_SERVER);*/
	memcpy(tudata.udata.buf + 2, (char*)&temp16, sizeof(uint16));
	tudata.udata.len = 2 * sizeof(uint16);

	dprintf("Sap Request data: ");
	ps_od((uint8*)tudata.udata.buf, (int)tudata.udata.len);

#ifdef NEVER
	if (t_sndudata(sapFd, &tudata, flags) == -1) {
#endif
	if (t_sndudata(sapFd, &tudata) == -1) {
		dprintf("Error sending SAP request\n");
		ccode = PSC_UNABLE_TO_GET_SERVER_ADDRESS;
		return(ccode);
	}

	/* wait for a response */
	startTime = time(NULL);

Repeat:
	flags = 0;
	if (startTime + SAP_DELAY <= time(NULL))
	{
		dprintf("No reponse from sap\n");
		ccode = PSC_NO_SUCH_PRINT_SERVER;
		return(ccode);
	}


	/* strip the response type */
	if (t_look(sapFd) != T_UDATA)
	{
		sleep(1);
		goto Repeat;
	}

	tudata.udata.maxlen = 2;
	rtn = t_rcvudata(sapFd, &tudata, &flags);
	if (rtn == -1)
	{
#ifdef BLABBY
		t_error("t_rcvudata");
#endif
		dprintf("Error receiveing SAP data; t_errno = %d\n", t_errno);
		ccode = PSC_UNABLE_TO_GET_SERVER_ADDRESS;
		return(ccode);
	}

	tudata.udata.maxlen = 2 + NWMAX_SERVER_NAME_LENGTH +
		IPX_ADDR_SIZE + 2;

	while (flags & T_MORE)
	{
		rtn = t_rcvudata(sapFd, &tudata, &flags);
		if (rtn == -1)
		{
			dprintf("Error receiveing SAP data\n");
			ccode = PSC_UNABLE_TO_GET_SERVER_ADDRESS;
			return(ccode);
		}

		dprintf("Sap Response packet: ");
		ps_od((uint8*)tudata.udata.buf, (int)tudata.udata.len);

		if (strncmp(tudata.udata.buf + 2, printServerName,
			NWMAX_SERVER_NAME_LENGTH) == 0)
		{
			/* copy the address of the print server */
			dprintf("Found a match\n");
			memcpy(&ipxAddress, tudata.udata.buf + 2 +
				NWMAX_SERVER_NAME_LENGTH, IPX_ADDR_SIZE);
			break;
		}

		if ((flags & T_MORE) == 0)
			goto Repeat;
	}

	dprintf("after while flags = %x\n", flags);
	t_close(sapFd);

	/*
		start an SPX connection with the print server client socket
	*/
	*connectID = OpenSpxConnection(&ipxAddress);

	if (*connectID == FALSE)
	{
		ccode = PSC_NO_AVAILABLE_SPX_CONNECTIONS;
		return(ccode);
	}

	dprintf("connectID=%x\n", *connectID);

	return(ccode);
}

/********************************************************************/
/********************************************************************/
