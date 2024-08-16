/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/npsd/nvt.c	1.7"
#ident	"$Id: nvt.c,v 1.11.2.1 1994/10/13 18:10:37 ericw Exp $"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include "npsd.h"

/*ARGSUSED*/
NVTFork( char *binDir)
{
	return( SUCCESS);
}

NVTConfigure( char *binDir)
{
	char    cmdStr[256];					/* system function string */
	char    netStr[9];						/* ipx net number string*/
	char    nodeStr[13];					/* ipx node number string*/
	char    portStr[5];						/* nvt port string */
	unsigned char	myNet[IPX_NET_SIZE];
	unsigned char	myNode[IPX_NODE_SIZE];
	int		nvtPort;

	/* Get my net number.
	 */
	if( doStrIoctl(ipx0Fd, IPX_GET_NET, (char *)&myNet, IPX_NET_SIZE, 0) < 0) {
		fprintf(stderr, MsgGetStr(NPS_IOCTL_FAIL), "IPX_GET_NET", "/dev/ipx0");
		perror("");
		return(FAILURE);
	}
	/* Get my node number.
	 */
	if( doStrIoctl(ipx0Fd, IPX_GET_NODE_ADDR, (char *)&myNode, 
				IPX_NODE_SIZE, 0) < 0) {
		fprintf(stderr, MsgGetStr(NPS_IOCTL_FAIL),
				"IPX_GET_NODE_ADDR", "/dev/ipx0");
		perror("");
		return(FAILURE);
	}

	/* Get NVT port number.
	 */
	nvtPort = NVT_SOCKET;

	/* Convert Net Node Socket to string */
	sprintf( netStr, "%02.2X%02.2X%02.2X%02.2X", myNet[0], myNet[1],
			myNet[2], myNet[3]);
	sprintf( nodeStr, "%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X", myNode[0], 
			myNode[1], myNode[2], myNode[3], myNode[4], myNode[5]);
	sprintf(portStr, "%04.4X", nvtPort);

	/* execute sacadm command string (add nvt listen port monitor)
	 * "listen spx" below means to use the netconfig entry "spx"
	 */

	system("/usr/sbin/sacadm -a -p nvt -t listen \
		-c \"/usr/lib/saf/listen -m \"nvt\" spx  2>/dev/null\" \
		-v `/usr/sbin/nlsadmin -V` -n 3 > /dev/null 2>&1");

	/* construct pmadm command string and execute (add nvt service)
	 */
	memset(cmdStr,'\0',sizeof(cmdStr));		/* clear cmdStr */
	strcpy(cmdStr, "/usr/sbin/pmadm  -a -p nvt -s nvt -i root -fu \
			-m `/usr/sbin/nlsadmin -c \"");
	strcat(cmdStr, binDir);			/* append binary directory */
	strcat(cmdStr, "/nvtd");		/* append server name */
	strcat(cmdStr, "\" -p ptem,tirdwr,ldterm,ttcompat -A \"\\x");
	strcat(cmdStr, netStr);			/* append my ipx net */
	strcat(cmdStr, nodeStr);		/* append my node */
	strcat(cmdStr, portStr);		/* append port number */
	strcat(cmdStr, "\"` -v `/usr/sbin/nlsadmin -V` > /dev/null 2>&1");
	system(cmdStr);					/* execute command */

	return( SUCCESS);
}

/*ARGSUSED*/
NVTDeconfigure( char *binDir)
{
	/* Deconfigure SAF/SAC, remove all nvt node in /dev.
	 */
	system("/usr/sbin/sacadm -r -p nvt > /dev/null 2>&1");
	system("rm /dev/nvt[0-9][0-9][0-9] > /dev/null 2>&1");
	return( SUCCESS);
}
